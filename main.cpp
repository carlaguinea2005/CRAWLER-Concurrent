#include <iostream>
#include <string>
#include <vector>

#include "download_def.hpp"
#include "downloader.hpp"
#include "concurrent_structures.hpp"
#include "parser_analyzer.hpp"


void worker_thread(SafeQueue& queue, StripedHashSet& visited, DownloadConfig& config) {
    CrawlTask task;
    
    //  get the next URL from the queue
    while (queue.pop(task)) {
        
        // download the HTML
        std::string html = Downloader::download_url(task.url, config);
        if (html.empty()) continue; // Skip if download failed
        
        // extract, normalize, and filter links
        std::vector<std::string> raw_links = Parser::extract_links(html);
        int outgoing_count = 0;

        for (const std::string& raw : raw_links) {
            std::string clean_url = Parser::normalize_url(task.url, raw);
            
            if (Parser::wikipedia_filter(clean_url)) {
                outgoing_count++;
                
                // update data structures
                visited.increment_incoming(clean_url);
                
                PageData new_page_data = {clean_url, task.depth + 1, task.url, 1, 0};
                
                // if it's a brand new page we haven't seen before, add it to the queue
                if (visited.insert_and_check(clean_url, new_page_data)) {
                    CrawlTask new_task = {clean_url, task.depth + 1, task.url};
                    queue.push(new_task);
                }
            }
        }
    }
}

int main() {
    // set up the actual threads and initial starting URL 
    return 0;
}