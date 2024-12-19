#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <time.h>

#define ROWS 10
#define COLS 10

int matrix[ROWS][COLS];
int temp_matrix[ROWS][COLS];

typedef struct {
    int start_row;
    int end_row;
    int window_size;
} ThreadArgs;

int compare(const void* a, const void* b);

int find_median(int* window, int size);

void* median_filter(void* args);

void copy_temp_to_matrix();

void generate_matrix();

void print_matrix(int mat[ROWS][COLS]);

sem_t start_sem;
sem_t end_sem;

int stop_threads = 0;

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: ./a.out <window_size> <iterations> <max_threads>\n");
        return EXIT_FAILURE;
    }

    int window_size = atoi(argv[1]);
    int iterations = atoi(argv[2]);
    int max_threads = atoi(argv[3]);

    if (window_size % 2 == 0 || window_size < 1 || iterations < 1 || max_threads < 1) {
        fprintf(stderr, "Invalid arguments. Window size must be odd and >= 1. Iterations and threads > 0.\n");
        return EXIT_FAILURE;
    }

    if (max_threads > ROWS) {
        max_threads = ROWS;
    }

    generate_matrix();

    printf("Original matrix:\n");
    print_matrix(matrix);

    pthread_t threads[max_threads];
    ThreadArgs thread_args[max_threads];
    int rows_per_thread = ROWS / max_threads;

    sem_init(&start_sem, 0, 0);
    sem_init(&end_sem, 0, 0);

    for (int i = 0; i < max_threads; i++) {
        thread_args[i].start_row = i * rows_per_thread;
        thread_args[i].end_row = (i == max_threads - 1) ? ROWS : (i + 1) * rows_per_thread;
        thread_args[i].window_size = window_size;

        if (pthread_create(&threads[i], NULL, median_filter, &thread_args[i]) != 0) {
            perror("pthread_create failed");
            return EXIT_FAILURE;
        }
    }

    clock_t start_time = clock();

    for (int iter = 0; iter < iterations; iter++) {
        for (int i = 0; i < max_threads; i++) {
            sem_post(&start_sem);
        }

        for (int i = 0; i < max_threads; i++) {
            sem_wait(&end_sem);
        }

        copy_temp_to_matrix();
    }

    stop_threads = 1;
    for (int i = 0; i < max_threads; i++) {
        sem_post(&start_sem);
    }

    for (int i = 0; i < max_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    sem_destroy(&start_sem);
    sem_destroy(&end_sem);

    clock_t end_time = clock();

    printf("Result matrix:\n");
    print_matrix(matrix);

    double time_spent = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    printf("Time taken: %f seconds\n", time_spent);

    return EXIT_SUCCESS;
}

int compare(const void* a, const void* b) {
    return (*(int*)a - *(int*)b);
}

int find_median(int* window, int size) {
    qsort(window, size, sizeof(int), compare);
    return window[size / 2];
}

void* median_filter(void* args) {
    ThreadArgs* thread_args = (ThreadArgs*)args;
    int start_row = thread_args->start_row;
    int end_row = thread_args->end_row;
    int window_size = thread_args->window_size;
    int offset = window_size / 2;

    while (1) {
        sem_wait(&start_sem);

        if (stop_threads) {
            break;
        }

        for (int i = start_row; i < end_row; i++) {
            for (int j = 0; j < COLS; j++) {
                if (i < offset || i >= ROWS - offset || j < offset || j >= COLS - offset) {
                    temp_matrix[i][j] = matrix[i][j];
                } else {
                    int window[window_size * window_size];
                    int idx = 0;
                    for (int wi = -offset; wi <= offset; wi++) {
                        for (int wj = -offset; wj <= offset; wj++) {
                            window[idx++] = matrix[i + wi][j + wj];
                        }
                    }
                    temp_matrix[i][j] = find_median(window, window_size * window_size);
                }
            }
        }

        sem_post(&end_sem);
    }

    return NULL;
}

void copy_temp_to_matrix() {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            matrix[i][j] = temp_matrix[i][j];
        }
    }
}

void generate_matrix() {
    srand(time(NULL));
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            matrix[i][j] = rand() % 100 + 1;
        }
    }
}

void print_matrix(int mat[ROWS][COLS]) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            printf("%3d ", mat[i][j]);
        }
        printf("\n");
    }
}