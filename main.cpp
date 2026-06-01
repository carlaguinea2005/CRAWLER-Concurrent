#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include "downloader.hpp"
#include "concurrent_structures.hpp"
#include "parser_analyzer.hpp"

const int MAX_DEPTH = 3;
const int MAX_PAGES = 500;
std::atomic<int> pages_crawled(0);

void worker_thread(SafeQueue& queue, StripedHashSet& visited, DownloadConfig& config) {
    CrawlTask task;
    
    // get the next URL from the queue
    while (queue.pop(task)) {

        // stop if we reached the page limit
        if (pages_crawled >= MAX_PAGES) {
            queue.shutdown();
            break;
        }

        // skip pages that are too deep
        if (task.depth >= MAX_DEPTH) continue;
        
        // download the HTML
        std::string html = Downloader::download_url(task.url, config);
        if (html.empty()) continue;

        pages_crawled++;

        // extract, normalize, and filter links
        std::vector<std::string> raw_links = Parser::extract_links(html);
        int outgoing_count = 0;

        for (const std::string& raw : raw_links) {
            std::string clean_url = Parser::normalize_url(task.url, raw);
            
            if (Parser::wikipedia_filter(clean_url)) {
                outgoing_count++;
                
                // add to hash set first
                PageData new_page_data = {clean_url, task.depth + 1, task.url, 0, 0};
                
                // if it's a brand new page we haven't seen before, add it to the queue
                if (visited.insert_and_check(clean_url, new_page_data)) {
                    CrawlTask new_task = {clean_url, task.depth + 1, task.url};
                    queue.push(new_task);
                }

                // update incoming links after insert
                visited.increment_incoming(clean_url);
            }
        }

        // save outgoing links count for this page
        visited.update_outgoing(task.url, outgoing_count);
    }
}

int main(int argc, char* argv[]) {

    // read number of threads from command line, default is 4
    int num_threads = 4;
    if (argc > 1) {
        num_threads = std::stoi(argv[1]);
    }

    // read start URL from command line
    std::string start_url = "https://en.wikipedia.org/wiki/Computer_science";
    if (argc > 2) {
        start_url = argv[2];
    }

    std::cout << "Starting crawler with " << num_threads << " threads" << std::endl;
    std::cout << "Start URL: " << start_url << std::endl;

    DownloadConfig config;
    SafeQueue queue;
    StripedHashSet visited;

    // push the start URL to kick off the crawl
    CrawlTask first_task;
    first_task.url = start_url;
    first_task.depth = 0;
    first_task.parent_url = "";
    queue.push(first_task);

    auto start_time = std::chrono::high_resolution_clock::now();

    // launch threads
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(worker_thread,
                            std::ref(queue),
                            std::ref(visited),
                            std::ref(config));
    }

    // wait for all threads to finish
    for (std::thread& t : threads) {
        t.join();
    }

    queue.shutdown();

    auto end_time = std::chrono::high_resolution_clock::now();
    double total_seconds = std::chrono::duration_cast<std::chrono::duration<double>>
                          (end_time - start_time).count();

    std::vector<PageData> results = visited.get_all_pages();
    std::cout << "Crawling done!" << std::endl;
    std::cout << "Total pages found: " << results.size() << std::endl;
    std::cout << "Total time: " << total_seconds << " seconds" << std::endl;
    std::cout << "Pages per second: " << results.size() / total_seconds << std::endl;

    // write results to CSV
    Benchmarker::generate_csv(results, "output.csv");
    std::cout << "CSV written to output.csv" << std::endl;

    return 0;
}