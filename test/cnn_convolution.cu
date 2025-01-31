#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <time.h>

#include "include/test.h"

#include "../src/common/include/memory_management.h"
#include "../src/cnn/include/convolution.h"
#include "../src/common/include/colors.h"
#include "../src/common/include/utils.h"
#include "../src/cnn/include/struct.h"


float random_float(float low, float high) {
    float t = (float)rand() / (float)RAND_MAX;
    return (1.0f - t) * low + t * high;
}


void fill_matrix_random(float ***matrix, int n, int p, int q, float max_val) {
    for (int i=0; i < n; i++) {
        for (int j=0; j < p; j++) {
            for (int k=0; k < q; k++) {
                matrix[i][j][k] = random_float(0.0f, max_val);
            }
        }
    }
}


void print_matrix(float** mat, int n, int p) {
    for (int i=0; i < n; i++) {
        printf("[\t");
        for (int j=0; j < p; j++) {
            printf("%0.1f\t", mat[i][j]);
        }
        printf("]\n");
    }
}


float*** create_matrix(int n, int p, int q, float max_val) {
    float*** matrix = (float***)nalloc(n, sizeof(float**));
    for (int i=0; i < n; i++) {
        matrix[i] = (float**)nalloc(p, sizeof(float*));
        for (int j=0; j < p; j++) {
            matrix[i][j] = (float*)nalloc(q, sizeof(float));
        }
    }

    fill_matrix_random(matrix, n, p, q, max_val);
    return matrix;
}


float*** create_empty_matrix(int n, int p, int q) {
    float*** matrix = (float***)nalloc(n, sizeof(float**));
    for (int i=0; i < n; i++) {
        matrix[i] = (float**)nalloc(p, sizeof(float*));
        for (int j=0; j < p; j++) {
            matrix[i][j] = (float*)nalloc(q, sizeof(float));
            for (int k=0; k < q; k++) {
                matrix[i][j][k] = 0.;
            }
        }
    }
    return matrix;
}

void free_matrix(float*** matrix, int n, int p) {
    for (int i=0; i < n; i++) {
        for (int j=0; j < p; j++) {
            gree(matrix[i][j], false);
        }
        gree(matrix[i], false);
    }
    gree(matrix, false);
}

bool check_matrices_equality(float*** m1, float*** m2, int n, int p, int q, int acceptation) {
    for (int i=0; i < n; i++) {
        for (int j=0; j < p; j++) {
            for (int k=0; k < q; k++) {
                if (fabs(m1[i][j][k] - m2[i][j][k]) > 0.01*acceptation) {
                    printf(RED "diff %d %d %d: %f val: %f et %f\n" RESET, i, j, k, fabs(m1[i][j][k] - m2[i][j][k]), m1[i][j][k], m2[i][j][k]);
                    return false;
                }
            }
        }
    }
    return true;
}

bool run_convolution_test(int input_width, int output_width, int rows, int columns) {
    assert(input_width >= output_width);
    int k_size = input_width - output_width +1;

    // Génération des données aléatoires
    Kernel_cnn* kernel = (Kernel_cnn*)nalloc(1, sizeof(Kernel_cnn));
    
    kernel->k_size = k_size;
    kernel->rows = rows;
    kernel->columns = columns;

    // bias[kernel->columns][output_width][output_width]
    kernel->bias = create_matrix(kernel->columns, output_width, output_width, 15.0f);

    // weights[rows][columns][k_size][k_size]
    kernel->weights = (float****)nalloc(kernel->rows, sizeof(float***));
    for (int i=0; i < kernel->rows; i++) {
        kernel->weights[i] = create_matrix(kernel->columns, kernel->k_size, kernel->k_size, 15.0f);
    }

    float*** input = create_matrix(kernel->rows, input_width, input_width, 5.0f);
    float*** output_cpu = create_empty_matrix(kernel->columns, output_width, output_width);
    float*** output_gpu = create_empty_matrix(kernel->columns, output_width, output_width);

    printf("(%d, %d, %d, %d) Data generation complete\n", rows, columns, input_width, output_width);


    // Lancement des calculs
    clock_t start_time, end_time;
    clock_t cpu_time_used, gpu_time_used;

    start_time = clock();
    make_convolution_device(kernel, input, output_gpu, output_width, 1, 0);
    end_time = clock();


    gpu_time_used = end_time - start_time;
    printf("(%d, %d, %d, %d) Time used for GPU: %lf seconds\n", rows, columns, input_width, output_width, gpu_time_used);


    start_time = clock();
    make_convolution_cpu(kernel, input, output_cpu, output_width, 1, 0);
    end_time = clock();

    cpu_time_used = end_time - start_time;
    printf("(%d, %d, %d, %d) Time used for CPU: %lf seconds\n", rows, columns, input_width, output_width, cpu_time_used);    

    // Vérification de l'égalité des matrices
    printf("(%d, %d, %d, %d) Checking equality.\n", rows, columns, input_width, output_width);
    bool valid = check_matrices_equality(output_gpu, output_cpu, kernel->columns, output_width, output_width, kernel->k_size);

    free_matrix(kernel->bias, kernel->columns, output_width);

    for (int i=0; i < kernel->rows; i++) {
        free_matrix(kernel->weights[i], kernel->columns, kernel->k_size);
    }
    gree(kernel->weights, false);

    free_matrix(input, kernel->rows, input_width);
    free_matrix(output_cpu, kernel->columns, output_width);
    free_matrix(output_gpu, kernel->columns, output_width);

    return valid;
}


int main() {
    _TEST_PRESENTATION("Cuda Convolution");

    printf("Checking CUDA compatibility.\n");
    bool cuda_compatible = cuda_setup(true);
    if (!cuda_compatible) {
        printf(RED "CUDA not compatible, skipping tests.\n" RESET);
        return 0;
    }
    printf(GREEN "OK\n" RESET);
    
    srand(clock());

    _TEST_ASSERT(run_convolution_test(20, 15, 30, 40), "Small");
    _TEST_ASSERT(run_convolution_test(30, 25, 40, 50), "Medium");
    _TEST_ASSERT(run_convolution_test(250, 200, 3, 3), "Big");
    
    return 0;
}