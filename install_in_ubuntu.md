---
title: Install Quarks in Ubuntu 20.04 and above
published: true
description: Step by step instructions to set up Quarks (https://github.com/lucpattyn/quarks) in Ubuntu Servers 20.04 and above
tags: 
# cover_image: https://direct_url_to_image.jpg
# Use a ratio of 100:42 for best results.
# published_at: 2025-03-19 11:52 +0000
---
# Installing and Running Quarks Database on Ubuntu

Quarks is a lightweight, high-performance database designed for specific workloads. This tutorial will guide you through installing and running Quarks on an Ubuntu system.

## Prerequisites

Before installing Quarks, ensure that your system is up to date. Run the following command:

sudo apt-get update -y

## Step 1: Install Required Dependencies

Quarks requires several essential dependencies to be installed before proceeding with the build.

```
sudo apt install cmake  
sudo apt-get install build-essential  
sudo apt-get install ninja-build  
```
These packages include essential build tools like CMake, Ninja, and build-essential.

## Step 2: Install Boost 1.69

Quarks depends on Boost libraries, so we need to install Boost 1.69 manually.

### Download Boost 1.69

Navigate to /usr/local/src and download the Boost 1.69 source code using wget:
```
cd /usr/local/src  
sudo wget -O boost_1_69_0.tar.gz https://archives.boost.io/release/1.69.0/source/boost_1_69_0.tar.gz  
```
### Extract the Archive

Unpack the downloaded archive:
```
sudo tar -xvzf boost_1_69_0.tar.gz  
cd boost_1_69_0  
```
### Build and Install Boost

Run the following commands to bootstrap and install Boost:
```
sudo ./bootstrap.sh --prefix=/usr/local  
sudo ./b2 install  
```
This process will compile and install Boost in /usr/local.

## Step 3: Install Additional Dependencies

Install other required dependencies:
```
sudo apt-get install libv8-dev  # (Optional: Only needed if using the V8 engine)  
sudo apt-get install librocksdb-dev  
sudo apt-get install libzmq3-dev  
```
These libraries are required for Quarks' functionality.

## Step 4: Clone and Build Quarks

### Clone the Repository

Retrieve the Quarks source code from GitHub:
```
git clone https://github.com/lucpattyn/quarks.git  
cd quarks  
```
### Build Quarks

Create a build directory and use CMake with Ninja to compile Quarks:
```
mkdir build  
cd build  
cmake .. -G Ninja  
ninja  
```
## Step 5: Run Quarks

Once the build is complete, you can run Quarks using the following command:
```
./ocv_microservice_crow  
```
This will start the Quarks microservice.

## Conclusion

By following this guide, you have successfully installed and run Quarks on your Ubuntu system. If you encounter any issues, ensure that all dependencies are correctly installed and try re-running the build steps.


