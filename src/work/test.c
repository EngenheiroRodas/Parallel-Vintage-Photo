/******************************************************************************
 * Programacao Concorrente
 * LEEC 24/25
 *
 * Projecto - Parte1
 *                           old-photo-parallel-A.c
 * 
 *****************************************************************************/
#include <gd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include "image-lib.h"
#include "helper_f.h"

int pipe_fd[2];

char *input_directory, *output_directory;

int main(int argc, char *argv[]) {
    char *output_txt;
    int num_threads;
    size_t file_count = 0;

    if (pipe(pipe_fd) == -1) {
        perror("Failed to create pipe");
        exit(EXIT_FAILURE);
    }

    // Edit timing.txt and output directory paths
    edit_paths(argc, argv, &output_txt, &output_directory);
    input_directory = argv[1];

    // No more threads than files
    num_threads = read_command_line(argc, argv, &file_count);

    // Debug: Print the contents of file_list
    printf("Debug: Contents of file_list (%zu files):\n", file_count);
    for (size_t i = 0; i < file_count; i++) {
        printf("file_list[%zu]: %s\n", i, file_list[i]);
    }

    // Write file names to the pipe
    char message[256];
    for (size_t i = 0; i < file_count; i++) {
        snprintf(message, sizeof(message), "%s\n", file_list[i]); // Add delimiter
        if (write(pipe_fd[1], message, strlen(message)) == -1) {
            perror("Failed to write to pipe");
            free(output_directory);
            exit(EXIT_FAILURE);
        }
    }
    close(pipe_fd[1]); // Close the write end of the pipe

    // Read file names from the pipe and print them
    char buffer[256];
    printf("\nDebug: Reading from the pipe:\n");
    for (size_t i = 0; i < file_count; i++) {
        ssize_t bytes_read = read(pipe_fd[0], buffer, sizeof(buffer) - 1);
        if (bytes_read == -1) {
            perror("Failed to read from pipe");
            free(output_directory);
            exit(EXIT_FAILURE);
        } else if (bytes_read == 0) {
            break; // End of pipe
        }

        buffer[bytes_read] = '\0'; // Null-terminate the string
        // Remove trailing newline
        char *newline_pos = strchr(buffer, '\n');
        if (newline_pos) *newline_pos = '\0';

        printf("Read from pipe: %s\n", buffer);
    }
    close(pipe_fd[0]); // Close the read end of the pipe

    // Thread return and cleanup
    for (size_t i = 0; i < file_count; i++) {
        free(file_list[i]);
    }
    free(file_list);
    free(output_directory);

    printf("\nAll images processed successfully.\n");

    free(output_txt);

    return 0;
}
