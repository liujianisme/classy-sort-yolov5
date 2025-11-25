#include "kalman_tracker.h"
#include <iostream>

int KalmanBoxTracker::count = 0;

KalmanBoxTracker::KalmanBoxTracker(const std::vector<float>& bbox) {
    // Initialize state: [x, y, s, r, x_dot, y_dot, s_dot]
    state = Eigen::VectorXd::Zero(7);
    covariance = Eigen::MatrixXd::Identity(7, 7);
    
    // Transition matrix F
    transitionMatrix = Eigen::MatrixXd::Identity(7, 7);
    transitionMatrix(0, 4) = 1;  // x = x + x_dot
    transitionMatrix(1, 5) = 1;  // y = y + y_dot
    transitionMatrix(2, 6) = 1;  // s = s + s_dot
    
    // Measurement matrix H
    measurementMatrix = Eigen::MatrixXd::Zero(4, 7);
    measurementMatrix(0, 0) = 1;
    measurementMatrix(1, 1) = 1;
    measurementMatrix(2, 2) = 1;
    measurementMatrix(3, 3) = 1;
    
    // Measurement noise covariance R
    measurementNoise = Eigen::MatrixXd::Identity(4, 4);
    measurementNoise.block<2, 2>(2, 2) *= 10.0;
    
    // Process noise covariance Q
    processNoise = Eigen::MatrixXd::Identity(7, 7);
    processNoise(6, 6) *= 0.5;
    processNoise.block<3, 3>(4, 4) *= 0.5;
    
    // Initial covariance
    covariance.block<3, 3>(4, 4) *= 1000.0;  // high uncertainty for velocities
    covariance *= 10.0;
    
    // Initialize state with bbox
    Eigen::Vector4d z = bboxToZ(bbox);
    state.head<4>() = z;
    
    // Initialize tracker attributes
    time_since_update = 0;
    id = count++;
    hits = 0;
    hit_streak = 0;
    age = 0;
    det_class = static_cast<int>(bbox[5]);
}

void KalmanBoxTracker::update(const std::vector<float>& bbox) {
    time_since_update = 0;
    hits++;
    hit_streak++;
    
    Eigen::Vector4d z = bboxToZ(bbox);
    kalmanUpdate(z);
    det_class = static_cast<int>(bbox[5]);
}

std::vector<float> KalmanBoxTracker::predict() {
    // Check for invalid scale
    if ((state(6) + state(2)) <= 0) {
        state(6) = 0;
    }
    
    kalmanPredict();
    age++;
    
    if (time_since_update > 0) {
        hit_streak = 0;
    }
    time_since_update++;
    
    return xToBbox(state);
}

std::vector<float> KalmanBoxTracker::getState() const {
    std::vector<float> bbox = xToBbox(state);
    std::vector<float> result(bbox.begin(), bbox.end());
    result.push_back(static_cast<float>(det_class));
    result.push_back(static_cast<float>(state(4)));  // x_dot
    result.push_back(static_cast<float>(state(5)));  // y_dot
    result.push_back(static_cast<float>(state(6)));  // s_dot
    return result;
}

Eigen::Vector4d KalmanBoxTracker::bboxToZ(const std::vector<float>& bbox) {
    // Convert [x1, y1, x2, y2] to [x_center, y_center, scale, ratio]
    double w = bbox[2] - bbox[0];
    double h = bbox[3] - bbox[1];
    double x = bbox[0] + w / 2.0;
    double y = bbox[1] + h / 2.0;
    double s = w * h;  // scale (area)
    double r = w / h;  // aspect ratio
    
    Eigen::Vector4d z;
    z << x, y, s, r;
    return z;
}

std::vector<float> KalmanBoxTracker::xToBbox(const Eigen::VectorXd& x) const {
    // Convert [x_center, y_center, scale, ratio] to [x1, y1, x2, y2]
    double w = std::sqrt(x(2) * x(3));
    double h = x(2) / w;
    
    std::vector<float> bbox(4);
    bbox[0] = static_cast<float>(x(0) - w / 2.0);  // x1
    bbox[1] = static_cast<float>(x(1) - h / 2.0);  // y1
    bbox[2] = static_cast<float>(x(0) + w / 2.0);  // x2
    bbox[3] = static_cast<float>(x(1) + h / 2.0);  // y2
    return bbox;
}

void KalmanBoxTracker::kalmanPredict() {
    // Predict step: x' = F * x, P' = F * P * F^T + Q
    state = transitionMatrix * state;
    covariance = transitionMatrix * covariance * transitionMatrix.transpose() + processNoise;
}

void KalmanBoxTracker::kalmanUpdate(const Eigen::Vector4d& z) {
    // Update step
    // Innovation: y = z - H * x
    Eigen::Vector4d y = z - measurementMatrix * state;
    
    // Innovation covariance: S = H * P * H^T + R
    Eigen::Matrix4d S = measurementMatrix * covariance * measurementMatrix.transpose() + measurementNoise;
    
    // Kalman gain: K = P * H^T * S^-1
    Eigen::MatrixXd K = covariance * measurementMatrix.transpose() * S.inverse();
    
    // State update: x = x + K * y
    state = state + K * y;
    
    // Covariance update: P = (I - K * H) * P
    Eigen::MatrixXd I = Eigen::MatrixXd::Identity(7, 7);
    covariance = (I - K * measurementMatrix) * covariance;
}
