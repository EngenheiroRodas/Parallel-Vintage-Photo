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
#include <time.h>
#include <pthread.h>
#include <ctype.h>
#include <errno.h>
#include "image-lib.h"
#include "helper_f.h"

int main(int argc, char *argv[]) {
    struct timespec start_time_total, end_time_total;
    struct timespec start_time_seq, end_time_seq;
    struct timespec start_time_par, end_time_par;

	clock_gettime(CLOCK_MONOTONIC, &start_time_total);
	clock_gettime(CLOCK_MONOTONIC, &start_time_seq);

    FILE *output_file;
    char *output_txt = NULL;
    size_t file_count = 0;

    char *output_directory = read_command_line(argc, argv, &file_count, &output_txt);
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

    output_file = fopen(output_txt, "w");
    if (output_file == NULL) {
        perror("Failed to open output file");
        free(output_txt);
        exit(EXIT_FAILURE);
    }

    fprintf(output_file, "\n\n\n\tseq \t %10jd.%09ld\n", seq_time.tv_sec, seq_time.tv_nsec);
    fprintf(output_file, "\tpar \t %10jd.%09ld\n", par_time.tv_sec, par_time.tv_nsec);
    fprintf(output_file, "total \t %10jd.%09ld\n", total_time.tv_sec, total_time.tv_nsec);

    fclose(output_file);

    return 0;
}
