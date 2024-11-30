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
#include <time.h>
#include <pthread.h>
#include <ctype.h>
#include <errno.h>
#include "image-lib.h"

#define COMMAND_LINE_OPTIONS 3

typedef struct input_ {
    int start_index;         // Starting index of files for the thread
    int end_index;           // Ending index of files for the thread
    char *output_directory;  // Output directory path
    char *input_directory;   // Input directory path
} input;

// Structure to store file information
typedef struct {
    char *name;
    off_t size;
} FileInfo;

char **file_list = NULL; // Shared file list accessible by threads

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
    return (file1->size < file2->size) ? -1 : (file1->size > file2->size);
}

// Function to collect, sort, and display .jpg files
void process_jpg_files(const char *directory, const char *sort_option, size_t *file_count) {
    DIR *d;
    struct dirent *f;
    struct stat st;
    FileInfo *files = NULL;
    size_t capacity = 10;

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

        // Check if file extension is ".jpg" or ".jpeg" (case-insensitive)
        const char *ext = strrchr(f->d_name, '.');
        if (!ext || 
            (strcasecmp(ext, ".jpg") != 0 && strcasecmp(ext, ".jpeg") != 0)) {
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

    // Allocate and populate the shared file list
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
        file_list[i] = files[i].name; // Transfer ownership
    }

    // Free the FileInfo array but not the names (now owned by file_list)
    free(files);
}

// Function to validate and parse command line arguments
char *read_command_line(int argc, char *argv[], size_t *file_count) {
    const char *OUTPUT_DIR = "/old-photo-PAR-A";
    if (argc < COMMAND_LINE_OPTIONS + 1) {
        fprintf(stderr, "Usage: %s <INPUT_DIR> <NUMBER_THREADS> <MODE>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[3], "-name") != 0 && strcmp(argv[3], "-size") != 0) {
        fprintf(stderr, "Invalid mode: Use '-name' or '-size'.\n");
        exit(EXIT_FAILURE);
    }

    process_jpg_files(argv[1], argv[3], file_count);

    if (*file_count == 0) {
        fprintf(stderr, "No .jpg files found in the specified directory.\n");
        exit(EXIT_FAILURE);
    }

    int num_threads = atoi(argv[2]);
    if (num_threads <= 0 || num_threads > *file_count) {
        fprintf(stderr, "Invalid number of threads. Must be between 1 and %zu (number of files).\n", *file_count);
        exit(EXIT_FAILURE);
    }

    size_t output_length = strlen(argv[1]) + strlen(OUTPUT_DIR) + 1; // +1 for '\0'
    char *output_directory = malloc(output_length);
    if (!output_directory) {
        perror("Failed to allocate memory for output directory");
        exit(EXIT_FAILURE);
    }
    snprintf(output_directory, output_length, "%s%s", argv[1], OUTPUT_DIR);

    return output_directory;
}

// Thread function to process images
void *process_image(void *input_struct) {
    input *data = (input *)input_struct;
    char full_path[512];
    char out_file_name[512]; // Adjusted for sufficient length

    gdImagePtr in_img, out_smoothed_img, out_contrast_img, out_textured_img, out_sepia_img;
    gdImagePtr in_texture_img = read_png_file("./paper-texture.png");

    if (!in_texture_img) {
        fprintf(stderr, "Error reading texture image.\n");
        pthread_exit(NULL);
    }

    for (int i = data->start_index; i <= data->end_index; i++) {
        snprintf(full_path, sizeof(full_path), "%s/%s", data->input_directory, file_list[i]);
        printf("Processing image: %s\n", full_path);

        in_img = read_jpeg_file(full_path);
        if (!in_img) {
            fprintf(stderr, "Cannot read image: %s\n", full_path);
            continue;
        }

        out_contrast_img = contrast_image(in_img);
        out_smoothed_img = smooth_image(out_contrast_img);
        out_textured_img = texture_image(out_smoothed_img, in_texture_img);
        out_sepia_img = sepia_image(out_textured_img);

        snprintf(out_file_name, sizeof(out_file_name), "%s/%s", data->output_directory, file_list[i]);
        if (!write_jpeg_file(out_sepia_img, out_file_name)) {
            fprintf(stderr, "Failed to write image: %s\n", out_file_name);
        }

        gdImageDestroy(out_contrast_img);
        gdImageDestroy(out_smoothed_img);
        gdImageDestroy(out_textured_img);
        gdImageDestroy(out_sepia_img);
        gdImageDestroy(in_img);
    }

    gdImageDestroy(in_texture_img);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    struct timespec start_time_total, end_time_total;
    struct timespec start_time_seq, end_time_seq;
    struct timespec start_time_par, end_time_par;



	clock_gettime(CLOCK_MONOTONIC, &start_time_total);
	clock_gettime(CLOCK_MONOTONIC, &start_time_seq);
    size_t file_count = 0;

    char *output_directory = read_command_line(argc, argv, &file_count);
    struct stat st = {0};

    if (stat(output_directory, &st) == -1) {
        if (mkdir(output_directory, 0777) == -1) {
            perror("Failed to create output directory");
            free(output_directory);
            exit(EXIT_FAILURE);
        }
    }

    int num_threads = atoi(argv[2]);
    pthread_t threads[num_threads];
    input thread_inputs[num_threads];

    int files_per_thread = file_count / num_threads;
    int remainder = file_count % num_threads;
    int current_file = 0;

    for (int i = 0; i < num_threads; i++) {
        thread_inputs[i].start_index = current_file;
        thread_inputs[i].output_directory = output_directory;
        thread_inputs[i].input_directory = argv[1];
        current_file += files_per_thread;
        if (remainder > 0) {
            current_file++;
            remainder--;
        }
        thread_inputs[i].end_index = current_file - 1;
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time_seq);
	clock_gettime(CLOCK_MONOTONIC, &start_time_par);

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

    clock_gettime(CLOCK_MONOTONIC, &end_time_par);

    free(output_directory);
    for (size_t i = 0; i < file_count; i++) {
        free(file_list[i]);
    }
    free(file_list);

    printf("All images processed successfully.\n");
    
	clock_gettime(CLOCK_MONOTONIC, &end_time_total);

struct timespec par_time = diff_timespec(&end_time_par, &start_time_par);
struct timespec seq_time = diff_timespec(&end_time_seq, &start_time_seq);
struct timespec total_time = diff_timespec(&end_time_total, &start_time_total);
    printf("\tseq \t %10jd.%09ld\n", seq_time.tv_sec, seq_time.tv_nsec);
    printf("\tpar \t %10jd.%09ld\n", par_time.tv_sec, par_time.tv_nsec);
    printf("total \t %10jd.%09ld\n", total_time.tv_sec, total_time.tv_nsec);

    return 0;
}
