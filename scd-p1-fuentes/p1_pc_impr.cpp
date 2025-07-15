// Ramón

#include <iostream>
#include <cassert>
#include <thread>    // Librería threads
#include <mutex>     // Librería cerrojos
#include <random>
#include "scd.h"     // Generador números aleatorios y scd::Semaphore

using namespace std ;
using namespace scd ;

//**********************************************************************
// Variables globales

const unsigned 
   num_items = 40 ,   // número de items
	tam_vec   = 10 ;   // tamaño del buffer
unsigned  
   cont_prod[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha producido.
   cont_cons[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha consumido.
   siguiente_dato       = 0 ;  // siguiente dato a producir en 'producir_dato' (solo se usa ahí)
Semaphore
   libres(tam_vec),     // Semáforo con las posiciones libres del buffer
   ocupados(0),         // Semáforo con las posiciones ocupadas del buffer
   impresora(0),        // Semáforo para controlar la impresora
   productor(0),        // Semáforo para controlar el productor tras usar la impresora
   exclusion_mutua = 1;
unsigned
   buffer[tam_vec] = {0},
   primera_libre = 0,
   primera_ocupada = 0;

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

unsigned producir_dato()
{
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   const unsigned dato_producido = siguiente_dato ;
   siguiente_dato++ ;
   cont_prod[dato_producido] ++ ;
   cout << "producido: " << dato_producido << endl << flush ;
   return dato_producido ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato )
{
   assert( dato < num_items );
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

   cout << "                  consumido: " << dato << endl ;

}


//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." ;
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  if ( cont_prod[i] != 1 )
      {  cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {  cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

//----------------------------------------------------------------------

void  funcion_hebra_productora(  )
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      int dato = producir_dato() ;
      
      sem_wait(libres);             // Espera a que haya alguna libre
      
      assert(0 <= primera_libre && primera_libre < tam_vec);

      // Semáforo para que la impresora lea "primera_ocupada" correcto,
      // sin que la hebra consumidora se adelante
      sem_wait(exclusion_mutua);

      buffer[primera_libre] = dato;
      primera_libre = (primera_libre + 1) % tam_vec;  // Siguiente posición

      cout << "Insertado de buffer: " << dato << endl;

      // Comprobamos si el dato es múltiplo de 5 y desbloquea la impresora
      if(dato%5 == 0) {
         sem_signal(impresora);  // Desbloqueamos la impresora
         sem_wait(productor);    // Productor espera
      }
      sem_signal(exclusion_mutua);
      

      sem_signal(ocupados);         // Añade un ocupado
   }
}

//----------------------------------------------------------------------

void funcion_hebra_impresora( )
{
   for(int i=0; i<num_items/5; i++){
      sem_wait(impresora);       // Espera a poder imprimir

      int num = primera_libre - primera_ocupada;
      if (num <0) num=-num;                     // Valor absoluto
      cout << "Impresora: " << num << endl;

      sem_signal(productor);     // Desbloquea productor
   }
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora(  )
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      int dato ;
      
      sem_wait(ocupados);
      
      assert(0<=primera_ocupada && primera_ocupada<tam_vec);

      sem_wait(exclusion_mutua);
      dato = buffer[primera_ocupada];                    // Lee el dato
      primera_ocupada = (primera_ocupada + 1) % tam_vec; // Siguiente posición
      sem_signal(exclusion_mutua);

      cout << "                  Extraído de buffer: " << dato << endl;

      sem_signal(libres);
      consumir_dato( dato ) ;
    }
}
//----------------------------------------------------------------------

int main()
{
   cout << "-----------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (solución FIFO)." << endl
        << "------------------------------------------------------------------" << endl
        << flush ;

   thread hebra_productora ( funcion_hebra_productora ),
          hebra_consumidora( funcion_hebra_consumidora ),
          hebra_impresora (funcion_hebra_impresora);

   hebra_productora.join() ;
   hebra_consumidora.join() ;
   hebra_impresora.join() ;

   test_contadores();
}
