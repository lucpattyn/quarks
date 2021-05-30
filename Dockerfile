FROM ubuntu:21.04

ENV DEBIAN_FRONTEND=noninteractive

RUN \
    apt update && \
    apt install -y build-essential doxygen git curl wget gdb ninja-build python3 bash xxd ripgrep neovim zlib1g-dev libbz2-dev pkg-config libjpeg-dev \
        libpng-dev libtiff-dev libavcodec-dev libavformat-dev libswscale-dev libv4l-dev libxvidcore-dev libx264-dev libatlas-base-dev gfortran python3-dev \
        unzip libgtk-3-dev libtcmalloc-minimal4 openssl libssl-dev libv8-dev librocksdb-dev libzmq3-dev

#RUN ln -s /usr/lib/libtcmalloc_minimal.so.4 /usr/lib/libtcmalloc_minimal.so

# CMake
RUN cd /tmp && \
    mkdir -p cmake && \
    cd cmake && \
    echo "downloading cmake ..." && \
    wget -O cmake-3.14.0-Linux-x86_64.tar.gz  https://github.com/Kitware/CMake/releases/download/v3.14.0/cmake-3.14.0-Linux-x86_64.tar.gz && \
    echo e687c0f3acfb15c880ddac67e2821907f833cb900c6ecedb4ab5df5102604d82753c948e3c7dca6e5bcce6278a09b7d577b1afade2e133aec5b2057ac48d3c74  cmake-3.14.0-Linux-x86_64.tar.gz | sha512sum -c && \
    tar xf cmake-3.14.0-Linux-x86_64.tar.gz && \
    mv cmake-3.14.0-Linux-x86_64 /opt/cmake

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
    wget -O boost_1_69_0.tar.bz2 https://boostorg.jfrog.io/artifactory/main/release/1.69.0/source/boost_1_69_0.tar.bz2 && \
    echo 8f32d4617390d1c2d16f26a27ab60d97807b35440d45891fa340fc2648b04406 boost_1_69_0.tar.bz2 | sha256sum -c && \
    tar xf boost_1_69_0.tar.bz2 && \
    cd boost_1_69_0 && \
    ./bootstrap.sh && \
    ./b2 variant=release link=static threading=multi --layout=system --with-system --with-stacktrace cxxflags=-std=c++17 install && \
    cd && \
    rm -rf /tmp/boost

# OpenCV 4.x
RUN \
    cd /tmp && \
    mkdir -p opencv && \
    cd opencv && \
    echo "downloading opencv ..." && \
    wget -O opencv_4.1.0.tar.gz https://github.com/opencv/opencv/archive/4.1.0.tar.gz && \
    echo 492168c1260cd30449393c4b266d75202e751493a8f1e184af6c085d8f4a38800ee954d84fe8c36fcceb690b1ebb5e511b68c05901f64be79a0915f3f8a46dc0  opencv_4.1.0.tar.gz | sha512sum -c && \
    tar xf opencv_4.1.0.tar.gz && \
    echo "downloading opencv-contrib ..." && \
    wget -O opencv_contrib_4.1.0.tar.gz https://github.com/opencv/opencv_contrib/archive/4.1.0.tar.gz && \
    echo 68b373dcb149891847927709bd4409711d74adc65c6c79e8d91c61eee673a4a2304535868d7f54324ac6156b2bec9608d4f9c8f24b3378d43893b0734e116c35  opencv_contrib_4.1.0.tar.gz | sha512sum -c && \
    tar xf opencv_contrib_4.1.0.tar.gz && \
    cd opencv-4.1.0 && mkdir build && cd build && \
    cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DOPENCV_EXTRA_MODULES_PATH=../../opencv_contrib-4.1.0/modules -DBUILD_EXAMPLES=1 && \
    ninja install && \
    cd && rm -rf /tmp/opencv

ENV HOME /quarks
WORKDIR /quarks

RUN echo 'export PS1="[\u@docker] \W # "' >> /root/.bash_profile

CMD ["bash"]
