#!/bin/bash

# Build script for ClassySORT C++

set -e

echo "======================================"
echo "ClassySORT C++ Build Script"
echo "======================================"
echo ""

# Check for required tools
echo "Checking for required tools..."

if ! command -v cmake &> /dev/null; then
    echo "Error: CMake is not installed"
    echo "Install with: sudo apt-get install cmake"
    exit 1
fi

if ! command -v g++ &> /dev/null; then
    echo "Error: G++ compiler is not installed"
    echo "Install with: sudo apt-get install build-essential"
    exit 1
fi

echo "✓ CMake found: $(cmake --version | head -n1)"
echo "✓ G++ found: $(g++ --version | head -n1)"
echo ""

# Check for OpenCV
echo "Checking for OpenCV..."
if pkg-config --exists opencv4; then
    echo "✓ OpenCV found: $(pkg-config --modversion opencv4)"
elif pkg-config --exists opencv; then
    echo "✓ OpenCV found: $(pkg-config --modversion opencv)"
else
    echo "Error: OpenCV not found"
    echo "Install with: sudo apt-get install libopencv-dev"
    exit 1
fi
echo ""

# Check for Eigen3
echo "Checking for Eigen3..."
if [ -d "/usr/include/eigen3" ] || [ -d "/usr/local/include/eigen3" ]; then
    echo "✓ Eigen3 found"
else
    echo "Error: Eigen3 not found"
    echo "Install with: sudo apt-get install libeigen3-dev"
    exit 1
fi
echo ""

# Create build directory
echo "Creating build directory..."
mkdir -p build
cd build

# Run CMake
echo "Running CMake..."
cmake ..

# Build
echo ""
echo "Building ClassySORT C++..."
make -j$(nproc)

echo ""
echo "======================================"
echo "Build completed successfully!"
echo "======================================"
echo ""
echo "Executable location: $(pwd)/classy_sort"
echo ""
echo "To run the application:"
echo "  ./build/classy_sort --help"
echo ""
echo "Example usage:"
echo "  ./build/classy_sort --source video.mp4 --view-img"
echo ""
