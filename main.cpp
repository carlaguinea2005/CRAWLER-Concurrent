#include "download_basis.hpp"
#include "download_stats.hpp"

#include <iostream>
#include <string>

int main() {
    Downloading_Stats stats;
    //not corrupted one 
    std::string url = "https://en.wikipedia.org/wiki/Web_crawler";
    std::string html = download_url(url, stats);
    if (!html.empty()) {
        std::cout << "Download worked!" << std::endl;
        std::cout << "the html size is: " << html.size() << " with characters" << std::endl;
    } else {
        std::cout << "Download failed :(." << std::endl;
    }
    stats.print_stats();
    return 0;
}