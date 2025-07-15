// Ramón

#include <iostream>
#include <iomanip>
#include <cassert>
#include <random>
#include <thread>
#include "scd.h"

using namespace std ;
using namespace scd ;

constexpr int
   num_items = 15 ;   // número de items a producir/consumir
   
constexpr int               
   min_ms    = 5,     // tiempo minimo de espera en sleep_for
   max_ms    = 20 ;   // tiempo máximo de espera en sleep_for


mutex
   mtx ;                 // mutex de escritura en pantalla
unsigned
   cont_prod[num_items] = {0}, // contadores de verificación: producidos
   cont_cons[num_items] = {0}; // contadores de verificación: consumidos

// Versión con varias hebras
const unsigned
   np = 3,  //Nº hebras productoras
   nc = 5;  //Nº hebras consumidoras
int
   items_prod_por_hebra[np] = {0};  // Nº de items que cada hebra ha producido
const int
   max_items_prod_por_hebra = num_items / np,   // Nº de items que debe producir cada hebra
   max_items_cons_por_hebra = num_items / nc;   // Nº de items que debe consumir cada hebra

// Buffer y celdas ocupadas
const int
   buffer_size = 10;   //   núm. de entradas del buffer
int
   buffer[buffer_size],//   buffer de tamaño fijo, con los datos
   primera_libre = 0,
   primera_ocupada = 0;


//**********************************************************************
//**********************************************************************
//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato(unsigned hebra)
{
   this_thread::sleep_for( chrono::milliseconds( aleatorio<min_ms,max_ms>() ));

   assert(items_prod_por_hebra[hebra] < max_items_prod_por_hebra);

   const int valor_producido = hebra * max_items_prod_por_hebra + items_prod_por_hebra[hebra];
   items_prod_por_hebra[hebra]++;

   mtx.lock();
   cout << "hebra productora " << hebra << ", produce " << valor_producido << endl << flush ;
   mtx.unlock();

   cont_prod[valor_producido]++ ;
   return valor_producido ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned valor_consumir )
{
   if ( num_items <= valor_consumir )
   {
      cout << " valor a consumir === " << valor_consumir << ", num_items == " << num_items << endl ;
      assert( valor_consumir < num_items );
   }
   cont_cons[valor_consumir] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<min_ms,max_ms>() ));
   mtx.lock();
   cout << "                  hebra consumidora, consume: " << valor_consumir << endl ;
   mtx.unlock();
}
//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." << endl ;

   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      if ( cont_prod[i] != 1 )
      {
         cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {
         cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

// *****************************************************************************
// *****************************************************************************
// *****************************************************************************
// clase para monitor buffer, version FIFO, semántica SC, multiples prod/cons

class ProdConsMuFIFO_NE : public HoareMonitor {
   private:
      int num_libres;
      int num_ocupadas = 0;
      
      CondVar           // colas condicion:
         ocupadas,      // cola donde espera el consumidor (n>0)
         libres;        // cola donde espera el productor  (n<num_celdas_total)

      // Variables para controlar el acceso único de productores y
      // el acceso único de consumidores
      bool
         insertando = false,
         extrayendo = false;
      CondVar
         exclusion_insercion,
         exclusion_extraccion;
   
   public:                          // constructor y métodos públicos
      ProdConsMuFIFO_NE(int buffer_size);  // constructor
      void inicio_insercion(int i, int v);
      void fin_insercion(int i, int v);
      void inicio_extraccion(int i);
      void fin_extraccion(int i, int v);
};
// -----------------------------------------------------------------------------
ProdConsMuFIFO_NE::ProdConsMuFIFO_NE(int buffer_size){
   num_libres        = buffer_size;
   ocupadas          = newCondVar();
   libres            = newCondVar();
   exclusion_insercion  = newCondVar();
   exclusion_extraccion = newCondVar();
}
// -----------------------------------------------------------------------------
void ProdConsMuFIFO_NE::inicio_insercion(int i, int v){
   // Espera a ser el único insertando
   if (insertando)
      exclusion_insercion.wait();
   insertando = true;

   // Espera a que haya libres
   if (num_libres == 0)
      libres.wait();
   num_libres--;

   // Mensaje
   cout << "        hebra productora " << i << ", empieza insercion de: "
        << v << endl;
}
// -----------------------------------------------------------------------------
void ProdConsMuFIFO_NE::fin_insercion(int i, int v){
   // Ocupa una posición
   num_ocupadas++;
   ocupadas.signal();

   // Permite a otros insertar
   insertando = false;
   exclusion_insercion.signal();

   // Mensaje
   cout << "        hebra productora " << i << ", termina insercion de: "
        << v << endl;
}
// -----------------------------------------------------------------------------
void ProdConsMuFIFO_NE::inicio_extraccion(int i){
   // Espera a ser el único extrayendo
   if (extrayendo)
      exclusion_extraccion.wait();
   extrayendo = true;

   // Espera a que haya ocupadas
   if (num_ocupadas == 0)
      ocupadas.wait();
   num_ocupadas--;

   // Mensaje
   cout << "        hebra consumidora " << i << ", empieza extracción" << endl;
}
// -----------------------------------------------------------------------------
void ProdConsMuFIFO_NE::fin_extraccion(int i, int v){
   // Libera una posición
   num_libres++;
   libres.signal();

   // Permite a otros extraer
   extrayendo = false;
   exclusion_extraccion.signal();

   // Mensaje
   cout << "        hebra consumidora " << i << ", termina extracción de: "
        << v << endl;
}

// *****************************************************************************
// *****************************************************************************
// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora( MRef<ProdConsMuFIFO_NE> monitor, unsigned hebra){
   for( unsigned i = 0 ; i < max_items_prod_por_hebra ; i++ ){

      // Produce el valor
      int valor = producir_dato(hebra);
      
      monitor->inicio_insercion(hebra, valor);
      
      // Operación de inserción
      buffer[primera_libre] = valor;
      primera_libre = (primera_libre + 1) % buffer_size;

      monitor->fin_insercion(hebra, valor);
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora( MRef<ProdConsMuFIFO_NE>  monitor, unsigned hebra){
   for( unsigned i = 0 ; i < max_items_cons_por_hebra ; i++ ){

      monitor->inicio_extraccion(hebra);
      
      // Operación de lectura
      const int valor = buffer[primera_ocupada];
      primera_ocupada = (primera_ocupada + 1) % buffer_size;

      monitor->fin_extraccion(hebra, valor);

      consumir_dato( valor ) ;
   }
}
// -----------------------------------------------------------------------------

int main(){
   // crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
   MRef<ProdConsMuFIFO_NE> monitor = Create<ProdConsMuFIFO_NE>(buffer_size);


   // LLama a cada hebra
   thread hebras_productoras[np],
          hebras_consumidoras[nc];

   for (unsigned i=0; i<np; i++)
      hebras_productoras[i] = thread(funcion_hebra_productora,monitor,i);
   
   for (unsigned i=0; i<nc; i++)
      hebras_consumidoras[i] = thread(funcion_hebra_consumidora,monitor,i);

   // Espera a las hebras
   for (unsigned i=0; i<np; i++)
      hebras_productoras[i].join();
   
   for (unsigned i=0; i<nc; i++)
      hebras_consumidoras[i].join();

   test_contadores();
}
