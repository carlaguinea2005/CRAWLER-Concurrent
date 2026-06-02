#include "parser_analyzer.hpp"
#include "concurrent_structures.hpp" 
#include <regex>
#include <iostream>
#include <fstream>

// ELISA

std::vector<std::string> Parser::extract_links(const std::string& html) {
    std::vector<std::string> links;
    std::regex link_regex(R"(href\s*=\s*["']([^"']+)["'])");
    
    auto words_begin = std::sregex_iterator(html.begin(), html.end(), link_regex);
    auto words_end = std::sregex_iterator();

    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        std::smatch match = *i;
        links.push_back(match[1].str());
    }
    
    return links;
}

std::string Parser::extract_base_domain(const std::string& url) {
    size_t pos = url.find("://");
    if (pos == std::string::npos) return ""; // invalid URL 
    
    // find the next slash after "://"
    size_t start = pos + 3;
    size_t end = url.find('/', start);
    
    if (end == std::string::npos) {
        return url; // the URL is just the domain
    }
    return url.substr(0, end); // returns "https://example.com"
}

std::string Parser::normalize_url(const std::string& base_url, const std::string& link) {
    if (link.empty() || link[0] == '#' || link.find("javascript:") == 0 || link.find("mailto:") == 0) {
        return ""; 
    }
    
    // case : already an absolute URL
    if (link.find("http://") == 0 || link.find("https://") == 0) {
        return link; 
    }
    
    // case : protocol-relative URL "//example.com/style.css"
    if (link.find("//") == 0) {
        size_t scheme_end = base_url.find(":");
        std::string scheme = (scheme_end != std::string::npos) ? base_url.substr(0, scheme_end) : "https";
        return scheme + ":" + link;
    }

    std::string base_domain = extract_base_domain(base_url);
    if (base_domain.empty()) return "";

    // case : root-relative URL "/about.html"
    if (link.find("/") == 0) {
        return base_domain + link;
    }
    
    // case : path-relative URL "contact.html" or "../images/logo.png"
    size_t last_slash = base_url.find_last_of('/');
    if (last_slash != std::string::npos && last_slash > 7) { 
        return base_url.substr(0, last_slash + 1) + link;
    } else {
        return base_url + "/" + link;
    }
}

bool Parser::is_internal_link(const std::string& url, const std::string& target_domain) {
    return url.find(target_domain) == 0;
}

void Benchmarker::generate_csv(const std::vector<PageData>& all_data, const std::string& filename) {
std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for writing.\n";
        return;
    }

    file << "URL,Depth,Parent,IncomingLinks,OutgoingLinks\n";
    for (const auto& page : all_data) {
        file << page.url << "," 
             << page.depth << "," 
             << page.parent << "," 
             << page.incoming_links << "," 
             << page.outgoing_links << "\n";
    }
    file.close();
    std::cout << "Successfully wrote " << all_data.size() << " pages to " << filename << "\n";
}

void Benchmarker::run_benchmark(int num_threads) {
    std::cout << "Starting benchmark with " << num_threads << " threads :\n";
}
