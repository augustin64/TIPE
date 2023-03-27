#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <omp.h>

#include "../src/include/memory_management.h"
#include "../src/cnn/include/convolution.h"
#include "../src/cnn/include/struct.h"
#include "../src/include/colors.h"
#include "../src/include/utils.h"


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
            gree(matrix[i][j]);
        }
        gree(matrix[i]);
    }
    gree(matrix);
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

void run_convolution_test(int input_dim, int output_dim, int rows, int columns) {
    assert(input_dim >= output_dim);
    int k_size = input_dim - output_dim +1;

    // Génération des données aléatoires
    Kernel_cnn* kernel = (Kernel_cnn*)nalloc(1, sizeof(Kernel_cnn));
    
    kernel->k_size = k_size;
    kernel->rows = rows;
    kernel->columns = columns;

    // bias[kernel->columns][dim_output][dim_output]
    kernel->bias = create_matrix(kernel->columns, output_dim, output_dim, 15.0f);
    kernel->d_bias = create_matrix(kernel->columns, output_dim, output_dim, 1.5f);
    #ifdef ADAM_CNN_BIAS
    kernel->s_d_bias = create_matrix(kernel->columns, output_dim, output_dim, 1.5f);
    kernel->v_d_bias = create_matrix(kernel->columns, output_dim, output_dim, 1.5f);
    #endif

    // weights[rows][columns][k_size][k_size]
    kernel->weights = (float****)nalloc(kernel->rows, sizeof(float***));
    kernel->d_weights = (float****)nalloc(kernel->rows, sizeof(float***));
    #ifdef ADAM_CNN_WEIGHTS
    kernel->s_d_weights = (float****)nalloc(kernel->rows, sizeof(float***));
    kernel->v_d_weights = (float****)nalloc(kernel->rows, sizeof(float***));
    #endif
    for (int i=0; i < kernel->rows; i++) {
        kernel->weights[i] = create_matrix(kernel->columns, kernel->k_size, kernel->k_size, 15.0f);
        kernel->d_weights[i] = create_matrix(kernel->columns, kernel->k_size, kernel->k_size, 1.5f);
        #ifdef ADAM_CNN_WEIGHTS
        kernel->s_d_weights[i] = create_matrix(kernel->columns, kernel->k_size, kernel->k_size, 1.5f);
        kernel->v_d_weights[i] = create_matrix(kernel->columns, kernel->k_size, kernel->k_size, 1.5f);
        #endif
    }

    float*** input = create_matrix(kernel->rows, input_dim, input_dim, 5.0f);
    float*** output_cpu = create_empty_matrix(kernel->columns, output_dim, output_dim);
    float*** output_gpu = create_empty_matrix(kernel->columns, output_dim, output_dim);

    printf("(%d, %d, %d, %d) Data generation complete\n", rows, columns, input_dim, output_dim);


    // Lancement des calculs
    double start_time, end_time;
    double cpu_time_used, gpu_time_used;

    start_time = omp_get_wtime();
    make_convolution_device(kernel, input, output_gpu, output_dim);
    end_time = omp_get_wtime();


    gpu_time_used = end_time - start_time;
    printf("(%d, %d, %d, %d) Time used for GPU: %lf seconds\n", rows, columns, input_dim, output_dim, gpu_time_used);


    start_time = omp_get_wtime();
    make_convolution_cpu(kernel, input, output_cpu, output_dim);
    end_time = omp_get_wtime();

    cpu_time_used = end_time - start_time;
    printf("(%d, %d, %d, %d) Time used for CPU: %lf seconds\n", rows, columns, input_dim, output_dim, cpu_time_used);    

    // Vérification de l'égalité des matrices
    printf("(%d, %d, %d, %d) Checking equality.\n", rows, columns, input_dim, output_dim);
    if (!check_matrices_equality(output_gpu, output_cpu, kernel->columns, output_dim, output_dim, kernel->k_size)) {// TODO: change acceptation
        exit(1);
    }
    printf(GREEN "OK\n" RESET);

    free_matrix(kernel->bias, kernel->columns, output_dim);
    free_matrix(kernel->d_bias, kernel->columns, output_dim);
    #ifdef ADAM_CNN_BIAS
    free_matrix(kernel->s_d_bias, kernel->columns, output_dim);
    free_matrix(kernel->v_d_bias, kernel->columns, output_dim);
    #endif

    for (int i=0; i < kernel->rows; i++) {
        free_matrix(kernel->weights[i], kernel->columns, kernel->k_size);
        free_matrix(kernel->d_weights[i], kernel->columns, kernel->k_size);
        #ifdef ADAM_CNN_WEIGHTS
        free_matrix(kernel->s_d_weights[i], kernel->columns, kernel->k_size);
        free_matrix(kernel->v_d_weights[i], kernel->columns, kernel->k_size);
        #endif
    }
    gree(kernel->weights);
    gree(kernel->d_weights);
    #ifdef ADAM_CNN_WEIGHTS
    gree(kernel->s_d_weights);
    gree(kernel->v_d_weights);
    #endif

    free_matrix(input, kernel->rows, input_dim);
    free_matrix(output_cpu, kernel->columns, output_dim);
    free_matrix(output_gpu, kernel->columns, output_dim);
}


int main() {
    printf("Checking CUDA compatibility.\n");
    bool cuda_compatible = check_cuda_compatibility();
    if (!cuda_compatible) {
        printf(RED "CUDA not compatible, skipping tests.\n" RESET);
        return 0;
    }
    printf(GREEN "OK\n" RESET);
    
    srand(time(NULL));

    run_convolution_test(20, 15, 30, 40);
    run_convolution_test(30, 25, 40, 50);
    run_convolution_test(250, 200, 3, 3);
    
    return 0;
}