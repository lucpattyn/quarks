#!/bin/bash

# Update the package list
sudo apt-get update -y

# Install build essentials
sudo apt-get install build-essential -y

# Install ninja build
sudo apt-get install ninja-build -y

# Install required dependencies for Boost
sudo apt-get install libbz2-dev libicu-dev libssl-dev -y

# Download Boost 1.69
wget https://dl.bintray.com/boostorg/release/1.69.0/source/boost_1_69_0.tar.gz

# Unpack the Boost source code
tar -xzf boost_1_69_0.tar.gz

# Build Boost
cd boost_1_69_0
./bootstrap.sh
./b2 install
cd ..

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


