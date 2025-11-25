FROM ubuntu:22.04

# Prevent interactive prompts during installation
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    pkg-config \
    libopencv-dev \
    libeigen3-dev \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Install Python packages for ONNX export
RUN pip3 install torch torchvision onnx onnx-simplifier

WORKDIR /app

# Copy source code
COPY . /app

# Build the C++ application
RUN mkdir -p build && cd build && \
    cmake .. && \
    make -j$(nproc)

# Set the entrypoint
ENTRYPOINT ["/app/build/classy_sort"]
CMD ["--help"]
