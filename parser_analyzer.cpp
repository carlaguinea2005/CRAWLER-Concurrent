#include "parser_analyzer.hpp"
#include "concurrent_structures.hpp" 
#include <iostream>
#include <fstream>
#include <algorithm>

// ELISA

std::vector<std::string> Parser::extract_links(const std::string& html) {
    std::vector<std::string> links;
    size_t pos = 0;

    // manual string parsing like a lexer to avoid std::regex which is slow
    while (pos < html.length()) {
        size_t a_tag_start = html.find("<a ", pos);
        if (a_tag_start == std::string::npos) {
            a_tag_start = html.find("<A ", pos);
        }

        if (a_tag_start == std::string::npos) {
            break;
        }

        size_t a_tag_end = html.find(">", a_tag_start);
        if (a_tag_end == std::string::npos) {
            break; 
        }

        std::string tag_content = html.substr(a_tag_start, a_tag_end - a_tag_start);
        
        size_t href_pos = tag_content.find("href=\"");
        char quote = '"';

        if (href_pos == std::string::npos) {
            href_pos = tag_content.find("href='");
            quote = '\'';
        }

        if (href_pos != std::string::npos) {
            size_t url_start = href_pos + 6; // jump past href="
            size_t url_end = tag_content.find(quote, url_start);
            
            if (url_end != std::string::npos) {
                std::string extracted_url = tag_content.substr(url_start, url_end - url_start);
                links.push_back(extracted_url);
            }
        }

        pos = a_tag_end + 1;
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
    std::string clean_link = link;

    size_t hash_pos = clean_link.find('#');
    if (hash_pos != std::string::npos) {
        clean_link = clean_link.substr(0, hash_pos);
    }

    if (clean_link.empty() || clean_link.find("javascript:") == 0 || clean_link.find("mailto:") == 0) {
        return ""; 
    }
    
    std::string final_url = "";

    // case : already an absolute URL
    if (clean_link.find("http://") == 0 || clean_link.find("https://") == 0) {
        final_url = clean_link; 
    }
    // case : protocol-relative URL "//example.com/style.css"
    else if (clean_link.find("//") == 0) {
        size_t scheme_end = base_url.find(":");
        std::string scheme = (scheme_end != std::string::npos) ? base_url.substr(0, scheme_end) : "https";
        final_url = scheme + ":" + clean_link;
    }
    else {
        std::string base_domain = extract_base_domain(base_url);
        if (base_domain.empty()) return "";

        // case : root-relative URL "/about.html"
        if (clean_link.find("/") == 0) {
            final_url = base_domain + clean_link;
        }
        // case : path-relative URL "contact.html" or "../images/logo.png"
        else {
            size_t last_slash = base_url.find_last_of('/');
            if (last_slash != std::string::npos && last_slash > 7) { 
                final_url = base_url.substr(0, last_slash + 1) + clean_link;
            } else {
                final_url = base_url + "/" + clean_link;
            }
        }
    }

    // mercator optimization
    // add default port 80 for standard HTTP if omitted
    if (final_url.find("http://") == 0) {
        // check if port is already specified
        std::string without_protocol = final_url.substr(7);
        size_t first_slash = without_protocol.find('/');
        size_t colon_pos = without_protocol.find(':');
        
        // if there is no colon before the first slash, it lacks a port
        if (colon_pos == std::string::npos || (first_slash != std::string::npos && colon_pos > first_slash)) {
            if (first_slash != std::string::npos) {
                final_url = "http://" + without_protocol.substr(0, first_slash) + ":80" + without_protocol.substr(first_slash);
            } else {
                final_url += ":80";
            }
        }
    }

    return final_url;
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