#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "util.c"

struct shm_buf {
    int size;
    char *data;
};

int main(int argc, char* argv[]) {
    int buf_size;
    struct timeval begin, end;
    char *source_file;
    char *target_file;
    FILE *source, *target;
    int n;
    struct shm_buf *buf;

    if (argc != 4) {
        printf("usage: ./cpy.out <buf_size> <source_file> <target_file>\n");
        return 1;
    }

    buf_size = atoi(argv[1]);
    source_file = argv[2];
    target_file = argv[3];

    source = fopen(source_file, "r");
    if (source == NULL) {
        perror("source file error");
        return 1;
    }

    target = fopen(target_file, "w");
    if (target == NULL) {
        perror("target file error");
        return 1;
    }

    buf = (struct shm_buf*) malloc(sizeof(int) + buf_size * sizeof(char));
    buf->data = (char *) malloc(buf_size * sizeof(char));

    gettimeofday(&begin, NULL);

    while (1) {
        n = fread(buf->data, sizeof(char), buf_size, source);
        buf->size = n;
        fwrite(buf->data, sizeof(char), buf->size, target);
        fflush(target);
        if (n < buf_size) {
            break;
        }
    }

    gettimeofday(&end, NULL);

    printf("%f", get_delta_timeofday(&begin, &end));
    fflush(stdout);

    return 0;
}


