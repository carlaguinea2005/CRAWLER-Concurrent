#ifndef DOWNLOAD_HPP
#define DOWNLOAD_HPP

#include <string>
#include "download_stats.hpp"

std::string download_url(const std::string& url);
std::string download_url(const std::string& url, Downloading_Stats& stats);

#endif