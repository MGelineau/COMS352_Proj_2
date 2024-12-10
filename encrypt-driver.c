#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "encrypt-module.h"

// Circular buffer structure
typedef struct {
    char* buf;
    int size;
    int in;
    int out;
    sem_t empty;
    sem_t full;
    pthread_mutex_t lock;
} Buffer;

// Buffers and shared flags
Buffer *inputBuffer, *outputBuffer;
volatile int eof_reached = 0; // Signals EOF reached
sem_t ready_to_process;       // Semaphore to signal data readiness

// Function to create a buffer
Buffer* create_buffer(int size) {
    Buffer* b = malloc(sizeof(Buffer));
    b->buf = malloc(size);
    b->size = size;
    b->in = b->out = 0;
    sem_init(&b->empty, 0, size);
    sem_init(&b->full, 0, 0);
    pthread_mutex_init(&b->lock, NULL);
    return b;
}

// Function to free buffer memory
void free_buffer(Buffer* b) {
    free(b->buf);
    sem_destroy(&b->empty);
    sem_destroy(&b->full);
    pthread_mutex_destroy(&b->lock);
    free(b);
}

// Reader thread function
void* reader_thread(void* arg) {
    printf("Reader started\n");
    while (1) {
        sem_wait(&inputBuffer->empty);
        pthread_mutex_lock(&inputBuffer->lock);

        int c = read_input();
        if (c == EOF) {
            eof_reached = 1;
            pthread_mutex_unlock(&inputBuffer->lock);
            sem_post(&inputBuffer->full); // Signal EOF to other threads
            sem_post(&ready_to_process); // Notify ready state
            break;
        }

        inputBuffer->buf[inputBuffer->in] = c;
        inputBuffer->in = (inputBuffer->in + 1) % inputBuffer->size;

        pthread_mutex_unlock(&inputBuffer->lock);
        sem_post(&inputBuffer->full);
        sem_post(&ready_to_process); // Notify that data is ready
    }
    printf("Reader finished\n");
    return NULL;
}

// Encryptor thread function
void* encryptor_thread(void* arg) {
    printf("Encryptor started\n");
    while (!eof_reached || inputBuffer->in != inputBuffer->out) {
        sem_wait(&ready_to_process);  // Wait until data is ready
        sem_wait(&inputBuffer->full); // Wait for input buffer data
        sem_wait(&outputBuffer->empty); // Wait for space in the output buffer

        pthread_mutex_lock(&inputBuffer->lock);
        pthread_mutex_lock(&outputBuffer->lock);

        // Process data
        char c = inputBuffer->buf[inputBuffer->out];
        inputBuffer->out = (inputBuffer->out + 1) % inputBuffer->size;

        c = encrypt(c);
        outputBuffer->buf[outputBuffer->in] = c;
        outputBuffer->in = (outputBuffer->in + 1) % outputBuffer->size;

        pthread_mutex_unlock(&outputBuffer->lock);
        pthread_mutex_unlock(&inputBuffer->lock);

        sem_post(&inputBuffer->empty); // Signal space in input buffer
        sem_post(&outputBuffer->full); // Signal data in output buffer
        sem_post(&ready_to_process);  // Signal readiness of next stage
    }
    printf("Encryptor finished\n");
    return NULL;
}

// Writer thread function
void* writer_thread(void* arg) {
    printf("Writer started\n");
    while (!eof_reached || outputBuffer->in != outputBuffer->out) {
        sem_wait(&outputBuffer->full); // Wait for output buffer data
        pthread_mutex_lock(&outputBuffer->lock);

        char c = outputBuffer->buf[outputBuffer->out];
        outputBuffer->out = (outputBuffer->out + 1) % outputBuffer->size;

        write_output(c);

        pthread_mutex_unlock(&outputBuffer->lock);
        sem_post(&outputBuffer->empty); // Signal space in output buffer
    }
    printf("Writer finished\n");
    return NULL;
}

// Reset handlers
void reset_requested() {
    log_counts();          // Log character counts
}

void reset_finished() {
    // Reset complete
}

// Main function
int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <input> <output> <log>\n", argv[0]);
        return 1;
    }

    init(argv[1], argv[2], argv[3]);

    int buffer_size;
    printf("Buffer size: ");
    scanf("%d", &buffer_size);

    if (buffer_size <= 1) {
        printf("Buffer size must be > 1\n");
        return 1;
    }

    // Create buffers
    inputBuffer = create_buffer(buffer_size);
    outputBuffer = create_buffer(buffer_size);

    // Initialize semaphore for signaling readiness
    sem_init(&ready_to_process, 0, 0);

    // Create threads
    pthread_t reader, encryptor, writer;
    pthread_create(&reader, NULL, reader_thread, NULL);
    pthread_create(&encryptor, NULL, encryptor_thread, NULL);
    pthread_create(&writer, NULL, writer_thread, NULL);

    // Wait for threads to finish
    pthread_join(reader, NULL);
    pthread_join(encryptor, NULL);
    pthread_join(writer, NULL);

    log_counts(); // Log final counts

    // Clean up
    free_buffer(inputBuffer);
    free_buffer(outputBuffer);
    sem_destroy(&ready_to_process);

    return 0;
}
