# Parallel Crawler (CSE305 Project)

A multithreaded C++ web crawler built for Wikipedia, designed to explore concurrent data structures and network performance bottlenecks.

## Team Members & Responsibilities
* **Carla:** Networking & Downloading 
* **Yasmine:** Concurrent Data Structures 
* **Elisa:** HTML Parsing, Filtering & Benchmarking Analysis

## How to Compile
You must have `libcurl` installed on your system. Compile the project using:
`g++ main.cpp downloader.cpp concurrent_structures.cpp parser_analyzer.cpp -o crawler -lcurl -pthread`

## How to Run
`./crawler` will run with default arguments being 4 threads, 50 pages and a starting url : "https://en.wikipedia.org/wiki/Crawling"

`./crawler [max_pages] [num_threads] [start_url]` allows to choose the parameters  

## Benchmarking

In order to analyse the behavior of our implementation depending on the number of threads involved, 
one can compile the file benchmarking.cpp as follows 

`g++ benchmarking.cpp -o benchmark`

then run it :

 `./benchmark`

The table will be printed.
The outputs is the benchmark.csv file.
