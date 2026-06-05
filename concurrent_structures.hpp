#ifndef CONCURRENT_STRUCTURES_HPP
#define CONCURRENT_STRUCTURES_HPP

#include <string>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <shared_mutex>
#include <unordered_set>
#include <atomic>  


// YASMINE
// manages thread-safe data storage and BFS task queuing for the crawler

// structure to hold our crawling tasks in the queue
struct CrawlTask {
    std::string url;
    int depth;
    std::string parent_url;
};

// structure to hold the final data we want to write to CSV
struct PageData {
    std::string url;
    int depth;
    std::string parent;
    int incoming_links;
    int outgoing_links;
};

class SafeQueue {
private:
    std::queue<CrawlTask> q;        // this is my actual to-do list of URLs
    std::mutex mtx;                 // this is my lock, only one thread can touch q at a time
    std::condition_variable cv;     // this lets threads sleep and wake up when something arrives
    bool done = false;              // starts as false, becomes true when crawling is finished
public:
    void push(const CrawlTask& task); // adds a URL to my list
    bool pop(CrawlTask& task);        // takes a URL from my list (returns false if we're done)
    void shutdown();                  // tells all sleeping threads to wake up and stop
};

class StripedHashSet {
private:

    static const int NUM_STRIPES = 16; // nbr of lists
    std::mutex locks[NUM_STRIPES];  // 16 locks, one per list
    std::vector<PageData> buckets[NUM_STRIPES];  // creates 16 lists that can hold PageData objects

public:
    // function insert_and_check : Tries to add a new URL to the hash table. 
    // input: const std::string& url (the URL to check)
    //        const PageData& data (the data associated with it)
    // output: bool (returns true if the URL was newly added, false if it was already in the set)
    bool insert_and_check(const std::string& url, const PageData& data);

    // function increment_incoming : adds +1 to the incoming_links count for a specific URL
    // input: const std::string& url (the target URL)
    // output: void
    void increment_incoming(const std::string& url);

    std::vector<PageData> get_all_pages(); // gives back all pages collected during crawling

    void update_outgoing(const std::string& url, int count);
};


// upgrade of StripedHashSet: resizes dynamically when buckets get too full
// based on Herlihy chapter 13 (RefinableHashSet)
class RefinableHashSet {
private:
    static const int LOAD_FACTOR = 4; // resize when entries/buckets > 4

    int num_buckets;
    std::atomic<int> size;

    std::vector<std::vector<PageData>> buckets;
    std::vector<std::mutex> locks;
    std::mutex resize_mutex; // separate lock just for resizing

    int get_stripe(const std::string& url) const {
        return std::hash<std::string>{}(url) % num_buckets;
    }

    void resize();

public:
    RefinableHashSet();

    bool insert_and_check(const std::string& url, const PageData& data);
    void increment_incoming(const std::string& url);
    void update_outgoing(const std::string& url, int count);
    std::vector<PageData> get_all_pages();
};



// content-seen test from the Mercator paper (section 3.5)
// uses a readers-writer lock instead of a plain mutex
// multiple threads can check simultaneously, only blocks on writes
class ConcurrentFingerprintSet {
private:
    std::shared_mutex rw_mutex; // allows concurrent reads, exclusive writes
    std::unordered_set<size_t> hashes; // stores fingerprints of seen HTML content

public:
    // returns true if this is new content, false if already seen
    bool insert_if_new(size_t hash);
};



#endif