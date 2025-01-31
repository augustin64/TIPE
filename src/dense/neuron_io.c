#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "include/neuron.h"

#define MAGIC_NUMBER       2023
#define DELTA_MAGIC_NUMBER 2024


Neuron* read_neuron(uint32_t nb_weights, FILE *ptr) {
    Neuron* neuron = (Neuron*)malloc(sizeof(Neuron));
    float activation;
    float bias;
    float tmp;

    (void) !fread(&activation, sizeof(float), 1, ptr);
    (void) !fread(&bias, sizeof(float), 1, ptr);

    neuron->bias = bias;

    neuron->z = 0.0;
    neuron->last_back_bias = 0.0;
    neuron->back_bias = 0.0;

    if (nb_weights != 0) {
        neuron->last_back_weights = (float*)malloc(sizeof(float)*nb_weights);
        neuron->back_weights = (float*)malloc(sizeof(float)*nb_weights);
        neuron->weights = (float*)malloc(sizeof(float)*nb_weights);

        for (int i=0; i < (int)nb_weights; i++) {
            (void) !fread(&tmp, sizeof(float), 1, ptr);
            neuron->weights[i] = tmp;
            neuron->back_weights[i] = 0.0;
            neuron->last_back_weights[i] = 0.0;
        }
    }

    return neuron;
}


Neuron** read_neurons(uint32_t nb_neurons, uint32_t nb_weights, FILE *ptr) {
    Neuron** neurons = (Neuron**)malloc(sizeof(Neuron*)*nb_neurons);
    for (int i=0; i < (int)nb_neurons; i++) {
        neurons[i] = read_neuron(nb_weights, ptr);
    }
    return neurons;
}


Network* read_network(char* filename) {
    FILE *ptr;
    Network* network = (Network*)malloc(sizeof(Network));

    ptr = fopen(filename, "rb");
    if (!ptr) {
        printf("Impossible de lire le fichier %s\n", filename);
        exit(1);
    }

    uint32_t magic_number;
    uint32_t nb_layers;
    uint32_t tmp;

    (void) !fread(&magic_number, sizeof(uint32_t), 1, ptr);
    if (magic_number != MAGIC_NUMBER) {
        printf("Incorrect magic number !\n");
        exit(1);
    }

    (void) !fread(&nb_layers, sizeof(uint32_t), 1, ptr);
    network->nb_layers = nb_layers;


    Layer** layers = (Layer**)malloc(sizeof(Layer*)*nb_layers);
    uint32_t nb_neurons_layer[nb_layers+1];

    network->layers = layers;

    for (int i=0; i < (int)nb_layers; i++) {
        layers[i] = (Layer*)malloc(sizeof(Layer));
        (void) !fread(&tmp, sizeof(tmp), 1, ptr);
        layers[i]->nb_neurons = tmp;
        nb_neurons_layer[i] = tmp;
    }
    nb_neurons_layer[nb_layers] = 0;

    for (int i=0; i < (int)nb_layers; i++) {
        layers[i]->neurons = read_neurons(layers[i]->nb_neurons, nb_neurons_layer[i+1], ptr);
    }

    fclose(ptr);
    return network;
}


void write_neuron(Neuron* neuron, int weights, FILE *ptr) {
    float buffer[weights+2];

    buffer[1] = neuron->bias;
    for (int i=0; i < weights; i++) {
        buffer[i+2] = neuron->weights[i];
    }

    fwrite(buffer, sizeof(buffer), 1, ptr);
}


void write_network(char* filename, Network* network) {
    FILE *ptr;
    int nb_layers = network->nb_layers;
    int nb_neurons[nb_layers+1];

    ptr = fopen(filename, "wb");
    if (!ptr) {
        printf("Impossible d'ouvrir le fichier %s en écriture\n", filename);
        exit(1);
    }

    uint32_t buffer[nb_layers+2];

    buffer[0] = MAGIC_NUMBER;
    buffer[1] = nb_layers;
    for (int i=0; i < nb_layers; i++) {
        buffer[i+2] = network->layers[i]->nb_neurons;
        nb_neurons[i] = network->layers[i]->nb_neurons;
    }
    nb_neurons[nb_layers] = 0;
    fwrite(buffer, sizeof(buffer), 1, ptr);
    for (int i=0; i < nb_layers; i++) {
        for (int j=0; j < nb_neurons[i]; j++) {
            write_neuron(network->layers[i]->neurons[j], nb_neurons[i+1], ptr);
        }
    }

    fclose(ptr);
}


Neuron* read_delta_neuron(uint32_t nb_weights, FILE *ptr) {
    Neuron* neuron = (Neuron*)malloc(sizeof(Neuron));
    float activation;
    float back_bias;
    float tmp;

    (void) !fread(&activation, sizeof(float), 1, ptr);
    (void) !fread(&back_bias, sizeof(float), 1, ptr);

    neuron->bias = 0.0;

    neuron->z = 0.0;
    neuron->last_back_bias = 0.0;
    neuron->back_bias = back_bias;

    neuron->last_back_weights = (float*)malloc(sizeof(float)*nb_weights);
    neuron->back_weights = (float*)malloc(sizeof(float)*nb_weights);
    neuron->weights = (float*)malloc(sizeof(float)*nb_weights);

    for (int i=0; i < (int)nb_weights; i++) {
        (void) !fread(&tmp, sizeof(float), 1, ptr);
        neuron->weights[i] = 0.0;
        neuron->back_weights[i] = tmp;
        neuron->last_back_weights[i] = 0.0;
    }
    return neuron;
}


Neuron** read_delta_neurons(uint32_t nb_neurons, uint32_t nb_weights, FILE *ptr) {
    Neuron** neurons = (Neuron**)malloc(sizeof(Neuron*)*nb_neurons);
    for (int i=0; i < (int)nb_neurons; i++) {
        neurons[i] = read_delta_neuron(nb_weights, ptr);
    }
    return neurons;
}


Network* read_delta_network(char* filename) {
    FILE *ptr;
    Network* network = (Network*)malloc(sizeof(Network));

    ptr = fopen(filename, "rb");
    if (!ptr) {
        printf("Impossible de lire le fichier %s\n", filename);
        exit(1);
    }

    uint32_t magic_number;
    uint32_t nb_layers;
    uint32_t tmp;

    (void) !fread(&magic_number, sizeof(uint32_t), 1, ptr);
    if (magic_number != DELTA_MAGIC_NUMBER) {
        printf("Incorrect magic number !\n");
        exit(1);
    }

    (void) !fread(&nb_layers, sizeof(uint32_t), 1, ptr);
    network->nb_layers = nb_layers;


    Layer** layers = (Layer**)malloc(sizeof(Layer*)*nb_layers);
    uint32_t nb_neurons_layer[nb_layers+1];

    network->layers = layers;

    for (int i=0; i < (int)nb_layers; i++) {
        layers[i] = (Layer*)malloc(sizeof(Layer));
        (void) !fread(&tmp, sizeof(tmp), 1, ptr);
        layers[i]->nb_neurons = tmp;
        nb_neurons_layer[i] = tmp;
    }
    nb_neurons_layer[nb_layers] = 0;

    for (int i=0; i < (int)nb_layers; i++) {
        layers[i]->neurons = read_delta_neurons(layers[i]->nb_neurons, nb_neurons_layer[i+1], ptr);
    }

    fclose(ptr);
    return network;
}


void write_delta_neuron(Neuron* neuron, int weights, FILE *ptr) {
    float buffer[weights+2];

    buffer[1] = neuron->back_bias;
    for (int i=0; i < weights; i++) {
        buffer[i+2] = neuron->back_weights[i];
    }

    fwrite(buffer, sizeof(buffer), 1, ptr);
}


void write_delta_network(char* filename, Network* network) {
    FILE *ptr;
    int nb_layers = network->nb_layers;
    int nb_neurons[nb_layers+1];

    ptr = fopen(filename, "wb");
    if (!ptr) {
        printf("Impossible d'ouvrir le fichier %s en écriture\n", filename);
        exit(1);
    }

    uint32_t buffer[nb_layers+2];

    buffer[0] = DELTA_MAGIC_NUMBER;
    buffer[1] = nb_layers;
    for (int i=0; i < nb_layers; i++) {
        buffer[i+2] = network->layers[i]->nb_neurons;
        nb_neurons[i] = network->layers[i]->nb_neurons;
    }
    nb_neurons[nb_layers] = 0;
    fwrite(buffer, sizeof(buffer), 1, ptr);
    for (int i=0; i < nb_layers; i++) {
        for (int j=0; j < nb_neurons[i]; j++) {
            write_delta_neuron(network->layers[i]->neurons[j], nb_neurons[i+1], ptr);
        }
    }

    fclose(ptr);
}
