#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h> // Is it used ?

#include "include/backpropagation.h"
#include "include/initialisation.h"
#include "include/function.h"
#include "include/creation.h"
#include "include/update.h"
#include "include/make.h"

#include "../include/colors.h"
#include "include/cnn.h"

// Augmente les dimensions de l'image d'entrée
#define PADDING_INPUT 2

int indice_max(float* tab, int n) {
    int indice = -1;
    float maxi = FLT_MIN;

    for (int i=0; i < n; i++) {
        if (tab[i] > maxi) {
            maxi = tab[i];
            indice = i;
        }
    }
    return indice;
}

int will_be_drop(int dropout_prob) {
    return (rand() % 100) < dropout_prob;
}

void write_image_in_network_32(int** image, int height, int width, float** input) {
    int padding = (32 - height)/2;
    for (int i=0; i < padding; i++) {
        for (int j=0; j < 32; j++) {
            input[i][j] = 0.;
            input[31-i][j] = 0.;
            input[j][i] = 0.;
            input[j][31-i] = 0.;
        }
    }

    for (int i=0; i < width; i++) {
        for (int j=0; j < height; j++) {
            input[i+2][j+2] = (float)image[i][j] / 255.0f;
        }
    }
}

void write_image_in_network_260(unsigned char* image, int height, int width, float*** input) {
    int size_input = 260;
    int padding = (size_input - height)/2;

    for (int i=0; i < padding; i++) {
        for (int j=0; j < size_input; j++) {
            for (int composante=0; composante < 3; composante++) {
                input[composante][i][j] = 0.;
                input[composante][size_input-1-i][j] = 0.;
                input[composante][j][i] = 0.;
                input[composante][j][size_input-1-i] = 0.;
            }
        }
    }

    for (int i=0; i < width; i++) {
        for (int j=0; j < height; j++) {
            for (int composante=0; composante < 3; composante++) {
                input[composante][i+2][j+2] = (float)image[(i*height+j)*3 + composante] / 255.0f;
            }
        }
    }
}

void forward_propagation(Network* network) {
    int activation, pooling, input_depth, input_width, output_depth, output_width;
    int n = network->size;
    float*** input;
    float*** output;
    float*** output_a;
    Kernel* k_i;
    for (int i=0; i < n-1; i++) {
        // Transférer les informations de 'input' à 'output'
        k_i = network->kernel[i];
        output_a = network->input_z[i+1];
        input = network->input[i];
        input_depth = network->depth[i];
        input_width = network->width[i];
        output = network->input[i+1];
        output_depth = network->depth[i+1];
        output_width = network->width[i+1];
        activation = k_i->activation;
        pooling = k_i->pooling;

        if (k_i->nn) {
            drop_neurones(input, 1, 1, input_width, network->dropout);
        } else {
            drop_neurones(input, input_depth, input_width, input_width, network->dropout);
        }

        if (k_i->cnn) { // Convolution
            make_convolution(k_i->cnn, input, output, output_width);
            copy_input_to_input_z(output, output_a, output_depth, output_width, output_width);
            choose_apply_function_matrix(activation, output, output_depth, output_width);
        }
        else if (k_i->nn) { // Full connection
            if (k_i->linearisation == 0) { // Vecteur -> Vecteur
                make_dense(k_i->nn, input[0][0], output[0][0], input_width, output_width);
            } else { // Matrice -> Vecteur
                make_dense_linearised(k_i->nn, input, output[0][0], input_depth, input_width, output_width);
            }
            copy_input_to_input_z(output, output_a, 1, 1, output_width);
            choose_apply_function_vector(activation, output, output_width);
        }
        else { // Pooling
            if (n-2==i) {
                printf("Le réseau ne peut pas finir par une pooling layer\n");
                return;
            } else { // Pooling sur une matrice
                if (pooling==1) {
                    make_average_pooling(input, output, activation, output_depth, output_width);
                } else if (pooling==2) {
                    make_max_pooling(input, output, activation, output_depth, output_width);
                } else {
                    printf("Erreur dans la reconnaissance de la couche de pooling: %d,%d \n", pooling, i);
                }
            }
            copy_input_to_input_z(output, output_a, output_depth, output_width, output_width);
        }
    }
}

void backward_propagation(Network* network, int wanted_number) {
    float* wanted_output = generate_wanted_output(wanted_number, network->width[network->size -1]);
    int n = network->size;
    int activation, input_depth, input_width, output_depth, output_width;
    float*** input;
    float*** input_z;
    float*** output;
    Kernel* k_i;

    softmax_backward(network->input[n-1][0][0], network->input_z[n-1][0][0], wanted_output, network->width[n-1]); // Backward sur la dernière colonne

    for (int i=n-2; i >= 0; i--) {
        // Modifie 'k_i' à partir d'une comparaison d'informations entre 'input' et 'output'
        k_i = network->kernel[i];
        input = network->input[i];
        input_z = network->input_z[i];
        input_depth = network->depth[i];
        input_width = network->width[i];
        output = network->input[i+1];
        output_depth = network->depth[i+1];
        output_width = network->width[i+1];
        activation = i==0?SIGMOID:network->kernel[i-1]->activation;


        if (k_i->cnn) { // Convolution
            ptr d_f = get_function_activation(-activation);
            backward_convolution(k_i->cnn, input, input_z, output, input_depth, input_width, output_depth, output_width, d_f, i==0);
        } else if (k_i->nn) { // Full connection
            ptr d_f = get_function_activation(-activation);
            if (k_i->linearisation == 0) { // Vecteur -> Vecteur
                backward_dense(k_i->nn, input[0][0], input_z[0][0], output[0][0], input_width, output_width, d_f, i==0);
            } else { // Matrice -> vecteur
                backward_linearisation(k_i->nn, input, input_z, output[0][0], input_depth, input_width, output_width, d_f);
            }
        } else { // Pooling
            backward_2d_pooling(input, output, input_width, output_width, input_depth); // Depth pour input et output a la même valeur
        }
    }
    free(wanted_output);
}

void drop_neurones(float*** input, int depth, int dim1, int dim2, int dropout) {
    for (int i=0; i<depth; i++) {
        for (int j=0; j<dim1; j++) {
            for (int k=0; k<dim2; k++) {
                if (will_be_drop(dropout))
                    input[i][j][k] = 0;
            }
        }
    }
}

void copy_input_to_input_z(float*** output, float*** output_a, int output_depth, int output_rows, int output_columns) {
    for (int i=0; i<output_depth; i++) {
        for (int j=0; j<output_rows; j++) {
            for (int k=0; k<output_columns; k++) {
                output_a[i][j][k] = output[i][j][k];
            }
        }
    }
}

float compute_mean_squared_error(float* output, float* wanted_output, int len) {
    if (len==0) {
        printf("Erreur MSE: la longueur de la sortie est de 0 -> division par 0 impossible\n");
        return 0.;
    }
    float loss=0.;
    for (int i=0; i < len ; i++) {
        loss += (output[i]-wanted_output[i])*(output[i]-wanted_output[i]);
    }
    return loss/len;
}

float compute_cross_entropy_loss(float* output, float* wanted_output, int len) {
    float loss=0.;
    for (int i=0; i < len ; i++) {
        if (wanted_output[i]==1) {
            if (output[i]==0.) {
                loss -= log(FLT_EPSILON);
            }
            else {
                loss -= log(output[i]);
            }
        }
    }
    return loss;
}

float* generate_wanted_output(int wanted_number, int size_output) {
    float* wanted_output = (float*)malloc(sizeof(float)*size_output);
    for (int i=0; i < size_output; i++) {
        if (i==wanted_number) {
            wanted_output[i]=1;
        }
        else {
            wanted_output[i]=0;
        }
    }
    return wanted_output;
}