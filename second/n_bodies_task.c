#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <pthread.h>
#include "barrier.h"

#define G 6.67430e-11
#define TIME_STEP 0.01

static int num_bodies;
static int num_threads;
static int t_end;
static double* m;
static double* vx;
static double* vy;
static double* rx;
static double* ry;

static double* fx_loc;
static double* fy_loc;
static my_barrier_t barrier;
static FILE* out;

void initialize_bodies(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    fscanf(file, "%d", &num_bodies);
    m  = malloc(num_bodies * sizeof(double));
    vx = malloc(num_bodies * sizeof(double));
    vy = malloc(num_bodies * sizeof(double));
    rx = malloc(num_bodies * sizeof(double));
    ry = malloc(num_bodies * sizeof(double));

    for (int i = 0; i < num_bodies; i++) {
        double rz, vz;
        fscanf(file, "%lf %lf %lf %lf %lf %lf %lf",
               &m[i], &rx[i], &ry[i], &rz, &vx[i], &vy[i], &vz);
    }
    fclose(file);
}

static void write_row(double t) {
    fprintf(out, "%g", t);
    for (int i = 0; i < num_bodies; i++)
        fprintf(out, " %g %g", rx[i], ry[i]);
    fprintf(out, "\n");
}

static void* worker(void* arg) {
    int tid = (int)(intptr_t)arg;
    int n = num_bodies;
    int T = num_threads;

    double* fx = fx_loc + (size_t)tid * n;
    double* fy = fy_loc + (size_t)tid * n;

    long steps = t_end / TIME_STEP;
    long logging_interval = steps / 10;

    for (long s = 0; s < steps; s++) {

        memset(fx, 0, n * sizeof(double));
        memset(fy, 0, n * sizeof(double));

        for (int q = tid; q < n; q += T) {
            for (int k = q + 1; k < n; k++) {
                double dx = rx[k] - rx[q];
                double dy = ry[k] - ry[q];
                double r2 = dx * dx + dy * dy;
                double r3 = r2 * sqrt(r2);
                double c  = G * m[q] * m[k] / r3;

                fx[q] += c * dx;  fy[q] += c * dy;
                fx[k] -= c * dx;  fy[k] -= c * dy;
            }
        }

        barrier_wait(&barrier);

        for (int q = tid; q < n; q += T) {
            double Fx = 0.0, Fy = 0.0;
            for (int t = 0; t < T; t++) {
                Fx += fx_loc[(size_t)t * n + q];
                Fy += fy_loc[(size_t)t * n + q];
            }
            rx[q] += vx[q] * TIME_STEP;
            ry[q] += vy[q] * TIME_STEP;
            vx[q] += Fx / m[q] * TIME_STEP;
            vy[q] += Fy / m[q] * TIME_STEP;
        }

        barrier_wait(&barrier);

        if (tid == 0 && (s + 1) % logging_interval == 0)
            write_row((s + 1) * (double)TIME_STEP);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s tend input_file nthreads\n", argv[0]);
        return EXIT_FAILURE;
    }

    t_end = atoi(argv[1]);
    initialize_bodies(argv[2]);
    num_threads = atoi(argv[3]);

    fx_loc = malloc((size_t)num_threads * num_bodies * sizeof(double));
    fy_loc = malloc((size_t)num_threads * num_bodies * sizeof(double));
    barrier_init(&barrier, num_threads);

    out = fopen("output.csv", "w");
    if (!out) { perror("output.csv"); return EXIT_FAILURE; }
    write_row(0.0);

    pthread_t* th = malloc(num_threads * sizeof(pthread_t));
    for (int i = 0; i < num_threads; i++)
        pthread_create(&th[i], NULL, worker, (void*)(intptr_t)i);
    for (int i = 0; i < num_threads; i++)
        pthread_join(th[i], NULL);

    fclose(out);
    barrier_destroy(&barrier);
    free(th); free(fx_loc); free(fy_loc);
    free(m); free(vx); free(vy); free(rx); free(ry);
    return 0;
}