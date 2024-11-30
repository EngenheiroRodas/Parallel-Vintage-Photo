#ifndef HElPER_F_H
#define HELPER_F_H

#include <stdlib.h>

typedef struct input_ {
    int start_index;         // Starting index of files for the thread
    int end_index;           // Ending index of files for the thread
    char *output_directory;  // Output directory path
    char *input_directory;   // Input directory path
} input;

extern char **file_list; // Shared file list accessible by threads

char *read_command_line(int argc, char *argv[], size_t *file_count, char **output_txt);

void *process_image(void *input_struct);
#endif