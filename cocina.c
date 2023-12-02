#include "cocina.h"

void initializeSharedMemory(SharedData *sd)
{
    sd->platosPreparadosEnElDia = 0;
    sd->flanesEnFeezer = 25;
    sd->platosEnMesada = 0;
    sd->inicializado = true;
}