#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define BUF_SIZE 4096

int main() {
    char buffer[BUF_SIZE];

    while (true) {
        ssize_t bytes = read(STDIN_FILENO, buffer, sizeof(buffer));

        if (bytes < 0) {
            const char msg[] = "error: failed to read from stdin in child1\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        }

        if (bytes == 0) {
            break;
        }

        for (int i = 0; i < bytes; i++) {
            buffer[i] = tolower((unsigned char)buffer[i]);
        }

        if (write(STDOUT_FILENO, buffer, bytes) != bytes) {
            const char msg[] = "error: failed to write to stdout in child1\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        }
    }

    exit(EXIT_SUCCESS);
}
