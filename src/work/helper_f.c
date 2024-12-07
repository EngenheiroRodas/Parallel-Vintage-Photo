#include <gd.h>
#include "image-lib.h"
#include "helper_f.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h> 
#include <sys/stat.h>   
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define COMMAND_LINE_OPTIONS 3

char **file_list = NULL; // Shared file list accessible by threads

// Structure to store file information
typedef struct {
    char *name;
    size_t size;
} FileInfo;


// Helper function to extract a numeric value from a string
int extract_number(const char *str) {
    while (*str && !isdigit(*str)) str++; // Skip non-digit characters
    return isdigit(*str) ? atoi(str) : 0; // Convert number to integer
}


// Comparison function for qsort, when using name sorting
int compare_by_name_natural(const void *a, const void *b) {
    FileInfo *file1 = (FileInfo *)a;
    FileInfo *file2 = (FileInfo *)b;

    int num1 = extract_number(file1->name);
    int num2 = extract_number(file2->name);

    // Compare by numeric value if both names contain numbers
    if (num1 != num2) {
        return num1 - num2;
    }

    // Fallback to lexicographical comparison if no numbers or equal numbers
    return strcmp(file1->name, file2->name);
}

// Comparison function for qsort when using size sorting
int compare_by_size(const void *a, const void *b) {
    FileInfo *file1 = (FileInfo *)a;
    FileInfo *file2 = (FileInfo *)b;
    return (file1->size < file2->size) ? -1 : (file1->size > file2->size);
}


// Function to collect and sort .jpg files in the input directory
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

        // Check if file extension is ".jpg" or ".jpeg"
        const char *ext = strrchr(f->d_name, '.');
        if (!ext || 
            (strcasecmp(ext, ".jpg") != 0 && strcasecmp(ext, ".jpeg") != 0)) {
            continue;
        }

        // Build full file path
        size_t path_length = strlen(directory) + strlen(f->d_name) + 2;
        char *filepath = malloc(path_length);
        if (!filepath) {
            perror("Failed to allocate memory for filepath");
            closedir(d);
            free(files);
            exit(EXIT_FAILURE);
        }
        snprintf(filepath, path_length, "%s/%s", directory, f->d_name);

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
        files[*file_count].name = strdup(f->d_name); // Duplicate file name
        files[*file_count].size = st.st_size;
        (*file_count)++;

        free(filepath);
    }
    closedir(d);

    // Sort files array
    if (strcmp(sort_option, "-size") == 0) {
        qsort(files, *file_count, sizeof(FileInfo), compare_by_size);
    } else if (strcmp(sort_option, "-name") == 0) {
        qsort(files, *file_count, sizeof(FileInfo), compare_by_name_natural);
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
    free(files);

    return;
}


// Function to validate and parse command line arguments
int read_command_line(int argc, char *argv[], size_t *file_count) {
    if (argc < COMMAND_LINE_OPTIONS + 1) {
        fprintf(stderr, "Usage: %s <INPUT_DIR> <NUMBER_THREADS> <MODE>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Retrieve all .jpg files and updates file_count
    process_jpg_files(argv[1], argv[3], file_count);

    if (*file_count == 0) {
        fprintf(stderr, "No .jpg files found in the specified directory.\n");
        exit(EXIT_FAILURE);
    }

    int num_threads = atoi(argv[2]);
    if (num_threads <= 0) {
        fprintf(stderr, "Invalid number of threads. The number of threads to create must be a positive number.\n");
        exit(EXIT_FAILURE);
    }

    // Main won't create more threads than files
    if (num_threads > *file_count) num_threads = *file_count;

    return num_threads;
}


void edit_paths(int argc, char *argv[], char **output_txt, char **output_directory) {
    const char *OUTPUT_DIR = "/old_photo_PAR_A";
    const char *OUTPUT_TXT_PREFIX = "timing_";

    // Validate the suffix argument
    char *suffix = NULL;
    if (strcmp(argv[3], "-size") == 0) {
        suffix = "-size.txt";
    } else if (strcmp(argv[3], "-name") == 0) {
        suffix = "-name.txt";
    } else {
        fprintf(stderr, "Invalid mode. Must be '-size' or '-name'.\n");
        exit(EXIT_FAILURE);
    }

    // Create output directory path
    size_t output_dir_len = strlen(argv[1]) + strlen(OUTPUT_DIR) + 1;
    *output_directory = malloc(output_dir_len);
    if (*output_directory == NULL) {
        perror("Failed to allocate memory for output directory path");
        exit(EXIT_FAILURE);
    }
    snprintf(*output_directory, output_dir_len, "%s%s", argv[1], OUTPUT_DIR);



    // Create output timing.txt file path
    size_t output_txt_len = strlen(argv[1]) + strlen("/") + strlen(OUTPUT_TXT_PREFIX) +
                            snprintf(NULL, 0, "%d", atoi(argv[2])) + strlen(suffix) + 1;
    *output_txt = malloc(output_txt_len);
    if (*output_txt == NULL) {
        perror("Failed to allocate memory for output txt path");
        free(*output_directory);
        exit(EXIT_FAILURE);
    }
    snprintf(*output_txt, output_txt_len, "%s/%s%d%s", argv[1], OUTPUT_TXT_PREFIX, atoi(argv[2]), suffix);


    struct stat st = {0};
    if (stat(*output_directory, &st) == -1) {
        if (mkdir(*output_directory, 0777) == -1) {
            perror("Failed to create output directory");
            free(*output_directory);
            exit(EXIT_FAILURE);
        }
    }

    return;
}


// Thread function to process images
void *process_image(void *input_struct) {   
    input *data = (input *)input_struct;
    
    clock_gettime(CLOCK_MONOTONIC, &data->start_thread);

    int file_index;
    char full_path[512];
    char out_file_name[512]; // Adjusted for sufficient length

    gdImagePtr in_img, out_smoothed_img, out_contrast_img, out_textured_img, out_sepia_img;


    for (int i = 0; i < data->file_count; i++) {
        file_index = data->file_indices[i];

        snprintf(out_file_name, sizeof(out_file_name), "%s/%s", data->output_directory, file_list[file_index]);
        if (access(out_file_name, F_OK) != -1)  
            continue;

        snprintf(full_path, sizeof(full_path), "%s/%s", data->input_directory, file_list[file_index]);

        printf("image %s\n", file_list[file_index]);

        in_img = read_jpeg_file(full_path);
        if (!in_img) {
            fprintf(stderr, "Cannot read image: %s\n", full_path);
            continue;
        }

        out_contrast_img = contrast_image(in_img);
        out_smoothed_img = smooth_image(out_contrast_img);
        out_textured_img = texture_image(out_smoothed_img, data->in_texture_img);
        out_sepia_img = sepia_image(out_textured_img);

        if (!write_jpeg_file(out_sepia_img, out_file_name)) {
            fprintf(stderr, "Failed to write image: %s\n", out_file_name);
        }

        gdImageDestroy(out_contrast_img);
        gdImageDestroy(out_smoothed_img);
        gdImageDestroy(out_textured_img);
        gdImageDestroy(out_sepia_img);
        gdImageDestroy(in_img);
    }

    clock_gettime(CLOCK_MONOTONIC, &data->end_thread);
    
    return NULL;
}