#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <unordered_set>
#include <functional>
#include <mutex>
#include <iomanip>

#include "downloader.hpp"
#include "concurrent_structures.hpp"
#include "parser_analyzer.hpp"

std::atomic<int> pages_crawled(0);
std::mutex timing_mtx;
double total_parsing_time = 0.0;
double total_structure_time = 0.0;
// track the longest single-thread wall time
double max_download_wall = 0.0;
double max_parsing_wall  = 0.0;
double max_struct_wall   = 0.0;

//--------------------------------------------------------------------------------------------------------------
// default settings max pages to crawl, number of threads, and starting URL here
// the settings can be changes by passing command line arguments:
// ./crawler [max_pages] [num_threads] [start_url]
// this is how we run the tests with different number of threads in the benchmarking.cpp file

int MAX_PAGES = 50; 
int num_threads = 8;
std::string start_url = "https://en.wikipedia.org/wiki/Crawling";

// --------------------------------------------------------------------------------------------------------------

// content-seen test structures from mercator paper
// upgraded to use readers-writer lock: multiple threads can check simultaneously
ConcurrentFingerprintSet fingerprints;

void worker_thread(SafeQueue& queue, RefinableHashSet& visited, DownloadConfig& config, const std::string target_domain, Downloading_Stats& stats) {
    CrawlTask task;
    double thread_parse_time = 0.0;
    double thread_struct_time = 0.0;
    double thread_download_time = 0.0;
    
    while (queue.pop(task)) {
        int a = pages_crawled.fetch_add(1);
        if (a >= MAX_PAGES) {
            queue.shutdown();
            break;
        }
        //starting time for downloading
        auto dl_start = std::chrono::high_resolution_clock::now();
        std::string html = Downloader::download_url_with_retry(task.url, config, stats);
        auto dl_end = std::chrono::high_resolution_clock::now();
        thread_download_time += std::chrono::duration<double>(dl_end - dl_start).count();
        if (html.empty()) continue; 

        //starting timer for concurrent structures
        auto struct_start = std::chrono::high_resolution_clock::now();
        // content-seen test
        size_t content_hash = std::hash<std::string>{}(html);
        bool is_duplicate = !fingerprints.insert_if_new(content_hash);
        auto struct_end = std::chrono::high_resolution_clock::now();
        thread_struct_time += std::chrono::duration<double>(struct_end - struct_start).count();
        if (is_duplicate) { 
            continue; 
        } 
        
        // starting timer for parsing
        auto parse_start = std::chrono::high_resolution_clock::now();
        std::vector<std::string> raw_links = Parser::extract_links(html);
        auto parse_end = std::chrono::high_resolution_clock::now();
        thread_parse_time += std::chrono::duration<double>(parse_end - parse_start).count();

        int outgoing_count = 0;

        for (const std::string& raw : raw_links) {
            std::string clean_url = Parser::normalize_url(task.url, raw);
            
            // we use the generic domain filter instead of just wikipedia
            if (Parser::is_internal_link(clean_url, target_domain)) {
                outgoing_count++;
                PageData new_page_data = {clean_url, task.depth + 1, task.url, 0, 0};
                
                struct_start = std::chrono::high_resolution_clock::now();
                if (visited.insert_and_check(clean_url, new_page_data)) {
                    CrawlTask new_task = {clean_url, task.depth + 1, task.url};
                    queue.push(new_task);
                }
                visited.increment_incoming(clean_url);
                struct_end = std::chrono::high_resolution_clock::now();
                thread_struct_time += std::chrono::duration<double>(struct_end - struct_start).count();
            }
        }

        struct_start = std::chrono::high_resolution_clock::now();
        visited.update_outgoing(task.url, outgoing_count);
        struct_end = std::chrono::high_resolution_clock::now();
        thread_struct_time += std::chrono::duration<double>(struct_end - struct_start).count();
        std::cout << "Crawled [" << (a + 1) << "/" << MAX_PAGES << "] : " << task.url << "\n";

    }
    std::lock_guard<std::mutex> lock(timing_mtx);
    total_parsing_time += thread_parse_time;
    total_structure_time += thread_struct_time;
    if (thread_download_time > max_download_wall) max_download_wall = thread_download_time;
    if (thread_parse_time    > max_parsing_wall)  max_parsing_wall  = thread_parse_time;
    if (thread_struct_time   > max_struct_wall)   max_struct_wall   = thread_struct_time;
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

    double total_seconds = std::chrono::duration<double>(end_time - start_time).count();
     
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

    std::cout << "\n";
 
    std::cout << "--------------------- Total run time with " << num_threads << " threads ---------------------\n";
    std::cout << "Total elapsed time:                   " << std::fixed << std::setprecision(3) << total_seconds         << " s\n";
    std::cout << "\n";
    std::cout << "  Downloading (slowest thread):          " << std::fixed << std::setprecision(3) << max_download_wall     << " s  ("
              << std::fixed << std::setprecision(1) << 100.0 * max_download_wall / total_seconds << "% of wall time)\n";
    
    std::cout << "  Parsing HTML (slowest thread):         " << std::fixed << std::setprecision(3) << max_parsing_wall      << " s  ("
              << std::fixed << std::setprecision(1) << 100.0 * max_parsing_wall / total_seconds  << "% of wall time)\n";
    
    std::cout << "  Locks & HashSets (slowest thread):     " << std::fixed << std::setprecision(3) << max_struct_wall       << " s  ("
              << std::fixed << std::setprecision(1) << 100.0 * max_struct_wall / total_seconds   << "% of wall time)\n";

    std::cout << "\n";
    std::cout << "--------------------- CPU time summed across all " << num_threads << " threads ---------------------\n";
    std::cout << "  Downloading (sum):                     " << std::fixed << std::setprecision(3) << stats.get_total_download_time() << " s\n";
    std::cout << "  Parsing HTML (sum):                    " << std::fixed << std::setprecision(3) << total_parsing_time    << " s\n";
    std::cout << "  Locks & HashSets (sum):                " << std::fixed << std::setprecision(3) << total_structure_time  << " s\n";
    std::cout << "\n";
 
    return 0;
}