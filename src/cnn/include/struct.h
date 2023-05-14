#ifndef DEF_STRUCT_H
#define DEF_STRUCT_H

#include "config.h"

#define NO_POOLING 0
#define AVG_POOLING 1
#define MAX_POOLING 2

#define DOESNT_LINEARISE 0
#define DO_LINEARISE 1

typedef struct Kernel_cnn {
    // Noyau ayant une couche matricielle en sortie
    int k_size; // k_size = 2*padding + input_width + stride - output_width*stride
    int rows; // Depth de l'input
    int columns; // Depth de l'output

    float*** bias; // bias[columns][output_width][output_width] <=> bias[output depth][output width][output width]
    float*** d_bias; // d_bias[columns][output_width][output_width]
    #ifdef ADAM_CNN_BIAS
        float*** s_d_bias; // s_d_bias[columns][output_width][output_width]
        float*** v_d_bias; // v_d_bias[columns][output_width][output_width]
    #endif

    float**** weights; // weights[rows][columns][k_size][k_size] <=> weights[input depth][output depth][kernel size][kernel size]
    float**** d_weights; // d_weights[rows][columns][k_size][k_size]
    #ifdef ADAM_CNN_WEIGHTS
        float**** s_d_weights; // s_d_weights[rows][columns][k_size][k_size]
        float**** v_d_weights; // v_d_weights[rows][columns][k_size][k_size]
    #endif
} Kernel_cnn;

typedef struct Kernel_nn {
    // Noyau ayant une couche vectorielle en sortie
    int size_input; // Nombre d'éléments en entrée
    int size_output; // Nombre d'éléments en sortie

    float* bias; // bias[size_output]
    float* d_bias; // d_bias[size_output]
    #ifdef ADAM_DENSE_BIAS
        float* s_d_bias; // s_d_bias[size_output]
        float* v_d_bias; // v_d_bias[size_output]
    #endif

    float** weights; // weight[size_input][size_output]
    float** d_weights; // d_weights[size_input][size_output]
    #ifdef ADAM_DENSE_WEIGHTS
        float** s_d_weights; // s_d_weights[size_input][size_output]
        float** v_d_weights; // v_d_weights[size_input][size_output]
    #endif
} Kernel_nn;

typedef struct Kernel {
    Kernel_cnn* cnn; // NULL si ce n'est pas un cnn
    Kernel_nn* nn; // NULL si ce n'est pas un nn

    int activation; // Id de la fonction d'activation et -Id de sa dérivée
    int linearisation; // 1 si c'est la linéarisation d'une couche, 0 sinon
    int pooling; // 0 si pas pooling, 1 si average_pooling, 2 si max_pooling
    int stride; // Valable uniquement une pooling et un cnn
    int padding; // Valable uniquement une pooling et un cnn
} Kernel;


typedef struct Network{
    int dropout; // Probabilité d'abandon d'un neurone dans [0, 100] (entiers)
    float learning_rate; // Taux d'apprentissage du réseau
    int initialisation; // Id du type d'initialisation

    int max_size; // Taille du tableau contenant le réseau
    int size; // Taille actuelle du réseau (size ≤ max_size)

    int* width; // width[size]
    int* depth; // depth[size]
    
    Kernel** kernel; // kernel[size], contient tous les kernels
    float**** input_z; // Tableau de toutes les couches du réseau input_z[size][couche->depth][couche->width][couche->width]
    float**** input; // input[i] = f(input_z[i]) où f est la fonction d'activation de la couche i
} Network;

#endif