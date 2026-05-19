#ifndef DOWNLOAD_DEF_HPP
#define DOWNLOAD_DEF_HPP
// we define the parameters we will follow before downloading anything 

#include <string>

struct DownloadConfig {
    long timeout_seconds;
    long connect_timeout_seconds;
    bool follow_redirects;
    long max_file_size_bytes;

    DownloadConfig();
};

#endif