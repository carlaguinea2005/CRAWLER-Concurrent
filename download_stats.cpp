#include "download_stats.hpp"
#include <iostream>

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
    std::cout << "Average download time: "
              << average_download_time()
              << " seconds" << std::endl;
    std::cout << "Total bytes downloaded: "
              << total_bytes_downloaded
              << std::endl;
}
