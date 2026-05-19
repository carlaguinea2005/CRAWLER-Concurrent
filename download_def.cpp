#include "download_def.hpp"

DownloadConfig::DownloadConfig() {
    
    timeout_seconds = 10; //max total time
    connect_timeout_seconds = 5;//max total time per connexion
    follow_redirects = true; //allow redirects 
    max_file_size_bytes = 5 * 1024 * 1024; //MAXIMUM SIZE FOR DOWNLOADING FILES (IN OUR CASE 5MB)
}