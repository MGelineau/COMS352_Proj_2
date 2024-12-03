#include <stdio.h>
#include "encrypt-module.h"
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>

void reset_requested() {
	log_counts();
}

void reset_finished() {
}

int main(int argc, char *argv[]) {
	    if (argc != 4) {
        fprintf(stderr, "Usage: %s <input_file> <output_file> <log_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Initialize the system
    init(argv[1], argv[2], argv[3]);

    int N, M;
    printf("Enter input buffer size (N > 1): ");
    scanf("%d", &N);
    printf("Enter output buffer size (M > 1): ");
    scanf("%d", &M);

    if (N <= 1 || M <= 1) {
        fprintf(stderr, "Buffer sizes must be greater than 1.\n");
        return EXIT_FAILURE;
    }

    init_buffer(&inputBuffer, N);
    init_buffer(&outputBuffer, M);

    pthread_t threads[5];
	printf("End of file reached.\n"); 
	log_counts();
}
