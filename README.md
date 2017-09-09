# BellmanFord
Implementation of BellmanFord algorithm to find shortest paths tree simulating an asynchronous network

It's a multithreaded C application where every thread simulates a node engaging in data communication in order to find the shortest path from a source node to every other node in the graph.

Technical details in order to successfully execute the project:

The project contains the following C files :

- README
- simulator_process.c
- config.txt (sample file for reference)
- a.out (executable file just for reference)

**********************************************************************************************
COMPILATION PROCESS :

gcc simulator_process.c -lpthread

This will produce "a.out" binary file
**********************************************************************************************

**********************************************************************************************
EXECUTION PROCESS

./a.out *file name*

For example :-

./a.out config.txt

Here,
config.txt is on local machine (present working directory)
**********************************************************************************************

**********************************************************************************************
OUTPUT [The output is for the config.txt file included in this project]

---Printing adjacency list for [ROOT NODE = 0] ---

0 --> 4 --> 1 [Distance from root = 1]
0 --> 2 [Distance from root = 0]
0 --> 2 --> 3 [Distance from root = 1]
0 --> 4 [Distance from root = 1]
0 --> 4 --> 5 [Distance from root = 1]
0 --> 2 --> 9 --> 6 [Distance from root = 2]
0 --> 7 [Distance from root = 2]
0 --> 2 --> 3 --> 8 [Distance from root = 1]
0 --> 2 --> 9 [Distance from root = 1]

**********************************************************************************************

THANK YOU
