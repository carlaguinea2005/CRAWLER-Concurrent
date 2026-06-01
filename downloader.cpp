#include "downloader.hpp"
#include <curl/curl.h>
#include <iostream>
#include <chrono>
#include <thread>

// CARLA

// dowload_def

DownloadConfig::DownloadConfig() {
    
    timeout_seconds = 10; //max total time
    connect_timeout_seconds = 5;//max total time per connexion
    follow_redirects = true; //allow redirects 
    max_file_size_bytes = 5 * 1024 * 1024; //MAXIMUM SIZE FOR DOWNLOADING FILES (IN OUR CASE 5MB)
}

// downloading stat part 

Downloading_Stats::Downloading_Stats() {
    total_downloads = 0;
    successful_downloads = 0;
    failed_downloads = 0;
    total_download_time = 0.0;
    total_bytes_downloaded = 0;
}

void Downloading_Stats::add_download_attempt() {
    std::lock_guard<std::mutex> lock(stats_mutex);
    total_downloads++;
}

void Downloading_Stats::add_success(double download_time, long html_size) {
    std::lock_guard<std::mutex> lock(stats_mutex);
    successful_downloads++;
    total_download_time += download_time;
    total_bytes_downloaded += html_size;
}

void Downloading_Stats::add_failure(double download_time) {
    std::lock_guard<std::mutex> lock(stats_mutex);
    failed_downloads++;
    total_download_time += download_time;
}

double Downloading_Stats::average_download_time() const {
    if (total_downloads == 0) {
        return 0.0;
    }
    return total_download_time / total_downloads;
}

//we want to do the statistics at the end of the main function, so we want to print them in a nice way
void Downloading_Stats::print_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex);
    std::cout << "Download statistics:" << std::endl;
    std::cout << "Total download attempts: " << total_downloads << std::endl;
    std::cout << "Successful downloads: " << successful_downloads << std::endl;
    std::cout << "Failed downloads: " << failed_downloads << std::endl;
    std::cout << "Average download time: " << average_download_time() << " seconds" << std::endl;
    std::cout << "Total bytes downloaded: " << total_bytes_downloaded << std::endl;
}


// download basis part

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total_size = size * nmemb; 
    std::string* html = static_cast<std::string*>(userp); 
    html->append(static_cast<char*>(contents), total_size); 
    return total_size;
};

std::string Downloader::download_url(const std::string& url, const DownloadConfig& config) {
    Downloading_Stats stats; 
    return download_url(url, config, stats);

};

std::string Downloader::download_url(const std::string& url, const DownloadConfig& config, Downloading_Stats& stats) {
    stats.add_download_attempt();
    std::string html;

    CURL* curl = curl_easy_init();

    if (curl == nullptr) {
        stats.add_failure(0.0);
        std::cerr << "Curl initialization failed for URL: " << url << std::endl;
        return "";
    }
    
    auto start = std::chrono::high_resolution_clock::now();

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);

    curl_easy_setopt(curl, CURLOPT_TIMEOUT, config.timeout_seconds);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, config.connect_timeout_seconds);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, config.follow_redirects ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_MAXFILESIZE_LARGE, static_cast<curl_off_t>(config.max_file_size_bytes));
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "CSE305Crawler/1.0");

    CURLcode curl_result = curl_easy_perform(curl);
    auto end = std::chrono::high_resolution_clock::now();
    double download_time = std::chrono::duration<double>(end - start).count();

    if (curl_result != CURLE_OK) {
        stats.add_failure(download_time);
        std::cerr << "Failed download for URL: " << url << std::endl;
        std::cerr << "Curl error: " << curl_easy_strerror(curl_result) << std::endl;
        curl_easy_cleanup(curl);
        return "";
    }

    long status_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
    curl_easy_cleanup(curl);

    bool is_http_ok = (status_code >= 200 && status_code < 300);

    if (!is_http_ok) {
        stats.add_failure(download_time);
        std::cerr << "HTTP error for URL: " << url << " with status code " << status_code << std::endl;
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

//just in case there are connexion errors, errors that dont come from the url, we try various times 
std::string Downloader::download_url_with_retry(const std::string& url, const DownloadConfig& config, Downloading_Stats& stats, int max_retries, int retry_delay_ms) {
    for (int attempt = 1; attempt <= max_retries; attempt++) {
        std::cout << "Attempt " << attempt << " for URL: " << url << std::endl;
        
        std::string html = download_url(url, config, stats);
        if (!html.empty()) {
            return html;
        }
        
        if (attempt < max_retries) {
            //we do a pause before retrying, to avoid doing too many requests in a short time, which can cause more problems
            std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
        }
    }
    std::cerr << "unable to download url: " << url << std::endl;
    return "";
}

