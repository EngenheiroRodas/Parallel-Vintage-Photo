/******************************************************************************
 * Programacao Concorrente
 * LEEC 24/25
 *
 * Projecto - Parte1
 *                           old-photo-parallel-A.c
 * 
 * Compilacao: gcc serial-complexo -o serial-complex -lgd
 *           
 *****************************************************************************/

#include <gd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include "image-lib.h"

#define OLD_IMAGE_DIR "./Old-image-dir/"
#define COMMAND_LINE_OPTIONS 1

typedef struct input_ {
    char **filename ;  // string with the name of the file to be processed
    int num_files;  // number of files to be processed
} input;

typedef struct command_line_options_ {
    int num_threads;  // number of threads to be used
    char *output_directory;
    char *mode;
} command_line_options;

void *process_image(void *input_struct) {
    input *data = (input *) input_struct;
    char **files = data->filename;
    int num_files = data->num_files;

    /* Input and output images */
    gdImagePtr in_img, out_smoothed_img, out_contrast_img, out_textured_img, out_sepia_img;
    gdImagePtr in_texture_img = read_png_file("./paper-texture.png");

    if (in_texture_img == NULL) {
        fprintf(stderr, "Error reading texture image.\n");
        pthread_exit(NULL);
    }

    for (int i = 0; i < num_files; i++) {
        char out_file_name[100];

        printf("Processing image: %s\n", files[i]);
        in_img = read_jpeg_file(files[i]);

        if (in_img == NULL) {
            fprintf(stderr, "Impossible to read %s image\n", files[i]);
            continue;
        }

        // Apply transformations
        out_contrast_img = contrast_image(in_img);
        out_smoothed_img = smooth_image(out_contrast_img);
        out_textured_img = texture_image(out_smoothed_img, in_texture_img);
        out_sepia_img = sepia_image(out_textured_img);

        // Generate output file path
        sprintf(out_file_name, "%s%s", OLD_IMAGE_DIR, files[i]);
        if (write_jpeg_file(out_sepia_img, out_file_name) == 0) {
            fprintf(stderr, "Impossible to write %s image\n", out_file_name);
        }

        // Free resources
        gdImageDestroy(out_smoothed_img);
        gdImageDestroy(out_sepia_img);
        gdImageDestroy(out_contrast_img);
        gdImageDestroy(in_img);
    }

    gdImageDestroy(in_texture_img); // Destroy texture image after use
    return NULL;
}


command_line_options *read_command_line(int argc, char *argv[]) {
    command_line_options *options = malloc(sizeof(command_line_options));
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [num_threads]\n", argv[0]);
        exit(1);
    }

    options->num_threads = atoi(argv[1]);

    if (options->num_threads <= 0 || options->num_threads > 10) {
        fprintf(stderr, "Invalid number of threads. Must be between 1 and %d\n", 10);
        exit(1);
    }

    return options;
}


int main(int argc, char *argv[]) {
    struct timespec start_time_total, end_time_total;
    struct timespec start_time_seq, end_time_seq;
    struct timespec start_time_par, end_time_par;

    clock_gettime(CLOCK_MONOTONIC, &start_time_total);
    clock_gettime(CLOCK_MONOTONIC, &start_time_seq);

    // Parse command line arguments
    command_line_options *options = read_command_line(argc, argv);

    int files_per_thread, remainder, num_files_for_thread, current_file = 0;

    // List of files to be processed
    char *files[] = {"img-IST-0.jpeg", "img-IST-1.jpeg", "img-IST-2.jpeg", 
                     "img-IST-3.jpeg", "img-IST-4.jpeg", "img-IST-5.jpeg",
                     "img-IST-6.jpeg", "img-IST-7.jpeg", "img-IST-8.jpeg", 
                     "img-IST-9.jpeg"};
    int nn_files = 10;

    files_per_thread = nn_files / options->num_threads;
    remainder = nn_files % options->num_threads;

    pthread_t *thread_array = malloc(options->num_threads * sizeof(pthread_t));
    input *input_array = malloc(options->num_threads * sizeof(input));

    // Create output directory
    if (create_directory(OLD_IMAGE_DIR) == 0) {
        fprintf(stderr, "Impossible to create %s directory\n", OLD_IMAGE_DIR);
        exit(-1);
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time_seq);
    clock_gettime(CLOCK_MONOTONIC, &start_time_par);

    // Distribute files among threads
    for (int i = 0; i < options->num_threads; i++) {
        num_files_for_thread = files_per_thread + (i < remainder ? 1 : 0);
        input_array[i].filename = malloc(num_files_for_thread * sizeof(char *));
        input_array[i].num_files = num_files_for_thread;
        for (int j = 0; j < num_files_for_thread; j++) {
            input_array[i].filename[j] = files[current_file++];
        }
    }

    // Create threads
    for (int i = 0; i < options->num_threads; i++) {
        if (pthread_create(&thread_array[i], NULL, process_image, &input_array[i]) != 0) {
            fprintf(stderr, "Error creating thread %d\n", i);
        }
    }

    for (int i = 0; i < options->num_threads; i++) {
        pthread_join(thread_array[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time_par);
    clock_gettime(CLOCK_MONOTONIC, &end_time_total);

struct timespec par_time = diff_timespec(&end_time_par, &start_time_par);
struct timespec seq_time = diff_timespec(&end_time_seq, &start_time_seq);
struct timespec total_time = diff_timespec(&end_time_total, &start_time_total);
    printf("\tseq \t %10jd.%09ld\n", seq_time.tv_sec, seq_time.tv_nsec);
    printf("\tpar \t %10jd.%09ld\n", par_time.tv_sec, par_time.tv_nsec);
    printf("total \t %10jd.%09ld\n", total_time.tv_sec, total_time.tv_nsec);

    // Cleanup memory
    for (int i = 0; i < options->num_threads; i++) {
        free(input_array[i].filename);
    }
    free(input_array);
    free(thread_array);
    free(options);

    return 0;
}