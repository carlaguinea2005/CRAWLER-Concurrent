#ifndef DOWNLOADER_HPP
#define DOWNLOADER_HPP

#include <mutex>
#include <string>

// CARLA
// handles all HTTP requests, timeouts, and error handling

//download def

struct DownloadConfig {
    long timeout_seconds;
    long connect_timeout_seconds;
    bool follow_redirects;
    long max_file_size_bytes;

    DownloadConfig();
};


//dowload stats 

class Downloading_Stats {
private:
    int total_downloads;
    int successful_downloads;
    int failed_downloads;

    double total_download_time;
    long total_bytes_downloaded;
    // mutex so multiple threads can update stats safely, since it can hve problem w lock
    mutable std::mutex stats_mutex;             

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
};


//download basis

class Downloader {
public:
    static std::string download_url(const std::string& url, const DownloadConfig& config);
    static std::string download_url(const std::string& url, const DownloadConfig& config, Downloading_Stats& stats);
    static std::string download_url_with_retry(const std::string& url, const DownloadConfig& config, Downloading_Stats& stats, int max_retries = 3, int retry_delay_ms = 500);};

#endif


// g++ main.cpp downloader.cpp download_def.cpp -o crawler -lcurl -pthread
// remember to link with -lcurl and -pthread when compiling, since we are using libcurl and threads.