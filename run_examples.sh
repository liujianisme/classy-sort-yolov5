#!/bin/bash

# Example usage script for ClassySORT C++
# This demonstrates various ways to use the tracker

set -e

EXECUTABLE="./build/classy_sort"
WEIGHTS="yolov5s.onnx"

# Check if executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo "Error: Executable not found at $EXECUTABLE"
    echo "Please build the project first: ./build_cpp.sh"
    exit 1
fi

# Check if ONNX weights exist
if [ ! -f "$WEIGHTS" ]; then
    echo "Error: ONNX weights not found at $WEIGHTS"
    echo "Please export the model first: ./export_to_onnx.sh"
    exit 1
fi

echo "======================================"
echo "ClassySORT C++ - Example Usage"
echo "======================================"
echo ""

# Example 1: Show help
echo "Example 1: Show help message"
echo "Command: $EXECUTABLE --help"
echo ""
$EXECUTABLE --help
echo ""
read -p "Press Enter to continue..."
echo ""

# Example 2: Process webcam (if available)
echo "Example 2: Process webcam feed"
echo "Command: $EXECUTABLE --source 0 --view-img --weights $WEIGHTS"
echo "Note: Press 'q' to quit"
echo ""
read -p "Press Enter to start (or Ctrl+C to skip)..."
$EXECUTABLE --source 0 --view-img --weights $WEIGHTS || echo "Webcam test skipped or failed"
echo ""

# Example 3: Process video file with visualization
if [ -f "test_video.mp4" ]; then
    echo "Example 3: Process video file with visualization"
    echo "Command: $EXECUTABLE --source test_video.mp4 --view-img --weights $WEIGHTS"
    echo ""
    read -p "Press Enter to start (or Ctrl+C to skip)..."
    $EXECUTABLE --source test_video.mp4 --view-img --weights $WEIGHTS
    echo ""
fi

# Example 4: Process video and save results
if [ -f "test_video.mp4" ]; then
    echo "Example 4: Process video and save text results"
    echo "Command: $EXECUTABLE --source test_video.mp4 --save-txt --output ./demo_output --weights $WEIGHTS"
    echo ""
    read -p "Press Enter to start (or Ctrl+C to skip)..."
    $EXECUTABLE --source test_video.mp4 --save-txt --output ./demo_output --weights $WEIGHTS
    echo ""
    echo "Results saved to ./demo_output/results.txt"
    echo "First 10 lines:"
    head -10 ./demo_output/results.txt 2>/dev/null || echo "No results file found"
    echo ""
fi

# Example 5: Process video and save output video
if [ -f "test_video.mp4" ]; then
    echo "Example 5: Process video and save output video"
    echo "Command: $EXECUTABLE --source test_video.mp4 --save-img --save-txt --output ./demo_output --weights $WEIGHTS"
    echo ""
    read -p "Press Enter to start (or Ctrl+C to skip)..."
    $EXECUTABLE --source test_video.mp4 --save-img --save-txt --output ./demo_output --weights $WEIGHTS
    echo ""
    echo "Output video saved to ./demo_output/output.mp4"
    echo "Results saved to ./demo_output/results.txt"
    echo ""
fi

# Example 6: Adjust tracking parameters
if [ -f "test_video.mp4" ]; then
    echo "Example 6: Adjust SORT tracking parameters"
    echo "Command: $EXECUTABLE --source test_video.mp4 --sort-max-age 10 --sort-min-hits 3 --sort-iou-thresh 0.3 --view-img --weights $WEIGHTS"
    echo ""
    echo "Parameters explained:"
    echo "  --sort-max-age 10: Keep tracks alive for 10 frames without detection"
    echo "  --sort-min-hits 3: Require 3 consecutive detections before starting a track"
    echo "  --sort-iou-thresh 0.3: Use 0.3 IoU threshold for matching"
    echo ""
    read -p "Press Enter to start (or Ctrl+C to skip)..."
    $EXECUTABLE --source test_video.mp4 --sort-max-age 10 --sort-min-hits 3 --sort-iou-thresh 0.3 --view-img --weights $WEIGHTS
    echo ""
fi

# Example 7: Adjust detection parameters
if [ -f "test_video.mp4" ]; then
    echo "Example 7: Adjust detection confidence threshold"
    echo "Command: $EXECUTABLE --source test_video.mp4 --conf-thres 0.5 --iou-thres 0.5 --view-img --weights $WEIGHTS"
    echo ""
    echo "Parameters explained:"
    echo "  --conf-thres 0.5: Higher confidence threshold (fewer false positives)"
    echo "  --iou-thres 0.5: Higher NMS threshold (fewer duplicate detections)"
    echo ""
    read -p "Press Enter to start (or Ctrl+C to skip)..."
    $EXECUTABLE --source test_video.mp4 --conf-thres 0.5 --iou-thres 0.5 --view-img --weights $WEIGHTS
    echo ""
fi

echo "======================================"
echo "Examples completed!"
echo "======================================"
echo ""
echo "For more information, see:"
echo "  - README_CPP.md for detailed documentation"
echo "  - BUILD_TEST_GUIDE.md for build and testing instructions"
echo "  - $EXECUTABLE --help for all command-line options"
echo ""
