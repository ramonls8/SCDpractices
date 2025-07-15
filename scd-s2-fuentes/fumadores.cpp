// Ramón

#include <iostream>
#include <thread>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "scd.h"

using namespace std;
using namespace scd;

// numero de fumadores 
const int num_fumadores = 3;

// *****************************************************************************
// Clase monitor Estanco
class Estanco : public HoareMonitor{
   private:
      const int NOINGR = -1;                 // Valor cuando no hay ingrediente
      int ingrediente = NOINGR;              // Nº del ingrediente disponible, -1 si no es ninguno
      CondVar cola_fumar[num_fumadores];     // Espera de fumadores
      CondVar cola_estanquero;               // Espera del estanquero

   public:
      Estanco();                             // Constructor
      void obtenerIngrediente(int i);
      void ponerIngrediente(int i);
      void esperarRecogidaIngrediente();
};
//-------------------------------------------------------------------------
Estanco::Estanco(){
   cola_estanquero = newCondVar();
   for (int i=0; i<num_fumadores; i++)
      cola_fumar[i] = newCondVar();
}
//-------------------------------------------------------------------------
void Estanco::obtenerIngrediente(int i){
   // Espera a tener el ingrediente
   if (ingrediente != i)
      cola_fumar[i].wait();
   
   cout << "    Retirado ingr: " << i << endl;

   // Deja el mostrador vacío
   ingrediente = NOINGR;
   cola_estanquero.signal();
}
//-------------------------------------------------------------------------
void Estanco::ponerIngrediente(int i){
   // Coloca el ingrediente
   ingrediente = i;
   cout << "    Puesto ingr: " << i << endl;
   cola_fumar[i].signal();
}
//-------------------------------------------------------------------------
void Estanco::esperarRecogidaIngrediente(){
   if (ingrediente != NOINGR)
      cola_estanquero.wait();
}

// *****************************************************************************

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

void funcion_hebra_estanquero(MRef<Estanco> estanco)
{
   while (true){
      int i = producir_ingrediente();
      estanco->ponerIngrediente(i);
      estanco->esperarRecogidaIngrediente();
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
void  funcion_hebra_fumador(MRef<Estanco> estanco, int num_fumador)
{
   while( true )
   {
      estanco->obtenerIngrediente(num_fumador);
      fumar(num_fumador);
   }
}

//----------------------------------------------------------------------

int main()
{
   // Crea el monitor
   MRef<Estanco> estanco = Create<Estanco>();
   
   // Crea la hebra estanquero
   thread estanquero(funcion_hebra_estanquero,estanco);
   // Crea las hebras de los fumadores
   thread fumadores[num_fumadores];
   for (int i=0; i<num_fumadores; i++)
      fumadores[i] = thread(funcion_hebra_fumador,estanco,i);


   // Espera a las hebras
   estanquero.join();
   for (int i=0; i<num_fumadores; i++)
      fumadores[i].join();
}
