#include <stdio.h>

#include "include/update.h"
#include "include/struct.h"

float clip(float a) {
    if (a > NETWORK_CLIP_VALUE) {
        return NETWORK_CLIP_VALUE;
    }
    if (a < -NETWORK_CLIP_VALUE) {
        return -NETWORK_CLIP_VALUE;
    }
    return a;
}

void update_weights(Network* network, Network* d_network) {
    int n = network->size;

    for (int i=0; i < (n-1); i++) {
        Kernel* k_i = network->kernel[i];
        Kernel* dk_i = d_network->kernel[i];

        int input_depth = network->depth[i];
        int input_width = network->width[i];

        int output_depth = network->depth[i+1];
        int output_width = network->width[i+1];

        if (k_i->cnn) { // Convolution
            Kernel_cnn* cnn = k_i->cnn;
            Kernel_cnn* d_cnn = dk_i->cnn;
            int k_size = cnn->k_size;
            for (int a=0; a < input_depth; a++) {
                for (int b=0; b < output_depth; b++) {
                    for (int c=0; c < k_size; c++) {
                        for (int d=0; d < k_size; d++) {
                            cnn->weights[a][b][c][d] -= network->learning_rate * d_cnn->d_weights[a][b][c][d];
                            d_cnn->d_weights[a][b][c][d] = 0;

                            cnn->weights[a][b][c][d] = clip(cnn->weights[a][b][c][d]);
                        }
                    }
                }
            }
        } else if (k_i->nn) { // Full connection
            if (k_i->linearisation == DOESNT_LINEARISE) { // Vecteur -> Vecteur
                Kernel_nn* nn = k_i->nn;
                Kernel_nn* d_nn = dk_i->nn;

                for (int a=0; a < input_width; a++) {
                    for (int b=0; b < output_width; b++) {
                        nn->weights[a][b] -= network->learning_rate * d_nn->d_weights[a][b];
                        d_nn->d_weights[a][b] = 0;
                    }
                }
            } else { // Matrice -> vecteur
                Kernel_nn* nn = k_i->nn;
                Kernel_nn* d_nn = dk_i->nn;

                int size_input = input_width*input_width*input_depth;

                for (int a=0; a < size_input; a++) {
                    for (int b=0; b < output_width; b++) {
                        nn->weights[a][b] -= network->learning_rate * d_nn->d_weights[a][b];
                        d_nn->d_weights[a][b] = 0;

                        nn->weights[a][b] = clip(nn->weights[a][b]);
                    }
                }
            }
        }
        // Une couche de pooling ne nécessite pas de traitement
    }
}

void update_bias(Network* network, Network* d_network) {
    int n = network->size;

    for (int i=0; i < (n-1); i++) {
        Kernel* k_i = network->kernel[i];
        Kernel* dk_i = d_network->kernel[i];
        int output_width = network->width[i+1];
        int output_depth = network->depth[i+1];

        if (k_i->cnn) { // Convolution
            Kernel_cnn* cnn = k_i->cnn;
            Kernel_cnn* d_cnn = dk_i->cnn;

            for (int a=0; a < output_depth; a++) {
                for (int b=0; b < output_width; b++) {
                    for (int c=0; c < output_width; c++) {
                        cnn->bias[a][b][c] -= network->learning_rate * d_cnn->d_bias[a][b][c];
                        d_cnn->d_bias[a][b][c] = 0;

                        cnn->bias[a][b][c] = clip(cnn->bias[a][b][c]);
                    }
                }
            }
        } else if (k_i->nn) { // Full connection
            Kernel_nn* nn = k_i->nn;
            Kernel_nn* d_nn = dk_i->nn;

            for (int a=0; a < output_width; a++) {
                nn->bias[a] -= network->learning_rate * d_nn->d_bias[a];
                d_nn->d_bias[a] = 0;

                nn->bias[a] = clip(nn->bias[a]);
            }
        }
        // Une couche de pooling ne nécessite pas de traitement
    }
}

void reset_d_weights(Network* network) {
    int n = network->size;

    for (int i=0; i < (n-1); i++) {
        Kernel* k_i = network->kernel[i];
        Kernel* k_i_1 = network->kernel[i+1];
        
        int input_depth = network->depth[i];
        int input_width = network->width[i];
        
        int output_depth = network->depth[i+1];
        int output_width = network->width[i+1];

        if (k_i->cnn) { // Convolution
            Kernel_cnn* cnn = k_i_1->cnn;

            int k_size = cnn->k_size;

            for (int a=0; a < input_depth; a++) {
                for (int b=0; b < output_depth; b++) {
                    for (int c=0; c < k_size; c++) {
                        for (int d=0; d < k_size; d++) {
                            cnn->d_weights[a][b][c][d] = 0;
                        }
                    }
                }
            }
        } else if (k_i->nn) { // Full connection
            if (k_i->linearisation == DOESNT_LINEARISE) { // Vecteur -> Vecteur
                Kernel_nn* nn = k_i_1->nn;

                for (int a=0; a < input_width; a++) {
                    for (int b=0; b < output_width; b++) {
                        nn->d_weights[a][b] = 0;
                    }
                }
            } else { // Matrice -> vecteur
                Kernel_nn* nn = k_i_1->nn;

                int size_input = input_width*input_width*input_depth;

                for (int a=0; a < size_input; a++) {
                    for (int b=0; b < output_width; b++) {
                        nn->d_weights[a][b] = 0;
                    }
                }
            }
        }
        // Une couche de pooling ne nécessite pas de traitement
    }
}

void reset_d_bias(Network* network) {
    int n = network->size;
    
    for (int i=0; i < (n-1); i++) {
        Kernel* k_i = network->kernel[i];
        Kernel* k_i_1 = network->kernel[i+1];
        
        int output_width = network->width[i+1];
        int output_depth = network->depth[i+1];

        if (k_i->cnn) { // Convolution
            Kernel_cnn* cnn = k_i_1->cnn;

            for (int a=0; a < output_depth; a++) {
                for (int b=0; b < output_width; b++) {
                    for (int c=0; c < output_width; c++) {
                        cnn->d_bias[a][b][c] = 0;
                    }
                }
            }
        } else if (k_i->nn) { // Full connection
            Kernel_nn* nn = k_i_1->nn;

            for (int a=0; a < output_width; a++) {
                nn->d_bias[a] = 0;
            }
        }
        // Une couche de pooling ne nécessite pas de traitement
    }
}