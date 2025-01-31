#ifndef DEF_CONFIG_H
#define DEF_CONFIG_H


//** Paramètres d'entraînement
#define EPOCHS 10 // Nombre d'époques par défaut (itérations sur toutes les images)
#define BATCHES 32 // Nombre d'images à voir avant de mettre le réseau à jour
#define LEARNING_RATE 3e-4 // Taux d'apprentissage
#define USE_MULTITHREADING // Commenter pour utiliser un seul coeur durant l'apprentissage (meilleur pour des tailles de batchs traités rapidement)

//#define DETAILED_TRAIN_TIMINGS // Afficher le temps de forward/ backward

//* Paramètres d'ADAM optimizer
#define ALPHA 3e-4
#define BETA_1 0.9
#define BETA_2 0.999
#define Epsilon 1e-7

//* Options d'ADAM optimizer
//* Activer ou désactiver Adam sur les couches dense
//#define ADAM_DENSE_WEIGHTS
//#define ADAM_DENSE_BIAS
//* Activer ou désactiver Adam sur les couches convolutives
//#define ADAM_CNN_WEIGHTS
//#define ADAM_CNN_BIAS


//* Limite du réseau
// Des valeurs trop grandes dans le réseau risqueraient de provoquer des overflows notamment.
// On utilise donc la méthode gradient_clipping,
// qui consiste à majorer tous les biais et poids par un hyper-paramètre choisi précédemment.
// https://arxiv.org/pdf/1905.11881.pdf
#define NETWORK_CLIP_VALUE 300


//** Paramètres CUDA
// Le produit des 3 dimensions doit être au maximum 1024 (atteignable avec 8*8*16)
// Le réduire permet d'éviter des erreurs "Out of memory" ou "too many resources requested" au lancement des Kernel
#define BLOCKSIZE_x 8
#define BLOCKSIZE_y 8
#define BLOCKSIZE_z 8


//** Paramètres d'optimisation
//* Optimisation de libération de la mémoire pour de larges réseaux
// En utilisant CUDA, de larges réseaux créés dans src/common/memory_management.cu
// peuvent prendre jusqu'à plusieurs heures pour être libérés
// Une optimisation consiste alors à considérer que seul le réseau est dans cet emplacement de mémoire.
// La libération d'un réseau entraîne alors la libération de toute la mémoire, ce qui peut poser problème
// dans certaines situations.
#define FREE_ALL_OPT

#endif