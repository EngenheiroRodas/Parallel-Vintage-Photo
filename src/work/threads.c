#include <gd.h>
#include <pthread.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> 
#include <sys/stat.h>   
#include <stdlib.h>
#include <unistd.h>

#include "image-lib.h"
#include "threads.h"
#include "helper_f.h"


void *process_image(void *arg) {
    struct timespec start_thread, end_thread, thread_time;
    clock_gettime(CLOCK_MONOTONIC, &start_thread);

    char full_path[512];
    char out_file_name[512];
    char *current_file;

    gdImagePtr in_img, out_smoothed_img, out_contrast_img, out_textured_img, out_sepia_img;

    while (1) {
        // Read a filename from the pipe
        ssize_t bytes_read = read(pipe_fd[0], &current_file, sizeof(char *));
        if (bytes_read == -1) {
            perror("Failed to read from pipe");
            pthread_exit(NULL);
        } else if (bytes_read == 0) {
            // No more data to read; exit the loop
            break;
        }

        // Generate output file path and check if it already exists
        snprintf(out_file_name, sizeof(out_file_name), "%s/%s", output_directory, current_file);
        if (access(out_file_name, F_OK) != -1) {
            continue; // Skip processing if the output file already exists
        }

        snprintf(full_path, sizeof(full_path), "%s/%s", input_directory, current_file);

        in_img = read_jpeg_file(full_path);
        if (!in_img) {
            fprintf(stderr, "Cannot read image: %s\n", full_path);
            continue;
        }

        printf("Processing file: %s\n", current_file);

        // Process the image through various filters
        out_contrast_img = contrast_image(in_img);
        out_smoothed_img = smooth_image(out_contrast_img);
        out_textured_img = texture_image(out_smoothed_img, in_texture_img);
        out_sepia_img = sepia_image(out_textured_img);

        // Write the processed image to the output file
        if (!write_jpeg_file(out_sepia_img, out_file_name)) {
            fprintf(stderr, "Failed to write image: %s\n", out_file_name);
        }

        // Cleanup intermediate images
        gdImageDestroy(out_contrast_img);
        gdImageDestroy(out_smoothed_img);
        gdImageDestroy(out_textured_img);
        gdImageDestroy(out_sepia_img);
        gdImageDestroy(in_img);

        // Increment the counter (shared variable)
        pthread_mutex_lock(&lock);
        counter++;
        pthread_mutex_unlock(&lock);
    }

    // Record thread execution time
    clock_gettime(CLOCK_MONOTONIC, &end_thread);
    thread_time = diff_timespec(&end_thread, &start_thread);

    struct timespec *thread_time_ptr = malloc(sizeof(struct timespec));
    *thread_time_ptr = thread_time;

    pthread_exit(thread_time_ptr);
}

void *handle_key_press(void *arg) {
    char c;
    while ((c = getchar()) != EOF) {
        if (c == 's' || c == 'S') {
            printf("Total images processed: %d\n", counter);
        }
    }
    return NULL;
}


