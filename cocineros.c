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
    sem_unlink("/semMesada");
    sem_unlink("/semMozoMesada");
    sem_unlink("/semInicializacion");
    shm_unlink("/shared_memory");
    sem_unlink("/semRespotero");
    sem_unlink("/semHeladeraEnUso");
}

int main()
{
    desvincularSemaforosYMemoriaCompartida();
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
    // Protege a platos preparados en el día
    sem_t *semPlatosEnElDia = sem_open("/semPlatosEnElDia", O_CREAT, 0666, 1);
    // Protege a la variable de inicialización
    sem_t *semInicializacion = sem_open("/semInicializacion", O_CREAT | O_EXCL, 0666, 1);
    // Protege a el acceso a la mesada
    sem_t *semMesada = sem_open("/semMesada", O_CREAT, 0666, 1); 

    if(semInicializacion != SEM_FAILED){
        initializeSharedMemory(sharedData);
    }
    else{
        while(!sharedData->inicializado){
            printf("Esperando inicialización\n");
            sleep(2);
        }
    }
    char nombre []= "Cocinero X";
    bool cocinaAbierta;

    int pid = fork();
    if (pid == 0)
    {
        nombre[9] = '1';
        pid = fork();
        if (pid > 0)
        {
            nombre[9] = '2';
        }
        if (pid < 0)
        {
            perror("Error en el fork.\n");
            exit(-1);
        }
    }
    else if (pid > 0)
    {
        nombre[9] = '3';
    }
    if (pid < 0)
    {
        perror("Error en el fork.\n");
        exit(-1);
    }


    do
    {
        sleep(TIEMPO_DE_COCCION);
        sem_wait(semPlatosEnElDia);
        printf("%s: Platos preparados en el día: %d\n", nombre, sharedData->platosPreparadosEnElDia);
        cocinaAbierta = sharedData->platosPreparadosEnElDia < MAX_PLATOS_DIA;
        if (cocinaAbierta)
        {
            sem_wait(semMesada);
            if(sharedData->platosEnMesada < MAX_PLATOS_MESADA){
                printf("%s: Hay %d platos, se cocina uno más.\n", nombre, sharedData->platosEnMesada);
                sharedData->platosEnMesada++;
                sharedData->platosPreparadosEnElDia++;
            }
            else{
                printf("%s: La mesada está llena.\n", nombre);
            }
            sem_post(semMesada);
        }
        sem_post(semPlatosEnElDia);
    } while (cocinaAbierta);
    printf("%s: Se cerró la cocina. Se prepararon: %d platos.\n", nombre, sharedData->platosPreparadosEnElDia);        
    // Cleanup
    munmap(sharedData, sizeof(SharedData));
    close(shmFd);
    desvincularSemaforosYMemoriaCompartida();

    return 0;
}