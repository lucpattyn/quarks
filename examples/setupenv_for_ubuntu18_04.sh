#!/bin/bash

# Update the package list
sudo apt-get update -y

# Install build essentials
sudo apt-get install build-essential -y

# Install ninja build
sudo apt-get install ninja-build -y

# Install Boost
sudo apt-get install libboost-system-dev -y

# Install V8
sudo apt-get install libv8-dev -y

# Install RocksDB
sudo apt-get install librocksdb-dev -y

# Install ZeroMQ
sudo apt-get install libzmq3-dev -y

# Install CMake
sudo apt-get install cmake -y

# Install GCC
sudo apt-get install g++ -y

echo "All dependencies have been installed!"

