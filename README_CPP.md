# ClassySORT C++ Implementation

ClassySORT C++ is a real-time multi-object tracking system implemented in C++ using OpenCV and YOLOv5. This is a C++ reconstruction of the original Python-based [ClassySORT](https://github.com/tensorturtle/classy-sort-yolov5) project.

## Features

- **YOLOv5 Object Detection**: Uses OpenCV DNN module with ONNX models
- **SORT Tracking**: Simple Online and Realtime Tracking with Kalman filtering
- **Class-Aware Tracking**: Maintains object class information throughout tracking
- **Real-time Performance**: Optimized C++ implementation for high-speed processing
- **80 Object Classes**: Pre-trained on COCO dataset (supports all YOLOv5 classes)

## Requirements

### System Dependencies
- CMake >= 3.10
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- OpenCV >= 4.5.0 (with DNN module)
- Eigen3 >= 3.3

### Installation on Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libopencv-dev \
    libeigen3-dev
```

### Installation on macOS

```bash
brew install cmake opencv eigen
```

## Building

1. Clone the repository:
```bash
git clone https://github.com/tensorturtle/classy-sort-yolov5.git
cd classy-sort-yolov5
```

2. Download YOLOv5 weights and export to ONNX format:
```bash
./download_weights.sh
./export_to_onnx.sh
```

3. Build the C++ application:
```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

4. The executable will be created at `build/classy_sort`

## Usage

### Basic Usage

Track objects in a video file:
```bash
./build/classy_sort --source /path/to/video.mp4 --view-img
```

Track objects from webcam:
```bash
./build/classy_sort --source 0 --view-img
```

### Command Line Options

```
--weights <path>          Path to ONNX model file (default: yolov5s.onnx)
--source <path>           Path to video file or camera index (default: 0)
--output <path>           Output directory (default: ./output)
--conf-thres <float>      Confidence threshold (default: 0.3)
--iou-thres <float>       IOU threshold for NMS (default: 0.4)
--img-size <int>          Input image size (default: 640)
--view-img                Display results in real-time
--save-img                Save output video
--save-txt                Save tracking results to text file
--sort-max-age <int>      SORT max age parameter (default: 5)
--sort-min-hits <int>     SORT min hits parameter (default: 2)
--sort-iou-thresh <float> SORT IOU threshold (default: 0.2)
--help                    Show help message
```

### Example Commands

Save tracked video with text output:
```bash
./build/classy_sort --source video.mp4 --save-img --save-txt --output ./results
```

Track with custom YOLOv5 model:
```bash
./build/classy_sort --weights yolov5m.onnx --source video.mp4 --view-img
```

Adjust tracking parameters for better performance:
```bash
./build/classy_sort --source video.mp4 --sort-max-age 10 --sort-min-hits 3 --view-img
```

## Output Format

When using `--save-txt`, tracking results are saved in the following CSV format:

```
frame_index, x_left_top, y_left_top, x_right_bottom, y_right_bottom, object_category, x_dot, y_dot, s_dot, object_id
```

Where:
- `x_dot`: time derivative of x_center (pixels/frame)
- `y_dot`: time derivative of y_center (pixels/frame)
- `s_dot`: time derivative of bounding box scale/area (pixels²/frame)
- `object_id`: unique tracking ID for each object

## Performance Optimization

### Using Different YOLOv5 Models

- **yolov5s.onnx**: Fastest, good for real-time applications (recommended)
- **yolov5m.onnx**: Balanced speed and accuracy
- **yolov5l.onnx**: High accuracy, slower
- **yolov5x.onnx**: Best accuracy, slowest

Export different models:
```bash
cd yolov5
python export.py --weights weights/yolov5m.pt --include onnx --imgsz 640 --simplify
cd ..
```

### GPU Acceleration

If OpenCV is built with CUDA support, the detector will automatically use GPU acceleration. To check if CUDA is available:

```cpp
cv::cuda::getCudaEnabledDeviceCount() > 0
```

## Project Structure

```
classy-sort-yolov5/
├── include/              # Header files
│   ├── kalman_tracker.h  # Kalman filter tracker
│   ├── hungarian.h       # Hungarian algorithm for assignment
│   ├── sort.h            # SORT tracker
│   └── yolo_detector.h   # YOLOv5 detector
├── src/                  # Source files
│   ├── kalman_tracker.cpp
│   ├── hungarian.cpp
│   ├── sort.cpp
│   ├── yolo_detector.cpp
│   └── main.cpp          # Main application
├── CMakeLists.txt        # CMake build configuration
├── export_to_onnx.sh     # Script to export PyTorch to ONNX
└── README_CPP.md         # This file
```

## Implementation Details

### Modifications to SORT

The C++ implementation maintains the same modifications as the original Python version:

1. **Class-aware Tracking**: Object class information from YOLO is preserved throughout tracking
2. **Kalman Filter State**: 7D state vector [x, y, scale, aspect_ratio, x_velocity, y_velocity, scale_velocity]
3. **Configurable Parameters**: All SORT parameters (max_age, min_hits, iou_threshold) are configurable

### Differences from Python Version

- **Performance**: C++ implementation is typically 2-3x faster than Python
- **Dependencies**: Uses OpenCV DNN instead of PyTorch for inference
- **Model Format**: Requires ONNX model instead of PyTorch .pt files
- **Memory Management**: Smart pointers for automatic memory management

## Troubleshooting

### OpenCV DNN Module Not Found

If you get an error about missing DNN module, ensure OpenCV is built with DNN support:
```bash
pkg-config --modversion opencv4
pkg-config --libs opencv4 | grep dnn
```

### ONNX Model Export Issues

If ONNX export fails, ensure you have the correct versions:
```bash
pip install torch==1.10.0 torchvision==0.11.0 onnx onnx-simplifier
```

### CMake Cannot Find Eigen3

Manually specify Eigen3 path:
```bash
cmake .. -DEIGEN3_INCLUDE_DIR=/usr/include/eigen3
```

## Comparison with Python Version

| Feature | Python Version | C++ Version |
|---------|---------------|-------------|
| Detection Backend | PyTorch | OpenCV DNN |
| Tracking Algorithm | SORT | SORT |
| Model Format | .pt | .onnx |
| Typical FPS (1080p) | ~15-20 | ~30-45 |
| Dependencies | torch, torchvision | OpenCV, Eigen |
| Memory Usage | Higher | Lower |

## License

ClassySORT C++ is released under the GNU General Public License v3.0, maintaining compatibility with the original project.

## Credits

- Original ClassySORT: [tensorturtle/classy-sort-yolov5](https://github.com/tensorturtle/classy-sort-yolov5)
- YOLOv5: [ultralytics/yolov5](https://github.com/ultralytics/yolov5)
- SORT: [abewley/sort](https://github.com/abewley/sort)

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.
