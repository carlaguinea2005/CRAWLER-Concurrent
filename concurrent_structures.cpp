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
    })

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

