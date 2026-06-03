#ifndef DOWNLOAD_HPP
#define DOWNLOAD_HPP

#include <string>
#include "download_stats.hpp"

std::string download_url(const std::string& url);
std::string download_url(const std::string& url, Downloading_Stats& stats);
std::string retries_download_url(const std::string& url,Downloading_Stats& stats,int max_retries = 3,int retry_delay_ms = 500);
//optimization part: function that measures if it's worth trying the download: with most common cases of mistakes in names
bool is_valid_url(const std::string& url);
#endif