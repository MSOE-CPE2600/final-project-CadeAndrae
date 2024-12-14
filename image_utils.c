/**************************************************************
Filename: image_utils.c 
Description:
  Provides utility functions for image control, including 
  JPG loading/saving, RGB-to-grayscale conversion, difference 
  computation, and binary threshold application.
Author: Cade Andrae
Date: 12/11/24
**************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

// Function to load a JPEG file into memory
unsigned char* load_jpeg(const char* filename, int* width, int* height) {
    FILE* file = fopen(filename, "rb");                             // Open the file in binary read mode
    if (!file) {                                                    // Check if the file could not be opened
        fprintf(stderr, "Error: Cannot open file %s\n", filename);  
        return NULL;                                                // Failed
    }

    struct jpeg_decompress_struct info;
    struct jpeg_error_mgr err;

    info.err = jpeg_std_error(&err);                                // Set up standard error handling
    jpeg_create_decompress(&info);                                  // Initialize the decompression object
    jpeg_stdio_src(&info, file);                                    // Specify the data source (file)
    jpeg_read_header(&info, TRUE);                                  // Read the JPEG header to get image info
    jpeg_start_decompress(&info);                                   // Start decompression

    *width = info.output_width;                                     // Extract image dimensions and components
    *height = info.output_height;
    int num_channels = info.output_components;                      // Number of color channels

    unsigned char* data = (unsigned char*)malloc((*width) * (*height) * num_channels);  // Allocate memory for the decompressed image
    unsigned char* rowptr[1];
    while (info.output_scanline < info.output_height) {                                 // Read each row of the image
        rowptr[0] = data + info.output_scanline * (*width) * num_channels;              // Point to row
        jpeg_read_scanlines(&info, rowptr, 1);                                          // Read row of scanlines
    }
    jpeg_finish_decompress(&info);      // Finish decompression
    jpeg_destroy_decompress(&info);     // Destroy the decompression object
    fclose(file);                       // Close the file
    return data;
}

// Function to save a grayscale JPEG image
void save_jpeg(const char* filename, unsigned char* data, int width, int height) {
    FILE* file = fopen(filename, "wb");     // Open the file in binary write mode
    if (!file) {                            // Check if the file could not be opened
        fprintf(stderr, "Error: Cannot open file %s for writing\n", filename);
        return;                             // Exit
    }

    struct jpeg_compress_struct info;
    struct jpeg_error_mgr err;

    info.err = jpeg_std_error(&err);        // Set up standard error handling
    jpeg_create_compress(&info);            // Initialize the compression object
    jpeg_stdio_dest(&info, file);           // Specify the data destination

    info.image_width = width;               // Image width
    info.image_height = height;             // Image height
    info.input_components = 1;              // Grayscale = 1 component
    info.in_color_space = JCS_GRAYSCALE;    // Specify grayscale color space

    jpeg_set_defaults(&info);               // Set default compression parameters
    jpeg_start_compress(&info, TRUE);       // Start compression

    unsigned char* rowptr[1];
    while (info.next_scanline < info.image_height) {        // Pointer to the current row in the image buffer
        rowptr[0] = data + info.next_scanline * width;      // Point to row
        jpeg_write_scanlines(&info, rowptr, 1);             // Write row of scanlines
    }
    jpeg_finish_compress(&info);    // Finish compression
    jpeg_destroy_compress(&info);   // Destroy the compression object
    fclose(file);                   // Close the file
}

// Function to convert an RGB image to grayscale
void rgb_to_grayscale(unsigned char* rgb, unsigned char* gray, int width, int height) {
    for (int i = 0; i < width * height; i++) {
        unsigned char r = rgb[3 * i];       // Red channel
        unsigned char g = rgb[3 * i + 1];   // Green channel
        unsigned char b = rgb[3 * i + 2];   // Blue channel
        gray[i] = (r * 0.2989) + (g * 0.5870) + (b * 0.1140); // Weighted average
    }
}

// Function to compute absolute difference between two grayscale images
void compute_difference(unsigned char* img1, unsigned char* img2, unsigned char* output, int width, int height) {
    for (int i = 0; i < width * height; i++) {
        output[i] = abs(img1[i] - img2[i]);
    }
}

// Function to apply a binary threshold
void apply_threshold(unsigned char* input, unsigned char* output, int width, int height, unsigned char threshold) {
    for (int i = 0; i < width * height; i++) {
        output[i] = (input[i] > threshold) ? 255 : 0;   // Set the output pixel to 255 if the input exceeds the threshold, otherwise 0
    }
}

// Function to get the number of CPU cores available
int get_cpu_cores() {
    long cores = sysconf(_SC_NPROCESSORS_ONLN);         // Get the number of online CPU cores
    if (cores < 1) {                                    // Check if the result is invalid
        perror("Error detecting CPU cores");
        exit(EXIT_FAILURE);
    }
    return (int)cores;                                  // Return the number of cores as an integer
}