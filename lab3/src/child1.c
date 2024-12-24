#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define SHM_NAME "shm_lab"
#define SEM_READ1_NAME "/sem_read1"
#define SEM_WRITE1_NAME "/sem_write1"
#define BUF_SIZE 4096

int main() {
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Ошибка открытия памяти в child1");
        exit(EXIT_FAILURE);
    }

    char *shared_memory = mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_memory == MAP_FAILED) {
        perror("Ошибка маппинга памяти в child1");
        exit(EXIT_FAILURE);
    }

    sem_t *sem_read1 = sem_open(SEM_READ1_NAME, 0);
    sem_t *sem_write1 = sem_open(SEM_WRITE1_NAME, 0);
    if (!sem_read1 || !sem_write1) {
        perror("Ошибка открытия семафоров в child1");
        exit(EXIT_FAILURE);
    }

    while (1) {
        sem_wait(sem_read1);

        if (strcmp(shared_memory, "END") == 0) {
            break;
        }

        for (size_t i = 0; shared_memory[i]; i++) {
            shared_memory[i] = tolower((unsigned char)shared_memory[i]);
        }

        sem_post(sem_write1);
    }

    munmap(shared_memory, BUF_SIZE);
    sem_close(sem_read1);
    sem_close(sem_write1);

    return 0;
}
