/**************************************************************
Filename: handle_motion.c 
Description:
  Contains functions to process frames for motion detection, 
  including multithreaded frame processing and frame count 
  calculation in a directory.
Author: Cade Andrae
Date: 12/11/24
**************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include "image_utils.h"
#include <dirent.h>
#include <regex.h>
#include "handle_motion.h"
#include <unistd.h>

// Function to process a batch of frames in a thread
void* process_frame_batch(void* arg) {
    ThreadData* data = (ThreadData*)arg;                                                                // Cast argument to ThreadData struct
    char frame1_path[256], frame_prev_path[256], output_file[256];

    for (int i = data->start_frame; i <= data->end_frame; ++i) {                                        // Iterate through the frames assigned to this thread
        snprintf(frame1_path, sizeof(frame1_path), "%s/frame_%d.jpg", data->input_path, i);             // Create paths for the current frame
        snprintf(output_file, sizeof(output_file), "%s/motion_frame_%d.jpg", data->output_path, i);     // Create path for output file

        int frame_width, frame_height;
        unsigned char* frame = load_jpeg(frame1_path, &frame_width, &frame_height);                     // Load the current frame as an RGB image
        if (!frame) {                                                                                   // Skip if the frame cannot be loaded
            printf("Error: Cannot load image %s. Skipping...\n", frame1_path);
            continue;
        }
        unsigned char* gray_frame = (unsigned char*)malloc(frame_width * frame_height);
        rgb_to_grayscale(frame, gray_frame, frame_width, frame_height);                                 // Convert the current frame to grayscale
        free(frame);                                                                                    

        snprintf(frame_prev_path, sizeof(frame_prev_path), "%s/frame_%d.jpg", data->input_path, i - 1); // Construct path for the previous frame
        unsigned char* prev_frame = load_jpeg(frame_prev_path, &frame_width, &frame_height);

        if (!prev_frame) {                                                                              // Check if previous frame can be loaded
            printf("Cannot load previous frame %s. Using current frame as reference.\n", frame_prev_path);
            
        } else {                                                                                        // The current frame will be used as the reference.
            unsigned char* prev_gray_frame = (unsigned char*)malloc(frame_width * frame_height);
            rgb_to_grayscale(prev_frame, prev_gray_frame, frame_width, frame_height);                   // Convert previous frame to greyscale
            free(prev_frame);

            unsigned char* diff = (unsigned char*)malloc(frame_width * frame_height);
            compute_difference(prev_gray_frame, gray_frame, diff, frame_width, frame_height);           // Compute the difference between the current and previous frames

            unsigned char* motion = (unsigned char*)malloc(frame_width * frame_height);
            apply_threshold(diff, motion, frame_width, frame_height, 20);                               // Apply a threshold to the difference to detect motion

            save_jpeg(output_file, motion, frame_width, frame_height);                                  // Save the motion-detected frame to the output file
            printf("Motion-detected image saved: %s\n", output_file);

            // Free allocated memory for difference and motion frames
            free(diff);
            free(motion);
            free(prev_gray_frame);
        }
        // Free the grayscale current frame
        free(gray_frame);
    }
    return NULL;
}

// Function to process frames using multiple threads
void process_frames_with_threads(const char* input_path, const char* output_path, int total_frames, int start_frame) {
    int num_threads = get_cpu_cores();                                                      // Determine the number of CPU cores to decide thread count
    pthread_t threads[num_threads];                                                         // Array to store thread identifiers
    ThreadData thread_data[num_threads];                                                    // Array to store thread-specific data

    int frames_per_thread = (total_frames - start_frame) / num_threads;                     // Calculate the number of frames each thread should process

    for (int i = 0; i < num_threads; ++i) {                                                 // Loop to create threads
        thread_data[i].start_frame = start_frame + i * frames_per_thread;                   // Assign the starting frame for the current thread
        thread_data[i].end_frame = (i == num_threads - 1) ? (total_frames - 1) : (start_frame + (i + 1) * frames_per_thread - 1); // Calculate the ending frame for the current thread
        thread_data[i].input_path = input_path;                                             // Set the input path for the current thread
        thread_data[i].output_path = output_path;                                           // Set the output path for the current thread
        if (pthread_create(&threads[i], NULL, process_frame_batch, &thread_data[i]) != 0) { // Create the thread to process its assigned frames
            fprintf(stderr, "Error: Could not create thread %d\n", i);
            exit(EXIT_FAILURE);                                                             // Exit if thread creation fails
        }
    }
    for (int i = 0; i < num_threads; ++i) {                                                 // Wait for all threads to complete
        pthread_join(threads[i], NULL);                                                     // Join threads to ensure completion
    }
}

// Function to count the number of frames in a directory
int count_frames_in_directory(const char* input_path) {
    DIR* dir = opendir(input_path);                                                     // Open the input directory             
    if (!dir) {                                                                         // Check if the directory could not be opened
        fprintf(stderr, "Error: Could not open input directory '%s'.\n", input_path);
        return -1;                                                                      // Failed
    }
    struct dirent* entry;
    int frame_count = 0;
    regex_t regex;
    if (regcomp(&regex, "^frame_[0-9]+\\.jpg$", REG_EXTENDED) != 0) {                   // Regular expression to match filenames like "frame_<number>.jpg"
        fprintf(stderr, "Error: Could not compile regex.\n");
        closedir(dir);                                                                  // Close the directory before returning
        return -1;                                                                      // Failed
    }
    while ((entry = readdir(dir)) != NULL) {                                            // Iterate through directory entries
        if (regexec(&regex, entry->d_name, 0, NULL, 0) == 0) {                          // Check if the entry name matches the regular expression
            frame_count++;                                                              // Increment the frame count for each match
        }
    }
    regfree(&regex);                                                                    // Free the compiled regular expression
    closedir(dir);                                                                      // Close the directory
    return frame_count;                                                                 // Return total number of frames
}
