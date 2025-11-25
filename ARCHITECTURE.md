# ClassySORT C++ Architecture

## System Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                         Main Application                             │
│                          (main.cpp)                                  │
└─────────────────────────────────────────────────────────────────────┘
                                  │
                                  │
                    ┌─────────────┴─────────────┐
                    │                           │
                    ▼                           ▼
      ┌──────────────────────┐    ┌──────────────────────┐
      │   YoloDetector       │    │   Video I/O          │
      │  (yolo_detector.cpp) │    │   (OpenCV)           │
      └──────────────────────┘    └──────────────────────┘
                │                            │
                │ Detections                 │ Frames
                │ [x1,y1,x2,y2,conf,cls]    │
                │                            │
                ▼                            │
      ┌──────────────────────┐              │
      │   Sort Tracker       │◄─────────────┘
      │   (sort.cpp)         │
      └──────────────────────┘
                │
                │ Manages multiple trackers
                │
                ▼
      ┌──────────────────────┐
      │ KalmanBoxTracker[]   │
      │ (kalman_tracker.cpp) │
      └──────────────────────┘
                │
                │ Uses
                │
                ▼
      ┌──────────────────────┐
      │ LinearAssignment     │
      │ (hungarian.cpp)      │
      └──────────────────────┘
```

## Data Flow

```
Video Frame
    │
    ▼
┌───────────────┐
│ Preprocessing │  (Resize to 640x640, Normalize)
└───────────────┘
    │
    ▼
┌───────────────┐
│ YOLO Detector │  (OpenCV DNN forward pass)
└───────────────┘
    │
    ▼
┌───────────────┐
│ NMS Filter    │  (Non-Maximum Suppression)
└───────────────┘
    │
    ▼  Detections: [[x1,y1,x2,y2,conf,class], ...]
┌───────────────┐
│ SORT Update   │
└───────────────┘
    │
    ├─────────────────────────────────────┐
    │                                     │
    ▼                                     ▼
┌────────────────┐              ┌──────────────────┐
│ Predict Step   │              │ Calculate IoU    │
│ (All Trackers) │              │ Matrix           │
└────────────────┘              └──────────────────┘
    │                                     │
    │                                     ▼
    │                           ┌──────────────────┐
    │                           │ Linear           │
    │                           │ Assignment       │
    │                           └──────────────────┘
    │                                     │
    │         ┌───────────────────────────┴────────────────┐
    │         │                                            │
    │         ▼                                            ▼
    │  ┌──────────────┐                        ┌──────────────────┐
    │  │ Update       │                        │ Create New       │
    │  │ Matched      │                        │ Trackers         │
    │  └──────────────┘                        └──────────────────┘
    │         │                                            │
    └─────────┴────────────────────────────────────────────┘
                                │
                                ▼
                      ┌──────────────────┐
                      │ Remove Dead      │
                      │ Trackers         │
                      └──────────────────┘
                                │
                                ▼
                      Tracked Objects:
                      [[x1,y1,x2,y2,cls,vx,vy,vs,id], ...]
                                │
                                ├────────────┬──────────────┐
                                │            │              │
                                ▼            ▼              ▼
                        ┌─────────────┐ ┌──────┐ ┌─────────────┐
                        │ Visualize   │ │ Save │ │ Save Video  │
                        │ (Display)   │ │ Text │ │ (Optional)  │
                        └─────────────┘ └──────┘ └─────────────┘
```

## Kalman Filter State

```
State Vector (7D):
┌────────────────┐
│ x (center)     │  ← Position
│ y (center)     │  ← Position
│ s (scale/area) │  ← Size
│ r (aspect)     │  ← Shape
│ ẋ (x velocity) │  ← Motion
│ ẏ (y velocity) │  ← Motion
│ ṡ (s velocity) │  ← Scale change
└────────────────┘

Prediction:  x' = F·x
Update:      x = x' + K·(z - H·x')

Where:
- F: State transition matrix (7×7)
- H: Measurement matrix (4×7)
- K: Kalman gain
- z: Measurement (bbox from detection)
```

## Component Responsibilities

```
┌───────────────────────────────────────────────────────┐
│ YoloDetector                                          │
├───────────────────────────────────────────────────────┤
│ • Load ONNX model                                     │
│ • Preprocess images (resize, normalize)              │
│ • Run inference                                       │
│ • Postprocess (NMS, scale coordinates)                │
│ • Return: detections [x1,y1,x2,y2,conf,class]        │
└───────────────────────────────────────────────────────┘

┌───────────────────────────────────────────────────────┐
│ KalmanBoxTracker                                      │
├───────────────────────────────────────────────────────┤
│ • Maintain 7D state (position, size, velocity)       │
│ • Predict next position                               │
│ • Update with new measurement                         │
│ • Track object class                                  │
│ • Manage track metadata (hits, age, id)              │
└───────────────────────────────────────────────────────┘

┌───────────────────────────────────────────────────────┐
│ LinearAssignment                                      │
├───────────────────────────────────────────────────────┤
│ • Calculate IoU between all detection-tracker pairs  │
│ • Solve assignment problem (greedy)                   │
│ • Return: matches, unmatched_dets, unmatched_trks    │
└───────────────────────────────────────────────────────┘

┌───────────────────────────────────────────────────────┐
│ Sort                                                  │
├───────────────────────────────────────────────────────┤
│ • Manage multiple KalmanBoxTracker instances         │
│ • Predict all tracker positions                       │
│ • Associate detections to trackers                    │
│ • Update matched trackers                             │
│ • Create new trackers for unmatched detections       │
│ • Delete old trackers (age > max_age)                │
│ • Return active tracks                                │
└───────────────────────────────────────────────────────┘
```

## Configuration Flow

```
Command Line Arguments
         │
         ▼
┌──────────────────┐
│ Parse Arguments  │
└──────────────────┘
         │
         ├─────────────────┬─────────────────┬──────────────────┐
         │                 │                 │                  │
         ▼                 ▼                 ▼                  ▼
┌────────────────┐ ┌────────────────┐ ┌──────────────┐ ┌──────────────┐
│ YoloDetector   │ │ Sort Tracker   │ │ Video I/O    │ │ Output Files │
├────────────────┤ ├────────────────┤ ├──────────────┤ ├──────────────┤
│ • weights      │ │ • max_age      │ │ • source     │ │ • output_dir │
│ • conf_thres   │ │ • min_hits     │ │ • view_img   │ │ • save_txt   │
│ • iou_thres    │ │ • iou_thresh   │ │ • save_img   │ │ • save_video │
│ • img_size     │ │                │ │              │ │              │
└────────────────┘ └────────────────┘ └──────────────┘ └──────────────┘
```

## Memory Layout

```
Application Memory (~100MB)
├── YOLO Model (~28MB)
│   └── ONNX weights in OpenCV DNN
├── Frame Buffers (~8MB per frame)
│   └── Input/output images
├── Trackers (~1KB each × N objects)
│   ├── Kalman state (7×1 vector)
│   ├── Covariance matrix (7×7)
│   └── Metadata
└── Detection Results (~100B per detection)
    └── Temporary storage for each frame
```

## Build Dependency Graph

```
classy_sort (executable)
    │
    ├─── main.cpp
    │
    ├─── yolo_detector.cpp ──┬─── OpenCV::dnn
    │                        └─── OpenCV::core
    │
    ├─── sort.cpp ───────────┬─── kalman_tracker.cpp ─── Eigen3
    │                        └─── hungarian.cpp
    │
    └─── CMakeLists.txt ─────┬─── Find OpenCV
                             └─── Find Eigen3
```

## Execution Timeline

```
Time →
│
├─ Initialize (once)
│  ├─ Load YOLO model
│  ├─ Open video source
│  ├─ Create SORT tracker
│  └─ Initialize output writers
│
└─ Main Loop (per frame)
   │
   ├─ Read frame ──────────────── ~1ms
   │
   ├─ YOLO detection ──────────── ~40-60ms (CPU) / ~10-15ms (GPU)
   │  ├─ Preprocess
   │  ├─ Inference
   │  └─ NMS
   │
   ├─ SORT update ─────────────── ~2-5ms
   │  ├─ Predict
   │  ├─ Associate
   │  ├─ Update
   │  └─ Cleanup
   │
   ├─ Visualization ───────────── ~5-10ms
   │  ├─ Draw boxes
   │  └─ Add labels
   │
   └─ Output ──────────────────── ~1-5ms
      ├─ Display (optional)
      ├─ Save text (optional)
      └─ Write video (optional)

Total: ~50-75ms/frame = ~15-20 FPS (CPU)
       ~20-30ms/frame = ~30-50 FPS (GPU)
```

## Error Handling Strategy

```
Error Occurred
    │
    ├─ CUDA not available ──────► Fall back to CPU
    │
    ├─ ONNX model not found ────► Exit with error message
    │
    ├─ Video source failed ─────► Exit with error message
    │
    ├─ Invalid camera index ────► Try as URL/file path
    │
    ├─ Output dir not exist ────► Create directory
    │
    └─ Tracker prediction NaN ──► Remove tracker
```
