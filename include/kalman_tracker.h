#ifndef KALMAN_TRACKER_H
#define KALMAN_TRACKER_H

#include <opencv2/opencv.hpp>
#include <Eigen/Dense>

/**
 * KalmanBoxTracker - Tracks a single object using Kalman filter
 * This class represents the internal state of individual tracked objects
 * observed as bounding boxes.
 */
class KalmanBoxTracker {
public:
    /**
     * Initialize tracker with initial bounding box
     * @param bbox: [x1, y1, x2, y2, score, class_id]
     */
    KalmanBoxTracker(const std::vector<float>& bbox);
    
    /**
     * Update the state vector with observed bbox
     * @param bbox: [x1, y1, x2, y2, score, class_id]
     */
    void update(const std::vector<float>& bbox);
    
    /**
     * Advance the state vector and return the predicted bounding box estimate
     * @return predicted bbox [x1, y1, x2, y2]
     */
    std::vector<float> predict();
    
    /**
     * Get the current bounding box estimate with additional information
     * @return [x1, y1, x2, y2, class_id, x_dot, y_dot, s_dot]
     */
    std::vector<float> getState() const;
    
    int getId() const { return id; }
    int getTimeSinceUpdate() const { return time_since_update; }
    int getHitStreak() const { return hit_streak; }
    int getHits() const { return hits; }
    
private:
    // Kalman filter state: [x, y, s, r, x_dot, y_dot, s_dot]
    // where (x,y) is center, s is scale/area, r is aspect ratio
    Eigen::VectorXd state;
    Eigen::MatrixXd covariance;
    Eigen::MatrixXd transitionMatrix;  // F
    Eigen::MatrixXd measurementMatrix; // H
    Eigen::MatrixXd processNoise;      // Q
    Eigen::MatrixXd measurementNoise;  // R
    
    int id;
    int time_since_update;
    int hits;
    int hit_streak;
    int age;
    int det_class;  // YOLO detected class
    
    static int count;
    
    // Helper functions
    Eigen::Vector4d bboxToZ(const std::vector<float>& bbox);
    std::vector<float> xToBbox(const Eigen::VectorXd& x) const;
    void kalmanPredict();
    void kalmanUpdate(const Eigen::Vector4d& z);
};

#endif // KALMAN_TRACKER_H
