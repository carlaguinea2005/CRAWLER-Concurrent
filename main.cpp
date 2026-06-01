#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include "downloader.hpp"
#include "concurrent_structures.hpp"
#include "parser_analyzer.hpp"

// this is the function every thread runs
void worker_thread(SafeQueue& queue, StripedHashSet& visited, DownloadConfig& config) {
    // this variable will hold the current task (URL to work on)
    // starts empty, gets filled by pop()
    CrawlTask task;
    
    //  get the next URL from the queue
    while (queue.pop(task)) {
        
        // download the HTML
        std::string html = Downloader::download_url(task.url, config);
        if (html.empty()) continue; // Skip if download failed
        
        // extract, normalize, and filter all links inside the HTML
        std::vector<std::string> raw_links = Parser::extract_links(html);
        int outgoing_count = 0; //this counts how many valid wikipedia links this page has

        for (const std::string& raw : raw_links) {
            std::string clean_url = Parser::normalize_url(task.url, raw);
            
            // ask Elisa: is this a valid Wikipedia article URL?
            if (Parser::wikipedia_filter(clean_url)) {
                outgoing_count++; // valid Wikipedia link found → count it as an outgoing link
                
                // update data structures
                visited.increment_incoming(clean_url);
                
                PageData new_page_data = {clean_url, task.depth + 1, task.url, 1, 0};  // create the data for this new page
                
                // if it's a brand new page we haven't seen before, add it to the queue
                if (visited.insert_and_check(clean_url, new_page_data)) {
                    CrawlTask new_task = {clean_url, task.depth + 1, task.url};
                    queue.push(new_task);
                }
            }
        }
    }
}


// example: ./crawler 4 → argc=2, argv[1]="4"
int main(int argc, char* argv[]) {

    // 1.SETTINGS:

    // read number of threads from command line
    int num_threads = 4;
    if (argc > 1) {
        num_threads = std::stoi(argv[1]); // convert text "4" to number 4
    }

    // read seed URL from command line
    std::string seed_url = "https://en.wikipedia.org/wiki/Computer_science";
    if (argc > 2) {
        seed_url = argv[2];
    }

    std::cout << "Starting crawler with " << num_threads << " threads" << std::endl;
    std::cout << "Seed URL: " << seed_url << std::endl;

    // 2.SETUP:

    DownloadConfig config;  // create download settings
    SafeQueue queue;  // create the shared queue
    StripedHashSet visited;  // create the shared hash set

    // create the first task manually
    CrawlTask first_task;
    first_task.url = seed_url;
    first_task.depth = 0;
    first_task.parent_url = "";

    // put the first task into MY queue
    // now threads have something to start with
    queue.push(first_task);

    //we measure how long the crawling takes
    auto start_time = std::chrono::high_resolution_clock::now();

    // 3.LAUNCH THREADS:

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(worker_thread, 
                            std::ref(queue), 
                            std::ref(visited), 
                            std::ref(config));
    }

    for (std::thread& t : threads) {
        t.join();
    }

    //5.SHUTDOWN:

    // tell all sleeping threads to wake up and stop
    queue.shutdown();

    auto end_time = std::chrono::high_resolution_clock::now();

    // calculate how many seconds the crawling took
    double total_seconds = std::chrono::duration_cast<std::chrono::duration<double>>
                          (end_time - start_time).count();

    // 6.COLLECT RESULTS:

    // get all page data from MY hash set
    std::vector<PageData> results = visited.get_all_pages();

    // 7.PRINT STATS :
    std::cout << "Crawling done!" << std::endl;
    std::cout << "Total pages found: " << results.size() << std::endl;
    std::cout << "Total time: " << total_seconds << " seconds" << std::endl;
    std::cout << "Pages per second: " << results.size() / total_seconds << std::endl;

    // 8.WRITE CSV:

    // ask Elisa to write everything to a CSV file
    Benchmarker::generate_csv(results, "output.csv");
    std::cout << "CSV written to output.csv" << std::endl;

    return 0;
}