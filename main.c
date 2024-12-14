/**************************************************************
Filename: main.c 
Description:
  Entry point for the motion detection application. Provides a 
  menu-driven interface for tasks such as video-to-frame 
  conversion, motion detection, and frame-to-video conversion. 
  Includes server and client modes for distributed processing.
Author: Cade Andrae
Date: 12/11/24
Compile Instructions:
  gcc -o main main.c handle_motion.c image_utils.c network_utils.c -lpthread -ljpeg
Test Instructions:
  - Run the program and follow the menu prompts.
  - Select "Convert video to frames" to test video frame extraction.
  - Select "Perform motion detection" to test motion detection.
  - Select "Convert frames to video" to test frame-to-video conversion.
  - Test server and client modes by running on two terminals.
**************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <pthread.h>
#include "main.h"
#include "handle_motion.h"
#include "network_utils.h"

// Displays the main menu
void show_menu() {
    printf("\n--- Motion Detection Program ---\n");
    printf("1. Convert video to frames\n");
    printf("2. Perform motion detection on frames\n");
    printf("3. Convert frames to video\n");
    printf("4. Run as server\n");
    printf("5. Run as client\n");
    printf("6. Exit\n");
    printf("Choose an option: ");
}

// Build the full path for a file or directory
void build_full_path(char* full_path, const char* filename) {
    if (getcwd(full_path, MAX_PATH) == NULL) {                      // Get the current working directory
        fprintf(stderr, "Error: Unable to get the directory.\n");
        exit(EXIT_FAILURE);                                         // Exit with failure status
    }
    strcat(full_path, "/");                                         // Append directory separator
    strcat(full_path, filename);                                    // Append the filename or directory name
}

// Check if a file exists
int file_exists(const char* path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);                              // Return true if the file exists
}

// Clear the input buffer to handle invalid or extra inputs
void clear_buffer() {
    while (getchar() != '\n');                                      // Clear input buffer
}

// Prompt the user for a file path and validate its existence
int prompt_file(const char* prompt, char* full_path) {
    char filename[256];
    while (1) {
        printf("%s", prompt);
        scanf("%s", filename);                                      // Get filename from user
        if (strcmp(filename, "home") == 0) {                        // Check for "home"
            clear_buffer();                                         // Clear buffer
            return 0;                                               // Return to main menu
        }
        build_full_path(full_path, filename);                       // Build the full path for the file
        if (file_exists(full_path)) {                               // Valid file breaks loop
            break;
        }
        fprintf(stderr, "Error: File '%s' does not exist. Please try again.\n", full_path);
    }
    return 1;                                                       // Valid input
}

// Prompt the user for a directory and validate its existence
int prompt_directory(const char* prompt, char* full_path) {
    char dirname[256];
    struct stat s;
    while (1) {
        printf("%s", prompt);
        scanf("%s", dirname);                                       // Get directory name from user
        if (strcmp(dirname, "home") == 0) {                         // Check for "home"
            clear_buffer();                                         // Clear buffer
            return 0;                                               // Return to main menu
        }
        build_full_path(full_path, dirname);                        // Build the full path for the directory
        if (stat(full_path, &s) == 0 && S_ISDIR(s.st_mode)) {       // Valid file breaks loop
            break;
        }
        fprintf(stderr, "Error: Directory '%s' does not exist. Please try again.\n", full_path);
    }
    return 1;                                                       // Valid input
}

// Prompt the user for a positive integer and validate the input
int prompt_positive_int(const char* prompt) {
    int value;
    while (1) {
        printf("%s", prompt);
        char input[32];
        scanf("%s", input);                                         // Get input from user
        if (strcmp(input, "home") == 0) {                           // Check if user wants to return to the main menu
            return 0;                                               // Return to main menu
        }
        if (sscanf(input, "%d", &value) == 1 && value > 0) {
            return value;                                           // Valid positive integer
        }
        fprintf(stderr, "Error: Please enter a positive integer.\n");
    }
}

// Prompt the user for a resolution in WIDTHxHEIGHT format and validate the input
int prompt_resolution(const char* prompt, char* resolution) {
    while (1) {
        printf("%s", prompt);
        scanf("%s", resolution);
        if (strcmp(resolution, "home") == 0) {                      // Check if user wants to return to the main menu
            clear_buffer();                                         // Clears buffer
            return 0;                                               // Return to main menu
        }
        if (strstr(resolution, "x") != NULL) {                      // Validate resolution format
            break;                                                  // Valid resolution breaks loop
        }
        fprintf(stderr, "Error: Resolution must be in the format WIDTHxHEIGHT (e.g., 1280x720). Please try again.\n");
    }
    return 1;                                                       // Valid input
}

// Start server mode to process frames in parallel with a client
void start_server_mode(const char* input_path, const char* output_path) {
    int total_frames = count_frames_in_directory(input_path);                       // Count total frames in the directory
    if (total_frames <= 0) {                                                        // Check for no frames
        fprintf(stderr, "Error: No frames found in input directory.\n");
        return;
    }
    int half_frames = total_frames / 2;                                             // Split the total frames for server and client
    printf("Server: Processing first half of the frames...\n");
    process_frames_with_threads(input_path, output_path, half_frames, 0);           // Process first half
    printf("Waiting for client to process remaining frames...\n");
    start_server(input_path, output_path, half_frames, total_frames - 1);           // Wait for client to process second half
    printf("Server processing completed.\n");
}

// Start client mode to process frames as a client
void start_client_mode() {
    printf("Client: Waiting for server command...\n");
    start_client();                                                                 // Start the client process
}

// Validate or create a directory
int validate_or_create_directory(const char* directory_name) {
    struct stat s;
    if (strcmp(directory_name, "home") == 0) {                                      // Check if the input is "home"
        return 0;                                                                   // Return to main menu
    }
    if (stat(directory_name, &s) == 0) {                                            // Check if the directory exists and get its attributes
        if (!S_ISDIR(s.st_mode)) {                                                  // Check if path is NOT a directory  
            fprintf(stderr, "Error: '%s' exists but is not a directory.\n", directory_name);
            return -1;                                                              // Error
        }
    } else {
        printf("Directory '%s' does not exist. Creating it...\n", directory_name);
        if (mkdir(directory_name, 0755) != 0) {                                     // Attempt to create the directory
            fprintf(stderr, "Error: Failed to create directory '%s'.\n", directory_name);
            return -1;                                                              // Error
        }
    }
    return 1;                                                                       // Directory validated or created
}

int main() {
    int choice;
    char input_full_path[MAX_PATH];
    char output_full_path[MAX_PATH];
    char resolution[32];
    int framerate;

    do {
        show_menu(); // Display the main menu
        if (scanf("%d", &choice) != 1) {
            fprintf(stderr, "Error: Invalid input. Please enter a number between 1 and 6.\n");
            clear_buffer(); // Clear buffer
            continue;
        }

        switch (choice) {
            case 1: // Convert video to frames
                if (!prompt_file("Enter input video filename (e.g., video.mp4): ", input_full_path)) continue;
                printf("Enter output directory name (e.g., frames): ");
                scanf("%s", output_full_path);
                if (strcmp(output_full_path, "home") == 0) continue;
                if (validate_or_create_directory(output_full_path) < 1) continue;
                vid_to_jpg(input_full_path, output_full_path);
                break;

            case 2: // Perform motion detection
                if (!prompt_directory("Enter input directory name of frames (e.g., frames): ", input_full_path)) continue;
                printf("Enter output directory name for motion-detected frames (e.g., motion_output): ");
                scanf("%s", output_full_path);
                if (strcmp(output_full_path, "home") == 0) continue;
                if (validate_or_create_directory(output_full_path) < 1) continue;
                int total_frames = count_frames_in_directory(input_full_path);
                process_frames_with_threads(input_full_path, output_full_path, total_frames, 0);
                printf("Motion detection completed.\n");
                break;

            case 3: // Convert frames to video
                if (!prompt_directory("Enter input directory name of frames (e.g., frames): ", input_full_path)) continue;
                printf("Enter output video filename (e.g., output.mp4): ");
                scanf("%s", output_full_path);
                if (strcmp(output_full_path, "home") == 0) continue;
                if ((framerate = prompt_positive_int("Enter desired framerate (e.g., 30): ")) == 0) continue;
                if (!prompt_resolution("Enter desired resolution (e.g., 1280x720): ", resolution)) continue;
                convert_to_video(input_full_path, output_full_path, framerate, resolution);
                break;

            case 4: // Server mode
                if (!prompt_directory("Server: Enter input directory name of frames: ", input_full_path)) continue;
                printf("Server: Enter output directory name for motion-detected frames: ");
                scanf("%s", output_full_path);
                if (strcmp(output_full_path, "home") == 0) continue;
                if (validate_or_create_directory(output_full_path) < 1) continue;
                start_server_mode(input_full_path, output_full_path);
                break;

            case 5: // Client mode
                start_client_mode();
                break;

            case 6: // Exit program
                printf("Exiting program.\n");
                break;

            default: // Invalid input
                fprintf(stderr, "Error: Invalid option. Please enter a number between 1 and 6.\n");
        }
    } while (choice != 6);
    return 0;
}

// Convert frames to a video using FFmpeg
void convert_to_video(const char* input_path, const char* output_filename, int framerate, const char* resolution) {
    char command[1024];

    // Build the FFmpeg command
    snprintf(command, sizeof(command),
             "ffmpeg -framerate %d -i %s/motion_frame_%%d.jpg -vf scale=%s -c:v libx264 -pix_fmt yuv420p %s",
             framerate, input_path, resolution, output_filename);

    printf("Executing command: %s\n", command);
    int result = system(command);

    if (result == 0) {
        printf("Video created successfully: %s\n", output_filename);
    } else {
        fprintf(stderr, "Error: Failed to create video. Make sure FFmpeg is installed and the frames exist.\n");
    }
}