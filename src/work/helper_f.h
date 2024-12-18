#ifndef HELPER_F_H
#define HELPER_F_H

#include <stdlib.h>
#include <time.h>

typedef struct {
    char *output_directory, *input_directory; // Output and input directories
    struct timespec start_thread, end_thread; // Start time of the thread
    gdImagePtr in_texture_img;                // Texture image
} input;

extern char **file_list; // Shared file list accessible by threads

extern int pipe_fd[2];

extern *output_directory, *input_directory;

extern gdImagePtr in_texture_img;

int read_command_line(int argc, char *argv[], size_t *file_count);

void edit_paths(int argc, char *argv[], char **output_txt, char **output_directory);

void *process_image(void *input_struct);
#endif