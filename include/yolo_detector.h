#ifndef YOLO_DETECTOR_H
#define YOLO_DETECTOR_H

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <string>
#include <vector>

/**
 * YOLOv5 Detector using OpenCV DNN module
 */
class YoloDetector {
public:
    /**
     * Initialize YOLO detector
     * @param model_path: path to ONNX model file
     * @param conf_threshold: confidence threshold for detections
     * @param nms_threshold: NMS IoU threshold
     * @param img_size: input image size
     */
    YoloDetector(const std::string& model_path,
                 float conf_threshold = 0.3f,
                 float nms_threshold = 0.4f,
                 int img_size = 640);
    
    /**
     * Detect objects in an image
     * @param image: input image
     * @param class_filter: optional vector of class IDs to filter (empty = all classes)
     * @return vector of detections [x1, y1, x2, y2, confidence, class_id]
     */
    std::vector<std::vector<float>> detect(const cv::Mat& image,
                                           const std::vector<int>& class_filter = {});
    
    /**
     * Load COCO class names
     * @return vector of class names
     */
    std::vector<std::string> getClassNames() const { return class_names; }
    
private:
    cv::dnn::Net net;
    float conf_threshold;
    float nms_threshold;
    int img_size;
    std::vector<std::string> class_names;
    
    /**
     * Preprocess image for YOLO
     * @param image: input image
     * @return preprocessed blob
     */
    cv::Mat preprocess(const cv::Mat& image);
    
    /**
     * Postprocess YOLO output
     * @param outputs: network outputs
     * @param image_shape: original image shape
     * @return detections [x1, y1, x2, y2, confidence, class_id]
     */
    std::vector<std::vector<float>> postprocess(const std::vector<cv::Mat>& outputs,
                                                const cv::Size& image_shape);
    
    /**
     * Scale coordinates from model size to original image size
     */
    void scaleCoords(std::vector<float>& coords, const cv::Size& img_shape,
                    const cv::Size& model_shape);
    
    /**
     * Load COCO class names
     */
    void loadClassNames();
};

#endif // YOLO_DETECTOR_H
