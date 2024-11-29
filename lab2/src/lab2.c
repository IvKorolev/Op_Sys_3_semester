#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define BUF_SIZE 4096

typedef struct {
    double** matrix;
    int n;
    int current_row;  // Общая текущая строка
    pthread_barrier_t* barrier;
} ThreadArgs;

void read_matrix(const char* filename, double*** matrix, int* n) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char buffer[BUF_SIZE];
    ssize_t bytes = read(fd, buffer, sizeof(buffer));
    if (bytes <= 0) {
        perror("Error reading file");
        close(fd);
        exit(EXIT_FAILURE);
    }
    close(fd);

    char* ptr = buffer;
    *n = strtol(ptr, &ptr, 10);

    *matrix = malloc(*n * sizeof(double*));
    for (int i = 0; i < *n; i++) {
        (*matrix)[i] = malloc((*n + 1) * sizeof(double));
        for (int j = 0; j <= *n; j++) {
            (*matrix)[i][j] = strtod(ptr, &ptr);
        }
    }
}

void print_matrix(double** matrix, int n) {
    char buf[BUF_SIZE];
    for (int i = 0; i < n; i++) {
        for (int j = 0; j <= n; j++) {
            int len = snprintf(buf, sizeof(buf), "%.2lf ", matrix[i][j]);
            write(STDOUT_FILENO, buf, len);
        }
        write(STDOUT_FILENO, "\n", 1);
    }
    write(STDOUT_FILENO, "\n", 1);
}

void* gaussian_elimination_thread(void* args) {
    ThreadArgs* data = (ThreadArgs*)args;
    double** matrix = data->matrix;
    int n = data->n;

    for (int row = 0; row < n; row++) {
        if (data->current_row == row) {
            int max_row = row;
            for (int i = row + 1; i < n; i++) {
                if (fabs(matrix[i][row]) > fabs(matrix[max_row][row])) {
                    max_row = i;
                }
            }
            if (max_row != row) {
                for (int j = 0; j <= n; j++) {
                    double temp = matrix[row][j];
                    matrix[row][j] = matrix[max_row][j];
                    matrix[max_row][j] = temp;
                }
            }
            if (fabs(matrix[row][row]) < 1e-9) {
                const char* msg = "Error: Singular matrix. Cannot solve system.\n";
                write(STDERR_FILENO, msg, strlen(msg));
                exit(EXIT_FAILURE);
            }
        }
        pthread_barrier_wait(data->barrier);
        for (int i = row + 1; i < n; i++) {
            double factor = matrix[i][row] / matrix[row][row];
            for (int j = row; j <= n; j++) {
                matrix[i][j] -= factor * matrix[row][j];
            }
        }
        pthread_barrier_wait(data->barrier);
    }
    return NULL;
}

void back_substitution(double** matrix, double* solution, int n) {
    for (int i = n - 1; i >= 0; i--) {
        solution[i] = matrix[i][n];
        for (int j = i + 1; j < n; j++) {
            solution[i] -= matrix[i][j] * solution[j];
        }
        solution[i] /= matrix[i][i];
    }
}

int main(int argc, char* argv[]) {
    clock_t start = clock();
    if (argc != 3) {
        const char* msg = "Usage: ./gauss_solver <matrix_file> <max_threads>\n";
        write(STDERR_FILENO, msg, strlen(msg));
        return EXIT_FAILURE;
    }

    double** matrix;
    int n;
    read_matrix(argv[1], &matrix, &n);

    int max_threads = strtol(argv[2], NULL, 10);
    if (max_threads <= 0) {
        const char* msg = "Error: max_threads must be a positive integer\n";
        write(STDERR_FILENO, msg, strlen(msg));
        return EXIT_FAILURE;
    }

    pthread_t threads[max_threads];
    ThreadArgs thread_args[max_threads];
    pthread_barrier_t barrier;

    pthread_barrier_init(&barrier, NULL, max_threads);

    for (int i = 0; i < max_threads; i++) {
        thread_args[i].matrix = matrix;
        thread_args[i].n = n;
        thread_args[i].current_row = 0;
        thread_args[i].barrier = &barrier;

        pthread_create(&threads[i], NULL, gaussian_elimination_thread, &thread_args[i]);
    }

    for (int i = 0; i < max_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_barrier_destroy(&barrier);

    print_matrix(matrix, n);

    double* solution = malloc(n * sizeof(double));
    back_substitution(matrix, solution, n);

    char buf[BUF_SIZE];
    for (int i = 0; i < n; i++) {
        int len = snprintf(buf, sizeof(buf), "x%d = %.6lf\n", i + 1, solution[i]);
        write(STDOUT_FILENO, buf, len);
    }

    for (int i = 0; i < n; i++) {
        free(matrix[i]);
    }
    free(matrix);
    free(solution);
    clock_t end = clock();
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    int len = snprintf(buf, sizeof(buf), "Time elapsed: %f\n", time_taken);
    write(STDOUT_FILENO, buf, len);
    return 0;
}
