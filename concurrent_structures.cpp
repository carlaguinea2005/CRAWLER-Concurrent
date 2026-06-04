#include "concurrent_structures.hpp"
#include <functional>

// YASMINE

// adds a new URL to the queue
void SafeQueue::push(const CrawlTask& task) {
    std::lock_guard<std::mutex> lock(mtx);
    q.push(task);
    cv.notify_one(); // signal one waiting thread
}

// takes the next URL from the queue, waits if empty
bool SafeQueue::pop(CrawlTask& task) {
    std::unique_lock<std::mutex> lock(mtx);
    // wait until queue has something or crawl is over
    cv.wait(lock, [this]{
        return !q.empty() || done;
    });

    if (q.empty()) return false; // crawl is done, no more tasks

    task = q.front();
    q.pop();
    return true;
}

// signals all threads to stop
void SafeQueue::shutdown() {
    std::lock_guard<std::mutex> lock(mtx);
    done = true;
    cv.notify_all();
}

// checks if url was already seen, adds it if not
bool StripedHashSet::insert_and_check(const std::string& url, const PageData& data) {
    int stripe = std::hash<std::string>{}(url) % 16;
    std::lock_guard<std::mutex> lock(locks[stripe]);

    // check if url was already visited
    for (PageData& page : buckets[stripe]) {
        if (page.url == url) return false;
    }

    // new url, add it
    buckets[stripe].push_back(data);
    return true;
}

// adds +1 to incoming links count for a given url
void StripedHashSet::increment_incoming(const std::string& url) {
    int stripe = std::hash<std::string>{}(url) % 16;
    std::lock_guard<std::mutex> lock(locks[stripe]);

    for (PageData& page : buckets[stripe]) {
        if (page.url == url) {
            page.incoming_links++;
            return;
        }
    }
}

// returns all collected pages from all buckets
std::vector<PageData> StripedHashSet::get_all_pages() {
    std::vector<PageData> all_pages;

    for (int i = 0; i < NUM_STRIPES; i++) {
        std::lock_guard<std::mutex> lock(locks[i]);
        for (PageData& page : buckets[i]) {
            all_pages.push_back(page);
        }
    }

    return all_pages;
}

// saves the outgoing links count for a given page
void StripedHashSet::update_outgoing(const std::string& url, int count) {
    int stripe = std::hash<std::string>{}(url) % 16;
    std::lock_guard<std::mutex> lock(locks[stripe]);

    for (PageData& page : buckets[stripe]) {
        if (page.url == url) {
            page.outgoing_links = count;
            return;
        }
    }
}

// Mercator paper section 3.5: content-seen test with readers-writer lock
// threads checking for duplicate content dont need to block each other
bool ConcurrentFingerprintSet::insert_if_new(size_t hash) {

    // read lock first: multiple threads can check simultaneously
    {
        std::shared_lock<std::shared_mutex> read_lock(rw_mutex);
        if (hashes.count(hash) > 0) {
            return false; // already seen, skip this page
        }
    }

    // not found: need to insert, so upgrade to write lock
    {
        std::unique_lock<std::shared_mutex> write_lock(rw_mutex);

        // check again: another thread might have inserted between our two locks
        if (hashes.count(hash) > 0) {
            return false;
        }

        hashes.insert(hash);
        return true; // new content, process it
    }
}