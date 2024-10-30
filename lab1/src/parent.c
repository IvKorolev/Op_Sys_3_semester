#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUF_SIZE 4096

int main() {
    int pipe1[2], pipe2[2], pipe3[2];
    char buffer[BUF_SIZE];

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1 || pipe(pipe3) == -1) {
        perror("Ошибка при создании pipe");
        exit(EXIT_FAILURE);
    }

    pid_t child1 = fork();
    if (child1 == -1) {
        perror("Ошибка при создании дочернего процесса");
        exit(EXIT_FAILURE);
    }

    if (child1 == 0) {
        close(pipe1[STDOUT_FILENO]);
        close(pipe2[STDIN_FILENO]);
        close(pipe3[STDIN_FILENO]);
        close(pipe3[STDOUT_FILENO]);

        dup2(pipe1[STDIN_FILENO], STDIN_FILENO);
        close(pipe1[STDIN_FILENO]);

        dup2(pipe2[STDOUT_FILENO], STDOUT_FILENO);
        close(pipe2[STDOUT_FILENO]);

        execl("./child1", "child1", NULL);
        perror("Ошибка при выполнении child1");
        exit(EXIT_FAILURE);
    }

    pid_t child2 = fork();
    if (child2 == -1) {
        perror("Ошибка при создании дочернего процесса");
        exit(EXIT_FAILURE);
    }

    if (child2 == 0) {
        close(pipe1[STDIN_FILENO]);
        close(pipe1[STDOUT_FILENO]);
        close(pipe2[STDOUT_FILENO]);
        close(pipe3[STDIN_FILENO]);

        dup2(pipe2[STDIN_FILENO], STDIN_FILENO);
        close(pipe2[STDIN_FILENO]);

        dup2(pipe3[STDOUT_FILENO], STDOUT_FILENO);
        close(pipe3[STDOUT_FILENO]);

        execl("./child2", "child2", NULL);
        perror("Ошибка при выполнении child2");
        exit(EXIT_FAILURE);
    }

    close(pipe1[STDIN_FILENO]);
    close(pipe2[STDIN_FILENO]);
    close(pipe2[STDOUT_FILENO]);
    close(pipe3[STDOUT_FILENO]);

    printf("Введите строки (нажмите Enter для завершения):\n");
    ssize_t bytes;
    while ((bytes = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
        if (buffer[0] == '\n' && bytes == 1) {
            break;
        }
        write(pipe1[STDOUT_FILENO], buffer, bytes);

        ssize_t nread = read(pipe3[STDIN_FILENO], buffer, sizeof(buffer));
        if (nread > 0) {
            write(STDOUT_FILENO, buffer, nread);
        }
    }

    close(pipe1[STDOUT_FILENO]);
    close(pipe3[STDIN_FILENO]);

    waitpid(child1, NULL, 0);
    waitpid(child2, NULL, 0);

    return 0;
}

