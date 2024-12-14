#ifndef HANDLE_MOTION_H
#define HANDLE_MOTION_H

typedef struct {
    int start_frame;
    int end_frame;
    const char* input_path;
    const char* output_path;
} ThreadData;

void process_frames_with_threads(const char* input_path, const char* output_path, int total_frames, int start_frame);
int count_frames_in_directory(const char* input_path);

#endif
