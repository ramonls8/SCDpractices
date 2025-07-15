// Ramón

#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "scd.h"

using namespace std ;
using namespace scd ;

//**********************************************************************
// Variables globales

const unsigned 
   num_items = 40 ,   // número de items
	tam_vec   = 10 ;   // tamaño del buffer
unsigned  
   cont_prod[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha producido.
   cont_cons[num_items] = {0}; // contadores de verificación: para cada dato, número de veces que se ha consumido.

// Versión LIFO
unsigned
   buffer[tam_vec] = {0},
   primera_libre = 0;
Semaphore
   libres = tam_vec,
   ocupados = 0,
   exclusion_mutua = 1;

// Versión con varias hebras
const unsigned
   np = 4,  //Nº hebras productoras
   nc = 4;  //Nº hebras consumidoras
unsigned
   siguiente_dato[np] = {0} ;  // Siguiente dato a producir en 'producir_dato'


//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

unsigned producir_dato(unsigned hebra)
{
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   const unsigned dato_producido = siguiente_dato[hebra] ;
   siguiente_dato[hebra]++ ;
   cont_prod[dato_producido] ++ ;
   cout << "producido: " << dato_producido << endl << flush ;
   return dato_producido ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato , unsigned hebra)
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

void  funcion_hebra_productora( unsigned hebra )
{
   const unsigned proc_inicial = hebra * (num_items / np);
   const unsigned proc_final = (hebra+1) * (num_items / np);

   for( unsigned i = proc_inicial; i < proc_final; i++ )
   {
      int dato = producir_dato(hebra) ;

      sem_wait(libres);             // Espera a que haya alguna libre
      assert(0<=primera_libre && primera_libre<tam_vec);
      
      sem_wait(exclusion_mutua);
      buffer[primera_libre++] = dato;
      sem_signal(exclusion_mutua);

      cout << "Insertado en buffer: " << dato << endl;
      sem_signal(ocupados);         // Añade un ocupado
   }
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora( unsigned hebra )
{
   const unsigned items_por_hebra = num_items / nc;

   for( unsigned i = 0; i < items_por_hebra; i++ )
   {
      int dato ;
      
      sem_wait(ocupados);
      // "<=tam_vec" ya que tras "--primera_libre" quedará "<tam_vec"
      assert(0<=primera_libre && primera_libre<=tam_vec);

      sem_wait(exclusion_mutua);
      dato = buffer[--primera_libre];
      sem_signal(exclusion_mutua);

      cout << "                  Extraído de buffer: " << dato << endl;
      sem_signal(libres);

      consumir_dato( dato , hebra) ;
    }
}
//----------------------------------------------------------------------

int main()
{
   // Inicializa los primeros datos que se producirán en producir_dato
   for (int i=0; i<np; i++)
      siguiente_dato[i] = i * (num_items / np);
   
   
   
   
   cout << "-----------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (solución LIFO)." << endl
        << "------------------------------------------------------------------" << endl
        << flush ;


   // LLama a cada hebra
   thread hebras_productoras[np],
          hebras_consumidoras[nc];

   for (int i=0; i<np; i++)
      hebras_productoras[i] = thread(funcion_hebra_productora,i);
   
   for (int i=0; i<nc; i++)
      hebras_consumidoras[i] = thread(funcion_hebra_consumidora,i);

   for (int i=0; i<np; i++)
      hebras_productoras[i].join();
   
   for (int i=0; i<nc; i++)
      hebras_consumidoras[i].join();

   test_contadores();
}
