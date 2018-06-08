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

// void is_empty(int sem_set_id) {
//     struct sembuf sem_op;

//     sem_op.sem_num = 0;
//     sem_op.sem_op = 0;
//     sem_op.sem_flg = 0;
//     semop(sem_set_id, &sem_op, 1);
// }

// void fill_it(int sem_set_id) {
//     struct sembuf sem_op;

//     sem_op.sem_num = 0;
//     sem_op.sem_op = 1;
//     sem_op.sem_flg = 0;
//     semop(sem_set_id, &sem_op, 1);
// }

void sem_lock(int sem_set_id, int sem_num) {
    struct sembuf sem_op;

    /* wait on the semaphore, unless it's value is non-negative. */
    sem_op.sem_num = sem_num;
    sem_op.sem_op = -1;
    sem_op.sem_flg = 0;
    semop(sem_set_id, &sem_op, 1);
}


void sem_unlock(int sem_set_id, int sem_num) {
    struct sembuf sem_op;

    sem_op.sem_num = sem_num;
    sem_op.sem_op = 1;
    sem_op.sem_flg = 0;
    semop(sem_set_id, &sem_op, 1);
}

struct shm_buf {
    int size;
    char *data;
};


int main(int argc, char* argv[]) {
    int buf_size;
    int sem_id;
    union semun {
        int                 val;
        struct semid_ds     *buf;
        ushort              *array;
    } sem_val;
    int shm_id;
    char* shm_addr;
    struct shmid_ds shm_desc;
    struct timeval begin, end;
    char *source_file;
    char *target_file;
    FILE *source, *target;
    int n;
    struct shm_buf *buf;

    if (argc != 4) {
        printf("usage: ./shm.out <buf_size> <source_file> <target_file>\n");
        return 1;
    }

    buf_size = atoi(argv[1]);
    source_file = argv[2];
    target_file = argv[3];
    

    sem_id = semget(IPC_PRIVATE, 2, IPC_CREAT | 0600);
    // First is writeable, second is readable
    if (sem_id == -1) {
        perror("main: semget");
        exit(1);
    }

    sem_val.val = 0; // At first shared mem is not readable
    if (semctl(sem_id, 1, SETVAL, sem_val) == -1) {
        perror("main: semctl");
        exit(1);
    }

    sem_val.val = 1; // At first shared mem is writable
    if (semctl(sem_id, 0, SETVAL, sem_val) == -1) {
        perror("main: semctl");
        exit(1);
    }

    shm_id = shmget(IPC_PRIVATE, buf_size + sizeof(int), IPC_CREAT | IPC_EXCL | 0600);
    if (shm_id == -1) {
        perror("main: shmget: ");
        exit(1);
    }

    // shm_addr = shmat(shm_id, 0, 0);
    // if (!shm_addr) {
    //     perror("main: shmat: ");
    //     exit(1);
    // }

    // buf = (struct shm_buf*) shm_addr;

    if (fork() == 0) { /* Child process, read from source file and write to shared mem */
        source = fopen(source_file, "r");
        if (source == NULL) {
            perror("source file error");
            return 1;
        }

        shm_addr = shmat(shm_id, 0, 0);
        if (!shm_addr) {
            perror("main: shmat: ");
            exit(1);
        }

        buf = (struct shm_buf*) shm_addr;

        while (1) {
            sem_lock(sem_id, 0); // Get write access, shmem is empty
            n = fread(&(buf->data), 1, buf_size, source);
            buf->size = n;
            sem_unlock(sem_id, 1); // Give read access, shmem is full
            if (n < buf_size) {
                break;
            }
        }

        fclose(source);

        exit(0);
    } else { /* Parent process */
        target = fopen(target_file, "w");
        if (target == NULL) {
            perror("target file error");
            return 1;
        }

        shm_addr = shmat(shm_id, 0, 0);
        if (!shm_addr) {
            perror("main: shmat: ");
            exit(1);
        }

        buf = (struct shm_buf*) shm_addr;

        gettimeofday(&begin, NULL);

        while (1) {
            sem_lock(sem_id, 1);
            fwrite(&(buf->data), 1, buf->size, target);
            n = buf->size;
            fflush(target);
            sem_unlock(sem_id, 0);
            if (n < buf_size) {
                break;
            }
        }

        gettimeofday(&end, NULL);

        /* wait for child process's terination. */
        {
            int child_status;

            wait(&child_status);
        }

        fclose(target);

        printf("%f", get_delta_timeofday(&begin, &end));
        fflush(stdout);

        /* detach the shared memory segment from our process's address space. */
        if (shmdt(shm_addr) == -1) {
            perror("main: shmdt: ");
        }

        /* de-allocate the shared memory segment. */
        if (shmctl(shm_id, IPC_RMID, &shm_desc) == -1) {
            perror("main: shmctl: ");
        }
    }

    return 0;
}


