#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h> 
#include <sys/stat.h>
#include <pthread.h>

#include "image-lib.h"
#include "helper_f.h"

#define COMMAND_LINE_OPTIONS 3

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


void process_jpg_files(const char *directory, const char *sort_option, size_t *file_count, char *output_directory) {
    DIR *d;
    struct dirent *f;
    struct stat st;
    FileInfo *files = NULL;
    size_t capacity = 10;

    // Allocate initial memory for the file array
    files = malloc(capacity * sizeof(FileInfo));
    if (!files) {
        perror("Failed to allocate memory for files array");
        exit(EXIT_FAILURE);
    }

    // Open the input directory
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

        // Check if file extension is ".jpeg"
        const char *ext = strrchr(f->d_name, '.');
        if (!ext || strcasecmp(ext, ".jpeg") != 0) {
            continue;
        }

        // Build the full file path
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", directory, f->d_name);

        // Check file stats
        if (stat(filepath, &st) == -1 || !S_ISREG(st.st_mode)) {
            perror("stat failed or not a regular file");
            continue;
        }

        // Check if the file already exists in the output directory
        char output_filepath[512];
        snprintf(output_filepath, sizeof(output_filepath), "%s/%s", output_directory, f->d_name);
        if (access(output_filepath, F_OK) == 0) {
            continue; // File already exists, skip
        }

        // Expand file array if necessary
        if (*file_count == capacity) {
            capacity *= 2;
            FileInfo *new_files = realloc(files, capacity * sizeof(FileInfo));
            if (!new_files) {
                perror("Failed to reallocate memory for files array");
                closedir(d);
                for (size_t i = 0; i < *file_count; i++) {
                    free(files[i].name);
                }
                free(files);
                exit(EXIT_FAILURE);
            }
            files = new_files;
        }

        // Add file information to the list
        files[*file_count].name = strdup(f->d_name);
        files[*file_count].size = st.st_size;
        (*file_count)++;
    }
    closedir(d);

    // Sort files array if necessary
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
}


// Function to validate and parse command line arguments
int read_command_line(int argc, char *argv[], size_t *file_count, char *output_directory) {
    if (argc < COMMAND_LINE_OPTIONS + 1) {
        fprintf(stderr, "Usage: %s <INPUT_DIR> <NUMBER_THREADS> <MODE>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Retrieve all .jpg files and updates file_count
    process_jpg_files(argv[1], argv[3], file_count, output_directory);

    int num_threads = atoi(argv[2]);
    if (num_threads <= 0) {
        fprintf(stderr, "Invalid number of threads. The number of threads to create must be a positive number.\n");
        exit(EXIT_FAILURE);
    }

    return num_threads;
}


void edit_paths(int argc, char *argv[], char **output_txt, char **output_directory) {
    const char *OUTPUT_DIR = "/old_photo_PAR_B";
    const char *OUTPUT_TXT_PREFIX = "timing_B_";

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