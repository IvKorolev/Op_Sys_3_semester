#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SHM_NAME "shm_lab"
#define SEM_READ_NAME "/sem_read"
#define SEM_WRITE_NAME "/sem_write"
#define BUF_SIZE 4096

int main() {
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        const char msg[] = "error: failed to create shared memory\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, BUF_SIZE) == -1) {
        const char msg[] = "error: failed to set size of shared memory\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    char *shared_memory = mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_memory == MAP_FAILED) {
        const char msg[] = "error: failed to map shared memory\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    sem_t *sem_read = sem_open(SEM_READ_NAME, O_CREAT, 0666, 0);
    sem_t *sem_write = sem_open(SEM_WRITE_NAME, O_CREAT, 0666, 1);
    if (sem_read == SEM_FAILED || sem_write == SEM_FAILED) {
        const char msg[] = "error: failed to create semaphores\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    pid_t child1 = fork();
    if (child1 == -1) {
        const char msg[] = "error: failed to create child1 process\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    if (child1 == 0) {
        execl("./child1", "child1", NULL);
        const char msg[] = "error: failed to execute child1\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    pid_t child2 = fork();
    if (child2 == -1) {
        const char msg[] = "error: failed to create child2 process\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    if (child2 == 0) {
        execl("./child2", "child2", NULL);
        const char msg[] = "error: failed to execute child2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    printf("Введите строки (нажмите Enter для завершения):\n");
    char buffer[BUF_SIZE];
    while (1) {
        ssize_t bytes = read(STDIN_FILENO, buffer, sizeof(buffer));
        if (bytes <= 0 || (bytes == 1 && buffer[0] == '\n')) {
            break;
        }

        sem_wait(sem_write);
        memcpy(shared_memory, buffer, bytes);
        sem_post(sem_read);

        sem_wait(sem_write);
        memcpy(buffer, shared_memory, BUF_SIZE);
        sem_post(sem_read);

        write(STDOUT_FILENO, buffer, strlen(buffer));
    }

    waitpid(child1, NULL, 0);
    waitpid(child2, NULL, 0);

    munmap(shared_memory, BUF_SIZE);
    shm_unlink(SHM_NAME);
    sem_close(sem_read);
    sem_close(sem_write);
    sem_unlink(SEM_READ_NAME);
    sem_unlink(SEM_WRITE_NAME);

    return 0;
}
