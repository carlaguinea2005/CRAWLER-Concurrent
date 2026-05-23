#ifndef PARSER_ANALYZER_HPP
#define PARSER_ANALYZER_HPP

#include <string>
#include <vector>
#include "concurrent_structures.hpp"

// ELISA
// extracts links from HTML, filters Wikipedia URLs, and handles stats, creates the CSV report

class Parser {
public:
    // function extract_links : searches through raw HTML to find all "href" tags
    // input: const std::string& html (the raw webpage content from Carla)
    // output: std::vector<std::string> (list of all raw links found on the page)
    static std::vector<std::string> extract_links(const std::string& html);

    // function normalize_url : converts relative links example: "https://en.wikipedia.org/wiki/Cat"
    // input: const std::string& base_url (the page the link was found on)
    //        const std::string& link (the extracted raw link)
    // output: std::string (usable URL)
    static std::string normalize_url(const std::string& base_url, const std::string& link);

    // function wikipedia_filter : checks if the URL belongs to Wikipedia and is a valid article 
    // input: const std::string& url (the normalized URL)
    // output: bool ( True if it's a valid Wikipedia article, False otherwise )
    static bool wikipedia_filter(const std::string& url);
};

class StripedHashSet;

class Benchmarker {
public:
    // function generate_csv : takes Yasmine's final Hash Table data and saves it to a file.
    // input: const StripedHashSet& final_data, const std::string& filename
    // output: void
    static void generate_csv(const std::vector<PageData>& all_data, const std::string& filename);
    // function run_benchmark : coordinates the tests for 1, 2, 4, 8, and 16 threads
    // input: int num_threads 
    // output: void (prints runtime, pages/sec, and hardware limits to the console)
    static void run_benchmark(int num_threads);
};

#endif