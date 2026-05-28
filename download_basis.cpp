#include "download_basis.hpp"
#include "status_http.hpp"

#include <curl/curl.h>
#include <chrono>
#include <iostream>
#include <string>

// first: we have that libcurl downloads the data bit by bit, so we need to add them into an html 
//string

static size_t adding_pieces(void* contents, size_t size, size_t nmemb, void* userp){
    size_t total_size= size* nmemb; 
    std::string* html = (std::string*) userp; 
    html->append((char*)contents, total_size); 
    return total_size; 
}; 

// in this function we download the url and store it, but in the header file we want to also take
//into account the stats that come with each URL
std::string download_url(const std::string& url){
    Downloading_Stats stats;
    std::string html = download_url(url, stats);
    return html;
}
// we want ot actually do the main download 
std::string download_url(const std::string& url, Downloading_Stats& stats) {
    stats.add_download_attempt();
    std::string html;
    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        stats.add_failure(0.0);
        std::cerr << "Curl initialization failed for URL: " << url << std::endl;
        return "";
    }
    return html;
}

