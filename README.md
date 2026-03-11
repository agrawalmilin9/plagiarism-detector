# C++ Plagiarism Detector

A command-line tool that detects similarity between C++ source code files.

## Features
- Token-based code comparison
- Multi-threaded file comparison
- Similarity scoring between programs
- Detects possible plagiarism in programming assignments

## Build

mkdir build
cd build
cmake ..
make

## Run

./plagcheck file1.cpp file2.cpp file3.cpp

Example:

./plagcheck samples/sample_a.cpp samples/sample_b.cpp samples/sample_c.cpp

## Technologies Used
- C++
- CMake
- Multithreading

## Author
Milin Agrawal
