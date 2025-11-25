#include <iostream>
#include <opencv2/opencv.hpp>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <sys/stat.h>
#include "yolo_detector.h"
#include "sort.h"

// Color palette for visualization
cv::Scalar computeColorForId(int id) {
    int palette[] = {2047, 32767, 1048575};
    int r = (palette[0] * (id * id - id + 1)) % 255;
    int g = (palette[1] * (id * id - id + 1)) % 255;
    int b = (palette[2] * (id * id - id + 1)) % 255;
    return cv::Scalar(b, g, r);
}

void drawBoxes(cv::Mat& img, const std::vector<std::vector<float>>& tracked_objects,
              const std::vector<std::string>& class_names) {
    for (const auto& obj : tracked_objects) {
        int x1 = static_cast<int>(obj[0]);
        int y1 = static_cast<int>(obj[1]);
        int x2 = static_cast<int>(obj[2]);
        int y2 = static_cast<int>(obj[3]);
        int cls = static_cast<int>(obj[4]);
        int track_id = static_cast<int>(obj[8]);
        
        cv::Scalar color = computeColorForId(track_id);
        
        // Draw bounding box
        cv::rectangle(img, cv::Point(x1, y1), cv::Point(x2, y2), color, 3);
        
        // Prepare label
        std::string label = class_names[cls] + " | " + std::to_string(track_id);
        
        // Get text size
        int baseline = 0;
        cv::Size text_size = cv::getTextSize(label, cv::FONT_HERSHEY_PLAIN, 2, 2, &baseline);
        
        // Draw text background
        cv::rectangle(img, cv::Point(x1, y1),
                     cv::Point(x1 + text_size.width + 3, y1 + text_size.height + 4),
                     color, -1);
        
        // Draw text
        cv::putText(img, label, cv::Point(x1, y1 + text_size.height + 4),
                   cv::FONT_HERSHEY_PLAIN, 2, cv::Scalar(255, 255, 255), 2);
    }
}

void printUsage(const char* program_name) {
    std::cout << "ClassySORT C++ - Multi-Object Tracking with YOLOv5\n\n";
    std::cout << "Usage: " << program_name << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --weights <path>        Path to ONNX model file (default: yolov5s.onnx)\n";
    std::cout << "  --source <path>         Path to video file or camera index (default: 0)\n";
    std::cout << "  --output <path>         Output directory (default: ./output)\n";
    std::cout << "  --conf-thres <float>    Confidence threshold (default: 0.3)\n";
    std::cout << "  --iou-thres <float>     IOU threshold for NMS (default: 0.4)\n";
    std::cout << "  --img-size <int>        Input image size (default: 640)\n";
    std::cout << "  --view-img              Display results\n";
    std::cout << "  --save-img              Save video output\n";
    std::cout << "  --save-txt              Save results to text file\n";
    std::cout << "  --sort-max-age <int>    SORT max age (default: 5)\n";
    std::cout << "  --sort-min-hits <int>   SORT min hits (default: 2)\n";
    std::cout << "  --sort-iou-thresh <float> SORT IOU threshold (default: 0.2)\n";
    std::cout << "  --help                  Show this help message\n";
}

int main(int argc, char** argv) {
    // Default parameters
    std::string weights = "yolov5s.onnx";
    std::string source = "0";
    std::string output_dir = "./output";
    float conf_thres = 0.3f;
    float iou_thres = 0.4f;
    int img_size = 640;
    bool view_img = false;
    bool save_img = false;
    bool save_txt = false;
    int sort_max_age = 5;
    int sort_min_hits = 2;
    float sort_iou_thresh = 0.2f;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--weights" && i + 1 < argc) {
            weights = argv[++i];
        } else if (arg == "--source" && i + 1 < argc) {
            source = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            output_dir = argv[++i];
        } else if (arg == "--conf-thres" && i + 1 < argc) {
            conf_thres = std::stof(argv[++i]);
        } else if (arg == "--iou-thres" && i + 1 < argc) {
            iou_thres = std::stof(argv[++i]);
        } else if (arg == "--img-size" && i + 1 < argc) {
            img_size = std::stoi(argv[++i]);
        } else if (arg == "--view-img") {
            view_img = true;
        } else if (arg == "--save-img") {
            save_img = true;
        } else if (arg == "--save-txt") {
            save_txt = true;
        } else if (arg == "--sort-max-age" && i + 1 < argc) {
            sort_max_age = std::stoi(argv[++i]);
        } else if (arg == "--sort-min-hits" && i + 1 < argc) {
            sort_min_hits = std::stoi(argv[++i]);
        } else if (arg == "--sort-iou-thresh" && i + 1 < argc) {
            sort_iou_thresh = std::stof(argv[++i]);
        }
    }
    
    // Create output directory
    struct stat st = {0};
    if (stat(output_dir.c_str(), &st) == -1) {
        #ifdef _WIN32
            _mkdir(output_dir.c_str());
        #else
            mkdir(output_dir.c_str(), 0700);
        #endif
    }
    
    // Initialize detector and tracker
    std::cout << "Loading YOLOv5 model from: " << weights << std::endl;
    YoloDetector detector(weights, conf_thres, iou_thres, img_size);
    std::vector<std::string> class_names = detector.getClassNames();
    
    Sort tracker(sort_max_age, sort_min_hits, sort_iou_thresh);
    
    // Open video source
    cv::VideoCapture cap;
    bool is_webcam = (source == "0" || source.find("rtsp") == 0 || source.find("http") == 0);
    
    if (is_webcam) {
        try {
            int cam_index = std::stoi(source);
            cap.open(cam_index);
        } catch (const std::invalid_argument&) {
            // If source is not a number, try opening as stream URL
            cap.open(source);
        }
    } else {
        cap.open(source);
    }
    
    if (!cap.isOpened()) {
        std::cerr << "Error: Cannot open video source: " << source << std::endl;
        return -1;
    }
    
    // Get video properties
    int frame_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int frame_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    double fps = cap.get(cv::CAP_PROP_FPS);
    
    std::cout << "Video properties: " << frame_width << "x" << frame_height 
              << " @ " << fps << " fps" << std::endl;
    
    // Video writer
    cv::VideoWriter writer;
    if (save_img) {
        std::string output_path = output_dir + "/output.mp4";
        int fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
        writer.open(output_path, fourcc, fps, cv::Size(frame_width, frame_height));
        std::cout << "Saving output to: " << output_path << std::endl;
    }
    
    // Text output
    std::ofstream txt_file;
    if (save_txt) {
        std::string txt_path = output_dir + "/results.txt";
        txt_file.open(txt_path);
        std::cout << "Saving results to: " << txt_path << std::endl;
    }
    
    int frame_idx = 0;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::cout << "\nProcessing video...\n" << std::endl;
    
    while (true) {
        cv::Mat frame;
        cap >> frame;
        
        if (frame.empty()) {
            break;
        }
        
        auto t1 = std::chrono::high_resolution_clock::now();
        
        // Detect objects
        std::vector<std::vector<float>> detections = detector.detect(frame);
        
        // Update tracker
        std::vector<std::vector<float>> tracked_objects = tracker.update(detections);
        
        auto t2 = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        
        // Draw boxes
        if (view_img || save_img) {
            drawBoxes(frame, tracked_objects, class_names);
        }
        
        // Save to text file
        if (save_txt) {
            for (const auto& obj : tracked_objects) {
                txt_file << frame_idx << ","
                        << obj[0] << "," << obj[1] << ","  // x1, y1
                        << obj[2] << "," << obj[3] << ","  // x2, y2
                        << static_cast<int>(obj[4]) << "," // class
                        << obj[5] << "," << obj[6] << ","  // x_dot, y_dot
                        << obj[7] << ","                    // s_dot
                        << static_cast<int>(obj[8]) << "\n"; // track_id
            }
        }
        
        // Display
        if (view_img) {
            // Add FPS text
            std::string fps_text = "FPS: " + std::to_string(1000.0 / duration);
            cv::putText(frame, fps_text, cv::Point(10, 30),
                       cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2);
            
            cv::imshow("ClassySORT", frame);
            if (cv::waitKey(1) == 'q') {
                break;
            }
        }
        
        // Save frame
        if (save_img) {
            writer.write(frame);
        }
        
        // Print progress
        std::cout << "\rFrame " << frame_idx << " - " << tracked_objects.size() 
                  << " tracked objects - " << duration << " ms" << std::flush;
        
        frame_idx++;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
    
    std::cout << "\n\nDone! Processed " << frame_idx << " frames in " 
              << total_duration << " seconds" << std::endl;
    
    // Cleanup
    cap.release();
    if (writer.isOpened()) {
        writer.release();
    }
    if (txt_file.is_open()) {
        txt_file.close();
    }
    cv::destroyAllWindows();
    
    return 0;
}
