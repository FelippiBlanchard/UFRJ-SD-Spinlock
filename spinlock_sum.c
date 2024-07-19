#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

typedef struct {
    atomic_bool held;
} spinlock_t;

void init_lock(spinlock_t *lock) {
    atomic_store(&lock->held, false);
}

void lock_acquire(spinlock_t *lock) {
    while (atomic_exchange(&lock->held, true)) {
        while (atomic_load(&lock->held)) {
        }
    }
}

void lock_release(spinlock_t *lock) {
    atomic_store(&lock->held, false);
}

typedef struct {
    int index_start;
    int local_sum;
    int thread_id;
    int thread_count;
    int size_vetor;
    int8_t *vetor;
} thread_args_t;

spinlock_t spin_lock;
int global_result_sum = 0;

void *multithread_sum(void *arg) {
    thread_args_t *multithread_args = (thread_args_t *)arg;
    int start = multithread_args->index_start;
    int end = (multithread_args->thread_id == multithread_args->thread_count - 1) ? multithread_args->size_vetor : start + multithread_args->size_vetor / multithread_args->thread_count;

    multithread_args->local_sum = 0;
    for (int i = start; i < end; i++) {
        multithread_args->local_sum += multithread_args->vetor[i];
    }

    // REGIÃO CRÍTICA -> SOMA GLOBAL EM QUE MULTIPLAS THREADS ACESSAM O MESMO RECURSO
    lock_acquire(&spin_lock);
    global_result_sum += multithread_args->local_sum;
    lock_release(&spin_lock);

    return NULL;
}

void *single_thread_sum(void *arg) {
    thread_args_t *args = (thread_args_t *)arg;
    int sum = 0;
    for (int i = 0; i < args->size_vetor; i++) {
        sum += args->vetor[i];
    }
    int *result = malloc(sizeof(int));
    *result = sum;
    return result;
}

double time_diff(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

int main() {
    int N[] = {10000000, 100000000, 1000000000};
    int K[] = {1, 2, 4, 8, 16, 32, 64, 128, 256};
    int qty_repeat = 10;

    FILE *output_file = fopen("performance_data.csv", "w");
    if (output_file == NULL) {
        fprintf(stderr, "Failed to open file for writing\n");
        return EXIT_FAILURE;
    }

    fprintf(output_file, "VecSize,ThreadCount,AvgMultithreadTime,AvgSingleThreadTime\n");

    for (int i = 0; i < sizeof(N) / sizeof(N[0]); i++) {
        int size_vetor = N[i];

        for (int j = 0; j < sizeof(K) / sizeof(K[0]); j++) {
            int qty_thread = K[j];

            if (qty_thread > size_vetor) {
                printf("Skipping: Thread count exceeds vector size for size_vetor=%d and thread_count=%d.\n", size_vetor, qty_thread);
                continue;
            }

            double multithread_time = 0.0;
            double singlethread_time = 0.0;

            for (int k = 0; k < qty_repeat; k++) {
                pthread_t threads[qty_thread];
                thread_args_t multithread_args[qty_thread];
                int8_t *vetor = malloc(size_vetor * sizeof(int8_t));
                if (vetor == NULL) {
                    fprintf(stderr, "Memory allocation failed\n");
                    return EXIT_FAILURE;
                }

                srand(time(NULL));
                for (int l = 0; l < size_vetor; l++) {
                    vetor[l] = rand() % 201 - 100;
                }

                init_lock(&spin_lock);
                global_result_sum = 0;

                struct timespec start_mt, end_mt;
                clock_gettime(CLOCK_MONOTONIC, &start_mt);
                
                for (int l = 0; l < qty_thread; l++) {
                    multithread_args[l].index_start = l * (size_vetor / qty_thread);
                    multithread_args[l].thread_count = qty_thread;
                    multithread_args[l].thread_id = l;
                    multithread_args[l].size_vetor = size_vetor;
                    multithread_args[l].vetor = vetor;
                    pthread_create(&threads[l], NULL, multithread_sum, &multithread_args[l]);
                }

                for (int l = 0; l < qty_thread; l++) {
                    pthread_join(threads[l], NULL);
                }

                clock_gettime(CLOCK_MONOTONIC, &end_mt);
                multithread_time += time_diff(start_mt, end_mt);

                pthread_t single_thread;
                thread_args_t singlethread_args;
                singlethread_args.size_vetor = size_vetor;
                singlethread_args.vetor = vetor;

                struct timespec start_st, end_st;
                clock_gettime(CLOCK_MONOTONIC, &start_st);
                pthread_create(&single_thread, NULL, single_thread_sum, &singlethread_args);
                int *st_result;
                pthread_join(single_thread, (void **)&st_result);
                clock_gettime(CLOCK_MONOTONIC, &end_st);
                singlethread_time += time_diff(start_st, end_st);

                //CASO SOMA DE 1 THREAD DIFERENTE DE SOMA DE K THREADS, PRINTA ERRO
                if (global_result_sum != *st_result) {
                    printf("Sum mismatch for size_vetor=%d and thread_count=%d on iteration %d.\n", size_vetor, qty_thread, k + 1);
                }

                free(vetor);
                free(st_result);
            }

            double avg_mt_time = multithread_time / qty_repeat;
            double avg_st_time = singlethread_time / qty_repeat;

            printf("size_vetor=%d, thread_count=%d, AvgMultithreadTime=%.5f seconds\n", size_vetor, qty_thread, avg_mt_time);
            printf("size_vetor=%d, thread_count=%d, AvgSingleThreadTime=%.5f seconds\n", size_vetor, qty_thread, avg_st_time);

            fprintf(output_file, "%d,%d,%.5f,%.5f\n", size_vetor, qty_thread, avg_mt_time, avg_st_time);
        }
    }

    fclose(output_file);

    return EXIT_SUCCESS;
}
