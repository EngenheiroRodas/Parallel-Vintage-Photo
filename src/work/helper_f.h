#ifndef HELPER_F_H
#define HELPER_F_H

#include <stdlib.h>

typedef struct {
    int start_index;         // Starting index of files for the thread
    int end_index;           // Ending index of files for the thread
    char *output_directory;  // Output directory path
    char *input_directory;   // Input directory path
} input;

extern char **file_list; // Shared file list accessible by threads

int read_command_line(int argc, char *argv[], size_t *file_count);

void edit_paths(int argc, char *argv[], char **output_txt, char **output_directory);

void *process_image(void *input_struct);
#endif