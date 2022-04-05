#ifndef DEF_NEURON_H
#define DEF_NEURON_H

typedef struct Neurone{
    float activation; // Caractérise l'activation du neurone
    float* poids_sortants; // Liste de tous les poids des arêtes sortants du neurone
    float biais; // Caractérise le biais du neurone
    float z; // Sauvegarde des calculs faits sur le neurone (programmation dynamique)

    float d_activation; // Changement d'activation lors de la backpropagation
    float *d_poids_sortants; // Changement des poids sortants lors de la backpropagation
    float d_biais; // Changement du biais lors de la backpropagation
    float d_z; // Quantité de changements générals à effectuer lors de la backpropagation
} Neurone;


typedef struct Couche{
    int nb_neurones; // Nombre de neurones dans la couche (longueur du tableau ci-dessous)
    Neurone** neurones; // Tableau des neurones dans la couche
} Couche;

typedef struct Reseau{
    int nb_couches; // Nombre de couches dans le réseau neuronal (longueur du tableau ci-dessous)
    Couche** couches; // Tableau des couches dans le réseau neuronal
} Reseau;

#endif