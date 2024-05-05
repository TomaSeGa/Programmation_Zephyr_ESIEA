#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

// On crée une structure snombre
typedef struct snombre {
    int x;
    int y;
    int z;
} SNOMBRE;

SNOMBRE coor = {2, 2, 2};
int nb_erreur = 0;
// pthread_mutex_t mutex1; // mutex de coordonnées

void *writerThread1(void *p) {
    while (1) {
        sleep(10);
        // pthread_mutex_lock(&mutex1);
        coor.x = 2;
        coor.y = 2;
        coor.z = 2;
        // pthread_mutex_unlock(&mutex1);
    }
    return NULL;
}

void *writerThread2(void *p) {
    while (1) {
        sleep(1);
        // pthread_mutex_lock(&mutex1);
        coor.x = 10;
        // sleep(3); ajout error
        coor.y = 10;
        // sleep(3); ajout error
        coor.z = 10;
        // pthread_mutex_unlock(&mutex1);
    }
    return NULL;
}

void *readerThread(void *p) {
    while (1) {
        //pthread_mutex_lock(&mutex1);
        int X = coor.x;
        int Y = coor.y;
        int Z = coor.z;
        //pthread_mutex_unlock(&mutex1);

        if((X != 2) || (Y != 2) || (Z != 2)) 
        {
            if((X != 10) || (Y != 10) || (Z != 10)) 
            {
                printf("x: %d, y: %d, z: %d \n",X,Y,Z);
                nb_erreur++;
            }
        }
        sleep(1); 
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    pthread_t writer1, writer2, reader;

    pthread_create(&writer1, NULL, writerThread1, NULL);
    pthread_create(&writer2, NULL, writerThread2, NULL);
    pthread_create(&reader, NULL, readerThread, NULL);
    
    sleep(20);

    // pthread_mutex_destroy(&mutex1);

    printf("nb errors : %d \n",nb_erreur);
    printf("END \n");
    return 0;
}
