#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <cstdio>
#include <iomanip>
#include <stdexcept>
#include <fstream> 


// ELISA

// execute a shell command and returns the console output as a string
std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    
    // used an LLM to figure out how to open a pipe to read the command output and debug
    auto deleter = [](FILE* file) { pclose(file); };
    std::unique_ptr<FILE, decltype(deleter)> pipe(popen(cmd, "r"), deleter);
    if (!pipe) {
        throw std::runtime_error("popen() failed! Could not execute command.");
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

// parser to find a keyword and extract the number that comes after it
double extract_metric(const std::string& output, const std::string& keyword) {
    size_t pos = output.find(keyword);
    if (pos != std::string::npos) {
        try {
            return std::stod(output.substr(pos + keyword.length()));
        } catch (...) {
            return 0.0;
        }
    }
    return 0.0;
}

// generates and runs a Python script to plot the CSV data
void generate_graph(const std::string& csv_filename) {

    std::cout << "Generating graph...\n";
    int result = system("python3 plot.py");
    
    if (result != 0) {
        std::cerr << "Failed to generate graph. Do you have matplotlib installed? (pip3 install matplotlib)\n";
    }
}

int main() {
    std::string executable = "./crawler";
    std::vector<std::string> max_pages = {"50"};
    std::string start_url = "https://en.wikipedia.org/wiki/Crawling";
    std::vector<int> thread_counts = {1, 2, 3, 4, 8, 16};
    std::string output_csv = "benchmark_results.csv";

    std::ofstream csv_file(output_csv);
    if (!csv_file.is_open()) {
        std::cerr << "Failed to open " << output_csv << " for writing.\n";
        return 1;
    }

    csv_file << "Threads,TotalTime_s,PagesPerSec,Speedup\n";

    // LLM helped me display the table header
    std::cout << "Benchmarking " << executable << " for " << max_pages[0] << " pages...\n";
    std::cout << std::string(65, '-') << "\n";
    std::cout << std::left << std::setw(10) << "Threads" 
              << "| " << std::setw(15) << "Total Time (s)" 
              << "| " << std::setw(15) << "Pages/sec" 
              << "| " << std::setw(10) << "Speedup" << "\n";
    std::cout << std::string(65, '-') << "\n";

    double base_time = 0.0;

    for (int threads : thread_counts) {

        std::string cmd = executable + " " + max_pages[0] + " " + std::to_string(threads) + " " + start_url;
        
        std::string output = exec(cmd.c_str());

        double total_time = extract_metric(output, "Total time: ");
        double pages_per_sec = extract_metric(output, "Pages per second: ");
        double speedup = 0.0;

        if (total_time > 0.0) {
            if (threads == 1) {
                base_time = total_time;
                speedup = 1.0;
            } else {
                speedup = base_time / total_time;
            }
            // LLM helped me format the output into a nice table and write the results to a CSV file for later analysis 
            // print the formatted raw to console
            std::cout << std::left << std::setw(10) << threads 
                      << "| " << std::setw(15) << std::fixed << std::setprecision(4) << total_time 
                      << "| " << std::setw(15) << std::fixed << std::setprecision(2) << pages_per_sec 
                      << "| " << std::setw(10) << std::fixed << std::setprecision(2) << speedup << "x\n";

            // write the raw data to CSV
            csv_file << threads << "," 
                     << std::fixed << std::setprecision(4) << total_time << "," 
                     << std::fixed << std::setprecision(2) << pages_per_sec << "," 
                     << std::fixed << std::setprecision(4) << speedup << "\n";

        } else {
            std::cout << std::left << std::setw(10) << threads 
                      << "| Error parsing output. Did the crawler run correctly?\n";
        }
    }

    csv_file.close();
    std::cout << std::string(65, '-') << "\n";
    std::cout << "Data saved to " << output_csv << "\n";

    generate_graph(output_csv);

    return 0;
}