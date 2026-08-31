# Reproducible build and test environment for bin_serializer.
#
#   docker build -t bin-serializer .
#   docker run --rm bin-serializer                      # run the test suite
#   docker build --build-arg CXX_COMPILER=clang++-18 .  # build with clang
#
# Pinned to a digest rather than a floating tag so the image a build produces
# today is the image it produces later.
FROM ubuntu@sha256:33ceb71981b602c1a7443a53469e4dba065f7503eab3078a2d7a57a2ab987517

ARG CXX_COMPILER=g++-14
ARG CMAKE_VERSION=3.31.6

RUN apt-get update && apt-get install -y --no-install-recommends \
        g++-14 \
        clang-18 \
        git \
        ca-certificates \
        ninja-build \
        python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Ubuntu 24.04 ships CMake 3.28; this project requires 3.30.
RUN pip3 install --break-system-packages --no-cache-dir "cmake==${CMAKE_VERSION}"

WORKDIR /src
COPY . .

# CMAKE_CXX_SCAN_FOR_MODULES=OFF: C++23 with the Ninja generator enables C++20
# module scanning, which requires clang-scan-deps. This project uses no modules.
RUN cmake -B build -S . \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_COMPILER=${CXX_COMPILER} \
        -DCMAKE_CXX_SCAN_FOR_MODULES=OFF \
        -DENABLE_WERROR=ON \
    && cmake --build build --parallel

CMD ["ctest", "--test-dir", "build", "--output-on-failure"]
