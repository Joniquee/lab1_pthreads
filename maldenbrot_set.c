#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "my_rand.h"

#define MAX_ITERATIONS 1000
#define OUTPUT_FILE "mandelbrot_points.csv"

typedef struct {
    double x;
    double y;
} point_t;

typedef struct {
    int rank;
    unsigned seed;
} thread_arg_t;

static point_t *points;
static int points_wanted;
static int points_found;
static pthread_mutex_t points_mutex = PTHREAD_MUTEX_INITIALIZER;

static int is_in_mandelbrot(double x, double y) {
    double zx = 0.0;
    double zy = 0.0;
    int iteration;

    for (iteration = 0; iteration < MAX_ITERATIONS; iteration++) {
        double next_zx = zx * zx - zy * zy + x;
        zy = 2.0 * zx * zy + y;
        zx = next_zx;
        if (zx * zx + zy * zy > 4.0) {
            return 0;
        }
    }
    return 1;
}

static void *find_points(void *argument) {
    thread_arg_t *thread = (thread_arg_t *)argument;
    unsigned seed = thread->seed;

    for (;;) {
        double x;
        double y;

        pthread_mutex_lock(&points_mutex);
        if (points_found >= points_wanted) {
            pthread_mutex_unlock(&points_mutex);
            break;
        }
        pthread_mutex_unlock(&points_mutex);

        /* Sample x in [-2, 1] and y in [-1.5, 1.5]. */
        x = -2.0 + 3.0 * my_drand(&seed);
        y = -1.5 + 3.0 * my_drand(&seed);

        if (is_in_mandelbrot(x, y)) {
            pthread_mutex_lock(&points_mutex);
            if (points_found < points_wanted) {
                points[points_found].x = x;
                points[points_found].y = y;
                points_found++;
                printf("Thread %d found point %d: x=%.17g, y=%.17g\n",
                       thread->rank, points_found, x, y);
            }
            pthread_mutex_unlock(&points_mutex);
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int thread_count = atoi(argv[1]);
    points_wanted = atoi(argv[2]);
    int i;
    pthread_t *threads;
    thread_arg_t *thread_args;
    FILE *output;

    (void)argc;
    printf("Searching for %d Mandelbrot points using %d threads\n",
            points_wanted, thread_count);
    threads = malloc(thread_count * sizeof(*threads));
    thread_args = malloc(thread_count * sizeof(*thread_args));
    points = malloc(points_wanted * sizeof(*points));

    for (i = 0; i < thread_count; i++) {
        thread_args[i].rank = i;
        thread_args[i].seed = (unsigned)(i + 1);
        pthread_create(&threads[i], NULL, find_points, &thread_args[i]);
    }
    for (i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
    output = fopen(OUTPUT_FILE, "w");
    fprintf(output, "x,y\n");
    for (i = 0; i < points_found; i++) {
        fprintf(output, "%.17g,%.17g\n", points[i].x, points[i].y);
    }
    fclose(output);
    printf("Finished: %d points saved to %s\n", points_found, OUTPUT_FILE);

    free(threads);
    free(thread_args);
    free(points);
    pthread_mutex_destroy(&points_mutex);
    return 0;
}