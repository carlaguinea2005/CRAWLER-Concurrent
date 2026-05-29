#include "download_basis.hpp"
#include "download_stats.hpp"

#include <iostream>
#include <string>

int main() {
    Downloading_Stats stats;
    //not corrupted one 
    std::vector<std::string> urls = {
        "https://en.wikipedia.org/wiki/Web_crawler",
        "https://www.corruption.com",
        "https://www.blablablaa.com"
    };
    for (const std::string& url : urls) {
        std::cout << "Trying to download the https: " << url << std::endl;

        std::string html = download_url(url, stats);
        if (!html.empty()) {
            std::cout << "download has worked!" << std::endl;
            std::cout << "the HTML size is: " << html.size() << " characters" << std::endl;
        } 
        else {
            std::cout << "Failed download" << std::endl;
        }
        std::cout << "--------------------------------" << std::endl;
    }
    stats.print_stats();
    return 0;
}