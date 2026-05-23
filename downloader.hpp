#ifndef DOWNLOADER_HPP
#define DOWNLOADER_HPP

#include <string>
#include "download_def.hpp"

// CARLA
// handles all HTTP requests, timeouts, and error handling

class Downloader {
public:
    // function: download_url
    // description: takes a URL, uses libcurl to fetch the webpage content, 
    //              returns the raw HTML
    //              should handle errors so the thread doesn't crash if a page is offline.
    // input: const std::string& url (the web address to download "https://en.wikipedia.org/...")
    // output: std::string - The raw HTML of the page. Returns an empty string ("") if it fails.
    static std::string download_url(const std::string& url, const DownloadConfig& config);
};

#endif


// g++ main.cpp downloader.cpp download_def.cpp -o crawler -lcurl -pthread
// remember to link with -lcurl and -pthread when compiling, since we are using libcurl and threads.