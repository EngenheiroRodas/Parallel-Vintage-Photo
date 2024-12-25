#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h> 
#include <sys/stat.h>
#include <pthread.h>

#include "helper_f.h"

// 3 arguments + program name
#define COMMAND_LINE_OPTIONS 4

/// @brief Structure to store file information from readdir and stat.
typedef struct {
    char *name; 
    size_t size; 
} FileInfo;

/// @brief Edits file paths of output_txt and global output_directory and creates it.
/// @param argc 
/// @param argv 
/// @param output_txt Pointer to store the path of the output timing file.
/// @return The modified output_txt path.
char *edit_paths(int argc, char *argv[], char **output_txt) {
    const char *OUTPUT_DIR = "/old_photo_PAR_B";  // Subdirectory to be appended to the input directory.
    const char *OUTPUT_TXT_PREFIX = "timing_B_"; // Prefix for the output text file name.

    char *suffix = NULL;
    if (strcmp(argv[3], "-size") == 0) {
        suffix = "-size.txt";
    } else if (strcmp(argv[3], "-name") == 0) {
        suffix = "-name.txt";
    } else {
        fprintf(stderr, "Invalid mode. Must be '-size' or '-name'.\n");
        exit(EXIT_FAILURE);  // Exit if the mode is invalid.
    }

    size_t output_dir_len = strlen(argv[1]) + strlen(OUTPUT_DIR) + 1;
    output_directory = malloc(output_dir_len);
    if (output_directory == NULL) {
        perror("Failed to allocate memory for output directory path");
        exit(EXIT_FAILURE);
    }
    // Concatenate the input directory path with the output subdirectory.
    snprintf(output_directory, output_dir_len, "%s%s", argv[1], OUTPUT_DIR);

    size_t output_txt_len = strlen(argv[1]) + strlen("/") + strlen(OUTPUT_TXT_PREFIX) +
                            snprintf(NULL, 0, "%d", atoi(argv[2])) + strlen(suffix) + 1;
    *output_txt = malloc(output_txt_len);
    if (*output_txt == NULL) {
        perror("Failed to allocate memory for output txt path");
        free(output_directory);  
        exit(EXIT_FAILURE);
    }
    // Concatenate the suffix to the output text file name.
    snprintf(*output_txt, output_txt_len, "%s/%s%d%s", argv[1], OUTPUT_TXT_PREFIX, atoi(argv[2]), suffix);

    // Check if the output directory exists, and create it if not.
    struct stat st = {0};
    if (stat(output_directory, &st) == -1) {
        if (mkdir(output_directory, 0777) == -1) {  // Create the directory with full permissions.
            perror("Failed to create output directory");
            free(output_directory); 
            exit(EXIT_FAILURE);
        }
    }
    return *output_txt;
}

/// @brief Extracts a numeric value from a string.
/// @param str Input string containing the numeric value.
/// @return The extracted numeric value as an integer. Returns 0 if no numeric value is found.
int extract_number(const char *str) {
    while (*str && !isdigit(*str)) str++;
    return isdigit(*str) ? atoi(str) : 0;
}

/// @brief Compares two files by name.
/// @param a Pointer to the first file.
/// @param b Pointer to the second file.
/// @return A negative value if file1 < file2, zero if file1 == file2, or a positive value if file1 > file2.
int compare_by_name(const void *a, const void *b) {
    FileInfo *file1 = (FileInfo *)a;
    FileInfo *file2 = (FileInfo *)b;

    int num1 = extract_number(file1->name);
    int num2 = extract_number(file2->name);

    if (num1 != num2) {
        return num1 - num2;
    }

    return strcmp(file1->name, file2->name);
}

/// @brief Compares two files by size.
/// @param a Pointer to the first file.
/// @param b Pointer to the second file.
/// @return A negative value if file1 < file2, zero if file1 == file2, or a positive value if file1 > file2.
int compare_by_size(const void *a, const void *b) {
    FileInfo *file1 = (FileInfo *)a;
    FileInfo *file2 = (FileInfo *)b;
    return (file1->size < file2->size) ? -1 : (file1->size > file2->size);
}

/// @brief Retrieves JPEG files from a directory and sorts them based on the specified option, loading them to file_list.
/// @param directory Path to the input directory.
/// @param sort_option Sorting option (-size or -name).
/// @param file_count Pointer to the number of files found.
/// @param output_directory Path to the output directory.
void get_jpeg_files(const char *sort_option, size_t *file_count) {
    DIR *d;
    struct dirent *f;
    struct stat st;
    FileInfo *files = NULL;
    size_t capacity = 10;

    char *extension = NULL;
    char filepath[512], output_filepath[512];

    // Struct to send to qsort
    files = malloc(capacity * sizeof(FileInfo));
    if (!files) {
        perror("Failed to allocate memory for files array");
        exit(EXIT_FAILURE);
    }

    d = opendir(input_directory);
    if (!d) {
        perror("Failed to open input directory");
        free(files);
        exit(EXIT_FAILURE);
    }

    while ((f = readdir(d)) != NULL) {
        if (strcmp(f->d_name, ".") == 0 || strcmp(f->d_name, "..") == 0) {
            continue;
        }

        // Check if the file is a JPEG file
        extension = strrchr(f->d_name, '.');
        if (!extension || strcasecmp(extension, ".jpeg") != 0) {
            continue;
        }

        snprintf(filepath, sizeof(filepath), "%s/%s", input_directory, f->d_name);
        if (stat(filepath, &st) == -1) {
            perror("Failed to get file size");
            continue;
        }

        // Checks if the file has already been processed
        snprintf(output_filepath, sizeof(output_filepath), "%s/%s", output_directory, f->d_name);
        if( access(output_filepath, F_OK ) != -1){
            continue;
        }

        // Reallocate memory if necessary
        if (*file_count == capacity) {
            capacity *= 2;
            FileInfo *temp = realloc(files, capacity * sizeof(FileInfo));
            if (!temp) {
                perror("Failed to reallocate memory for files array");
                closedir(d);
                for (size_t i = 0; i < *file_count; i++) {
                    free(files[i].name);
                }
                free(files);
                exit(EXIT_FAILURE);
            }
            files = temp;
        }

        files[*file_count].name = strdup(f->d_name);
        files[*file_count].size = st.st_size;
        (*file_count)++;
    }
    closedir(d);

    // Sort the files based on the specified option
    if (strcmp(sort_option, "-size") == 0) {
        qsort(files, *file_count, sizeof(FileInfo), compare_by_size);
    } else if (strcmp(sort_option, "-name") == 0) {
        qsort(files, *file_count, sizeof(FileInfo), compare_by_name);
    }

    // Load the file names into file_list
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
        file_list[i] = files[i].name;
    }

    // Free the memory allocated for the temporary files array
    free(files);
}

/// @brief Parses command-line arguments and calls the function to load the file names into file_list. Uses previously edited paths.
/// @param argc 
/// @param argv 
/// @param file_count Pointer to the number of files found.
/// @return The number of threads specified in the command line.
int read_command_line(int argc, char *argv[], size_t *file_count) {
    if (argc < COMMAND_LINE_OPTIONS) {
        fprintf(stderr, "Usage: %s <INPUT_DIR> <NUMBER_THREADS> <MODE>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    get_jpeg_files(argv[3], file_count);

    int num_threads = atoi(argv[2]);
    if (num_threads <= 0) {
        fprintf(stderr, "Invalid number of threads. The number of threads to create must be a positive number.\n");
        exit(EXIT_FAILURE);
    }

    return num_threads;
}