#include "downloader.hpp"
#include <curl/curl.h>
#include <iostream>
#include <chrono>
#include <thread>

// CARLA

// -----------------------------------------------------------------------
// status_http
// -----------------------------------------------------------------------

bool is_http_ok(long status_code) {
    return status_code >= 200 && status_code < 300;
}

bool is_http_redirect(long status_code) {
    return status_code >= 300 && status_code < 400;
}

bool is_http_client_error(long status_code) {
    return status_code >= 400 && status_code < 500;
}

bool is_http_server_error(long status_code) {
    return status_code >= 500 && status_code < 600;
}

void what_status_message(long status_code) {
    if (is_http_ok(status_code)) {
        std::cout << "http status: it is a success" << std::endl;
    }
    else if (is_http_redirect(status_code)) {
        std::cout << "HTTP status: redirect" << std::endl;
    }
    else if (is_http_client_error(status_code)) {
        std::cout << "HTTP status: client error" << std::endl;
        if (status_code == 400) {
            std::cout << "bad request" << std::endl;
        }
        else if (status_code == 401) {
            std::cout << " access not authorized " << std::endl;
        }
        else if (status_code == 403) {
            std::cout << "forbidden page" << std::endl;
        }
        else if (status_code == 404) {
            std::cout << "page not found" << std::endl;
        }
    }
    else if (is_http_server_error(status_code)) {
        std::cout << "http status: server error" << std::endl;
    }
    else {
        std::cout << "http status: unknown response" << std::endl;
    }
}

// -----------------------------------------------------------------------
// download_def
// -----------------------------------------------------------------------

DownloadConfig::DownloadConfig() {
    timeout_seconds = 10; //max total time
    connect_timeout_seconds = 5;//max total time per connexion
    follow_redirects = true; //allow redirects 
    max_file_size_bytes = 5 * 1024 * 1024; //MAXIMUM SIZE FOR DOWNLOADING FILES (IN OUR CASE 5MB)
}

// -----------------------------------------------------------------------
// download_stats
// -----------------------------------------------------------------------

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

// i implement the public version that acquires the lock, then delegates to the unsafe helper
double Downloading_Stats::average_download_time_unsafe() const {
    if (total_downloads == 0) {
        return 0.0;
    }
    return total_download_time / total_downloads;
}

<<<<<<< HEAD
double Downloading_Stats::average_download_time() const {
    std::lock_guard<std::mutex> lock(stats_mutex);
    return average_download_time_unsafe();
=======
double Downloading_Stats::get_total_download_time() const {
    std::lock_guard<std::mutex> lock(stats_mutex);
    return total_download_time;
>>>>>>> d33b000214b820f5f621a974478a255c01586a08
}

//we want to do the statistics at the end of the main function, so we want to print them in a nice way
void Downloading_Stats::print_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex);
<<<<<<< HEAD
    std::cout << "Download statistics:" << std::endl;
=======
    double average = 0.0;
    if (total_downloads > 0) {
        average = total_download_time / total_downloads;
    }
    std::cout << "\n";
    std::cout << "--------------------Download statistics--------------------" << std::endl;
    std::cout << "\n";
>>>>>>> d33b000214b820f5f621a974478a255c01586a08
    std::cout << "Total download attempts: " << total_downloads << std::endl;
    std::cout << "Successful downloads: " << successful_downloads << std::endl;
    std::cout << "Failed downloads: " << failed_downloads << std::endl;
    std::cout << "Average download time: "
              << average_download_time_unsafe()
              << " seconds" << std::endl;
    std::cout << "Total bytes downloaded: "
              << total_bytes_downloaded
              << std::endl;
}

// -----------------------------------------------------------------------
// download_basis
// -----------------------------------------------------------------------

//before calling url, we see if it's worth trying 
static bool is_valid_url(const std::string& url) {
    if (url.empty()) {return false;
    }
    bool starts_with_http =
        url.rfind("http://", 0) == 0;
    bool starts_with_https =
        url.rfind("https://", 0) == 0;
    if (!starts_with_http && !starts_with_https) {
        return false;
    }
    if (url.find(' ') != std::string::npos) {
        return false;
    }
    if (url.find('.') == std::string::npos) {
        return false;
    }
    return true;
}

// first: we have that libcurl downloads the data bit by bit, so we need to add them into an html 
//string
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp){
    size_t total_size= size* nmemb; 
    std::string* html = (std::string*) userp; 
    html->append((char*)contents, total_size); 
    return total_size; 
}

// in this function we download the url and store it, but in the header file we want to also take
//into account the stats that come with each URL
std::string Downloader::download_url(const std::string& url, const DownloadConfig& config) {
    Downloading_Stats stats; 
    return download_url(url, config, stats);
}

// we want to actually do the main download 
std::string Downloader::download_url(const std::string& url, const DownloadConfig& config, Downloading_Stats& stats) {
    //validity of teh url
    if (!is_valid_url(url)) {
        stats.add_failure(0.0);
        std::cerr << "invalid url (rejected before the download): " << url << std::endl;
        return "";
    }
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
    what_status_message(status_code); //type of error if we find one, helper function above
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

//just in case there are connexion errors, errors that dont come from the url, we try various times 
std::string Downloader::download_url_with_retry(const std::string& url, const DownloadConfig& config, Downloading_Stats& stats, int max_retries, int retry_delay_ms) {
    for (int attempt = 1; attempt <= max_retries; attempt++) {
        std::cout << "attempt " << attempt << " for url: " << url << std::endl;
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
