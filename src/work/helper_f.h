#ifndef HELPER_F_H
#define HELPER_F_H

extern int pipe_fd[2];

extern char **file_list, *output_directory, *input_directory;

extern size_t counter, file_count;

extern gdImagePtr in_texture_img;

extern pthread_mutex_t lock;

int read_command_line(int argc, char *argv[], size_t *file_count);

void edit_paths(int argc, char *argv[], char **output_txt);

#endif