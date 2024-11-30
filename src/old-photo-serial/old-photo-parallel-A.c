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
#include <ctype.h>
#include <errno.h>
#include "image-lib.h"

#define COMMAND_LINE_OPTIONS 3


typedef struct input_ {
    int start_index;    // Number of files to be processed
    int end_index;      // Number of files to be processed
    char *output_directory; // Output directory path
} input;

typedef struct command_line_options_ {
    int num_threads;        // Number of threads to use
    char *output_directory; // Output directory path
    char **file_list;       // List of files to process
} command_line_options;

// Structure to store file information
typedef struct {
    char *name;
    off_t size;
} FileInfo;

// Helper function to extract a numeric value from a string
int extract_number(const char *str) {
    while (*str && !isdigit(*str)) str++; // Skip non-digit characters
    return isdigit(*str) ? atoi(str) : 0; // Convert number to integer
}

// Comparison function for sorting by name (natural sorting)
int compare_by_name_natural(const void *a, const void *b) {
    FileInfo *file1 = (FileInfo *)a;
    FileInfo *file2 = (FileInfo *)b;

    // Extract numbers from the names
    int num1 = extract_number(file1->name);
    int num2 = extract_number(file2->name);

    // Compare by numeric value if both names contain numbers
    if (num1 != num2) {
        return num1 - num2;
    }

    // Fallback to lexicographical comparison if no numbers or equal numbers
    return strcmp(file1->name, file2->name);
}

// Comparison function for sorting by size
int compare_by_size(const void *a, const void *b) {
    FileInfo *file1 = (FileInfo *)a;
    FileInfo *file2 = (FileInfo *)b;
    if (file1->size < file2->size) return -1;
    if (file1->size > file2->size) return 1;
    return 0;
}

// Function to collect, sort, and display .jpg files
char **process_jpg_files(const char *directory, const char *sort_option, size_t *file_count) {
    DIR *d;
    struct dirent *f;
    struct stat st;
    FileInfo *files = NULL;
    size_t capacity = 10;

    char **file_list;

    // Allocate initial memory for file array
    files = malloc(capacity * sizeof(FileInfo));
    if (!files) {
        perror("Failed to allocate memory for files array");
        exit(EXIT_FAILURE);
    }

    // Open the directory
    d = opendir(directory);
    if (!d) {
        perror("Failed to open input directory");
        free(files);
        exit(EXIT_FAILURE);
    }

    // Read directory entries
    while ((f = readdir(d)) != NULL) {
        // Skip "." and ".."
        if (strcmp(f->d_name, ".") == 0 || strcmp(f->d_name, "..") == 0) {
            continue;
        }

        // Check if file extension is ".jpg" (case-insensitive)
        const char *ext = strrchr(f->d_name, '.');
        if (!ext || (strcasecmp(ext, ".jpg") != 0)) {
            continue;
        }

        // Build full file path
        size_t path_length = strlen(directory) + strlen(f->d_name) + 2; // +2 for '/' and '\0'
        char *filepath = malloc(path_length);
        if (!filepath) {
            perror("Failed to allocate memory for filepath");
            closedir(d);
            free(files);
            exit(EXIT_FAILURE);
        }
        snprintf(filepath, path_length, "%s/%s", directory, f->d_name);

        // Get file stats
        if (stat(filepath, &st) == -1) {
            perror("stat failed");
            free(filepath);
            continue;
        }

        // Expand array if needed
        if (*file_count == capacity) {
            capacity *= 2;
            files = realloc(files, capacity * sizeof(FileInfo));
            if (!files) {
                perror("Failed to reallocate memory for files array");
                closedir(d);
                free(filepath);
                exit(EXIT_FAILURE);
            }
        }

        // Store file info
        files[*file_count].name = strdup(f->d_name); // Duplicate file name
        files[*file_count].size = st.st_size;
        (*file_count)++;

        // Free filepath
        free(filepath);
    }

    // Close the directory
    closedir(d);

    // Sort files array
    if (strcmp(sort_option, "-size") == 0) {
        qsort(files, *file_count, sizeof(FileInfo), compare_by_size);
    } else if (strcmp(sort_option, "-name") == 0) {
        qsort(files, *file_count, sizeof(FileInfo), compare_by_name_natural);
    } else {
        fprintf(stderr, "Invalid sorting option. Use -size or -name.\n");
        for (size_t i = 0; i < *file_count; i++) {
            free(files[i].name);
        }
        free(files);
        exit(EXIT_FAILURE);
    }

    file_list = malloc((*file_count) * sizeof(char *));
    if (!file_list) {
        perror("Failed to allocate memory for file list");
        for (size_t i = 0; i < *file_count; i++) {
            free(files[i].name);
        }
        free(files);
        exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < *file_count; i++) {
        file_list[i] = strdup(files[i].name);
    }


    for (size_t i = 0; i < *file_count; i++) {
        free(file_list[i]);
    }
    // Free file array
    free(files);

    return file_list;
}


command_line_options *read_command_line(int argc, char *argv[], size_t *file_count) {
    const char *OUTPUT_DIR = "/old-photo-PAR-A";
    if (argc < COMMAND_LINE_OPTIONS + 1) {
        fprintf(stderr, "Usage: %s <INPUT_DIR> <NUMBER_THREADS> <MODE>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    command_line_options *options = malloc(sizeof(command_line_options));
    if (!options) {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }



    if (strcmp(argv[3], "-name") != 0 && strcmp(argv[3], "-size") != 0) {
        fprintf(stderr, "Invalid mode: Use 'name' or 'size'.\n");
        free(options);
        exit(EXIT_FAILURE);
    }

    options->file_list = process_jpg_files(argv[1], argv[3], file_count);

    options->num_threads = atoi(argv[2]);
    if (options->num_threads <= 0 || options->num_threads > *file_count) {
        fprintf(stderr, "Invalid number of threads. Must be between 1 and %d (number of files).\n", file_count);
        free(options);
        exit(EXIT_FAILURE);
    }

    char *output_path = malloc(strlen(argv[1]) + strlen(OUTPUT_DIR) + 2);
    if (!output_path) {
        perror("Failed to allocate memory");
        free(options);
        exit(EXIT_FAILURE);
    }
    snprintf(output_path, strlen(argv[1]) + strlen(OUTPUT_DIR) + 2, "%s%s", argv[1], OUTPUT_DIR);
    options->output_directory = strdup(output_path);
    free(output_path);
    printf("Output Directory: %s\n", options->output_directory);
    printf("Number of Files: %d\n", file_count);
    printf("Number of Threads: %d\n", options->num_threads);

    return options;
}

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



int main(int argc, char *argv[]) {
    size_t file_count = 0;
    command_line_options *options = read_command_line(argc, argv);

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