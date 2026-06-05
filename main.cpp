#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <unordered_set>
#include <functional>
#include <mutex>

#include "downloader.hpp"
#include "concurrent_structures.hpp"
#include "parser_analyzer.hpp"

std::atomic<int> pages_crawled(0);

//--------------------------------------------------------------------------------------------------------------
// default settings max pages to crawl, number of threads, and starting URL here
// the settings can be changes by passing command line arguments:
// ./crawler [max_pages] [num_threads] [start_url]
// this is how we run the tests with different number of threads in the benchmarking.cpp file

int MAX_PAGES = 50; 
int num_threads = 4;
std::string start_url = "https://en.wikipedia.org/wiki/Crawling";

// --------------------------------------------------------------------------------------------------------------

// content-seen test structures from mercator paper
// upgraded to use readers-writer lock: multiple threads can check simultaneously
ConcurrentFingerprintSet fingerprints;

void worker_thread(SafeQueue& queue, RefinableHashSet& visited, DownloadConfig& config, const std::string target_domain, Downloading_Stats& stats) {
    CrawlTask task;
    
    while (queue.pop(task)) {
        int a = pages_crawled.fetch_add(1);
        if (a >= MAX_PAGES) {
            queue.shutdown();
            break;
        }

        std::string html = Downloader::download_url_with_retry(task.url, config, stats);
        if (html.empty()) continue; 

        // content-seen test
        size_t content_hash = std::hash<std::string>{}(html);
        if (!fingerprints.insert_if_new(content_hash)) {
            std::cout << "Skipping duplicate content: " << task.url << "\n";
            continue;
        }
        
        std::vector<std::string> raw_links = Parser::extract_links(html);
        int outgoing_count = 0;

        for (const std::string& raw : raw_links) {
            std::string clean_url = Parser::normalize_url(task.url, raw);
            
            // we use the generic domain filter instead of just wikipedia
            if (Parser::is_internal_link(clean_url, target_domain)) {
                outgoing_count++;
                PageData new_page_data = {clean_url, task.depth + 1, task.url, 0, 0};
                
                if (visited.insert_and_check(clean_url, new_page_data)) {
                    CrawlTask new_task = {clean_url, task.depth + 1, task.url};
                    queue.push(new_task);
                }
                visited.increment_incoming(clean_url);
            }
        }

        visited.update_outgoing(task.url, outgoing_count);
        std::cout << "Crawled [" << (a + 1) << "/" << MAX_PAGES << "] : " << task.url << "\n";

        //std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

int main(int argc, char* argv[]) {

    if (argc > 1) {
        MAX_PAGES = std::stoi(argv[1]);
    }
    if (argc > 2) {
        num_threads = std::stoi(argv[2]);
    }
    if (argc > 3) {
        start_url = argv[3];
    }

    SafeQueue queue;
    RefinableHashSet visited;
    DownloadConfig config;
    Downloading_Stats stats;
    
    std::string url = start_url;
    std::string target_domain = Parser::extract_base_domain(start_url);
    
    PageData root_data = {url, 0, "NONE", 1, 0};
    visited.insert_and_check(url, root_data);
    queue.push({url, 0, "NONE"});
    
    std::vector<std::thread> threads;
    
    std::cout << "Starting multithreaded crawler on " << target_domain << " with " << num_threads << " threads.\n";

    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker_thread,
                             std::ref(queue),
                             std::ref(visited),
                             std::ref(config),
                             target_domain,
                             std::ref(stats));
    }
    
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();

    double total_seconds =
        std::chrono::duration<double>(end_time - start_time).count();
    
    std::cout << "\nCrawling finished.\n";

    std::vector<PageData> final_data = visited.get_all_pages();

    Benchmarker::generate_csv(final_data, "crawler_results.csv");

    Benchmarker::bfs_path(final_data);

    std::cout << "Total pages found: " << final_data.size() << std::endl;
    std::cout << "Total time: " << total_seconds << " seconds" << std::endl;
    std::cout << "Pages per second: "
              << final_data.size() / total_seconds
              << std::endl;

    stats.print_stats();

    return 0;
}