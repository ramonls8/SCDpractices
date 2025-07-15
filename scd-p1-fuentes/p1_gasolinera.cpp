// Ramón

#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "scd.h"

using namespace std;
using namespace scd;
using namespace chrono;

// Semáforos
Semaphore
    surtidores_gasolina(3),     // Semáforo con los surtidores de gasolina libres
    surtidores_gasoil(2);       // Semáforo con los surtidores de gasoil libres
// Tipos de coche
enum coche {
    GASOLINA, GASOIL
};
// Cerrojo para la salida
mutex salida;

//-------------------------------------------------------------------------------

void repostar(int num_coche, coche tipo)
{
    milliseconds duration(aleatorio<20,100>());

    salida.lock();
    cout << "Coche " << num_coche << " " << (tipo==coche::GASOLINA ? "gasolina: " : "gasoil: ") << "empieza a repostar" << endl;
    salida.unlock();

    this_thread::sleep_for(duration);

    salida.lock();
    cout << "Coche " << num_coche << " " << (tipo==coche::GASOLINA ? "gasolina: " : "gasoil: ") << "termina de repostar" << endl;
    salida.unlock();
}

//-------------------------------------------------------------------------------

void esperar(int num_coche, coche tipo)
{
    milliseconds duration(aleatorio<20,100>());

    salida.lock();
    cout << "Coche " << num_coche << " " << (tipo==coche::GASOLINA ? "gasolina: " : "gasoil: ") << "espera fuera" << endl;
    salida.unlock();

    this_thread::sleep_for(duration);

    salida.lock();
    cout << "Coche " << num_coche << " " << (tipo==coche::GASOLINA ? "gasolina: " : "gasoil: ") << " termina de esperar" << endl;
    salida.unlock();
}

//-------------------------------------------------------------------------------

void funcion_hebra_gasolina(int n)
{
    while(true){
        sem_wait(surtidores_gasolina);
        repostar(n,coche::GASOLINA);
        sem_signal(surtidores_gasolina);
        esperar(n,coche::GASOLINA);
    }
}
//-------------------------------------------------------------------------------

void funcion_hebra_gasoil(int n)
{
    while(true){
        sem_wait(surtidores_gasoil);
        repostar(n,coche::GASOIL);
        sem_signal(surtidores_gasoil);
        esperar(n,coche::GASOIL);
    }
}
//-------------------------------------------------------------------------------

int main()
{
    unsigned
        coches_gasolina = 6,
        coches_gasoil = 4;

    thread hebra_coches[coches_gasolina+coches_gasoil];
    
    // Ejecuta las hebras para gasolina y gasoil
    for(int i=0; i<coches_gasolina+coches_gasoil;i++)
        if (i<coches_gasolina)
            hebra_coches[i] = thread(funcion_hebra_gasolina,i);
        else
            hebra_coches[i] = thread(funcion_hebra_gasoil,i);

    // Espera a la finalización de hebras
    for(int i=0; i<coches_gasolina+coches_gasoil; i++)
        hebra_coches[i].join();

    return 0;
}