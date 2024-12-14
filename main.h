#ifndef MAIN_H
#define MAIN_H

#define MAX_PATH 512

void vid_to_jpg(const char* input_path, const char* output_path);
void process_frames_with_threads(const char* input_path, const char* output_path, int total_frames, int start_frame);
void convert_to_video(const char* input_path, const char* output_filename, int framerate, const char* resolution);
int count_frames_in_directory(const char* input_path);
void start_server_mode(const char* input_path, const char* output_path);
void start_client_mode();

#endif
