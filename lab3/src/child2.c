#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define SHM_NAME "shm_lab"
#define SEM_READ_NAME "/sem_read"
#define SEM_WRITE_NAME "/sem_write"
#define BUF_SIZE 4096

int main() {
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        const char msg[] = "error: failed to open shared memory in child2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    char *shared_memory = mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_memory == MAP_FAILED) {
        const char msg[] = "error: failed to map shared memory in child2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    sem_t *sem_read = sem_open(SEM_READ_NAME, 0);
    sem_t *sem_write = sem_open(SEM_WRITE_NAME, 0);
    if (sem_read == SEM_FAILED || sem_write == SEM_FAILED) {
        const char msg[] = "error: failed to open semaphores in child2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    char buffer[BUF_SIZE];
    while (1) {
        sem_wait(sem_read);

        memcpy(buffer, shared_memory, BUF_SIZE);
        if (strlen(buffer) == 0) {
            break;
        }

        for (size_t i = 0; i < strlen(buffer); i++) {
            if (isspace((unsigned char)buffer[i])) {
                buffer[i] = '_';
            }
        }

        memcpy(shared_memory, buffer, BUF_SIZE);
        sem_post(sem_write);
    }

    munmap(shared_memory, BUF_SIZE);
    sem_close(sem_read);
    sem_close(sem_write);

    return 0;
}
