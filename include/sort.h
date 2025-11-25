#ifndef SORT_H
#define SORT_H

#include <vector>
#include <memory>
#include "kalman_tracker.h"

/**
 * SORT: Simple Online and Realtime Tracking
 * Modified for ClassySORT to maintain object class information
 */
class Sort {
public:
    /**
     * Initialize SORT tracker
     * @param max_age: Maximum number of frames to keep alive a track without detections
     * @param min_hits: Minimum number of associated detections before track is initialized
     * @param iou_threshold: Minimum IOU for match
     */
    Sort(int max_age = 5, int min_hits = 2, double iou_threshold = 0.2);
    
    /**
     * Update tracker with new detections
     * @param detections: vector of detections [x1, y1, x2, y2, conf, class_id]
     * @return tracked objects [x1, y1, x2, y2, class_id, x_dot, y_dot, s_dot, track_id]
     */
    std::vector<std::vector<float>> update(const std::vector<std::vector<float>>& detections);
    
private:
    int max_age;
    int min_hits;
    double iou_threshold;
    int frame_count;
    std::vector<std::shared_ptr<KalmanBoxTracker>> trackers;
    
    /**
     * Associate detections to tracked objects
     * @param detections: current frame detections
     * @param trackers: predicted tracker positions
     * @return tuple of (matches, unmatched_detections, unmatched_trackers)
     */
    std::tuple<std::vector<std::pair<int, int>>, 
               std::vector<int>, 
               std::vector<int>> 
    associateDetectionsToTrackers(const std::vector<std::vector<float>>& detections,
                                  const std::vector<std::vector<float>>& trackers);
};

#endif // SORT_H
