#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "include/neuron.h"
#include "include/neural_network.h"



#ifndef __CUDACC__
// The functions and macros below are already defined when using NVCC
#define INT_MIN -2147483648

float max(float a, float b){
    return a < b ? b : a;
}

#endif

bool drop(float prob) {
    return (rand() % 100) > 100*prob;
}

float sigmoid(float x){
    return 1/(1 + exp(-x));
}

float sigmoid_derivative(float x){
    float tmp = exp(-x);
    return tmp/((1+tmp)*(1+tmp));
}

float leaky_ReLU(float x){
    if (x > 0)
        return x;
    return COEFF_LEAKY_RELU;
}

float leaky_ReLU_derivative(float x){
    if (x > 0)
        return 1;
    return COEFF_LEAKY_RELU;
}

void network_creation(Network* network, int* neurons_per_layer, int nb_layers) {
    Layer* layer;

    network->nb_layers = nb_layers;
    network->layers = (Layer**)malloc(sizeof(Layer*)*nb_layers);

    for (int i=0; i < nb_layers; i++) {
        network->layers[i] = (Layer*)malloc(sizeof(Layer));
        layer = network->layers[i];
        layer->nb_neurons = neurons_per_layer[i];
        layer->neurons = (Neuron**)malloc(sizeof(Neuron*)*network->layers[i]->nb_neurons);

        for (int j=0; j < layer->nb_neurons; j++) {
            layer->neurons[j] = (Neuron*)malloc(sizeof(Neuron));

            if (i != network->nb_layers-1) { // On exclut la dernière couche dont les neurones ne contiennent pas de poids sortants
                layer->neurons[j]->weights = (float*)malloc(sizeof(float)*neurons_per_layer[i+1]);
                layer->neurons[j]->back_weights = (float*)malloc(sizeof(float)*neurons_per_layer[i+1]);
                layer->neurons[j]->last_back_weights = (float*)malloc(sizeof(float)*neurons_per_layer[i+1]);
            }
        }
    }
}




void deletion_of_network(Network* network) {
    Layer* layer;
    Neuron* neuron;

    for (int i=0; i < network->nb_layers; i++) {
        layer = network->layers[i];
        
        for (int j=0; j < network->layers[i]->nb_neurons; j++) {
            neuron = layer->neurons[j];
            if (i != network->nb_layers-1) { // On exclut la dernière couche dont les neurones ne contiennent pas de poids sortants
                free(neuron->weights);
                free(neuron->back_weights);
                free(neuron->last_back_weights);
            }
            free(neuron);
        }
        free(layer->neurons);
        free(network->layers[i]);
    }
    free(network->layers);
    free(network);
}




void forward_propagation(Network* network, bool is_training) {
    Layer* layer; // Couche actuelle
    Layer* pre_layer; // Couche précédente
    Neuron* neuron;
    float sum;
    float max_z = INT_MIN;

    for (int i=1; i < network->nb_layers; i++) { // La première couche contient déjà des valeurs
        sum = 0;
        max_z = INT_MIN;
        layer = network->layers[i];
        pre_layer = network->layers[i-1];

        for (int j=0; j < layer->nb_neurons; j++) {
            neuron = layer->neurons[j];
            neuron->z = neuron->bias;

            for (int k=0; k < pre_layer->nb_neurons; k++) {
                neuron->z += pre_layer->neurons[k]->z * pre_layer->neurons[k]->weights[j];
            }

            if (i < network->nb_layers-1) {
                if (!is_training) {
                    if (j == 0) {
                        neuron->z = ENTRY_DROPOUT*leaky_ReLU(neuron->z);
                    } else {
                        neuron->z = DROPOUT*leaky_ReLU(neuron->z);
                    }
                } else if (!drop(DROPOUT)) {
                    neuron->z = leaky_ReLU(neuron->z);
                } else {
                    neuron->z = 0.;
                }
                
            } else { // Softmax seulement pour la dernière couche
                max_z = max(max_z, neuron->z);
            }
        }
    }
    layer = network->layers[network->nb_layers-1];
    int size_last_layer = layer->nb_neurons;

    for (int j=0; j < size_last_layer; j++) {
        neuron = layer->neurons[j];
        neuron->z = exp(neuron->z - max_z);
        sum += neuron->z;
    }
    for (int j=0; j < size_last_layer; j++) {
        neuron = layer->neurons[j];
        neuron->z = neuron->z / sum;
    }
}




int* desired_output_creation(Network* network, int wanted_number) {
    int nb_neurons = network->layers[network->nb_layers-1]->nb_neurons;

    int* desired_output = (int*)malloc(sizeof(int)*nb_neurons);

    for (int i=0; i < nb_neurons; i++) // On initialise toutes les sorties à 0 par défaut
        desired_output[i] = 0;
    desired_output[wanted_number] = 1; // Seule la sortie voulue vaut 1
    return desired_output;
}



void backward_propagation(Network* network, int* desired_output) {
    Neuron* neuron;
    Neuron* neuron2;
    float changes;
    float tmp;

    int i = network->nb_layers-2;
    int neurons_nb = network->layers[i+1]->nb_neurons;
    for (int j=0; j < network->layers[i+1]->nb_neurons; j++) { // Dernière couche en première
        neuron = network->layers[i+1]->neurons[j];
        tmp = (desired_output[j]==1) ? neuron->z - 1 : neuron->z;
        for (int k=0; k < network->layers[i]->nb_neurons; k++) {
            neuron2 = network->layers[i]->neurons[k];
            neuron2->back_weights[j] += neuron2->z*tmp;
            neuron2->last_back_weights[j] = neuron2->z*tmp;
        }
        neuron->last_back_bias = tmp;
        neuron->back_bias += tmp;
    }
    for (i--; i >= 0; i--) { // Autres couches ensuite
        neurons_nb =  network->layers[i+1]->nb_neurons;
        for (int j=0; j < neurons_nb; j++) {
            neuron = network->layers[i+1]->neurons[j];
            changes = 0;
            for (int k=0; k < network->layers[i+2]->nb_neurons; k++) {
                changes += (neuron->weights[k]*neuron->last_back_weights[k])/neurons_nb;
            }
            changes = changes*leaky_ReLU_derivative(neuron->z);
            if (neuron->z != 0) {
                neuron->back_bias += changes;
                neuron->last_back_bias = changes;
            }
            for (int l=0; l < network->layers[i]->nb_neurons; l++){
                neuron2 = network->layers[i]->neurons[l];
                if (neuron->z != 0) {
                    neuron2->back_weights[j] += neuron2->weights[j]*changes;
                    neuron2->last_back_weights[j] = neuron2->weights[j]*changes;
                }
            }
        }
    }
}




void network_modification(Network* network, uint32_t nb_modifs) {
    Neuron* neuron;

    for (int i=0; i < network->nb_layers; i++) { // on exclut la dernière couche
        for (int j=0; j < network->layers[i]->nb_neurons; j++) {
            neuron = network->layers[i]->neurons[j];
            if (neuron->bias != 0 && PRINT_BIAIS)
                printf("C %d\tN %d\tb: %f      \tDb: %f\n", i, j, neuron->bias,  (LEARNING_RATE/nb_modifs) * neuron->back_bias);
            neuron->bias -= (LEARNING_RATE/nb_modifs) * neuron->back_bias;
            neuron->back_bias = 0;

            if (neuron->bias > MAX_RESEAU)
                neuron->bias = MAX_RESEAU;
            else if (neuron->bias < -MAX_RESEAU)
                neuron->bias = -MAX_RESEAU;

            if (i != network->nb_layers-1) {
                for (int k=0; k < network->layers[i+1]->nb_neurons; k++) {
                    if (neuron->weights[k] != 0 && PRINT_POIDS)
                        printf("C %d\tN %d -> %d\tp: %f  \tDp: %f\n", i, j, k, neuron->weights[k],  (LEARNING_RATE/nb_modifs) * neuron->back_weights[k]);
                    neuron->weights[k] -= (LEARNING_RATE/nb_modifs) * neuron->back_weights[k];
                    neuron->back_weights[k] = 0;

                    if (neuron->weights[k] > MAX_RESEAU) {
                        neuron->weights[k] = MAX_RESEAU;
                        printf("Erreur, max du réseau atteint");
                    }
                    else if (neuron->weights[k] < -MAX_RESEAU) {
                        neuron->weights[k] = -MAX_RESEAU;
                        printf("Erreur, min du réseau atteint");
                    }
                }
            }
        }
    }
}




void network_initialisation(Network* network) {
    Neuron* neuron;
    double upper_bound;
    double lower_bound;
    double bound_gap;

    int nb_layers_loop = network->nb_layers -1;

    upper_bound = 1/sqrt((double)network->layers[nb_layers_loop]->nb_neurons);
    lower_bound = -upper_bound;
    bound_gap = upper_bound - lower_bound;
    
    srand(time(0));
    for (int i=0; i < nb_layers_loop; i++) { // On exclut la dernière couche
        for (int j=0; j < network->layers[i]->nb_neurons; j++) {

            neuron = network->layers[i]->neurons[j];

            if (i!=nb_layers_loop) {
                for (int k=0; k < network->layers[i+1]->nb_neurons; k++) {
                    neuron->weights[k] = lower_bound + RAND_DOUBLE()*bound_gap;
                    neuron->back_weights[k] = 0;
                    neuron->last_back_weights[k] = 0;
                }
            }
            if (i > 0) { // On exclut la première couche
                neuron->bias = lower_bound + RAND_DOUBLE()*bound_gap;
                neuron->back_bias = 0;
                neuron->last_back_bias = 0;
            }
        }
    }
}

void patch_network(Network* network, Network* delta, uint32_t nb_modifs) {
    Neuron* neuron;
    Neuron* dneuron;

    for (int i=0; i < network->nb_layers; i++) {
        for (int j=0; j < network->layers[i]->nb_neurons; j++) {
            neuron = network->layers[i]->neurons[j];
            dneuron = delta->layers[i]->neurons[j];
            neuron->bias -= (LEARNING_RATE/nb_modifs) * dneuron->back_bias;
            dneuron->back_bias = 0;

            if (i != network->nb_layers-1) {
                for (int k=0; k < network->layers[i+1]->nb_neurons; k++) {
                    neuron->weights[k] -= (LEARNING_RATE/nb_modifs) * dneuron->back_weights[k]; // On modifie le poids du neurone à partir des données de la propagation en arrière
                    dneuron->back_weights[k] = 0;
                }
            }
        }
    }
}

void patch_delta(Network* network, Network* delta, uint32_t nb_modifs) {
    Neuron* neuron;
    Neuron* dneuron;

    for (int i=0; i < network->nb_layers; i++) {
        for (int j=0; j < network->layers[i]->nb_neurons; j++) {
            neuron = network->layers[i]->neurons[j];
            dneuron = delta->layers[i]->neurons[j];
            neuron->back_bias += dneuron->back_bias/nb_modifs;

            if (i != network->nb_layers-1) {
                for (int k=0; k < network->layers[i+1]->nb_neurons; k++) {
                    neuron->back_weights[k] += dneuron->back_weights[k]/nb_modifs;
                }
            }
        }
    }
}

Network* copy_network(Network* network) {
    Network* network2 = (Network*)malloc(sizeof(Network));
    Layer* layer;
    Neuron* neuron1;
    Neuron* neuron;

    network2->nb_layers = network->nb_layers;
    network2->layers = (Layer**)malloc(sizeof(Layer*)*network->nb_layers);
    for (int i=0; i < network2->nb_layers; i++) {
        layer = (Layer*)malloc(sizeof(Layer));
        layer->nb_neurons = network->layers[i]->nb_neurons;
        layer->neurons = (Neuron**)malloc(sizeof(Neuron*)*layer->nb_neurons);
        for (int j=0; j < layer->nb_neurons; j++) {
            neuron = (Neuron*)malloc(sizeof(Neuron));

            neuron1 = network->layers[i]->neurons[j];
            neuron->bias = neuron1->bias;
            neuron->z = neuron1->z;
            neuron->back_bias = neuron1->back_bias;
            neuron->last_back_bias = neuron1->last_back_bias;
            if (i != network2->nb_layers-1) {
                (void)network2->layers[i+1]->nb_neurons;
                neuron->weights = (float*)malloc(sizeof(float)*network->layers[i+1]->nb_neurons);
                neuron->back_weights = (float*)malloc(sizeof(float)*network->layers[i+1]->nb_neurons);
                neuron->last_back_weights = (float*)malloc(sizeof(float)*network->layers[i+1]->nb_neurons);
                for (int k=0; k < network->layers[i+1]->nb_neurons; k++) {
                    neuron->weights[k] = neuron1->weights[k];
                    neuron->back_weights[k] = neuron1->back_weights[k];
                    neuron->last_back_weights[k] = neuron1->last_back_weights[k];
                }
            }
            layer->neurons[j] = neuron;
        }
    network2->layers[i] = layer;
    }
    return network2;
}


float loss_computing(Network* network, int wanted_number){
    float erreur = 0;
    float neuron_value;
    for (int i=0; i < network->nb_layers-1; i++) {
        neuron_value = network->layers[network->nb_layers-1]->neurons[i]->z;

        if (i == wanted_number) {
            erreur += (1-neuron_value)*(1-neuron_value);
        }
        else {
            erreur += neuron_value*neuron_value;
        }
    }
    return erreur;
}