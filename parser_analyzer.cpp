#include "parser_analyzer.hpp"
#include "concurrent_structures.hpp" // To know what PageData looks like
#include <regex>
#include <iostream>
#include <fstream>

// ELISA

std::vector<std::string> Parser::extract_links(const std::string& html) {
    
};

std::string Parser::normalize_url(const std::string& base_url, const std::string& link) {
    
};

bool Parser::wikipedia_filter(const std::string& url) {
    
};

void Benchmarker::generate_csv(const std::vector<PageData>& all_data, const std::string& filename) {

};

void Benchmarker::run_benchmark(int num_threads) {

};