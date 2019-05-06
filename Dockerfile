FROM ubuntu:19.04

RUN \
    apt update && \
    apt install -y build-essential doxygen git curl wget gdb ninja-build python3 bash xxd ripgrep vim zlib1g-dev libbz2-dev pkg-config libjpeg-dev \
        libpng-dev libtiff-dev libavcodec-dev libavformat-dev libswscale-dev libv4l-dev libxvidcore-dev libx264-dev libatlas-base-dev gfortran python3-dev \
        unzip libgtk-3-dev libtcmalloc-minimal4 openssl libssl-dev

RUN ln -s /usr/lib/libtcmalloc_minimal.so.4 /usr/lib/libtcmalloc_minimal.so

# CMake
RUN wget -O - https://github.com/Kitware/CMake/releases/download/v3.14.0/cmake-3.14.0-Linux-x86_64.tar.gz | tar xz && mv cmake-3.14.0-Linux-x86_64 /opt/cmake
RUN \
    update-alternatives --install /usr/bin/cmake cmake /opt/cmake/bin/cmake 30 && \
    update-alternatives --install /usr/bin/ccmake ccmake /opt/cmake/bin/ccmake 30 && \
    update-alternatives --install /usr/bin/cmake-gui cmake-gui /opt/cmake/bin/cmake-gui 30 && \
    update-alternatives --install /usr/bin/cpack cpack /opt/cmake/bin/cpack 30 && \
    update-alternatives --install /usr/bin/ctest ctest /opt/cmake/bin/ctest 30

# Boost
RUN \
    cd /tmp && \
    mkdir -p boost && \
    cd boost && \
    echo "downloading boost ..." && \
    wget -qO- https://dl.bintray.com/boostorg/release/1.69.0/source/boost_1_69_0.tar.bz2 | tar xj && \
    cd boost_1_69_0 && \
    ./bootstrap.sh && \
    ./b2 variant=release link=static threading=multi --layout=system --with-system --with-stacktrace --with-coroutine install && \
    cd && \
    rm -rf /tmp/boost

# OpenCV 4.x
RUN \
    cd /tmp && \
    mkdir -p opencv && \
    cd opencv && \
    echo "downloading opencv ..." && \
    wget -qO- https://github.com/opencv/opencv/archive/4.1.0.tar.gz | tar xz && \
    echo "downloading opencv-contrib ..." && \
    wget -qO- https://github.com/opencv/opencv_contrib/archive/4.1.0.tar.gz | tar xz && \
    cd opencv-4.1.0 && mkdir build && cd build && \
    cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DOPENCV_EXTRA_MODULES_PATH=../../opencv_contrib-4.1.0/modules -DBUILD_EXAMPLES=1 && \
    ninja install && \
    cd && rm -rf /tmp/opencv

ENV HOME /quarks
WORKDIR /quarks

RUN echo 'export PS1="[\u@docker] \W # "' >> /root/.bash_profile

CMD ["bash"]
