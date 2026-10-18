#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define N_POINTS 1000000
#define MAX_ITER 100
#define EPSILON 0.001

typedef struct {
    double x, y;
    int cluster;
} Point;

double distance(Point p, double centre_x, double centre_y) {
    return sqrt((p.x - centre_x) * (p.x - centre_x) + (p.y - centre_y) * (p.y - centre_y));
}

int main(int argc, char *argv[]) {
    int N_CLUSTERS = 5;

    if (argc > 1) {
        N_CLUSTERS = atoi(argv[1]);
        if (N_CLUSTERS < 1)
            N_CLUSTERS = 1;
        if (N_CLUSTERS > N_POINTS)
            N_CLUSTERS = N_POINTS;
    }

    srand(time(NULL));
    Point *points = malloc(N_POINTS * sizeof(Point));

    for (int i = 0; i < N_POINTS; i++) {
        points[i].x = (double)rand() / RAND_MAX * 100.0;
        points[i].y = (double)rand() / RAND_MAX * 100.0;
        points[i].cluster = -1;
    }

    double *centre_x = malloc(N_CLUSTERS * sizeof(double));
    double *centre_y = malloc(N_CLUSTERS * sizeof(double));
    for (int k = 0; k < N_CLUSTERS; k++) {
        centre_x[k] = points[k].x;
        centre_y[k] = points[k].y;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    double *sum_x = calloc(N_CLUSTERS, sizeof(double));
    double *sum_y = calloc(N_CLUSTERS, sizeof(double));
    int *count = calloc(N_CLUSTERS, sizeof(int));

    for (int iter = 0; iter < MAX_ITER; iter++) {
        for (int i = 0; i < N_POINTS; i++) {
            double min_distance = distance(points[i], centre_x[0], centre_y[0]);
            int best = 0;
            for(int k = 1; k < N_CLUSTERS; k++) {
                double temp_dist = distance(points[i], centre_x[k], centre_y[k]);
                if (temp_dist < min_distance) {
                    min_distance = temp_dist;
                    best = k;
                }
            }
            points[i].cluster = best;
        }
    
        for (int k= 0; k < N_CLUSTERS; k++) {
            sum_x[k] = 0;
            sum_y[k] = 0;
            count[k] = 0;
        }
 
        for (int i = 0; i < N_POINTS; i++) {
            int k = points[i].cluster;
            sum_x[k] += points[i].x;
            sum_y[k] += points[i].y;
            count[k]++;
        }

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

    free(sum_x);
    free(sum_y);
    free(count);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1e6;
    printf("Время: %.2f мс\n", time_ms);

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
    return 0;
}