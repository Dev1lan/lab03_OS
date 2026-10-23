#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define OPS_PER_THREAD 100000
#define REPEATS 5

typedef struct {
    const int *arrays;
    long long *result;
    size_t k, n, begin, end;
} Task;

static void *sum_part(void *arg) {
    Task *task = arg;
    // if (task->begin != 0) {
    //    sleep(5);
    //}
    for (size_t j = task->begin; j < task->end; j++) {
        long long sum = 0;
        for (size_t i = 0; i < task->k; i++) {
            sum += task->arrays[i * task->n + j];
        }
        task->result[j] = sum;
    }
    return NULL;
}

static size_t choose_threads(size_t k, size_t n, size_t limit) {
    size_t operations = k * n;
    size_t count = 1 + (operations - 1) / OPS_PER_THREAD;
    if (count > limit) count = limit;
    if (count > n) count = n;
    return count;
}

static int sum_arrays(const int *arrays, long long *result,
                      size_t k, size_t n, size_t count) {
    if (count == 1) {
        Task task = {arrays, result, k, n, 0, n};
        sum_part(&task);
        return 0;
    }
    pthread_t *threads = malloc((count - 1) * sizeof(*threads));
    Task *tasks = malloc(count * sizeof(*tasks));
    if ((count > 1 && threads == NULL) || tasks == NULL) {
        free(threads);
        free(tasks);
        return -1;
    }
    for (size_t t = 0; t < count; t++) {
        tasks[t] = (Task){arrays, result, k, n,
                          n * t / count, n * (t + 1) / count};
    }
    size_t started = 0;
    for (size_t t = 1; t < count; t++) {
        if (pthread_create(&threads[t - 1], NULL, sum_part, &tasks[t]) != 0) {
            for (size_t i = 0; i < started; i++) {
                pthread_join(threads[i], NULL);
            }
            free(threads);
            free(tasks);
            return -1;
        }
        started++;
    }

    sum_part(&tasks[0]);
    for (size_t i = 0; i < started; i++) {
        pthread_join(threads[i], NULL);
    }
    free(threads);
    free(tasks);
    return 0;
}

static double seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

static double measure(const int *arrays, long long *result,
                      size_t k, size_t n, size_t count) {
    double total = 0;
    for (int i = 0; i < REPEATS; i++) {
        double start = seconds();
        if (sum_arrays(arrays, result, k, n, count) != 0) return -1;
        total += seconds() - start;
    }
    return total / REPEATS;
}

static int read_size(const char *text, size_t *value) {
    char *end;
    errno = 0;
    unsigned long long number = strtoull(text, &end, 10);
    if (errno || *text == '-' || end == text || *end != '\0' ||
        number == 0 || (uintmax_t)number > SIZE_MAX) return 0;
    *value = (size_t)number;
    return 1;
}

int main(int argc, char *argv[]) {
    size_t k, n, limit;
    if (argc != 7 || strcmp(argv[1], "--arrays") != 0 ||
        strcmp(argv[3], "--length") != 0 ||
        strcmp(argv[5], "--threads") != 0 ||
        !read_size(argv[2], &k) || !read_size(argv[4], &n) ||
        !read_size(argv[6], &limit)) {
        fprintf(stderr, "Usage: %s --arrays K --length N --threads T\n", argv[0]);
        return 1;
    }
    if (k > SIZE_MAX / n || k * n > SIZE_MAX / sizeof(int) ||
        n > SIZE_MAX / sizeof(long long)) {
        fprintf(stderr, "Array sizes are too large\n");
        return 1;
    }
    int *arrays = malloc(k * n * sizeof(*arrays));
    long long *single = malloc(n * sizeof(*single));
    long long *parallel = malloc(n * sizeof(*parallel));
    if (!arrays || !single || !parallel) {
        fprintf(stderr, "Not enough memory\n");
        free(arrays);
        free(single);
        free(parallel);
        return 1;
    }
    for (size_t i = 0; i < k * n; i++) {
        arrays[i] = (int)(i % 201) - 100;
    }
    size_t count = choose_threads(k, n, limit);
    double t1 = measure(arrays, single, k, n, 1);
    double tp = measure(arrays, parallel, k, n, count);
    if (t1 <= 0 || tp <= 0 ||
        memcmp(single, parallel, n * sizeof(*single)) != 0) {
        fprintf(stderr, "Calculation error\n");
        free(arrays);
        free(single);
        free(parallel);
        return 1;
    }
    printf("Arrays: %zu, length: %zu\n", k, n);
    printf("Thread limit: %zu, used: %zu\n", limit, count);
    printf("Time with 1 thread: %.6f s\n", t1);
    printf("Time with %zu threads: %.6f s\n", count, tp);
    if (count == 1) {
        printf("Speedup: 1.000, efficiency: 100.00%%\n");
    } else if (t1 < 0.0001 || tp < 0.0001) {
        printf("Times are too small for reliable speedup measurement\n");
    } else {
        double speedup = t1 / tp;
        printf("Speedup: %.3f, efficiency: %.2f%%\n", speedup,
               100.0 * speedup / count);
    }
    printf("First result elements:");
    for (size_t i = 0; i < n && i < 10; i++) {
        printf(" %lld", parallel[i]);
    }
    putchar('\n');
    free(arrays);
    free(single);
    free(parallel);
    return 0;
}
