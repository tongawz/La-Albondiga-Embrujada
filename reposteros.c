#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include "cocina.h"

void desvincularSemaforosYMemoriaCompartida()
{
    sem_unlink("/semPlatosEnElDia");
    sem_unlink("/semRespotero");
}

int main()
{
    //desvincularSemaforosYMemoriaCompartida();
    // Crea o abre el objeto de memoria compartida
    int shmFd = shm_open("/shared_memory", O_RDWR | O_CREAT, 0666);
    if (shmFd == -1)
    {
        perror("shm_open");
    }

    if (ftruncate(shmFd, sizeof(SharedData)) == -1)
    {
        perror("ftruncate");
    }

    // Mapeo la memoria compartida
    SharedData *sharedData = (SharedData *)mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0);
    if (sharedData == MAP_FAILED)
    {
        perror("mmap");
    }

    /* ---- CREAR O ABRIR SEMÁFOROS ---- */
    // Despierta al repostero
    sem_t *semRespotero = sem_open("/semRespotero", O_CREAT, 0666, 0);
    // Protege a platos preparados en el día
    sem_t *semPlatosEnElDia = sem_open("/semPlatosEnElDia", O_CREAT, 0666, 1);
    // Protege a la variable de inicialización
    sem_t *semInicializacion = sem_open("/semInicializacion", O_CREAT | O_EXCL, 0666, 1);
    
    if(semInicializacion != SEM_FAILED){
        initializeSharedMemory(sharedData);
    }
    else{
        while(!sharedData->inicializado){
            printf("Esperando inicialización\n");
            sleep(2);
        }
    }
    
    bool cocinaAbierta = true;
    do
    {
        printf("Repostero: Esperando llamado\n");
        sem_wait(semRespotero);
        printf("Repostero: Recibiendo llamada\n");
        sem_wait(semPlatosEnElDia);
        cocinaAbierta = sharedData->platosPreparadosEnElDia < MAX_PLATOS_DIA;
        sem_post(semPlatosEnElDia);
        if (cocinaAbierta)
        {
            printf("Repostero: Se llena la heladera con %d flanes.\n", MAX_HELADERA);
            sharedData->flanesEnFeezer = MAX_HELADERA;
        }
        sem_post(semRespotero);
        sleep(TIEMPO_CHECKEO_HELADERA);
    } while (cocinaAbierta);

    printf("Se cerró la cocina. Fin de la jornada laboral.\n");
    // Cleanup
    munmap(sharedData, sizeof(SharedData));
    close(shmFd);
    desvincularSemaforosYMemoriaCompartida();

    return 0;
}