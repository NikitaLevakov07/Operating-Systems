#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

#define N_POINTS 1000000
#define MAX_ITER 100
#define EPSILON 0.001

typedef struct {
    double x, y;
    int cluster;
} Point;

typedef struct {
    Point *points;
    int start, end;
    double *centre_x;
    double *centre_y;
    int N_CLUSTERS;
    double *sum_x;
    double *sum_y;
    int *count;
    pthread_mutex_t *mutex;
} ThreadArgs;

double distance(Point p, double centre_x, double centre_y) {
    return sqrt((p.x - centre_x) * (p.x - centre_x) + (p.y - centre_y) * (p.y - centre_y));
}

void *assign_task(void * _args) {
    ThreadArgs *args = (ThreadArgs *)_args;

    for (int i = args->start; i < args->end; i++) {
        double min_distance = distance(args->points[i], args->centre_x[0], args->centre_y[0]);
        int best = 0;

        for (int k = 1; k < args->N_CLUSTERS; k++) {
            double temp_dist = distance(args->points[i], args->centre_x[k], args->centre_y[k]);
            if (temp_dist < min_distance) {
                min_distance = temp_dist;
                best = k;
            }
        }
        args->points[i].cluster = best;
    }
    return NULL;
}

void *update_task(void *_args) {
    ThreadArgs *args = (ThreadArgs *)_args;
    double *local_sum_x = calloc(args->N_CLUSTERS, sizeof(double));
    double *local_sum_y = calloc(args->N_CLUSTERS, sizeof(double));
    int *local_count = calloc(args->N_CLUSTERS, sizeof(int));

    for (int i = args->start; i < args->end; i++) {
        int k = args->points[i].cluster;
        local_sum_x[k] += args->points[i].x;
        local_sum_y[k] += args->points[i].y;
        local_count[k]++;
    }

    pthread_mutex_lock(args->mutex);
    for (int k = 0; k < args->N_CLUSTERS; k++) {
        args->sum_x[k] += local_sum_x[k];
        args->sum_y[k] += local_sum_y[k];
        args->count[k] += local_count[k];
    }
    pthread_mutex_unlock(args->mutex);

    free(local_sum_x);
    free(local_sum_y);
    free(local_count);
    return NULL;
}

int main(int argc, char *argv[]) {
    int N_THREADS = 4;
    int N_CLUSTERS = 5;

    if (argc > 1) 
        N_THREADS = atoi(argv[1]);
    
    if (argc > 2)
        N_CLUSTERS = atoi(argv[2]);

    if (N_THREADS < 1)
        N_THREADS = 1;
    
    if (N_CLUSTERS < 1)
        N_CLUSTERS = 1;

    if (N_CLUSTERS > N_POINTS)
        N_CLUSTERS = N_POINTS;

    srand(time(NULL));
    Point *points = malloc(N_POINTS * sizeof(Point));

    for (int i = 0; i < N_POINTS; i++) {
        points[i].x = (double)rand() / RAND_MAX * 100.0;
        points[i].y = (double)rand() / RAND_MAX * 100.0;
        points[i].cluster = -1;
    }

    double *sum_x = calloc(N_CLUSTERS, sizeof(double));
    double *sum_y = calloc(N_CLUSTERS, sizeof(double));
    int *count = calloc(N_CLUSTERS, sizeof(int));

    double *centre_x = malloc(N_CLUSTERS * sizeof(double));
    double *centre_y = malloc(N_CLUSTERS * sizeof(double));
    for (int k = 0; k < N_CLUSTERS; k++) {
    centre_x[k] = points[k].x;
    centre_y[k] = points[k].y;
    }

    pthread_t *threads = malloc(N_THREADS * sizeof(pthread_t));
    ThreadArgs *args = malloc(N_THREADS * sizeof(ThreadArgs));
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    int chunk = N_POINTS / N_THREADS;
    if (chunk == 0) { chunk = 1; }

    for (int t = 0; t < N_THREADS; t++) {
        args[t].points = points;
        args[t].centre_x = centre_x;
        args[t].centre_y = centre_y;
        args[t].N_CLUSTERS = N_CLUSTERS;
        args[t].sum_x = sum_x;
        args[t].sum_y = sum_y;
        args[t].count = count;
        args[t].mutex = &mutex;

        args[t].start = t * chunk;
        args[t].end = ( t == N_THREADS - 1 ) ? N_POINTS : (t + 1) * chunk;

        if (args[t].start > N_POINTS)
            args[t].start = N_POINTS;
        
        if (args[t].end > N_POINTS)
            args[t].end = N_POINTS;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int iter = 0; iter < MAX_ITER; iter++) {
        for (int t = 0; t < N_THREADS; t++)
            pthread_create(&threads[t], NULL, assign_task, &args[t]);

        for (int t = 0; t < N_THREADS; t++)
            pthread_join(threads[t], NULL);

        for (int k = 0; k < N_CLUSTERS; k++) {
            sum_x[k] = 0;
            sum_y[k] = 0;
            count[k] = 0;
        }

        for (int t = 0; t < N_THREADS; t++) 
            pthread_create(&threads[t], NULL, update_task, &args[t]);
        
        for (int t = 0; t < N_THREADS; t++)
            pthread_join(threads[t], NULL);

        double max_shift = 0.0;
        for (int k = 0; k < N_CLUSTERS; k++) {
            if (count[k] > 0) {
                double new_centre_x = sum_x[k] / count[k];
                double new_centre_y = sum_y[k] / count[k];
                double shift = sqrt((new_centre_x - centre_x[k]) * (new_centre_x - centre_x[k]) + 
                (new_centre_y - centre_y[k]) * (new_centre_y - centre_y[k]));

                if (shift > max_shift)
                    max_shift = shift;
                
                centre_x[k] = new_centre_x;
                centre_y[k] = new_centre_y;
            }
        }

        if (max_shift < EPSILON) {
            printf("Сошлось на итерации: %d\n", iter + 1);
            break;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1e6;
    printf("Параллельная версия. Потоков - %d, кластеров - %d. Время работы: %.2f мс\n", N_THREADS, N_CLUSTERS, time_ms);

    int *cluster_size = calloc(N_CLUSTERS, sizeof(int));
    for (int i = 0; i < N_POINTS; i++)
        cluster_size[points[i].cluster]++;

    printf("Размеры кластеров:\n");
    for (int k = 0; k < N_CLUSTERS; k++)
        printf("Кластер %d: %d точек\n", k, cluster_size[k]);

    free(cluster_size);
    free(points);
    free(centre_x);
    free(centre_y);
    free(sum_x);
    free(sum_y);
    free(count);
    free(threads);
    free(args);
    
    return 0;
}