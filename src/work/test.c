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

    for (size_t i = 0; i < file_count; i++) {
        // Write the pointer to file_list[i] instead of the string content
        if (write(pipe_fd[1], &file_list[i], sizeof(file_list[i])) == -1) {
            perror("Failed to write to pipe");
            free(output_directory);
            exit(EXIT_FAILURE);
        }
    }
    close(pipe_fd[1]); // Close the write end of the pipe

    // Read file names from the pipe and print them
    char *current_file;
    printf("\nDebug: Reading from the pipe:\n");
    while (1) {
        ssize_t bytes_read = read(pipe_fd[0], &current_file, sizeof(current_file));
        if (bytes_read == -1) {
            perror("Failed to read from pipe");
            pthread_exit(NULL);
        } else if (bytes_read == 0) {
            // No more data to read; exit the loop
            break;
        }

        // Use the pointer to access the filename
        printf("Processing file: %s\n", current_file);
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
