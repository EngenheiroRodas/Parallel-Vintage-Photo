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

// Structure to store file information
typedef struct {
    char *name;
    off_t size;
} FileInfo;

char **file_list;


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
}


char *read_command_line(int argc, char *argv[], size_t *file_count) {
    const char *OUTPUT_DIR = "/old-photo-PAR-A";
    char *output_directory;
    if (argc < COMMAND_LINE_OPTIONS + 1) {
        fprintf(stderr, "Usage: %s <INPUT_DIR> <NUMBER_THREADS> <MODE>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[3], "-name") != 0 && strcmp(argv[3], "-size") != 0) {
        fprintf(stderr, "Invalid mode: Use 'name' or 'size'.\n");
        exit(EXIT_FAILURE);
    }

    process_jpg_files(argv[1], argv[3], file_count);

    if (atoi(argv[2]) <= 0 || atoi(argv[2])> *file_count) {
        fprintf(stderr, "Invalid number of threads. Must be between 1 and %d (number of files).\n", *file_count);
        exit(EXIT_FAILURE);
    }

    char *output_path = malloc(strlen(argv[1]) + strlen(OUTPUT_DIR) + 2);
    if (!output_path) {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }
    snprintf(output_path, strlen(argv[1]) + strlen(OUTPUT_DIR) + 2, "%s%s", argv[1], OUTPUT_DIR);
    output_directory = strdup(output_path);
    free(output_path);

    return output_directory;
}

void *process_image(void *input_struct) {
    input *data = (input *)input_struct;  
    char *out_file_name = malloc(strlen(data->output_directory) + 100);  

    gdImagePtr in_img, out_smoothed_img, out_contrast_img, out_textured_img, out_sepia_img;
    gdImagePtr in_texture_img = read_png_file("./paper-texture.png");

    if (in_texture_img == NULL) {
        fprintf(stderr, "Error reading texture image.\n");
        pthread_exit(NULL);
    }

    for (int i = data->start_index; i < data->end_index; i++) {
        printf("Processing image: %s\n", file_list[i]);

        in_img = read_jpeg_file(file_list[i]);
        if (in_img == NULL) {
            fprintf(stderr, "Cannot read image: %s\n", file_list[i]);
            continue;
        }

        out_contrast_img = contrast_image(in_img);
        out_smoothed_img = smooth_image(out_contrast_img);
        out_textured_img = texture_image(out_smoothed_img, in_texture_img);
        out_sepia_img = sepia_image(out_textured_img);

		sprintf(out_file_name, "%s%s", data->output_directory, file_list[i]);
		if(write_jpeg_file(out_sepia_img, out_file_name) == 0){
			fprintf(stderr, "Impossible to write %s image\n", out_file_name);
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
    char *output_directory = read_command_line(argc, argv, &file_count);
    int num_threads = atoi(argv[2]);

    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    input *thread_inputs = malloc(num_threads * sizeof(input));

    int files_per_thread = file_count / num_threads;
    int remainder = file_count % num_threads;
    int current_file = 0;

    for (int i = 0; i < num_threads; i++) {
        thread_inputs[i].start_index = current_file;
        thread_inputs[i].output_directory = output_directory;
        current_file += files_per_thread;
        if (remainder > 0) {
            current_file++;
            remainder--;
        }
        thread_inputs[i].end_index = current_file - 1;
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, process_image, &thread_inputs[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }


    for (int i = 0; i < file_count; i++) {
        free(file_list[i]);
    }
    free(file_list);
    free(thread_inputs);
    free(threads);

    return 0;
}