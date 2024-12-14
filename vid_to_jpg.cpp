/**************************************************************
Filename: vid_to_jpg.cpp
Description:
  Extracts frames from a video file and saves them as 
  JPG images. Uses OpenCV for video frame extraction and 
  image saving.
Author: Cade Andrae
Date: 12/11/24
**************************************************************/

#include <opencv2/opencv.hpp>
#include <iostream>

// Expose C++ function to be callable from C code
extern "C" void vid_to_jpg(const char* input_path, const char* output_path) {
    cv::VideoCapture capture(input_path);   // Open the video file

    if (!capture.isOpened()) {              // Check if the video file was successfully opened
        std::cerr << "Error: Cannot open video file " << input_path << std::endl;
        return;                             // Exit if the video cannot be opened
    }
    cv::Mat frame;                          // Holds each frame of the video
    int frameCount = 0;                     // Frame counter

    while (true) {                          // Loop to process each frame of the video
        capture >> frame;                   // Read the next frame from the video
        if (frame.empty())                  // Check for end of video
            break;
        std::string frameFileName = std::string(output_path) + "/frame_" + std::to_string(frameCount) + ".jpg"; // Generate the file name
        cv::imwrite(frameFileName, frame);  // Save the current frame
        std::cout << "Saved " << frameFileName << std::endl;
        frameCount++;                       // Increment the frame counter
    }
    std::cout << "Total frames processed: " << frameCount << std::endl;
}
