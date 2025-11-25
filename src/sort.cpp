#include "sort.h"
#include "hungarian.h"
#include <algorithm>
#include <iostream>

Sort::Sort(int max_age, int min_hits, double iou_threshold)
    : max_age(max_age), min_hits(min_hits), iou_threshold(iou_threshold), frame_count(0) {
}

std::vector<std::vector<float>> Sort::update(const std::vector<std::vector<float>>& detections) {
    frame_count++;
    
    // Get predicted locations from existing trackers
    std::vector<std::vector<float>> predicted_boxes;
    std::vector<int> to_delete;
    
    for (size_t t = 0; t < trackers.size(); t++) {
        std::vector<float> pos = trackers[t]->predict();
        
        // Check for invalid predictions
        bool is_nan = false;
        for (float val : pos) {
            if (std::isnan(val)) {
                is_nan = true;
                break;
            }
        }
        
        if (is_nan) {
            to_delete.push_back(t);
        } else {
            predicted_boxes.push_back(pos);
        }
    }
    
    // Remove trackers with invalid predictions
    for (int i = to_delete.size() - 1; i >= 0; i--) {
        trackers.erase(trackers.begin() + to_delete[i]);
    }
    
    // Associate detections to trackers
    auto [matches, unmatched_dets, unmatched_trks] = 
        associateDetectionsToTrackers(detections, predicted_boxes);
    
    // Update matched trackers with assigned detections
    for (const auto& match : matches) {
        trackers[match.second]->update(detections[match.first]);
    }
    
    // Create and initialize new trackers for unmatched detections
    for (int i : unmatched_dets) {
        auto trk = std::make_shared<KalmanBoxTracker>(detections[i]);
        trackers.push_back(trk);
    }
    
    // Prepare output
    std::vector<std::vector<float>> result;
    std::vector<int> trackers_to_remove;
    
    for (int i = trackers.size() - 1; i >= 0; i--) {
        auto& trk = trackers[i];
        std::vector<float> state = trk->getState();
        
        // Return tracks that have been updated recently and have enough hits
        if (trk->getTimeSinceUpdate() < 1 && 
            (trk->getHitStreak() >= min_hits || frame_count <= min_hits)) {
            
            // Format: [x1, y1, x2, y2, class_id, x_dot, y_dot, s_dot, track_id]
            std::vector<float> output;
            output.push_back(state[0]);  // x1
            output.push_back(state[1]);  // y1
            output.push_back(state[2]);  // x2
            output.push_back(state[3]);  // y2
            output.push_back(state[4]);  // class_id
            output.push_back(state[5]);  // x_dot
            output.push_back(state[6]);  // y_dot
            output.push_back(state[7]);  // s_dot
            output.push_back(static_cast<float>(trk->getId() + 1));  // track_id (1-indexed)
            
            result.push_back(output);
        }
        
        // Remove dead tracklets
        if (trk->getTimeSinceUpdate() > max_age) {
            trackers_to_remove.push_back(i);
        }
    }
    
    // Remove dead trackers
    for (int i : trackers_to_remove) {
        trackers.erase(trackers.begin() + i);
    }
    
    return result;
}

std::tuple<std::vector<std::pair<int, int>>, std::vector<int>, std::vector<int>>
Sort::associateDetectionsToTrackers(const std::vector<std::vector<float>>& detections,
                                   const std::vector<std::vector<float>>& trackers) {
    std::vector<std::pair<int, int>> matches;
    std::vector<int> unmatched_detections;
    std::vector<int> unmatched_trackers;
    
    if (trackers.empty()) {
        for (size_t i = 0; i < detections.size(); i++) {
            unmatched_detections.push_back(i);
        }
        return {matches, unmatched_detections, unmatched_trackers};
    }
    
    // Calculate IoU matrix
    std::vector<std::vector<double>> iou_matrix = iouBatch(detections, trackers);
    
    // Convert to cost matrix (negative IoU for maximization)
    std::vector<std::vector<double>> cost_matrix = iou_matrix;
    
    // Solve assignment problem
    std::vector<int> assignment;
    HungarianAlgorithm::solve(cost_matrix, assignment);
    
    // Process assignments
    std::vector<bool> det_matched(detections.size(), false);
    std::vector<bool> trk_matched(trackers.size(), false);
    
    for (size_t d = 0; d < assignment.size(); d++) {
        int t = assignment[d];
        if (t != -1) {
            // Check if IoU is above threshold
            if (iou_matrix[d][t] >= iou_threshold) {
                matches.push_back({d, t});
                det_matched[d] = true;
                trk_matched[t] = true;
            }
        }
    }
    
    // Find unmatched detections
    for (size_t d = 0; d < detections.size(); d++) {
        if (!det_matched[d]) {
            unmatched_detections.push_back(d);
        }
    }
    
    // Find unmatched trackers
    for (size_t t = 0; t < trackers.size(); t++) {
        if (!trk_matched[t]) {
            unmatched_trackers.push_back(t);
        }
    }
    
    return {matches, unmatched_detections, unmatched_trackers};
}
