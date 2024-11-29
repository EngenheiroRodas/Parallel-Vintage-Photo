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
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include "image-lib.h"

#define COMMAND_LINE_OPTIONS 3
#define MAX_FILENAME_LEN 256

typedef struct input_ {
    char **file_list;       // List of all file names
    int start_index;        // Start index in the file list for this thread
    int num_files;          // Number of files to process for this thread
    char *output_directory; // Output directory path
} input;

typedef struct command_line_options_ {
    int num_threads;        // Number of threads to use
    char *input_directory;  // Input directory containing the images
    char *output_directory; // Output directory path
    char *mode;             // Processing mode
} command_line_options;

void *process_image(void *input_struct) {
    input *data = (input *)input_struct;
    char **file_list = data->file_list;
    int start_index = data->start_index;
    int num_files = data->num_files;
    char *output_directory = data->output_directory;

    gdImagePtr in_img, out_smoothed_img, out_contrast_img, out_textured_img, out_sepia_img;
    gdImagePtr in_texture_img = read_png_file("./paper-texture.png");

    if (in_texture_img == NULL) {
        fprintf(stderr, "Error reading texture image. Ensure './paper-texture.png' exists.\n");
        pthread_exit(NULL);
    }

    for (int i = 0; i < num_files; i++) {
        char out_file_name[MAX_FILENAME_LEN];
        printf("Processing image: %s\n", file_list[start_index + i]);

        in_img = read_jpeg_file(file_list[start_index + i]);
        if (in_img == NULL) {
            fprintf(stderr, "Cannot read image: %s\n", file_list[start_index + i]);
            continue;
        }

        out_contrast_img = contrast_image(in_img);
        out_smoothed_img = smooth_image(out_contrast_img);
        out_textured_img = texture_image(out_smoothed_img, in_texture_img);
        out_sepia_img = sepia_image(out_textured_img);

        snprintf(out_file_name, MAX_FILENAME_LEN, "%s/%s", output_directory, strrchr(file_list[start_index + i], '/') ? strrchr(file_list[start_index + i], '/') + 1 : file_list[start_index + i]);
        if (write_jpeg_file(out_sepia_img, out_file_name) == 0) {
            fprintf(stderr, "Cannot write image: %s\n", out_file_name);
        }

        gdImageDestroy(out_smoothed_img);
        gdImageDestroy(out_sepia_img);
        gdImageDestroy(out_contrast_img);
        gdImageDestroy(in_img);
    }

    gdImageDestroy(in_texture_img);
    return NULL;
}

command_line_options *read_command_line(int argc, char *argv[], char **output_dir) {
    if (argc < COMMAND_LINE_OPTIONS + 1) {
        fprintf(stderr, "Usage: %s <INPUT_DIR> <NUMBER_THREADS> <MODE>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    command_line_options *options = malloc(sizeof(command_line_options));
    if (!options) {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    options->input_directory = argv[1];

    DIR *input_dir = opendir(options->input_directory);
    if (!input_dir) {
        perror("Failed to open input directory");
        free(options);
        exit(EXIT_FAILURE);
    }

    // Count files in the input directory
    struct dirent *entry;
    int file_count = 0;

    while ((entry = readdir(input_dir)) != NULL) {
        if (entry->d_type == DT_REG && 
            (strstr(entry->d_name, ".jpg") || strstr(entry->d_name, ".jpeg"))) {
            file_count++;
        }
    }

    closedir(input_dir);

    options->num_threads = atoi(argv[2]);
    if (options->num_threads <= 0 || options->num_threads > file_count) {
        fprintf(stderr, "Invalid number of threads. Must be between 1 and %d (number of files).\n", file_count);
        free(options);
        exit(EXIT_FAILURE);
    }

    options->mode = argv[3];
    if (strcmp(options->mode, "-name") != 0 && strcmp(options->mode, "-size") != 0) {
        fprintf(stderr, "Invalid mode: Use '-name' or '-size'.\n");
        free(options);
        exit(EXIT_FAILURE);
    }

    // Construct the output directory path
    size_t dir_len = strlen(options->input_directory) + strlen("/old_photo_PAR_A") + 1;
    *output_dir = malloc(dir_len);
    if (!*output_dir) {
        perror("Failed to allocate memory for output directory");
        free(options);
        exit(EXIT_FAILURE);
    }
    snprintf(*output_dir, dir_len, "%s/old_photo_PAR_A", options->input_directory);

    if (mkdir(*output_dir, 0755) != 0 && errno != EEXIST) {
        perror("Failed to create output directory");
        free(*output_dir);
        free(options);
        exit(EXIT_FAILURE);
    }

    options->output_directory = *output_dir;
    return options;
}

int main(int argc, char *argv[]) {
    char *output_dir = NULL;
    command_line_options *options = read_command_line(argc, argv, &output_dir);

    DIR *input_dir = opendir(options->input_directory);
    if (!input_dir) {
        perror("Failed to open input directory");
        free(output_dir);
        free(options);
        return EXIT_FAILURE;
    }

    // Create a list of files to process
    struct dirent *entry;
    char **file_list = NULL;
    int file_count = 0;

    while ((entry = readdir(input_dir)) != NULL) {
        if (entry->d_type == DT_REG && 
            (strstr(entry->d_name, ".jpg") || strstr(entry->d_name, ".jpeg"))) {
            file_list = realloc(file_list, (file_count + 1) * sizeof(char *));
            file_list[file_count] = malloc(MAX_FILENAME_LEN);
            snprintf(file_list[file_count], MAX_FILENAME_LEN, "%s/%s", options->input_directory, entry->d_name);
            file_count++;
        }
    }

    closedir(input_dir);

    pthread_t *threads = malloc(options->num_threads * sizeof(pthread_t));
    input *thread_inputs = malloc(options->num_threads * sizeof(input));

    int files_per_thread = file_count / options->num_threads;
    int remainder = file_count % options->num_threads;
    int current_index = 0;

    for (int i = 0; i < options->num_threads; i++) {
        thread_inputs[i].file_list = file_list;
        thread_inputs[i].start_index = current_index;
        thread_inputs[i].num_files = files_per_thread + (i < remainder ? 1 : 0);
        thread_inputs[i].output_directory = output_dir;

        current_index += thread_inputs[i].num_files;

        if (pthread_create(&threads[i], NULL, process_image, &thread_inputs[i]) != 0) {
            fprintf(stderr, "Failed to create thread %d\n", i);
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < options->num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    for (int i = 0; i < file_count; i++) {
        free(file_list[i]);
    }
    free(file_list);
    free(thread_inputs);
    free(threads);
    free(output_dir);
    free(options);

    return 0;
}
