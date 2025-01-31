#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <stdbool.h>
#include <pthread.h>

#ifdef __linux__
    #include <sys/sysinfo.h>
#elif defined(__APPLE__)
    #include <sys/sysctl.h>
#else
    #error Unknown platform
#endif

#include "include/neural_network.h"
#include "../common/include/colors.h"
#include "../common/include/mnist.h"
#include "include/neuron_io.h"

#include "include/main.h"

#define EPOCHS 10
#define BATCHES 100

/*
* Structure donnée en argument à la fonction 'train_thread'
*/
typedef struct TrainParameters {
    Network* network;
    int*** images;
    int* labels;
    int* shuffle_indices;
    int start;
    int nb_images;
    int height;
    int width;
    float accuracy;
    bool offset;
} TrainParameters;


void print_image(unsigned int width, unsigned int height, int** image, float* previsions) {
    char tab[] = {' ', '.', ':', '%', '#', '\0'};

    for (int i=0; i < (int)height; i++) {
        for (int j=0; j < (int)width; j++) {
            printf("%c", tab[image[i][j]/52]);
        }
        if (i < 10) {
            printf("\t%d : %f", i, previsions[i]);
        }
        printf("\n");
    }
}

int indice_max(float* tab, int n) {
    int indice = -1;
    float maxi = -FLT_MAX;
    
    for (int i=0; i < n; i++) {
        if (tab[i] > maxi) {
            maxi = tab[i];
            indice = i;
        }
    }
    return indice;
}

void help(char* call) {
    printf("Usage: %s ( train | recognize | test ) [OPTIONS]\n\n", call);
    printf("OPTIONS:\n");
    printf("\ttrain:\n");
    printf("\t\t--epochs  | -e [int]\tNombre d'époques (itérations sur tout le set de données).\n");
    printf("\t\t--recover | -r [FILENAME]\tRécupérer depuis un modèle existant.\n");
    printf("\t\t--images  | -i [FILENAME]\tFichier contenant les images.\n");
    printf("\t\t--labels  | -l [FILENAME]\tFichier contenant les labels.\n");
    printf("\t\t--out     | -o [FILENAME]\tFichier où écrire le réseau de neurones.\n");
    printf("\t\t--delta   | -d [FILENAME]\tFichier où écrire le réseau différentiel.\n");
    printf("\t\t--nb-images | -N [int]\tNombres d'images à traiter.\n");
    printf("\t\t--start   | -s [int]\tPremière image à traiter.\n");
    printf("\t\t--offset            \tActiver le décalage aléatoire.\n");
    printf("\trecognize:\n");
    printf("\t\t--modele  | -m [FILENAME]\tFichier contenant le réseau de neurones.\n");
    printf("\t\t--in      | -i [FILENAME]\tFichier contenant les images à reconnaître.\n");
    printf("\t\t--out     | -o (text|json)\tFormat de sortie.\n");
    printf("\ttest:\n");
    printf("\t\t--images  | -i [FILENAME]\tFichier contenant les images.\n");
    printf("\t\t--labels  | -l [FILENAME]\tFichier contenant les labels.\n");
    printf("\t\t--modele  | -m [FILENAME]\tFichier contenant le réseau de neurones.\n");
    printf("\t\t--preview-fails | -p\tAfficher les images ayant échoué.\n");
    printf("\t\t--offset            \tActiver le décalage aléatoire.\n");
}


void write_image_in_network(int** image, Network* network, int height, int width, bool random_offset) {
    int i_offset = 0;
    int j_offset = 0;
    int min_col = 0;
    int min_ligne = 0;

    if (random_offset) {
        int sum_colonne[width];
        int sum_ligne[height];

        for (int i=0; i < width; i++) {
            sum_colonne[i] = 0;
        }
        for (int j=0; j < height; j++) {
            sum_ligne[j] = 0;
        }

        for (int i=0; i < width; i++) {
            for (int j=0; j < height; j++) {
                sum_ligne[i] += image[i][j];
                sum_colonne[j] += image[i][j];
            }
        }

        min_ligne = -1;
        while (sum_ligne[min_ligne+1] == 0 && min_ligne < width+1) {
            min_ligne++;
        }

        int max_ligne = width;
        while (sum_ligne[max_ligne-1] == 0 && max_ligne > 0) {
            max_ligne--;
        }

        min_col = -1;
        while (sum_colonne[min_col+1] == 0 && min_col < height+1) {
            min_col++;
        }

        int max_col = height;
        while (sum_colonne[max_col-1] == 0 && max_col > 0) {
            max_col--;
        }

        i_offset = 27-max_ligne+min_ligne == 0 ? 0 : rand()%(27-max_ligne+min_ligne);
        j_offset = 27 - max_col + min_col == 0 ? 0 : rand()%(27-max_col+min_col);
    }

    for (int i=0; i < width; i++) {
        for (int j=0; j < height; j++) {
            int adjusted_i = i + min_ligne - i_offset;
            int adjusted_j = j + min_col - j_offset;
            // Make sure not to be out of the image
            if (!drop(ENTRY_DROPOUT) && adjusted_i < height && adjusted_j < width && adjusted_i >= 0 && adjusted_j >= 0) {
                network->layers[0]->neurons[i*height+j]->z = (float)image[adjusted_i][adjusted_j] / 255.0f;
            } else {
                network->layers[0]->neurons[i*height+j]->z = 0.;
            }
        }
    }
}

void* train_thread(void* parameters) {
    TrainParameters* param = (TrainParameters*)parameters;
    Network* network = param->network;
    Layer* last_layer = network->layers[network->nb_layers-1];
    int nb_neurons_last_layer = last_layer->nb_neurons;

    int*** images = param->images;
    int* labels = param->labels;
    int* shuffle = param->shuffle_indices;

    int start = param->start;
    int nb_images = param->nb_images;
    int height = param->height;
    int width = param->width;
    float accuracy = 0.;
    float* sortie = (float*)malloc(sizeof(float)*nb_neurons_last_layer);
    int* desired_output;

    for (int i=start; i < start+nb_images; i++) {
        write_image_in_network(images[shuffle[i]], network, height, width, param->offset);
        desired_output = desired_output_creation(network, labels[shuffle[i]]);
        forward_propagation(network, true);
        backward_propagation(network, desired_output);

        for (int k=0; k < nb_neurons_last_layer; k++) {
            sortie[k] = last_layer->neurons[k]->z;
        }
        if (indice_max(sortie, nb_neurons_last_layer) == labels[shuffle[i]]) {
            accuracy += 1.;
        }
        free(desired_output);
    }
    free(sortie);
    param->accuracy = accuracy;

    return NULL;
}


void train(int epochs, char* recovery, char* image_file, char* label_file, char* out, char* delta, int nb_images_to_process, int start, bool offset) {
    // Entraînement du réseau sur le set de données MNIST
    Network* network;
    Network* delta_network;

    //int* repartition = malloc(sizeof(int)*layers);
    int layers = 2;
    int neurons = 784;
    int nb_neurons_last_layer = 10;
    int repartition[2] = {neurons, nb_neurons_last_layer};

    float accuracy;
    float current_accuracy;

    #ifdef __linux__
        int nb_threads = get_nprocs();
    #elif defined(__APPLE__)
        int nb_threads;
        size_t len = sizeof(nb_threads);

        if (sysctlbyname("hw.logicalcpu", &nb_threads, &len, NULL, 0) == -1) {
            perror("sysctl");
            exit(1);
        }
    #endif
    pthread_t *tid = (pthread_t *)malloc(nb_threads * sizeof(pthread_t));

    /*
    * On repart d'un réseau déjà créée stocké dans un fichier
    * ou on repart de zéro si aucune backup n'est fournie
    * */
    if (! recovery) {
        network = (Network*)malloc(sizeof(Network));
        network_creation(network, repartition, layers);
        network_initialisation(network);
    } else {
        network = read_network(recovery);
        printf("Backup restaurée.\n");
    }

    if (delta != NULL) {
        // On initialise un réseau complet mais la seule partie qui nous intéresse est la partie différentielle
        delta_network = (Network*)malloc(sizeof(Network));

        int* repart = (int*)malloc(sizeof(network->nb_layers));
        for (int i=0; i < network->nb_layers; i++) {
            repart[i] = network->layers[i]->nb_neurons;
        }

        network_creation(delta_network, repart, network->nb_layers);
        network_initialisation(delta_network);
        free(repart);
    }

    // Chargement des images du set de données MNIST
    int* parameters = read_mnist_images_parameters(image_file);
    int nb_images_total = parameters[0];
    int nb_remaining_images = 0; // Nombre d'images restantes dans un batch
    int height = parameters[1];
    int width = parameters[2];
    free(parameters);

    int*** images = read_mnist_images(image_file);
    unsigned int* labels = read_mnist_labels(label_file);
    
    int* shuffle_indices = (int*)malloc(sizeof(int)*nb_images_total);
    for (int i=0; i < nb_images_total; i++) {
        shuffle_indices[i] = i;
    }

    if (nb_images_to_process != -1) {
        nb_images_total = nb_images_to_process;
    }

    TrainParameters** train_parameters = (TrainParameters**)malloc(sizeof(TrainParameters*)*nb_threads);
    for (int j=0; j < nb_threads; j++) {
        train_parameters[j] = (TrainParameters*)malloc(sizeof(TrainParameters));
        train_parameters[j]->images = (int***)images;
        train_parameters[j]->labels = (int*)labels;
        train_parameters[j]->height = height;
        train_parameters[j]->width = width;
        train_parameters[j]->nb_images = BATCHES / nb_threads;
        train_parameters[j]->shuffle_indices = shuffle_indices;
        train_parameters[j]->offset = offset;
    }

    for (int i=0; i < epochs; i++) {
        knuth_shuffle(shuffle_indices, nb_images_total); // Shuffle images between each epoch
        accuracy = 0.;
        for (int k=0; k < nb_images_total / BATCHES; k++) {
            nb_remaining_images = BATCHES;
            for (int j=0; j < nb_threads; j++) {
                train_parameters[j]->network = copy_network(network);
                train_parameters[j]->start = nb_images_total - BATCHES*(nb_images_total / BATCHES - k -1) - nb_remaining_images + start;

                if (j == nb_threads-1) {
                    train_parameters[j]->nb_images = nb_remaining_images;
                }
                nb_remaining_images -= train_parameters[j]->nb_images;

                // Création des threads sur le CPU
                pthread_create( &tid[j], NULL, train_thread, (void*) train_parameters[j]);
            }
            for(int j=0; j < nb_threads; j++ ) {
                // On récupère les threads créés sur le CPU
                pthread_join( tid[j], NULL );
                
                accuracy += train_parameters[j]->accuracy / (float) nb_images_total;
                if (delta != NULL)
                    patch_delta(delta_network, train_parameters[j]->network, train_parameters[j]->nb_images);
                patch_network(network, train_parameters[j]->network, train_parameters[j]->nb_images);
                deletion_of_network(train_parameters[j]->network);
            }
            current_accuracy = accuracy*(nb_images_total/(BATCHES*(k+1)));
            printf("\rThreads [%d]\tÉpoque [%d/%d]\tImage [%d/%d]\tAccuracy: "YELLOW"%0.1f%%"RESET, nb_threads, i, epochs, BATCHES*(k+1), nb_images_total, current_accuracy*100);
            fflush(stdout);
        }
        printf("\rThreads [%d]\tÉpoque [%d/%d]\tImage [%d/%d]\tAccuracy: "GREEN"%0.1f%%"RESET"\n", nb_threads, i, epochs, nb_images_total, nb_images_total, accuracy*100);
        write_network(out, network);
        if (delta != NULL)
            write_delta_network(delta, delta_network);

        test(out, "data/mnist/t10k-images-idx3-ubyte", "data/mnist/t10k-labels-idx1-ubyte", false, offset);
    }
    write_network(out, network);
    if (delta != NULL) {
        deletion_of_network(delta_network);
    }
    deletion_of_network(network);
    for (int j=0; j < nb_threads; j++) {
        free(train_parameters[j]);
    }

    for (int i=0; i < nb_images_total; i++) {
        for (int j=0; j < height; j++) {
            free(images[i][j]);
        }
        free(images[i]);
    }
    free(images);
    free(labels);
    
    free(shuffle_indices);
    free(train_parameters);
    // On libère les espaces mémoire utilisés spécialement sur le CPU
    free(tid);
}

void swap(int* tab, int i, int j) {
    int tmp = tab[i];
    tab[i] = tab[j];
    tab[j] = tmp;
}

void knuth_shuffle(int* tab, int n) {
    for(int i=1; i < n; i++) {
        swap(tab, i, rand() %i);
    }
}

float** recognize(char* modele, char* entree, bool offset) {
    Network* network = read_network(modele);
    Layer* last_layer = network->layers[network->nb_layers-1];

    int* parameters = read_mnist_images_parameters(entree);
    int nb_images = parameters[0];
    int height = parameters[1];
    int width = parameters[2];
    free(parameters);

    int*** images = read_mnist_images(entree);
    float** results = (float**)malloc(sizeof(float*)*nb_images);

    for (int i=0; i < nb_images; i++) {
        results[i] = (float*)malloc(sizeof(float)*last_layer->nb_neurons);

        write_image_in_network(images[i], network, height, width, offset);
        forward_propagation(network, false);

        for (int j=0; j < last_layer->nb_neurons; j++) {
            results[i][j] = last_layer->neurons[j]->z;
        }
    }
    deletion_of_network(network);
    return results;
}

void print_recognize(char* modele, char* entree, char* sortie, bool offset) {
    Network* network = read_network(modele);
    int nb_last_layer = network->layers[network->nb_layers-1]->nb_neurons;

    deletion_of_network(network);

    int* parameters = read_mnist_images_parameters(entree);
    int nb_images = parameters[0];

    float** results = recognize(modele, entree, offset);

    if (! strcmp(sortie, "json")) {
        printf("{\n");
    }
    for (int i=0; i < nb_images; i++) {
        if (! strcmp(sortie, "text"))
            printf("Image %d\n", i);
        else
            printf("\"%d\" : [", i);

        for (int j=0; j < nb_last_layer; j++) {
            if (! strcmp(sortie, "json")) {
                printf("%f", results[i][j]);

                if (j+1 < nb_last_layer) {
                    printf(", ");
                }
            } else
                printf("Probabilité %d: %f\n", j, results[i][j]);
        }
        free(results[i]);
        if (! strcmp(sortie, "json")) {
            if (i+1 < nb_images) {
                printf("],\n");
            } else {
                printf("]\n");
            }
        }
    }
    free(results);
    free(parameters);
    if (! strcmp(sortie, "json")) {
        printf("}\n");
    }
}

void test(char* modele, char* fichier_images, char* fichier_labels, bool preview_fails, bool offset) {
    Network* network = read_network(modele);
    int nb_last_layer = network->layers[network->nb_layers-1]->nb_neurons;

    deletion_of_network(network);

    int* parameters = read_mnist_images_parameters(fichier_images);
    int nb_images = parameters[0];
    int width = parameters[1];
    int height = parameters[2];
    int*** images = read_mnist_images(fichier_images);

    float** results = recognize(modele, fichier_images, offset);
    unsigned int* labels = read_mnist_labels(fichier_labels);
    float accuracy = 0.;

    for (int i=0; i < nb_images; i++) {
        if (indice_max(results[i], nb_last_layer) == (int)labels[i]) {
            accuracy += 1. / (float)nb_images;
        } else if (preview_fails) {
            printf("--- Image %d, %d --- Prévision: %d ---\n", i, labels[i], indice_max(results[i], nb_last_layer));
            print_image(width, height, images[i], results[i]);
        }
        free(results[i]);
    }
    printf("%d Images\tAccuracy: %0.1f%%\n", nb_images, accuracy*100);
    free(parameters);
    free(results);
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Pas d'action spécifiée\n");
        help(argv[0]);
        return 1;
    }
    if (! strcmp(argv[1], "train")) {
        int epochs = EPOCHS;
        int nb_images = -1;
        int start = 0;
        char* images = NULL;
        char* labels = NULL;
        char* recovery = NULL;
        char* out = NULL;
        char* delta = NULL;
        bool offset = false;

        int i = 2;
        while (i < argc) {
            // Utiliser un switch serait sans doute plus élégant
            if ((! strcmp(argv[i], "--epochs"))||(! strcmp(argv[i], "-e"))) {
                epochs = strtol(argv[i+1], NULL, 10);
                i += 2;
            } else if ((! strcmp(argv[i], "--images"))||(! strcmp(argv[i], "-i"))) {
                images = argv[i+1];
                i += 2;
            } else if ((! strcmp(argv[i], "--labels"))||(! strcmp(argv[i], "-l"))) {
                labels = argv[i+1];
                i += 2;
            } else if ((! strcmp(argv[i], "--recover"))||(! strcmp(argv[i], "-r"))) {
                recovery = argv[i+1];
                i += 2;
            } else if ((! strcmp(argv[i], "--out"))||(! strcmp(argv[i], "-o"))) {
                out = argv[i+1];
                i += 2;
            } else if ((! strcmp(argv[i], "--delta"))||(! strcmp(argv[i], "-d"))) {
                delta = argv[i+1];
                i += 2;
            } else if ((! strcmp(argv[i], "--nb-images"))||(! strcmp(argv[i], "-N"))) {
                nb_images = strtol(argv[i+1], NULL, 10);
                i += 2;
            } else if ((! strcmp(argv[i], "--start"))||(! strcmp(argv[i], "-s"))) {
                start = strtol(argv[i+1], NULL, 10);
                i += 2;
            } else if (! strcmp(argv[i], "--offset")) {
                offset = true;
                i++;
            } else {
                printf("%s : Argument non reconnu\n", argv[i]);
                i++;
            }
        }
        if (! images) {
            printf("Pas de fichier d'images spécifié\n");
            return 1;
        }
        if (! labels) {
            printf("Pas de fichier de labels spécifié\n");
            return 1;
        }
        if (! out) {
            printf("Pas de fichier de sortie spécifié, default: out.bin\n");
            out = "out.bin";
        }
        // Entraînement (dans neural_network.c)
        train(epochs, recovery, images, labels, out, delta, nb_images, start, offset);
        return 0;
    }
    if (! strcmp(argv[1], "recognize")) {
        char* in = NULL;
        char* modele = NULL;
        char* out = NULL;
        int i = 2;
        while(i < argc) {
            if ((! strcmp(argv[i], "--in"))||(! strcmp(argv[i], "-i"))) {
                in = argv[i+1];
                i += 2;
            } else if ((! strcmp(argv[i], "--modele"))||(! strcmp(argv[i], "-m"))) {
                modele = argv[i+1];
                i += 2;
            } else if ((! strcmp(argv[i], "--out"))||(! strcmp(argv[i], "-o"))) {
                out = argv[i+1];
                i += 2;
            } else {
                printf("%s : Argument non reconnu\n", argv[i]);
                i++;
            }
        }
        if (! in) {
            printf("Pas d'entrée spécifiée\n");
            return 1;
        }
        if (! modele) {
            printf("Pas de modèle spécifié\n");
            return 1;
        }
        if (! out) {
            out = "text";
        }
        print_recognize(modele, in, out, false);
        // Reconnaissance puis affichage des données sous le format spécifié
        return 0;
    }
    if (! strcmp(argv[1], "test")) {
        char* modele = NULL;
        char* images = NULL;
        char* labels = NULL;
        bool preview_fails = false;
        bool offset = false;
        int i = 2;
        while (i < argc) {
            if ((! strcmp(argv[i], "--images"))||(! strcmp(argv[i], "-i"))) {
                images = argv[i+1];
                i += 2;
            } else if ((! strcmp(argv[i], "--labels"))||(! strcmp(argv[i], "-l"))) {
                labels = argv[i+1];
                i += 2;
            } else if ((! strcmp(argv[i], "--modele"))||(! strcmp(argv[i], "-m"))) {
                modele = argv[i+1];
                i += 2;
            } else if ((! strcmp(argv[i], "--preview-fails"))||(! strcmp(argv[i], "-p"))) {
                preview_fails = true;
                i++;
            } else if (! strcmp(argv[i], "--offset")) {
                offset = true;
                i++;
            } else {
                printf("%s : Argument non reconnu\n", argv[i]);
                i++;
            }
        }
        test(modele, images, labels, preview_fails, offset);
        return 0;
    }
    printf("Option choisie non reconnue: %s\n", argv[1]);
    help(argv[0]);
    return 1;
}
