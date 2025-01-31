#include "function.h"
#include "struct.h"

#ifndef DEF_BACKPROPAGATION_H
#define DEF_BACKPROPAGATION_H


#ifdef __CUDACC__
extern "C"
#endif
/*
* Transfert les informations d'erreur de la sortie voulue à la sortie réelle
*/
void softmax_backward_mse(float* input, float* output, int size);


#ifdef __CUDACC__
extern "C"
#endif
/*
* Transfert les informations d'erreur de la sortie voulue à la sortie réelle
* en considérant MSE (Mean Squared Error) comme fonction d'erreur
*/
void softmax_backward_cross_entropy(float* input, float* output, int size);


#ifdef __CUDACC__
extern "C"
#endif
/*
* Transfert les informations d'erreur à travers une couche d'average pooling
* en considérant cross_entropy comme fonction d'erreur
*/
void backward_average_pooling(float*** input, float*** output, int input_width, int output_width, int depth, int kernel_size, int stride, int padding);


#ifdef __CUDACC__
extern "C"
#endif
/*
* Transfert les informations d'erreur à travers une couche de max pooling
* en considérant cross_entropy comme fonction d'erreur
*/
void backward_max_pooling(float*** input, float*** output, int input_width, int output_width, int depth, int kernel_size, int stride, int padding);


#ifdef __CUDACC__
extern "C"
#endif
/*
* Transfert les informations d'erreur à travers une couche fully connected
*/
void backward_dense(Kernel_nn* ker, D_Kernel_nn* d_ker, float* input, float* input_z, float* output, int size_input, int size_output, int activation, int is_first);


#ifdef __CUDACC__
extern "C"
#endif
/*
* Transfert les informations d'erreur à travers une couche de linéarisation
*/
void backward_linearisation(Kernel_nn* ker, D_Kernel_nn* d_ker, float*** input, float*** input_z, float* output, int input_depth, int input_width, int size_output, int activation);


#ifdef __CUDACC__
extern "C"
#endif
/*
* Transfert les informations d'erreur à travers un couche de convolution
*/
void backward_convolution(Kernel_cnn* ker, D_Kernel_cnn* d_ker, float*** input, float*** input_z, float*** output, int input_depth, int input_width, int output_depth, int output_width, int activation, int is_first, int kernel_size, int padding, int stride);

#endif
