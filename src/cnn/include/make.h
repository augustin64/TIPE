#include "struct.h"

#ifndef DEF_MAKE_H
#define DEF_MAKE_H


/*
* Effectue la propagation d'une convolution avec stride et padding choisis sur le processeur
*/
void make_convolution_cpu(Kernel_cnn* kernel, float*** input, float*** output, int output_width, int stride, int padding);

/*
* Effectue la propagation d'une convolution avec stride et padding choisis sur le CPU ou GPU
*/
void make_convolution(Kernel_cnn* kernel, float*** input, float*** output, int output_width, int stride, int padding);

#ifdef __CUDACC__
extern "C"
#endif
/*
* Effectue propagation d'average pooling avec stride et padding choisis
*/
void make_average_pooling(float*** input, float*** output, int size, int output_depth, int output_width, int stride, int padding);

#ifdef __CUDACC__
extern "C"
#endif
/*
* Effectue propagation de max pooling avec stride et padding choisis
*/
void make_max_pooling(float*** input, float*** output, int size, int output_depth, int output_width, int stride, int padding);

#ifdef __CUDACC__
extern "C"
#endif
/*
* Effectue la propagation d'une couche dense
*/
void make_dense(Kernel_nn* kernel, float* input, float* output, int size_input, int size_output);

#ifdef __CUDACC__
extern "C"
#endif
/*
* Effectue la propagation d'une couche dense qui passe d'une matrice à un vecteur
*/
void make_dense_linearized(Kernel_nn* kernel, float*** input, float* output, int input_depth, int input_width, int size_output);

#endif