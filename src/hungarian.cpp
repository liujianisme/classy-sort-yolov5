#include "hungarian.h"
#include <algorithm>
#include <cmath>
#include <numeric>

double calculateIoU(const std::vector<float>& bb1, const std::vector<float>& bb2) {
    // Calculate intersection
    float xx1 = std::max(bb1[0], bb2[0]);
    float yy1 = std::max(bb1[1], bb2[1]);
    float xx2 = std::min(bb1[2], bb2[2]);
    float yy2 = std::min(bb1[3], bb2[3]);
    
    float w = std::max(0.0f, xx2 - xx1);
    float h = std::max(0.0f, yy2 - yy1);
    float intersection = w * h;
    
    // Calculate union
    float area1 = (bb1[2] - bb1[0]) * (bb1[3] - bb1[1]);
    float area2 = (bb2[2] - bb2[0]) * (bb2[3] - bb2[1]);
    float union_area = area1 + area2 - intersection;
    
    if (union_area <= 0) return 0.0;
    
    return intersection / union_area;
}

std::vector<std::vector<double>> iouBatch(const std::vector<std::vector<float>>& detections,
                                          const std::vector<std::vector<float>>& trackers) {
    std::vector<std::vector<double>> iou_matrix(detections.size(), 
                                                std::vector<double>(trackers.size(), 0.0));
    
    for (size_t i = 0; i < detections.size(); i++) {
        for (size_t j = 0; j < trackers.size(); j++) {
            iou_matrix[i][j] = calculateIoU(detections[i], trackers[j]);
        }
    }
    
    return iou_matrix;
}

// Simplified Hungarian algorithm implementation
double HungarianAlgorithm::solve(const std::vector<std::vector<double>>& costMatrix,
                                 std::vector<int>& assignment) {
    if (costMatrix.empty()) {
        return 0.0;
    }
    
    int rows = costMatrix.size();
    int cols = costMatrix[0].size();
    
    assignment.assign(rows, -1);
    
    // For this use case (IoU matching), we use a greedy approach
    // which works well for the SORT tracker
    std::vector<bool> col_used(cols, false);
    std::vector<std::pair<double, std::pair<int, int>>> values;
    
    // Collect all values
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            values.push_back({costMatrix[i][j], {i, j}});
        }
    }
    
    // Sort in descending order (for IoU, higher is better)
    std::sort(values.begin(), values.end(), std::greater<>());
    
    double total_cost = 0.0;
    
    // Greedy assignment
    for (const auto& val : values) {
        int row = val.second.first;
        int col = val.second.second;
        
        if (assignment[row] == -1 && !col_used[col]) {
            assignment[row] = col;
            col_used[col] = true;
            total_cost += val.first;
        }
    }
    
    return total_cost;
}
