#ifndef HUNGARIAN_H
#define HUNGARIAN_H

#include <vector>
#include <limits>

/**
 * Linear Assignment solver using greedy algorithm
 * Note: This is a simplified greedy approach suitable for real-time tracking
 * For optimal assignment, consider implementing the full Hungarian algorithm
 */
class LinearAssignment {
public:
    /**
     * Solve the assignment problem using greedy algorithm
     * @param costMatrix: NxM cost matrix (higher values = better matches for IoU)
     * @param assignment: output vector of assignments (size N)
     * @return total cost
     */
    static double solve(const std::vector<std::vector<double>>& costMatrix, 
                       std::vector<int>& assignment);
};

/**
 * Calculate IoU (Intersection over Union) between two bounding boxes
 * @param bb1: [x1, y1, x2, y2]
 * @param bb2: [x1, y1, x2, y2]
 * @return IoU value
 */
double calculateIoU(const std::vector<float>& bb1, const std::vector<float>& bb2);

/**
 * Calculate IoU matrix for batch of detections and trackers
 * @param detections: vector of detection bboxes
 * @param trackers: vector of tracker bboxes
 * @return IoU matrix
 */
std::vector<std::vector<double>> iouBatch(const std::vector<std::vector<float>>& detections,
                                          const std::vector<std::vector<float>>& trackers);

#endif // HUNGARIAN_H
