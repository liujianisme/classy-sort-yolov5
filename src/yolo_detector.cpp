#include "yolo_detector.h"
#include <fstream>
#include <algorithm>

YoloDetector::YoloDetector(const std::string& model_path,
                           float conf_threshold,
                           float nms_threshold,
                           int img_size)
    : conf_threshold(conf_threshold), nms_threshold(nms_threshold), img_size(img_size) {
    
    // Load the network
    net = cv::dnn::readNetFromONNX(model_path);
    
    // Set backend and target
    net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    
    // Use CUDA if available (with error handling)
    try {
        if (cv::cuda::getCudaEnabledDeviceCount() > 0) {
            net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
            std::cout << "Using CUDA acceleration" << std::endl;
        } else {
            net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        }
    } catch (const cv::Exception&) {
        // OpenCV not built with CUDA support, fall back to CPU
        net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
    }
    
    loadClassNames();
}

std::vector<std::vector<float>> YoloDetector::detect(const cv::Mat& image,
                                                     const std::vector<int>& class_filter) {
    cv::Size original_size = image.size();
    
    // Preprocess
    cv::Mat blob = preprocess(image);
    net.setInput(blob);
    
    // Forward pass
    std::vector<cv::Mat> outputs;
    net.forward(outputs, net.getUnconnectedOutLayersNames());
    
    // Postprocess
    std::vector<std::vector<float>> detections = postprocess(outputs, original_size);
    
    // Filter by class if needed
    if (!class_filter.empty()) {
        std::vector<std::vector<float>> filtered;
        for (const auto& det : detections) {
            int cls = static_cast<int>(det[5]);
            if (std::find(class_filter.begin(), class_filter.end(), cls) != class_filter.end()) {
                filtered.push_back(det);
            }
        }
        return filtered;
    }
    
    return detections;
}

cv::Mat YoloDetector::preprocess(const cv::Mat& image) {
    cv::Mat resized;
    cv::resize(image, resized, cv::Size(img_size, img_size));
    
    // Convert to blob
    cv::Mat blob = cv::dnn::blobFromImage(resized, 1.0 / 255.0, 
                                         cv::Size(img_size, img_size),
                                         cv::Scalar(0, 0, 0), true, false);
    return blob;
}

std::vector<std::vector<float>> YoloDetector::postprocess(const std::vector<cv::Mat>& outputs,
                                                          const cv::Size& image_shape) {
    std::vector<int> class_ids;
    std::vector<float> confidences;
    std::vector<cv::Rect> boxes;
    
    // YOLOv5 output format: [batch, num_detections, 85]
    // where 85 = 4 (bbox) + 1 (objectness) + 80 (classes)
    for (const auto& output : outputs) {
        float* data = (float*)output.data;
        
        for (int i = 0; i < output.size[1]; i++) {
            float* detection = data + i * output.size[2];
            
            // Get class scores
            float* class_scores = detection + 5;
            cv::Mat scores(1, 80, CV_32FC1, class_scores);
            cv::Point class_id_point;
            double max_class_score;
            cv::minMaxLoc(scores, 0, &max_class_score, 0, &class_id_point);
            
            // Check confidence (objectness * class_score)
            float confidence = detection[4] * max_class_score;
            
            if (confidence >= conf_threshold) {
                // Get bbox coordinates (center_x, center_y, width, height)
                float cx = detection[0];
                float cy = detection[1];
                float w = detection[2];
                float h = detection[3];
                
                // Convert to (x1, y1, width, height) for NMS
                int left = static_cast<int>(cx - w / 2);
                int top = static_cast<int>(cy - h / 2);
                
                class_ids.push_back(class_id_point.x);
                confidences.push_back(confidence);
                boxes.push_back(cv::Rect(left, top, static_cast<int>(w), static_cast<int>(h)));
            }
        }
    }
    
    // Apply Non-Maximum Suppression
    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, confidences, conf_threshold, nms_threshold, indices);
    
    // Prepare output
    std::vector<std::vector<float>> detections;
    cv::Size model_shape(img_size, img_size);
    
    for (int idx : indices) {
        std::vector<float> det(6);
        
        // Convert back to (x1, y1, x2, y2)
        det[0] = static_cast<float>(boxes[idx].x);
        det[1] = static_cast<float>(boxes[idx].y);
        det[2] = static_cast<float>(boxes[idx].x + boxes[idx].width);
        det[3] = static_cast<float>(boxes[idx].y + boxes[idx].height);
        
        // Scale coordinates to original image size
        scaleCoords(det, image_shape, model_shape);
        
        det[4] = confidences[idx];
        det[5] = static_cast<float>(class_ids[idx]);
        
        detections.push_back(det);
    }
    
    return detections;
}

void YoloDetector::scaleCoords(std::vector<float>& coords, const cv::Size& img_shape,
                               const cv::Size& model_shape) {
    // Calculate scaling factor
    float gain = std::min(static_cast<float>(model_shape.width) / img_shape.width,
                         static_cast<float>(model_shape.height) / img_shape.height);
    
    // Calculate padding
    float pad_w = (model_shape.width - img_shape.width * gain) / 2;
    float pad_h = (model_shape.height - img_shape.height * gain) / 2;
    
    // Remove padding and scale
    coords[0] = (coords[0] - pad_w) / gain;
    coords[1] = (coords[1] - pad_h) / gain;
    coords[2] = (coords[2] - pad_w) / gain;
    coords[3] = (coords[3] - pad_h) / gain;
    
    // Clip to image boundaries
    coords[0] = std::max(0.0f, std::min(coords[0], static_cast<float>(img_shape.width)));
    coords[1] = std::max(0.0f, std::min(coords[1], static_cast<float>(img_shape.height)));
    coords[2] = std::max(0.0f, std::min(coords[2], static_cast<float>(img_shape.width)));
    coords[3] = std::max(0.0f, std::min(coords[3], static_cast<float>(img_shape.height)));
}

void YoloDetector::loadClassNames() {
    // COCO class names (80 classes)
    class_names = {
        "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat",
        "traffic light", "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat",
        "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra", "giraffe", "backpack",
        "umbrella", "handbag", "tie", "suitcase", "frisbee", "skis", "snowboard", "sports ball",
        "kite", "baseball bat", "baseball glove", "skateboard", "surfboard", "tennis racket",
        "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
        "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair",
        "couch", "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote",
        "keyboard", "cell phone", "microwave", "oven", "toaster", "sink", "refrigerator", "book",
        "clock", "vase", "scissors", "teddy bear", "hair drier", "toothbrush"
    };
}
