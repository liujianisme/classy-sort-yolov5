cmake_minimum_required(VERSION 3.10)
project(ClassySORT_CPP VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenCV
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# Find Eigen3 for Kalman filter
find_package(Eigen3 3.3 REQUIRED NO_MODULE)

# Source files
set(SOURCES
    src/main.cpp
    src/sort.cpp
    src/kalman_tracker.cpp
    src/hungarian.cpp
    src/yolo_detector.cpp
)

# Header files
set(HEADERS
    include/sort.h
    include/kalman_tracker.h
    include/hungarian.h
    include/yolo_detector.h
)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Create executable
add_executable(classy_sort ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(classy_sort ${OpenCV_LIBS} Eigen3::Eigen)

# Compiler flags
target_compile_options(classy_sort PRIVATE -Wall -Wextra -O3)

# Installation
install(TARGETS classy_sort DESTINATION bin)
