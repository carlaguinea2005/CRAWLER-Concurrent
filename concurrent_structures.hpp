#ifndef CONCURRENT_STRUCTURES_HPP
#define CONCURRENT_STRUCTURES_HPP

#include <string>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>

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
    std::queue<CrawlTask> q;
    std::mutex mtx;
    std::condition_variable cv;

public:
    // push funcion : adds a new URL task to the queue and wakes up a waiting thread
    // input: const CrawlTask& task (the URL and its metadata to be processed)
    // output: void
    void push(const CrawlTask& task);

    // function pop : takes a task from the queue safely. If the queue is empty, 
    //              the thread waits until a new task arrives.
    // input: CrawlTask& task (a reference to store the popped task)
    // output: bool (returns true if successful, false if the queue is shutting down)
    bool pop(CrawlTask& task);
};

class StripedHashSet {
private:
    // array of mutexes and buckets here
    // example: std::vector<std::mutex> locks;

    static const int NUM_STRIPES = 16; //change this to increase/decrease the number of stripes in the hash set
    
    std::mutex locks[NUM_STRIPES];
    
    std::vector<PageData> buckets[NUM_STRIPES];

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
};

#endif