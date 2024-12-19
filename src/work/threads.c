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
#include <stdbool.h>
#include <sys/select.h>

#include "image-lib.h"
#include "threads.h"
#include "helper_f.h"

bool done_flag = false;

struct timespec total_pic_time;

void *process_image(void *arg) {
    struct timespec start_thread, end_thread, thread_time;

    struct timespec pic_start, pic_end, pic_time;

    clock_gettime(CLOCK_MONOTONIC, &start_thread);

    char full_path[512];
    char out_file_name[512];
    char *current_file;

    gdImagePtr in_img, out_smoothed_img, out_contrast_img, out_textured_img, out_sepia_img;

    while (1) {
        clock_gettime(CLOCK_MONOTONIC, &pic_start);
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

        clock_gettime(CLOCK_MONOTONIC, &pic_end);
        pic_time = diff_timespec(&pic_end, &pic_start);

        // Increment the total time (shared variable)
        pthread_mutex_lock(&lock);
        total_pic_time.tv_sec += pic_time.tv_sec;
        total_pic_time.tv_nsec += pic_time.tv_nsec;
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
    struct timeval timeout;
    fd_set read_fds;
    int stdin_fd = fileno(stdin); // File descriptor for stdin

    while (1) {
        if (done_flag) {
            break;
        }

        // Set up the timeout and file descriptor set
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; // Check every 100 ms
        FD_ZERO(&read_fds);
        FD_SET(stdin_fd, &read_fds);

        // Use select to check for input
        int ret = select(stdin_fd + 1, &read_fds, NULL, NULL, &timeout);
        if (ret > 0 && FD_ISSET(stdin_fd, &read_fds)) {
            char c = getchar();
            if (c == 's' || c == 'S') {
                pthread_mutex_lock(&lock);
                printf("\n");
                printf("======================== STATISTICS ========================\n");
                printf("Images processed:        %d\n", counter);
                printf("Images remaining:        %d\n", file_count - counter);
                printf("Average processing time: %ld.%09ld seconds per image\n", 
                       total_pic_time.tv_sec / counter, total_pic_time.tv_nsec / counter);
                printf("============================================================\n");
                printf("\n");

                pthread_mutex_unlock(&lock);
            }
        }
    }

    pthread_exit(NULL);
}
