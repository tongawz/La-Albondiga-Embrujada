#ifndef COCINA_H
#define COCINA_H

#include<stdbool.h>

#define MAX_PLATOS_DIA 180
#define MAX_MOZOS 5
#define MAX_COCINEROS 3
#define MAX_PLATOS_MESADA 27
#define MAX_HELADERA 25

#define ALBONDIGA 0
#define FLAN 1

#define MAX_ALBONDIGAS_BANDEJA 4
#define MAX_FLANES_BANDEJA 6

#define TIEMPO_CHECKEO_HELADERA 2
#define TIEMPO_DE_COCCION 2
#define TIEMPO_ENTRE_PEDIDOS 1

typedef struct
{
    bool inicializado;
    int flanesEnFeezer; // Número de flanes en el freezer
    int platosPreparadosEnElDia;  
    int platosEnMesada;
} SharedData;

// Function declaration for initialization
void initializeSharedMemory(SharedData * sd);

#endif