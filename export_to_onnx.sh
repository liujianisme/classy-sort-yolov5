#!/bin/bash

# Script to export YOLOv5 PyTorch model to ONNX format
# This is required to use YOLOv5 with OpenCV DNN module in C++

set -e

echo "Exporting YOLOv5 model to ONNX format..."

# Check if Python is available
if ! command -v python3 &> /dev/null; then
    echo "Error: Python 3 is not installed"
    exit 1
fi

# Create virtual environment if it doesn't exist
if [ ! -d "venv_onnx" ]; then
    echo "Creating virtual environment..."
    python3 -m venv venv_onnx
fi

# Activate virtual environment
source venv_onnx/bin/activate

# Install required packages with pinned versions for reproducibility
echo "Installing required Python packages..."
pip install --upgrade pip
pip install torch==1.10.0 torchvision==0.11.0 onnx==1.12.0 onnx-simplifier==0.4.17

# Check if yolov5 weights exist
WEIGHTS_DIR="yolov5/weights"
if [ ! -d "$WEIGHTS_DIR" ]; then
    echo "Error: YOLOv5 weights directory not found. Please run ./download_weights.sh first"
    deactivate
    exit 1
fi

# Export yolov5s model
WEIGHT_FILE="$WEIGHTS_DIR/yolov5s.pt"
if [ -f "$WEIGHT_FILE" ]; then
    echo "Exporting yolov5s.pt to ONNX..."
    cd yolov5
    python export.py --weights weights/yolov5s.pt --include onnx --imgsz 640 --simplify
    cd ..
    
    # Move ONNX file to root directory
    if [ -f "$WEIGHTS_DIR/yolov5s.onnx" ]; then
        cp "$WEIGHTS_DIR/yolov5s.onnx" ./yolov5s.onnx
        echo "Successfully exported yolov5s.onnx"
    fi
else
    echo "Warning: $WEIGHT_FILE not found"
fi

# Deactivate virtual environment
deactivate

echo "ONNX export completed!"
echo ""
echo "To use other YOLOv5 models (yolov5m, yolov5l, yolov5x), run:"
echo "  source venv_onnx/bin/activate"
echo "  cd yolov5"
echo "  python export.py --weights weights/yolov5m.pt --include onnx --imgsz 640 --simplify"
echo "  cd .."
echo "  deactivate"
