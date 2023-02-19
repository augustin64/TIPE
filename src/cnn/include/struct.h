#ifndef DEF_STRUCT_H
#define DEF_STRUCT_H

typedef struct Kernel_cnn {
    int k_size; // k_size = dim_input - dim_output + 1
    int rows; // Depth of the input
    int columns; // Depth of the output
    float*** bias; // bias[columns][dim_output][dim_output]
    float*** d_bias; // d_bias[columns][dim_output][dim_output]
    float**** w; // w[rows][columns][k_size][k_size]
    float**** d_w; // d_w[rows][columns][k_size][k_size]
} Kernel_cnn;

typedef struct Kernel_nn {
    int size_input; // Nombre d'éléments en entrée
    int size_output; // Nombre d'éléments en sortie
    float* bias; // bias[size_output]
    float* d_bias; // d_bias[size_output]
    float** weights; // weight[size_input][size_output]
    float** d_weights; // d_weights[size_input][size_output]
} Kernel_nn;

typedef struct Kernel {
    Kernel_cnn* cnn; // NULL si ce n'est pas un cnn
    Kernel_nn* nn; // NULL si ce n'est pas un nn
    int activation; // Vaut l'identifiant de la fonction d'activation
    int linearisation; // Vaut 1 si c'est la linéarisation d'une couche, 0 sinon
    int pooling; // 0 si pas pooling, 1 si average_pooling, 2 si max_pooling
} Kernel;


typedef struct Network{
    int dropout; // Contient la probabilité d'abandon d'un neurone dans [0, 100] (entiers)
    float learning_rate; // Taux d'apprentissage du réseau
    int initialisation; // Contient le type d'initialisation
    int max_size; // Taille du tableau contenant le réseau
    int size; // Taille actuelle du réseau (size ≤ max_size)
    int* width; // width[size]
    int* depth; // depth[size]
    Kernel** kernel; // kernel[size], contient tous les kernels
    float**** input; // Tableau de toutes les couches du réseau input[size][couche->depth][couche->width][couche->width]
    float**** input_z; // Même tableau que input mais ne contient pas la dernière fonction d'activation à chaque ligne
} Network;

#endif