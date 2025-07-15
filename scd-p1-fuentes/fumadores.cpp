// Ramón

#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "scd.h"
#include <vector>

using namespace std ;
using namespace scd ;

// numero de fumadores 
const int num_fumadores = 3 ;

Semaphore mostr_vacio(1);
vector<Semaphore> ingr_disp;

//-------------------------------------------------------------------------
// Función que simula la acción de producir un ingrediente, como un retardo
// aleatorio de la hebra (devuelve número de ingrediente producido)

int producir_ingrediente()
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_produ( aleatorio<10,100>() );

   // informa de que comienza a producir
   cout << "Estanquero : empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
   this_thread::sleep_for( duracion_produ );

   const int num_ingrediente = aleatorio<0,num_fumadores-1>() ;

   // informa de que ha terminado de producir
   cout << "Estanquero : termina de producir ingrediente " << num_ingrediente << endl;

   return num_ingrediente ;
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(  )
{
   int i;
   while (true){
      i = producir_ingrediente();
      sem_wait(mostr_vacio);
      cout << "    Puesto ingr: " << i << endl;
      sem_signal(ingr_disp[i]);
   }
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar

    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar

    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;

}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador )
{
   while( true )
   {
      sem_wait(ingr_disp[num_fumador]);
      cout << "    Retirado ingr: " << num_fumador << endl;
      sem_signal(mostr_vacio);
      fumar(num_fumador);
   }
}

//----------------------------------------------------------------------

int main()
{
   // Inicializa el semáforo de cada ingrediente
   for (int i=0; i<num_fumadores; i++)
      ingr_disp.push_back(Semaphore(0));


   // Crea la hebra estanquero
   thread estanquero(funcion_hebra_estanquero);

   // Crea las hebras de los fumadores
   thread fumadores[num_fumadores];
   for (int i=0; i<num_fumadores; i++)
      fumadores[i] = thread(funcion_hebra_fumador,i);


   // Espera a las hebras, a pesar de que estas no terminan nunca
   estanquero.join();
   for (int i=0; i<num_fumadores; i++)
      fumadores[i].join();
}
