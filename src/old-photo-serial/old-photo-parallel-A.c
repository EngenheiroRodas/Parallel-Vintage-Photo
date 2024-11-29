/******************************************************************************
 * Programacao Concorrente
 * LEEC 24/25
 *
 * Projecto - Parte1
 *                           old-photo-parallel-A.c
 * 
 *           
 *****************************************************************************/

#include <gd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include "image-lib.h"

#define OUTPUT_DIR "old_photo_PAR_A"
#define COMMAND_LINE_OPTIONS 3
#define MAX_FILENAME_LEN 256

typedef struct input_ {
    char **filename;  // Array of file names to be processed
    int num_files;    // Number of files to be processed
} input;

typedef struct command_line_options_ {
    int num_threads;        // Number of threads to use
    char *input_directory;  // Input directory containing the images
    char *output_directory; // Output directory path
    char *mode;             // Processing mode
} command_line_options;

void *process_image(void *input_struct) {
    input *data = (input *)input_struct;
    char **files = data->filename;
    int num_files = data->num_files;

    gdImagePtr in_img, out_smoothed_img, out_contrast_img, out_textured_img, out_sepia_img;
    gdImagePtr in_texture_img = read_png_file("./paper-texture.png");

    if (in_texture_img == NULL) {
        fprintf(stderr, "Error reading texture image.\n");
        pthread_exit(NULL);
    }

    for (int i = 0; i < num_files; i++) {
        char out_file_name[MAX_FILENAME_LEN];
        printf("Processing image: %s\n", files[i]);

        in_img = read_jpeg_file(files[i]);
        if (in_img == NULL) {
            fprintf(stderr, "Cannot read image: %s\n", files[i]);
            continue;
        }

        out_contrast_img = contrast_image(in_img);
        out_smoothed_img = smooth_image(out_contrast_img);
        out_textured_img = texture_image(out_smoothed_img, in_texture_img);
        out_sepia_img = sepia_image(out_textured_img);

        sprintf(out_file_name, "%s/%s", OUTPUT_DIR, strrchr(files[i], '/') ? strrchr(files[i], '/') + 1 : files[i]);
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

command_line_options *read_command_line(int argc, char *argv[]) {
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

    int file_count = 0;
    struct dirent *entry;
    while ((entry = readdir(input_dir)) != NULL) {
        if (entry->d_type == DT_REG) {
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
        fprintf(stderr, "Invalid mode: Use 'name' or 'size'.\n");
        free(options);
        exit(EXIT_FAILURE);
    }

    char output_path[MAX_FILENAME_LEN];
    sprintf(output_path, "%s/%s", options->input_directory, OUTPUT_DIR);
    if (mkdir(output_path, 0755) != 0 && errno != EEXIST) {
        perror("Failed to create output directory");
        free(options);
        exit(EXIT_FAILURE);
    }
    options->output_directory = strdup(output_path);

    printf("Input Directory: %s\n", options->input_directory);
    printf("Output Directory: %s\n", options->output_directory);
    printf("Number of Files: %d\n", file_count);
    printf("Number of Threads: %d\n", options->num_threads);
    printf("Mode: %s\n", options->mode);

    return options;
}

char **list_jpeg_files(const char *dir_path, int *file_count) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("Cannot open directory");
        return NULL;
    }

    struct dirent *entry;
    char **file_list = malloc(100 * sizeof(char *));
    *file_count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG && 
            (strstr(entry->d_name, ".jpeg") || strstr(entry->d_name, ".jpg") || strstr(entry->d_name, ".JPEG") || strstr(entry->d_name, ".JPG"))) {
            char full_path[MAX_FILENAME_LEN];
            sprintf(full_path, "%s/%s", dir_path, entry->d_name);
            file_list[*file_count] = strdup(full_path);
            (*file_count)++;
        }
    }
    closedir(dir);
    return file_list;
}

int main(int argc, char *argv[]) {
    command_line_options *options = read_command_line(argc, argv);
    int file_count;

    char **file_list = list_jpeg_files(options->input_directory, &file_count);
    if (!file_list || file_count == 0) {
        fprintf(stderr, "No .jpeg or .jpg files found in %s\n", options->input_directory);
        free(options);
        return EXIT_FAILURE;
    }

    pthread_t *threads = malloc(options->num_threads * sizeof(pthread_t));
    input *thread_inputs = malloc(options->num_threads * sizeof(input));

    int files_per_thread = file_count / options->num_threads;
    int remainder = file_count % options->num_threads;
    int current_file = 0;

    for (int i = 0; i < options->num_threads; i++) {
        int num_files = files_per_thread + (i < remainder ? 1 : 0);
        thread_inputs[i].filename = malloc(num_files * sizeof(char *));
        thread_inputs[i].num_files = num_files;
        for (int j = 0; j < num_files; j++) {
            thread_inputs[i].filename[j] = file_list[current_file++];
        }
    }

    for (int i = 0; i < options->num_threads; i++) {
        pthread_create(&threads[i], NULL, process_image, &thread_inputs[i]);
    }

    for (int i = 0; i < options->num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    for (int i = 0; i < options->num_threads; i++) {
        free(thread_inputs[i].filename);
    }

    for (int i = 0; i < file_count; i++) {
        free(file_list[i]);
    }
    free(file_list);
    free(thread_inputs);
    free(threads);
    free(options->output_directory);
    free(options);

    return 0;
}