#include <stdio.h>      
#include <stdlib.h> 
#include <pthread.h>  
#include <time.h> 
#include <unistd.h>

typedef struct {
    long thread_id;          
    int **arrays;            
    int *sum_array;    
    long num_arrays;
    long arr_length;
    long num_threads; 
    pthread_mutex_t *lock;
} ThreadArgs;

void run_sequential(int **arrays, int *sum_array, long num_arrays, long arr_length) {
    for (long j = 0; j < arr_length; j++) {
        sum_array[j] = 0;
    }
    for (long i = 0; i < num_arrays; i++) {
        for (long j = 0; j < arr_length; j++) {
            sum_array[j] = sum_array[j] + arrays[i][j];
        }
    }
}

void* worker(void* _args) {
    ThreadArgs *args = (ThreadArgs*)_args;
    long my_id = args->thread_id;
    int **arrays = args->arrays;
    int *sum_array = args->sum_array;
    long num_arrays = args->num_arrays;
    long arr_length = args->arr_length;
    long num_threads = args->num_threads;
    pthread_mutex_t *lock = args->lock;

    if (arr_length >= num_arrays) {
        long items_per_thread = arr_length / num_threads;
        long start = my_id * items_per_thread;
        long end = start + items_per_thread;
        if (my_id == num_threads - 1) end = arr_length;
        for (long j = start; j < end; j++) {
            int local_sum = 0;
            for (long i = 0; i < num_arrays; i++) {
                local_sum = local_sum + arrays[i][j];
            }
            sum_array[j] = local_sum;
        }
    } else {
        long arrays_per_thread = num_arrays / num_threads;
        long start = my_id * arrays_per_thread;
        long end = start + arrays_per_thread;
        if (my_id == num_threads - 1) end = num_arrays; // :
        int *local_sum = (int*)calloc(arr_length, sizeof(int));
        if (local_sum == NULL) return NULL;

        for (long i = start; i < end; i++) {
            for (long j = 0; j < arr_length; j++) {
                local_sum[j] = local_sum[j] + arrays[i][j];
            }
        }

        // закрывает замок. Если другой поток уже переносит данные, наш поток засыпает и 
        // ждет здесь своей очереди. Это защита от состояния гонки (data race).
        pthread_mutex_lock(lock);
        for (long j = 0; j < arr_length; j++) {
            sum_array[j] = sum_array[j] + local_sum[j];
        }
        pthread_mutex_unlock(lock);

        free(local_sum);
    }
    //sleep(5);

    return NULL;
}

double get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

int main(int argc, char** argv) {
    if (argc < 4) {
        printf("Использование: ./lab2 <массивов> <длина> <потоки>\n");
        return 1;
    }
    long num_arrays = atoi(argv[1]);
    long arr_length = atoi(argv[2]);
    long num_threads = atoi(argv[3]);

    if (num_arrays <= 0 || arr_length <= 0 || num_threads <= 0) {
        printf("Ошибка: Все аргументы должны быть больше 0\n");
        return 1;
    }

    srand(time(NULL));
    int **arrays = (int**)malloc(num_arrays * sizeof(int*));
    for (long i = 0; i < num_arrays; i++) {
        arrays[i] = (int*)malloc(arr_length * sizeof(int));
        for (long j = 0; j < arr_length; j++) {
            arrays[i][j] = rand() % 10;
        }
    }
    int *sum_array = (int*)calloc(arr_length, sizeof(int));

    double t1 = get_time_ms();
    run_sequential(arrays, sum_array, num_arrays, arr_length);
    double t2 = get_time_ms();
    double seq_time = t2 - t1;
    printf("Последовательная версия выполнена за: %.2f мс\n", seq_time);

    for (long j = 0; j < arr_length; j++) {
        sum_array[j] = 0;
    }

    if (arr_length >= num_arrays) {
        printf("Выбрана стратегия 1 -деление по элементам массива\n");
    } else {
        printf("Выбрана стратегия 2 - деление по количеству массивов\n");
    }

    pthread_t *threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    ThreadArgs *thread_args = (ThreadArgs*)malloc(num_threads * sizeof(ThreadArgs));
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

    t1 = get_time_ms();
    for (long i = 0; i < num_threads; i++) {
        thread_args[i] = (ThreadArgs){
            .thread_id = i,
            .arrays = arrays,
            .sum_array = sum_array,
            .num_arrays = num_arrays,
            .arr_length = arr_length,
            .num_threads = num_threads,
            .lock = &lock,
        };
        pthread_create(&threads[i], NULL, worker, &thread_args[i]);
    }
    
    for (long i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    t2 = get_time_ms();
    double par_time = t2 - t1;

    double speedup = seq_time / par_time;
    double efficiency = speedup / (double)num_threads;

    printf("\nПараллельные результаты для %ld потоков:\n", num_threads);
    printf("Время выполнения: %.2f мс\n", par_time);
    printf("Ускорение: %.2f\n", speedup);
    printf("Эффективность: %.2f\n", efficiency);
    printf("\nВсё! Нажмите Enter для выхода...\n");
    getchar();
    free(threads);
    free(thread_args);
    for (long i = 0; i < num_arrays; i++) {
        free(arrays[i]);
    }
    free(arrays);
    free(sum_array);

    return 0;
}
