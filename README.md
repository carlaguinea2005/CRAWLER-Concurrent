# Parallel Crawler (CSE305 Project)

A multithreaded C++ web crawler built for Wikipedia, designed to explore concurrent data structures and network performance bottlenecks.

## Team Members & Responsibilities
* **Carla:** Networking & Downloading 
* **Yasmine:** Concurrent Data Structures 
* **Elisa:** HTML Parsing, Filtering & Benchmarking Analysis

## How to Compile
You must have `libcurl` installed on your system. Compile the project using:
`g++ main.cpp downloader.cpp download_def.cpp concurrent_structures.cpp parser_analyzer.cpp -o crawler -lcurl -pthread`

## How to Run
`./crawler`