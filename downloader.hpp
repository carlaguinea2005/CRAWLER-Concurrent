#ifndef DOWNLOADER_HPP
#define DOWNLOADER_HPP

#include <mutex>
#include <string>

// CARLA
// handles all HTTP requests, timeouts, and error handling

// -----------------------------------------------------------------------
// status_http
// -----------------------------------------------------------------------

bool is_http_ok(long status_code);
bool is_http_redirect(long status_code);
bool is_http_client_error(long status_code);
bool is_http_server_error(long status_code);
void what_status_message(long status_code);

// -----------------------------------------------------------------------
// download_def : we define the parameters we will follow before downloading anything
// -----------------------------------------------------------------------

struct DownloadConfig {
    long timeout_seconds;
    long connect_timeout_seconds;
    bool follow_redirects;
    long max_file_size_bytes;

    DownloadConfig();
};

// -----------------------------------------------------------------------
// download_stats : we want to measure the stats when downloading: what goes wrong, how many files download, how many don't
// -----------------------------------------------------------------------

class Downloading_Stats {
private:
    int total_downloads;
    int successful_downloads;
    int failed_downloads;

    double total_download_time;
    long total_bytes_downloaded;
    // mutex so multiple threads can update stats safely, since it can hve problem w lock
    mutable std::mutex stats_mutex;
    //avoids deadlock when printing stats
    double average_download_time_unsafe() const;
public:
    // Constructor: starts all counters at zero
    Downloading_Stats();
    //if we are trying to do copy it will fail, so we want to avoid that the compiler does this
    Downloading_Stats(const Downloading_Stats&) = delete;
    Downloading_Stats& operator=(const Downloading_Stats&) = delete;
    //downloaded files: attempt, succes and failure counters
    void add_download_attempt();
    void add_success(double download_time, long html_size);
    void add_failure(double download_time);

    //no matter the result before, how much time did it take
    double average_download_time() const;
    void print_stats() const;
    double get_total_download_time() const;
};

// -----------------------------------------------------------------------
// download_basis
// -----------------------------------------------------------------------

class Downloader {
public:
    static std::string download_url(const std::string& url, const DownloadConfig& config);
    static std::string download_url(const std::string& url, const DownloadConfig& config, Downloading_Stats& stats);
    //optimization part: function that measures if it's worth trying the download: with most common cases of mistakes in names
    static std::string download_url_with_retry(const std::string& url, const DownloadConfig& config, Downloading_Stats& stats, int max_retries = 3, int retry_delay_ms = 500);
};

#endif

// g++ main.cpp downloader.cpp concurrent_structures.cpp parser_analyzer.cpp -o crawler -lcurl -pthread