//we want to measure the stats when downloading: what goes wrong, how many files download, how many don't 
#ifndef DOWNLOAD_STATS_HPP
#define DOWNLOAD_STATS_HPP
#include <mutex>

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

#endif