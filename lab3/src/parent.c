#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SHM_NAME "shm_lab"
#define SEM_READ1_NAME "/sem_read1"
#define SEM_WRITE1_NAME "/sem_write1"
#define SEM_READ2_NAME "/sem_read2"
#define SEM_WRITE2_NAME "/sem_write2"
#define BUF_SIZE 4096

int main() {
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Ошибка создания разделяемой памяти");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(shm_fd, BUF_SIZE) == -1) {
        perror("Ошибка установки размера памяти");
        exit(EXIT_FAILURE);
    }
    char *shared_memory = mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
                               shm_fd, 0);
    if (shared_memory == MAP_FAILED) {
        perror("Ошибка маппинга памяти");
        exit(EXIT_FAILURE);
    }
    sem_t *sem_read1 = sem_open(SEM_READ1_NAME, O_CREAT, 0666, 0);
    sem_t *sem_write1 = sem_open(SEM_WRITE1_NAME, O_CREAT, 0666, 1);
    sem_t *sem_read2 = sem_open(SEM_READ2_NAME, O_CREAT, 0666, 0);
    sem_t *sem_write2 = sem_open(SEM_WRITE2_NAME, O_CREAT, 0666, 0);
    if (!sem_read1 || !sem_write1 || !sem_read2 || !sem_write2) {
        perror("Ошибка создания семафоров");
        exit(EXIT_FAILURE);
    }
    pid_t child1 = fork();
    if (child1 == -1) {
        perror("Ошибка создания child1");
        exit(EXIT_FAILURE);
    }
    if (child1 == 0) {
        execl("./child1", "child1", NULL);
        perror("Ошибка запуска child1");
        exit(EXIT_FAILURE);
    }
    pid_t child2 = fork();
    if (child2 == -1) {
        perror("Ошибка создания child2");
        exit(EXIT_FAILURE);
    }
    if (child2 == 0) {
        execl("./child2", "child2", NULL);
        perror("Ошибка запуска child2");
        exit(EXIT_FAILURE);
    }
    printf("Введите строки (Enter для завершения):\n");
    char buffer[BUF_SIZE];
    while (1) {
        if (!fgets(buffer, BUF_SIZE, stdin)) {
            break;
        }
        if (buffer[0] == '\n') {
            sem_wait(sem_write1);
            strcpy(shared_memory, "END");
            sem_post(sem_read1);
            sem_wait(sem_write1);
            sem_post(sem_read2);
            break;
        }
        sem_wait(sem_write1);
        strcpy(shared_memory, buffer);
        sem_post(sem_read1);
        sem_wait(sem_write1);
        sem_post(sem_read2);
        sem_wait(sem_write2);
        printf("> %s\n", shared_memory);
        sem_post(sem_write1);
    }


    sem_wait(sem_write1);
    strcpy(shared_memory, "END");
    sem_post(sem_read1);
    sem_wait(sem_write1);
    sem_post(sem_read2);
    waitpid(child1, NULL, 0);
    waitpid(child2, NULL, 0);
    munmap(shared_memory, BUF_SIZE);
    shm_unlink(SHM_NAME);
    sem_close(sem_read1);
    sem_close(sem_write1);
    sem_close(sem_read2);
    sem_close(sem_write2);
    sem_unlink(SEM_READ1_NAME);
    sem_unlink(SEM_WRITE1_NAME);
    sem_unlink(SEM_READ2_NAME);
    sem_unlink(SEM_WRITE2_NAME);

    return 0;
}