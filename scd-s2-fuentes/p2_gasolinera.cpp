// Ramón

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono> // duraciones (duration), unidades de tiempo
#include "scd.h"

using namespace std;
using namespace scd;
using namespace chrono;

// Tipos de coche
enum coche {
    GASOLINA, GASOIL
};
// Cerrojo para la salida
mutex salida;

//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------

// Clase monitor Gasolinera
class Gasolinera : public HoareMonitor{
    private:
        unsigned
            surtidores_gasoil_libre = 2,
            surtidores_gasolina_libre = 3;
        CondVar
            esperar_gasoil,
            esperar_gasolina;
    public:
        Gasolinera();
        void entra_coche_gasoil(int i);
        void entra_coche_gasolina(int i);
        void sale_coche_gasoil(int i);
        void sale_coche_gasolina(int i);
};
//-------------------------------------------------------------------------------
Gasolinera::Gasolinera(){
    esperar_gasoil = newCondVar();
    esperar_gasolina = newCondVar();
}
//-------------------------------------------------------------------------------
void Gasolinera::entra_coche_gasoil(int i){
    // Espera a tener sutidores libres
    if (surtidores_gasoil_libre == 0)
        esperar_gasoil.wait();

    // Toma un surtidor
    surtidores_gasoil_libre--;

    // Muestra un mensaje
    cout << "Coche " << i << " gasoil: entra a repostar. Quedan "
         << surtidores_gasoil_libre << " surtidores de gasoil libres." << endl;
}
//-------------------------------------------------------------------------------
void Gasolinera::entra_coche_gasolina(int i){
    // Espera a tener sutidores libres
    if (surtidores_gasolina_libre == 0)
        esperar_gasolina.wait();

    // Toma un surtidor
    surtidores_gasolina_libre--;

    // Muestra un mensaje
    cout << "Coche " << i << " gasolina: entra a repostar. Quedan "
         << surtidores_gasolina_libre << " surtidores de gasolina libres." << endl;

}
//-------------------------------------------------------------------------------
void Gasolinera::sale_coche_gasoil(int i){
    // Deja un surtidor
    surtidores_gasoil_libre++;

    // Muestra un mensaje
    cout << "Coche " << i << " gasoil: sale de repostar. Quedan "
         << surtidores_gasoil_libre << " surtidores de gasoil libres." << endl;

    // Manda señal para otros coches
    esperar_gasoil.signal();
}
//-------------------------------------------------------------------------------
void Gasolinera::sale_coche_gasolina(int i){
    // Deja un surtidor
    surtidores_gasolina_libre++;

    // Muestra un mensaje
    cout << "Coche " << i << " gasolina: sale de repostar. Quedan "
         << surtidores_gasolina_libre << " surtidores de gasolina libres." << endl;

    // Manda señal para otros coches
    esperar_gasolina.signal();
}


//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------

void repostar(int num_coche, coche tipo){
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
void esperar(int num_coche, coche tipo){
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
//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------

void HebraCocheGasolina(int i, MRef<Gasolinera> monitor){
    while(true){
        monitor->entra_coche_gasolina(i);
        repostar(i,coche::GASOLINA);
        monitor->sale_coche_gasolina(i);
        esperar(i,coche::GASOLINA);
    }
}
//-------------------------------------------------------------------------------
void HebraCocheGasoil(int i, MRef<Gasolinera> monitor){
    while(true){
        monitor->entra_coche_gasoil(i);
        repostar(i,coche::GASOIL);
        monitor->sale_coche_gasoil(i);
        esperar(i,coche::GASOIL);
    }
}

//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------

int main()
{
    // Nº de coches
    unsigned
        coches_gasolina = 6,
        coches_gasoil = 4;
    
    // Monitor
    MRef<Gasolinera> monitor = Create<Gasolinera>();

    // Hebras
    thread hebra_coches[coches_gasolina+coches_gasoil];
    
    // Ejecuta las hebras para gasolina y gasoil
    for(int i=0; i<coches_gasolina+coches_gasoil;i++)
        if (i<coches_gasolina)
            hebra_coches[i] = thread(HebraCocheGasolina,i,monitor);
        else
            hebra_coches[i] = thread(HebraCocheGasoil,i,monitor);

    // Espera a la finalización de hebras
    for(int i=0; i<coches_gasolina+coches_gasoil; i++)
        hebra_coches[i].join();

    return 0;
}