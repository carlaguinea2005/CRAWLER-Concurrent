#ifndef PARSER_ANALYZER_HPP
#define PARSER_ANALYZER_HPP

#include <string>
#include <vector>
#include "concurrent_structures.hpp"

// ELISA
// extracts links from HTML, filters URLs, and handles stats, creates the CSV report

class Parser {
public:
    static std::vector<std::string> extract_links(const std::string& html);

    static std::string normalize_url(const std::string& base_url, const std::string& link);

    static bool is_internal_link(const std::string& url, const std::string& target_domain);

    static std::string extract_base_domain(const std::string& url);
};

class Benchmarker {
public:
    static void generate_csv(const std::vector<PageData>& all_data, const std::string& filename);
    static void run_benchmark(int num_threads);
};

#endif