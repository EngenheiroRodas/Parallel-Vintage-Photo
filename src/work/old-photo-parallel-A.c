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
#include "image-lib.h"
#include "helper_f.h"

int pipe_fd[2];

int main(int argc, char *argv[]) {
    struct timespec start_time_total, end_time_total;
    struct timespec start_time_serial, end_time_serial;

	clock_gettime(CLOCK_MONOTONIC, &start_time_total);
	clock_gettime(CLOCK_MONOTONIC, &start_time_serial);

    FILE *output_file_txt;
    char *output_txt, *output_directory;
    int num_threads;
    size_t file_count = 0;

    // Texture read only one time
    gdImagePtr texture_img = read_png_file("./paper-texture.png");
    if (!texture_img) {
        fprintf(stderr, "Error reading texture image.\n");
        pthread_exit(NULL);
    }

    if (pipe(pipe_fd) == -1) {
        perror("Failed to create pipe");
        exit(EXIT_FAILURE);
    }

    // Edit timing.txt and output directory paths
    edit_paths(argc, argv, &output_txt, &output_directory);

    // No more threads than files
    num_threads = read_command_line(argc, argv, &file_count);

    // Prep of thread argument parsing
    pthread_t threads[num_threads];
    input thread_inputs[num_threads];

    for (size_t i = 0; i < file_count; i++) {
        if (write(pipe_fd[1], file_list[i], strlen(file_list[i])) == -1) {
            perror("Failed to write to pipe");
            free(output_directory);
            exit(EXIT_FAILURE);
        }
    }


    clock_gettime(CLOCK_MONOTONIC, &end_time_serial);

    // Thread launch
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&threads[i], NULL, process_image, &thread_inputs[i]) != 0) {
            perror("Failed to create thread");
            free(output_directory);
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Failed to join thread");
            free(output_directory);
            exit(EXIT_FAILURE);
        }
    }
    // Thread return and cleanup

    free(output_directory);
    for (size_t i = 0; i < file_count; i++) {
        free(file_list[i]);
    }
    free(file_list);

    gdImageDestroy(texture_img);

    printf("All images processed successfully.\n");
    
	clock_gettime(CLOCK_MONOTONIC, &end_time_total);

struct timespec thread_time[num_threads];
struct timespec serial_time = diff_timespec(&end_time_serial, &start_time_serial);
struct timespec total_time = diff_timespec(&end_time_total, &start_time_total);

    // Printing of times to timing_<n>-<MODE>.txt
    for (int i = 0; i < num_threads; i++) {
        thread_time[i] = diff_timespec(&thread_inputs[i].end_thread, &thread_inputs[i].start_thread);
    }

    output_file_txt = fopen(output_txt, "w");
    if (output_file_txt == NULL) {
        perror("Failed to open output file");
        free(output_txt);
        exit(EXIT_FAILURE);
    }

    fprintf(output_file_txt, "\n\nserial: \t %10jd.%09ld\n", serial_time.tv_sec, serial_time.tv_nsec);
    for (int i = 0; i < num_threads; i++) {
        fprintf(output_file_txt, "\tthread: %d \t %10jd.%09ld\n", i, thread_time[i].tv_sec, thread_time[i].tv_nsec);
    }
    fprintf(output_file_txt, "total: \t %10jd.%09ld\n", total_time.tv_sec, total_time.tv_nsec);

    fclose(output_file_txt);
    free(output_txt);

    return 0;
}
