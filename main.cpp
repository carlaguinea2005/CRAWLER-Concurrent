#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>

#include "downloader.hpp"
#include "concurrent_structures.hpp"
#include "parser_analyzer.hpp"

std::atomic<int> pages_crawled(0);

//--------------------------------------------------------------------------------------------------------------
// choose max pages to crawl, number of threads, and starting URL here

const int MAX_PAGES = 50; 
int threads = 4;
std::string start_url = "https://en.wikipedia.org/wiki/Crawling";

// --------------------------------------------------------------------------------------------------------------

void worker_thread(SafeQueue& queue, StripedHashSet& visited, DownloadConfig& config, const std::string target_domain) {
    CrawlTask task;
    
    while (queue.pop(task)) {
        int a = pages_crawled.fetch_add(1);
        if (a >= MAX_PAGES) {
            queue.shutdown();
            break;
        }

        std::string html = Downloader::download_url(task.url, config);
        if (html.empty()) continue; 
        
        std::vector<std::string> raw_links = Parser::extract_links(html);
        int outgoing_count = 0;

        for (const std::string& raw : raw_links) {
            std::string clean_url = Parser::normalize_url(task.url, raw);
            
            // we use the generic domain filter instead of just wikipedia
            if (Parser::is_internal_link(clean_url, target_domain)) {
                outgoing_count++;
                visited.increment_incoming(clean_url);
                
                PageData new_page_data = {clean_url, task.depth + 1, task.url, 1, 0};
                
                if (visited.insert_and_check(clean_url, new_page_data)) {
                    CrawlTask new_task = {clean_url, task.depth + 1, task.url};
                    queue.push(new_task);
                }
            }
        }

        visited.update_outgoing(task.url, outgoing_count);
        std::cout << "Crawled [" << (a + 1) << "/" << MAX_PAGES << "] : " << task.url << "\n";
    }
}

int main() {
    SafeQueue queue;
    StripedHashSet visited;
    DownloadConfig config;
    
    std::string url = start_url;
    std::string target_domain = Parser::extract_base_domain(start_url);
    
    PageData root_data = {url, 0, "NONE", 1, 0};
    visited.insert_and_check(url, root_data);
    queue.push({url, 0, "NONE"});
    
    int num_threads = threads;
    std::vector<std::thread> threads;
    
    std::cout << "Starting multithreaded crawler on " << target_domain << " with " << num_threads << " threads :\n";
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker_thread, std::ref(queue), std::ref(visited), std::ref(config), target_domain);
    }
    
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }
    
    std::cout << "\nCrawling finished.\n";
    std::vector<PageData> final_data = visited.get_all_pages();
    Benchmarker::generate_csv(final_data, "crawler_results.csv");
    
    return 0;
}