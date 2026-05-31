#include "concurrent_structures.hpp"
#include <functional> 

// YASMINE
// PUSH : a thread found a new URL, add it

void SafeQueue::push(const CrawlTask& task) {

    std::lock_guard<std::mutex> lock(mtx);
    q.push(task);// add the task to the end of my queue
    cv.notify_one();
    // the lock is released automatically here
    // because lock_guard does it for me when the function ends
}



// POP: a thread wants the next URL to work on

bool SafeQueue::pop(CrawlTask& task) {

    // I need unique_lock here (not lock_guard) because cv.wait needs to 
    // temporarily release the lock while the thread is sleeping
    std::unique_lock<std::mutex> lock(mtx);

    // go to sleep until ONE of these is true:
    // - the queue has something in it (!q.empty())
    // - OR shutdown was called (done == true)
    // while sleeping, the lock is released so other threads can push
    cv.wait(lock, [this]{
        return !q.empty() || done;
    });

    // I woke up but WHY did I wake up?
    // if the queue is still empty, it means shutdown() was called → time to stop
    // so I return false to tell the while loop to stop
    if (q.empty()) {
        return false;
    }

    // queue has something → grab the first task
    task = q.front();

    // remove it from the queue so no other thread takes the same one
    q.pop();

    // tell the caller: yes I got a task, keep working
    return true;

    // lock is released automatically here
} 



// SHUTDOWN: crawling is done, wake everyone up

void SafeQueue::shutdown() {

    // grab the lock before touching done
    std::lock_guard<std::mutex> lock(mtx);

    // set the flag so threads know we're finished
    done = true;

    // wake up ALL sleeping threads (not just one)
    // each one will check the condition, see done=true, and exit
    cv.notify_all();

} 



bool StripedHashSet::insert_and_check(const std::string& url, const PageData& data) {

    // step 1: figure out which list this url belongs to
    // this gives me a number between 0 and 15
    int stripe = std::hash<std::string>{}(url) % 16;

    // step 2: lock that specific list
    // only I can touch this list now
    std::lock_guard<std::mutex> lock(locks[stripe]);

    // step 3: go through every page already in this list
    // and check if this url is already there
    for (PageData& page : buckets[stripe]) {
        if (page.url == url) {
            // found it — already seen before
            return false;
        }
    }

    // step 4: url is not in the list → add it
    buckets[stripe].push_back(data);

    // tell the caller: this is a new url
    return true;

    // lock released automatically here
}


void StripedHashSet::increment_incoming(const std::string& url) {

    // step 1: figure out which list this url belongs to
    int stripe = std::hash<std::string>{}(url) % 16;

    // step 2: lock that specific list
    std::lock_guard<std::mutex> lock(locks[stripe]);

    // step 3: go through every page in this list
    // find the one with this url and add +1 to its incoming_links
    for (PageData& page : buckets[stripe]) {
        if (page.url == url) {
            page.incoming_links++;
            return;
        }
    }
}


std::vector<PageData> StripedHashSet::get_all_pages() {

    // I will collect everything here
    // starts empty, will grow as we go through each list
    std::vector<PageData> all_pages;

    // go through each of my 16 lists, one by one
    // i goes from 0 to 15
    for (int i = 0; i < NUM_STRIPES; i++) {

        // lock list number i
        // so no thread adds something while I am reading it
        std::lock_guard<std::mutex> lock(locks[i]);

        // go through every page stored in list i
        for (PageData& page : buckets[i]) {

            // add this page to my big collection
            all_pages.push_back(page);

        } // lock released automatically here, then i moves to next list

    }

    // give back the complete collection to whoever called me
    return all_pages;
}