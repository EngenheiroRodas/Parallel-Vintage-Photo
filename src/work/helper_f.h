#ifndef HElPER_F_H
#define HELPER_F_H

#include <stdlib.h>

extern char **file_list; // Shared file list accessible by threads


char *read_command_line(int argc, char *argv[], size_t *file_count);
#endif