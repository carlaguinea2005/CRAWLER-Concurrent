#include "download_basis.hpp"
#include "status_http.hpp"
#include "download_def.hpp"

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
    DownloadConfig config;

    CURL* curl = curl_easy_init();

    if (curl == nullptr) {
        stats.add_failure(0.0);
        std::cerr << "Curl initialization failed for URL: " << url << std::endl;
        return "";
    }
    auto start = std::chrono::high_resolution_clock::now();

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, adding_pieces);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);

    curl_easy_setopt(curl, CURLOPT_TIMEOUT, config.timeout_seconds);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, config.connect_timeout_seconds);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, config.follow_redirects ? 1L : 0L);

    curl_easy_setopt(curl, CURLOPT_MAXFILESIZE_LARGE,
                     static_cast<curl_off_t>(config.max_file_size_bytes));

    curl_easy_setopt(curl, CURLOPT_USERAGENT, "CSE305Crawler/1.0");

    CURLcode curl_result = curl_easy_perform(curl);

    auto end = std::chrono::high_resolution_clock::now();

    double download_time =
        std::chrono::duration<double>(end - start).count();
    if (curl_result != CURLE_OK) {
        stats.add_failure(download_time);
        std::cerr << " failed download for URL: " << url << std::endl;
        std::cerr << "Curl error: " << curl_easy_strerror(curl_result) << std::endl;

        curl_easy_cleanup(curl);
        return "";
    }

    long status_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);

    curl_easy_cleanup(curl);

    if (!is_http_ok(status_code)) {
        stats.add_failure(download_time);

        std::cerr << "HTTP error for URL: "
                  << url
                  << " with status code "
                  << status_code
                  << std::endl;

        return "";
    }

    if (html.empty()) {
        stats.add_failure(download_time);

        std::cerr << "Empty HTML for URL: " << url << std::endl;

        return "";
    }

    stats.add_success(download_time, html.size());

    return html;
}

