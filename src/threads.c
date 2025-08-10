#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

#include "image-lib.h"
#include "threads.h"
#include "helper_f.h"

/// @brief Process an image producing old-photo effect, and write the output in the specified folder.
/// @return Pointer to the total execution time of the thread.
void *process_image(void *arg) {
    struct timespec start_thread, end_thread, thread_time;
    struct timespec pic_start, pic_end, pic_time;

    clock_gettime(CLOCK_MONOTONIC, &start_thread);

    char input_path[512];
    char out_file_name[512];
    char *current_file;

    gdImagePtr in_img, out_smoothed_img, out_contrast_img, out_textured_img, out_sepia_img;

    // Process images until the pipe is empty. Write end is already closed.
    while (read(pipe_fd[0], &current_file, sizeof(char *)) > 0) {
        clock_gettime(CLOCK_MONOTONIC, &pic_start);

        // Generate output and input file paths
        snprintf(out_file_name, sizeof(out_file_name), "%s/%s", output_directory, current_file);
        snprintf(input_path, sizeof(input_path), "%s/%s", input_directory, current_file);

        in_img = read_jpeg_file(input_path);
        if (!in_img) {
            fprintf(stderr, "Cannot read image: %s\n", input_path);
            continue;
        }
        printf("Processing file: %s\n", current_file);

        // Process the image through various filters
        out_contrast_img = contrast_image(in_img);
        gdImageDestroy(in_img);
        out_smoothed_img = smooth_image(out_contrast_img);
        gdImageDestroy(out_contrast_img);
        out_textured_img = texture_image(out_smoothed_img, in_texture_img);
        gdImageDestroy(out_smoothed_img);
        out_sepia_img = sepia_image(out_textured_img);
        gdImageDestroy(out_textured_img);

        // Write the processed image to the output file
        if (!write_jpeg_file(out_sepia_img, out_file_name)) {
            fprintf(stderr, "Failed to write image: %s\n", out_file_name);
        }

        // Cleanup
        gdImageDestroy(out_sepia_img);
        

        clock_gettime(CLOCK_MONOTONIC, &pic_end);
        pic_time = diff_timespec(&pic_end, &pic_start);

        // Increment the total time and counter
        pthread_mutex_lock(&lock);
        counter++;
        total_pic_time.tv_nsec += pic_time.tv_nsec;
        if (total_pic_time.tv_nsec >= 1000000000) { // Normalize the time
            total_pic_time.tv_nsec -= 1000000000;
            total_pic_time.tv_sec += 1;
        }
        total_pic_time.tv_sec += pic_time.tv_sec;
        pthread_mutex_unlock(&lock);
    }
    clock_gettime(CLOCK_MONOTONIC, &end_thread);
    thread_time = diff_timespec(&end_thread, &start_thread);

    struct timespec *thread_time_ptr = malloc(sizeof(struct timespec));
    *thread_time_ptr = thread_time;

    pthread_exit(thread_time_ptr);
}

/// @brief Single thread to watch for key press and print statistics.
/// @return None.
void *handle_key_press(void *arg) {
    char c;
    double total_time_in_seconds;
    // read() will return 0 when stdin is closed by main thread
    while (read(STDIN_FILENO, &c, sizeof(char)) > 0) {
        if (c == 's' || c == 'S') {
            printf("\n");
            printf("======================== STATISTICS ========================\n");
            pthread_mutex_lock(&lock);
            printf("Images processed:        %ld\n", counter);
            printf("Images remaining:        %ld\n", file_count - counter);
            if (counter > 0) {
                total_time_in_seconds = total_pic_time.tv_sec + total_pic_time.tv_nsec / 1e9;
                printf("Average processing time: %10.3f seconds per image\n", total_time_in_seconds / counter);
            } else {
                printf("Average processing time: 0.000 seconds per image\n");
            }
            pthread_mutex_unlock(&lock);
            printf("============================================================\n");
            printf("\n");

        }
    }
    pthread_exit(NULL);
}
