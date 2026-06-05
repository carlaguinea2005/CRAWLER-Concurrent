# Python script to read benchmark results from CSV and 
# generate a graph comparing nb of threads vs time taken to crawl

import matplotlib.pyplot as plt
import csv

threads = []
times = []

with open('benchmark_results.csv', 'r') as f:
    reader = csv.DictReader(f)
    for row in reader:
        threads.append(int(row['Threads']))
        times.append(float(row['TotalTime_s']))

plt.figure(figsize=(8, 5))
plt.plot(threads, times, marker='o', linestyle='-', color='b', linewidth=2)
plt.title('Crawler Performance: Threads vs Total Time')
plt.xlabel('Number of Threads')
plt.ylabel('Total Time (seconds)')
plt.grid(True, linestyle='--', alpha=0.7)
plt.xticks(threads)
plt.tight_layout()
plt.savefig('graph.png')
print('Graph saved as graph.png')
