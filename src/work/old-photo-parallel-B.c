/******************************************************************************
 * Programacao Concorrente
 * LEEC 24/25
 *
 * 
 *                           old-photo-parallel-B.c
 * 
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

#include "image-lib.h"
#include "helper_f.h"
#include "threads.h"

/* 
 * Global Variables:
 * 
 * - pipe_fd[2]: File descriptors for a pipe that holds the file names.
 * 
 * - file_list: Dynamically allocated array of strings holding file names to be processed.
 *   - Initialized to NULL and allocated during runtime.
 * 
 * - input_directory, output_directory: Strings holding the paths to the input and output directories.
 *   - Set once at the start of the program; read-only after initialization.
 * 
 * - counter: Tracks the number of files processed so far.
 * - file_count: Total number of files to be processed.
 * 
 * - total_pic_time: Holds the cumulative processing time for all pictures.
 * 
 * - in_texture_img: Pointer to an `gdImage` structure holding the texture image.
 * 
 * - lock: Mutex used to protect shared resources.
 */
int pipe_fd[2];
char **file_list = NULL;
char *input_directory, *output_directory;
size_t counter = 0, file_count = 0;
struct timespec total_pic_time;
gdImagePtr in_texture_img = NULL;
pthread_mutex_t lock;


int main(int argc, char *argv[]) {
    struct timespec start_time_serial, end_time_serial, serial_time;
    struct timespec start_time_total, end_time_total, total_time;

	clock_gettime(CLOCK_MONOTONIC, &start_time_total);
	clock_gettime(CLOCK_MONOTONIC, &start_time_serial);

    FILE *output_file_txt;
    char *output_txt;
    int num_threads;

    input_directory = argv[1]; // makes argv[1] global
    
    // Texture read only one time
    in_texture_img = read_png_file("./paper-texture.png");
    if (!in_texture_img) {
        fprintf(stderr, "Error reading texture image.\n");
        pthread_exit(NULL);
    }

    if (pipe(pipe_fd) == -1) {
        perror("Failed to create pipe");
        exit(EXIT_FAILURE);
    }

    if (pthread_mutex_init(&lock, NULL) != 0) {
        perror("Failed to create mutex");
        exit(EXIT_FAILURE);
    }

    output_txt = edit_paths(argc, argv, &output_txt);

    num_threads = read_command_line(argc, argv, &file_count);
    
    // Write addresses to pipe
    for (size_t i = 0; i < file_count; i++) {
        if (write(pipe_fd[1], &file_list[i], sizeof(char *)) == -1) {
            perror("Failed to write to pipe");
            free(output_directory);
            exit(EXIT_FAILURE);
        }
    }
    close(pipe_fd[1]); // Close the write end of the pipe

    // Prep of thread argument parsing
    pthread_t thread_ids[num_threads + 1];
    struct timespec thread_time[num_threads];

    clock_gettime(CLOCK_MONOTONIC, &end_time_serial);

    // =======================================================================================
    // Threads that process image
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&thread_ids[i], NULL, process_image, NULL) != 0) {
            perror("Failed to create thread");
            free(output_directory);
            exit(EXIT_FAILURE);
        }
    }

    // Thread launch to handle S key press
    if (pthread_create(&thread_ids[num_threads], NULL, handle_key_press, NULL) != 0) {
        perror("Failed to create thread");
        free(output_directory);
        exit(EXIT_FAILURE);
    }
    // =======================================================================================
    // Wait for all threads to finish
    for (int i = 0; i < num_threads; i++) {
        struct timespec *thread_time_ptr;
        if (pthread_join(thread_ids[i], (void **)&thread_time_ptr) != 0) {
            perror("Failed to join thread");
            free(output_directory);
            exit(EXIT_FAILURE);
        }
        thread_time[i] = *thread_time_ptr; // Copy the thread time
        free(thread_time_ptr); // Free the allocated memory
    }
    close(pipe_fd[0]);
    close(STDIN_FILENO);
    // =======================================================================================

    // Cleanup
    for (size_t i = 0; i < file_count; i++) {
        free(file_list[i]);
    }
    free(file_list);
    free(output_directory);

    gdImageDestroy(in_texture_img);

    pthread_mutex_destroy(&lock);

    printf("All images processed successfully.\n");


    // =========================== Stat Printing Section =====================================
    clock_gettime(CLOCK_MONOTONIC, &end_time_total);

    serial_time = diff_timespec(&end_time_serial, &start_time_serial);
    total_time = diff_timespec(&end_time_total, &start_time_total);

    // Starts writing to text file
    output_file_txt = fopen(output_txt, "w");
    if (output_file_txt == NULL) {
        perror("Failed to open output file");
        free(output_txt);
        exit(EXIT_FAILURE);
    }

    // Print serial time
    fprintf(output_file_txt, "\nserial: \t %10jd.%03lds\n", 
            serial_time.tv_sec, serial_time.tv_nsec / 1000000);

    // Print thread times
    for (int i = 0; i < num_threads; i++) {
        fprintf(output_file_txt, "\tthread: %d \t %10jd.%03lds\n", i, 
                thread_time[i].tv_sec, thread_time[i].tv_nsec / 1000000);
    }

    // Print total time
    fprintf(output_file_txt, "total: \t %10jd.%03lds\n", 
            total_time.tv_sec, total_time.tv_nsec / 1000000);

    fclose(output_file_txt);
    free(output_txt);

    exit(EXIT_SUCCESS);
}