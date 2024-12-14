#ifndef IMAGE_UTILS_H
#define IMAGE_UTILS_H

unsigned char* load_jpeg(const char* filename, int* width, int* height);
void save_jpeg(const char* filename, unsigned char* data, int width, int height);
void rgb_to_grayscale(unsigned char* rgb, unsigned char* gray, int width, int height);
void compute_difference(unsigned char* img1, unsigned char* img2, unsigned char* output, int width, int height);
void apply_threshold(unsigned char* input, unsigned char* output, int width, int height, unsigned char threshold);
int get_cpu_cores();

#endif