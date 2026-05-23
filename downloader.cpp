#include "downloader.hpp"
#include <curl/curl.h>
#include <iostream>

// CARLA
// callback function that libcurl uses
// every time it downloads a chunk of data, it runs this function to save it

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
};

std::string Downloader::download_url(const std::string& url, const DownloadConfig& config) {
};

