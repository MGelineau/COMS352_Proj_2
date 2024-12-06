#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "encrypt-module.h"

typedef struct {
    char* buf;
    int size;
    int in;
    int out;
    sem_t empty;
    sem_t full;
} Buffer;

Buffer* buffer;
volatile int done = 0;

void reset_requested() {
    log_counts();
}

void reset_finished() {
}

Buffer* create_buffer(int size) {
    Buffer* b = malloc(sizeof(Buffer));
    b->buf = malloc(size);
    b->size = size;
    b->in = b->out = 0;
    sem_init(&b->empty, 0, size);
    sem_init(&b->full, 0, 0);
    return b;
}

void* reader_thread(void* arg) {
    int c;
    printf("Reader started\n");
    while ((c = read_input()) != EOF) {
        printf("Read: %c\n", c);
        sem_wait(&buffer->empty);
        buffer->buf[buffer->in] = c;
        buffer->in = (buffer->in + 1) % buffer->size;
        count_input(c);
        sem_post(&buffer->full);
    }
    done = 1;
    printf("Reader finished\n");
    return NULL;
}

void* writer_thread(void* arg) {
    printf("Writer started\n");
    while (!done || buffer->in != buffer->out) {
        sem_wait(&buffer->full);
        char c = buffer->buf[buffer->out];
        write_output(c);
        printf("Wrote: %c\n", c);
        count_output(c);
        buffer->out = (buffer->out + 1) % buffer->size;
        sem_post(&buffer->empty);
    }
    printf("Writer finished\n");
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <input> <output> <log>\n", argv[0]);
        return 1;
    }

    init(argv[1], argv[2], argv[3]);

    int size;
    printf("Buffer size: ");
    scanf("%d", &size);

    if (size <= 1) {
        printf("Buffer size must be > 1\n");
        return 1;
    }

    buffer = create_buffer(size);

    pthread_t reader, writer;
    pthread_create(&reader, NULL, reader_thread, NULL);
    pthread_create(&writer, NULL, writer_thread, NULL);

    pthread_join(reader, NULL);
    pthread_join(writer, NULL);

    free(buffer->buf);
    free(buffer);
    return 0;
}