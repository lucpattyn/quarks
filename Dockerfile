# Use an official Ubuntu 18.04 image as a base
FROM ubuntu:18.04

# Update the package lists
RUN apt-get update -y

# # Install build essentials and necessary dependencies
RUN apt-get install -y \
    build-essential \
    ninja-build \
    libboost-system-dev \
    librocksdb-dev \
    libzmq3-dev \
    cmake

# Set the working directory inside the container
WORKDIR /usr/app

# Copy the source code into the container
COPY . .

# Compile the C++ code (assuming you have a CMakeLists.txt file)
RUN mkdir build && cd build && cmake .. -G Ninja && ninja



# Command to run your application
CMD ["./build/ocv_microservice_crow", "-timeout", "125"]
