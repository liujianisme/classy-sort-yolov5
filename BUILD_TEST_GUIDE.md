# C++ Build and Testing Guide

This guide provides instructions for building and testing the C++ implementation of ClassySORT.

## Prerequisites

### Required Dependencies

1. **CMake** (>= 3.10)
2. **C++17 compiler** (GCC 7+, Clang 5+, or MSVC 2017+)
3. **OpenCV** (>= 4.5.0 with DNN module)
4. **Eigen3** (>= 3.3)

### Installing Dependencies

#### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    libopencv-dev \
    libeigen3-dev
```

#### macOS (using Homebrew)

```bash
brew install cmake opencv eigen
```

#### Fedora/RHEL

```bash
sudo dnf install cmake gcc-c++ opencv-devel eigen3-devel
```

#### Windows

1. Install Visual Studio 2019 or later
2. Install CMake from https://cmake.org/download/
3. Install OpenCV from https://opencv.org/releases/
4. Install Eigen3 from http://eigen.tuxfamily.org/

## Building

### Method 1: Using the build script (Linux/macOS)

```bash
./build_cpp.sh
```

This script will:
- Check for required dependencies
- Create a build directory
- Run CMake
- Build the project

### Method 2: Manual build

```bash
# Create build directory
mkdir build
cd build

# Run CMake
cmake ..

# Build (adjust -j flag based on CPU cores)
make -j4

# The executable will be at: ./classy_sort
```

### Method 3: Using Docker

Build the Docker image:

```bash
docker build -t classy-sort-cpp .
```

Run the container:

```bash
# Show help
docker run --rm classy-sort-cpp --help

# Process a video (mount your video directory)
docker run --rm -v /path/to/videos:/videos classy-sort-cpp \
    --source /videos/input.mp4 \
    --output /videos/output \
    --save-txt
```

## Exporting YOLOv5 Model to ONNX

Before running the C++ application, you need to export the YOLOv5 PyTorch model to ONNX format:

1. Download YOLOv5 weights:
```bash
./download_weights.sh
```

2. Export to ONNX:
```bash
./export_to_onnx.sh
```

This will create `yolov5s.onnx` in the project root directory.

### Manual ONNX Export

If the script doesn't work, you can manually export:

```bash
# Install requirements
pip install torch torchvision onnx onnx-simplifier

# Export YOLOv5s
cd yolov5
python export.py --weights weights/yolov5s.pt --include onnx --imgsz 640 --simplify
cd ..

# Copy ONNX file to root
cp yolov5/weights/yolov5s.onnx ./
```

## Testing

### Test 1: Help Command

Verify the executable works:

```bash
./build/classy_sort --help
```

Expected output: Help message with all command-line options

### Test 2: Webcam Test (if available)

```bash
./build/classy_sort --source 0 --view-img --weights yolov5s.onnx
```

Press 'q' to quit.

### Test 3: Video File Test

Download a test video:

```bash
# Example: Download a sample video
wget https://www.pexels.com/video/3044127/download/ -O test_video.mp4

# Run tracking
./build/classy_sort \
    --source test_video.mp4 \
    --weights yolov5s.onnx \
    --view-img \
    --save-txt \
    --output ./test_output
```

Check results:
- Tracked video (if `--save-img` was used)
- `test_output/results.txt` with tracking data

### Test 4: Performance Test

Test with different models to compare performance:

```bash
# YOLOv5s (fastest)
time ./build/classy_sort --source video.mp4 --weights yolov5s.onnx

# YOLOv5m (balanced)
time ./build/classy_sort --source video.mp4 --weights yolov5m.onnx
```

## Validation

### Expected Output Format

The `results.txt` file should contain CSV data:

```
frame_idx,x1,y1,x2,y2,class_id,x_dot,y_dot,s_dot,track_id
0,100.5,200.3,250.7,400.2,0,1.2,-0.5,10.3,1
0,500.1,150.4,650.8,350.9,2,0.8,1.1,5.2,2
1,101.7,199.8,251.9,399.7,0,1.2,-0.5,10.3,1
```

### Checking Build Success

1. **Executable exists**: `ls -lh build/classy_sort`
2. **Executable runs**: `./build/classy_sort --help` (should not crash)
3. **Libraries linked**: `ldd build/classy_sort` (check OpenCV, Eigen dependencies)

### Common Issues

#### Issue: "OpenCV DNN module not found"

**Solution**: Ensure OpenCV is built with DNN module:
```bash
pkg-config --libs opencv4 | grep dnn
```

If not present, rebuild OpenCV with `-DBUILD_opencv_dnn=ON`

#### Issue: "cannot find -lEigen3"

**Solution**: Eigen is header-only, but CMake needs to find it:
```bash
# Ubuntu/Debian
sudo apt-get install libeigen3-dev

# Or specify path manually
cmake .. -DEIGEN3_INCLUDE_DIR=/usr/include/eigen3
```

#### Issue: "undefined reference to cv::cuda::getCudaEnabledDeviceCount"

**Solution**: OpenCV was not built with CUDA. The code will fall back to CPU automatically.

#### Issue: ONNX model loading fails

**Solution**: 
1. Verify ONNX file exists and is valid
2. Check OpenCV version supports ONNX: `pkg-config --modversion opencv4`
3. OpenCV >= 4.5.0 has better ONNX support

## Performance Benchmarks

Expected performance on different hardware (1080p video):

| Hardware | Model | FPS |
|----------|-------|-----|
| CPU (i7-9700K) | YOLOv5s | ~15-20 |
| CPU (i7-9700K) | YOLOv5m | ~8-12 |
| GPU (RTX 2070) | YOLOv5s | ~45-60 |
| GPU (RTX 2070) | YOLOv5m | ~30-40 |

*Note: GPU acceleration requires OpenCV built with CUDA support*

## Debugging

### Enable verbose CMake

```bash
cmake .. --trace
```

### Check linked libraries

```bash
ldd build/classy_sort
```

### Run with debugger

```bash
gdb ./build/classy_sort
(gdb) run --source video.mp4
```

### Compiler warnings

```bash
cmake .. -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic"
make
```

## Integration Testing

To verify the C++ implementation produces results similar to the Python version:

1. Run Python version on a test video
2. Run C++ version on the same video
3. Compare output files:
   - Number of tracked objects should be similar (±10%)
   - Track IDs should follow similar patterns
   - Bounding box coordinates should be close (±5 pixels)

## Continuous Integration

Example GitHub Actions workflow (`.github/workflows/cpp-build.yml`):

```yaml
name: C++ Build

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y libopencv-dev libeigen3-dev
      - name: Build
        run: ./build_cpp.sh
      - name: Test
        run: ./build/classy_sort --help
```

## Next Steps

After successful build and testing:

1. **Profile the code** to identify bottlenecks
2. **Add unit tests** for individual components
3. **Optimize critical paths** (detection, tracking loops)
4. **Add GPU acceleration** for OpenCV operations
5. **Implement multi-threading** for video I/O and processing

## Support

If you encounter issues:

1. Check this guide's "Common Issues" section
2. Verify all dependencies are correctly installed
3. Check OpenCV build configuration: `opencv_version --verbose`
4. Open an issue on GitHub with:
   - Your OS and version
   - CMake output
   - Build errors (if any)
   - Runtime errors (if any)
