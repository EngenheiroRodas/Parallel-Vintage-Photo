#ifndef HELPER_F_H
#define HELPER_F_H

#include <stdlib.h>
#include <time.h>

extern char **file_list, *output_directory, *input_directory;

extern int pipe_fd[2];

extern int counter;

extern gdImagePtr in_texture_img;

extern pthread_mutex_t lock;

int read_command_line(int argc, char *argv[], size_t *file_count);

void edit_paths(int argc, char *argv[], char **output_txt, char **output_directory);

void *process_image(void *input_struct);
#endif