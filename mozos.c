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
#include <time.h>
#include "cocina.h"

void desvincularSemaforosYMemoriaCompartida()
{
    sem_unlink("/semPlatosEnElDia");
    sem_unlink("/semMesada");
    sem_unlink("/semMozoMesada");
    sem_unlink("/semInicializacion");
    shm_unlink("/shared_memory");
}

// Se define albóndigas como 0 y flanes como 1
void generarPedido(int *tipoComida, int *cantidad)
{
    srand(time(NULL));
    *tipoComida = rand() % 2;
    switch (*tipoComida)
    {
    case 0:
        *cantidad = ((rand() % MAX_ALBONDIGAS_BANDEJA) + 1);
        break;
    case 1:
        *cantidad = ((rand() % MAX_FLANES_BANDEJA) + 1);
        break;
    default:
        break;
    };
}

int main()
{
    int cantidad, tipoComida = 0;
    // desvincularSemaforosYMemoriaCompartida();
    //  Crea o abre el objeto de memoria compartida
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
    // Protege a la variable de inicialización
    sem_t *semInicializacion = sem_open("/semInicializacion", O_CREAT | O_EXCL, 0666, 1);
    // Protege a el acceso a la mesada
    sem_t *semMesada = sem_open("/semMesada", O_CREAT, 0666, 1);
    // Protege a flanes en freezer
    sem_t *semHeladeraEnUso = sem_open("/semHeladeraEnUso", O_CREAT, 0666, 1);
    // Protege a platos preparados en el día
    sem_t *semPlatosEnElDia = sem_open("/semPlatosEnElDia", O_CREAT, 0666, 1);
    // Protege la mesada para que la use un solo mozo a la vez
    sem_t *semMozoMesada = sem_open("/semMozoMesada", O_CREAT, 0666, 1);
    // Despierta al repostero
    sem_t *semRespotero = sem_open("/semRespotero", O_CREAT, 0666, 0);

    if (semInicializacion != SEM_FAILED)
    {
        initializeSharedMemory(sharedData);
    }
    else
    {
        while (!sharedData->inicializado)
        {
            printf("Esperando inicialización\n");
            sleep(2);
        }
    }
    char nombre[] = "Mozo  ";
    bool cocinaAbierta;
    int cantidadLocal = 0;

    int pid = fork();
    if (pid == 0)
    {
        nombre[5] = '1';
        pid = fork();
        if (pid > 0)
        {
            nombre[5] = '2';
        }
        if (pid < 0)
        {
            perror("Error en el fork.\n");
            exit(-1);
        }
    }
    else if (pid > 0)
    {
        nombre[5] = '3';
        pid = fork();
        if (pid > 0)
        {
            nombre[5] = '4';
            pid = fork();
            if (pid > 0)
            {
                nombre[5] = '5';
            }
            if (pid < 0)
            {
                perror("Error en el fork.\n");
                exit(-1);
            }
        }
        if (pid < 0)
        {
            perror("Error en el fork.\n");
            exit(-1);
        }
    }
    if (pid < 0)
    {
        perror("Error en el fork.\n");
        exit(-1);
    }

    do
    {
        sleep(TIEMPO_ENTRE_PEDIDOS);
        generarPedido(&tipoComida, &cantidad);
        cantidadLocal = cantidad;
        sem_wait(semPlatosEnElDia);
        cocinaAbierta = sharedData->platosPreparadosEnElDia < MAX_PLATOS_DIA;
        if (cocinaAbierta)
        {
            if (tipoComida == ALBONDIGA &&
                cantidad <= sharedData->platosEnMesada +
                                (MAX_PLATOS_DIA - sharedData->platosPreparadosEnElDia))
            {
                if (sem_trywait(semMozoMesada))
                {

                    printf("%s: Se recibió pedido de %d albóndiga/s\n", nombre, cantidadLocal);
                    while (cantidad > 0)
                    {
                        if (sharedData->platosEnMesada < cantidad)
                        {
                            cantidad -= sharedData->platosEnMesada;
                            sharedData->platosEnMesada = 0;
                            sem_post(semPlatosEnElDia);
                            sleep(TIEMPO_DE_COCCION);
                            sem_wait(semPlatosEnElDia);
                        }
                        else
                        {
                            sharedData->platosEnMesada -= cantidad;
                            cantidad = 0;
                            printf("%s: Se entregó el pedido de %d albóndiga/s\n", nombre, cantidadLocal);
                            sem_post(semMozoMesada);
                            sem_post(semPlatosEnElDia);
                        }
                    }
                }
                else
                {
                    sem_post(semPlatosEnElDia);
                }
            }
            else if (tipoComida == ALBONDIGA)
            {
                printf("%s: Se rechaza el pedido de %d albóndiga/s porque no alcanzan.\n", nombre, cantidad);
                sem_post(semPlatosEnElDia);
            }
            else
            {
                sem_post(semPlatosEnElDia);
                sem_wait(semHeladeraEnUso);
                printf("%s: Se recibió pedido de %d flan/es\n", nombre, cantidadLocal);

                if (cantidad <= sharedData->flanesEnFeezer)
                {
                    sharedData->flanesEnFeezer -= cantidad;
                    cantidad = 0;
                    printf("%s: Se entregó el pedido de %d flan/es\n", nombre, cantidadLocal);
                }
                else
                {
                    cantidad -= sharedData->flanesEnFeezer;
                    sharedData->flanesEnFeezer = 0;

                    sem_post(semRespotero);
                    sleep(TIEMPO_CHECKEO_HELADERA);
                    sem_wait(semRespotero);

                    if (sharedData->flanesEnFeezer != 0)
                    {
                        sharedData->flanesEnFeezer -= cantidad;
                        cantidad = 0;
                        printf("%s: Se entregó el pedido de %d flan/es\n", nombre, cantidadLocal);
                    }
                    else
                    {
                        printf("%s: Se rechaza el pedido de %d flan/es porque no alcanzan.\n", nombre, cantidadLocal);
                    }
                }
                printf("%s: Quedan %d flan/es en la heladera\n", nombre, sharedData->flanesEnFeezer);
                sem_post(semHeladeraEnUso);
            }
        }
        else
        {
            if (tipoComida == ALBONDIGA)
            {
                sem_wait(semMesada);
                printf("%s: Se recibió pedido de %d albóndigas\n", nombre, cantidad);
                if (sharedData->platosEnMesada < cantidad)
                {
                    printf("%s: Se rechaza el pedido de %d albóndiga/s porque no alcanzan.\n", nombre, cantidad);
                }
                else
                {
                    sharedData->platosEnMesada -= cantidad;
                    printf("%s: Se entregó el pedido de %d albóndiga/s.\n", nombre, cantidad);
                }
                sem_post(semMesada);
                sem_post(semPlatosEnElDia);
            }
            else
            {
                sem_post(semPlatosEnElDia);
                sem_wait(semHeladeraEnUso);
                if (sharedData->flanesEnFeezer < cantidad)
                {
                    printf("%s: Se rechaza el pedido de %d flan/es porque no alcanzan.\n", nombre, cantidad);
                }
                else
                {
                    sharedData->flanesEnFeezer -= cantidad;
                    printf("%s: Se entregó el pedido de %d flan/es.\n", nombre, cantidad);
                }
                printf("%s: Quedan %d flan/es en la heladera\n", nombre, sharedData->flanesEnFeezer);
                sem_post(semHeladeraEnUso);
            }
        }
    } while (cocinaAbierta);
    sem_post(semRespotero);
    printf("%s: Se cerró la cocina. Se prepararon: %d platos.\n", nombre, sharedData->platosPreparadosEnElDia);
    // Cleanup
    // munmap(sharedData, sizeof(SharedData));
    // close(shmFd);
    // desvincularSemaforosYMemoriaCompartida();

    return 0;
}