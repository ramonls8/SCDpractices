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
   np = 5,  //Nº hebras productoras
   nc = 3;  //Nº hebras consumidoras
int
   items_prod_por_hebra[np] = {0};  // Nº de items que cada hebra ha producido
const int
   max_items_prod_por_hebra = num_items / np,   // Nº de items que debe producir cada hebra
   max_items_cons_por_hebra = num_items / nc;   // Nº de items que debe consumir cada hebra

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
   cout << "hebra productora, produce " << valor_producido << endl << flush ;
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
// clase para monitor buffer, version FIFO, semántica SC, multiples prod/cons

class ProdConsMu : public HoareMonitor
{
 private:
 static const int           // constantes ('static' ya que no dependen de la instancia)
   num_celdas_total = 10;   //   núm. de entradas del buffer
 int                        // variables permanentes
   buffer[num_celdas_total],//   buffer de tamaño fijo, con los datos
   primera_libre,           //   indice de celda de la próxima inserción ( == número de celdas ocupadas)
   primera_ocupada,
   num_ocupadas,
   multiplos5impresos = 0,  // Nº de múltiplos de 5 insertados e impresos
   multiplos5insertados = 0;

 CondVar                    // colas condicion:
   ocupadas,                //  cola donde espera el consumidor (n>0)
   libres,                  //  cola donde espera el productor  (n<num_celdas_total)
   impresora;

 public:                    // constructor y métodos públicos
   ProdConsMu() ;             // constructor
   int  leer();                // extraer un valor (sentencia L) (consumidor)
   void escribir( int valor ); // insertar un valor (sentencia E) (productor)
   bool metodo_impresora();
} ;
// -----------------------------------------------------------------------------
ProdConsMu::ProdConsMu(  ){
   primera_libre = 0;
   primera_ocupada = 0;
   ocupadas      = newCondVar();
   libres        = newCondVar();
   impresora     = newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato
int ProdConsMu::leer(  ){
   // esperar bloqueado hasta que 0 < primera_libre
   if ( num_ocupadas == 0 )
      ocupadas.wait();

   assert(0 < num_ocupadas);

   // hacer la operación de lectura, actualizando estado del monitor
   const int valor = buffer[primera_ocupada];
   num_ocupadas--;
   primera_ocupada = (primera_ocupada + 1) % num_celdas_total;
   
   // señalar al productor que hay un hueco libre, por si está esperando
   libres.signal();

   // devolver valor
   return valor ;
}
// -----------------------------------------------------------------------------
void ProdConsMu::escribir( int valor ){
   // esperar bloqueado hasta que primera_libre < num_celdas_total
   if ( num_ocupadas == num_celdas_total )
      libres.wait();

   assert( num_ocupadas < num_celdas_total );

   // hacer la operación de inserción, actualizando estado del monitor
   buffer[primera_libre] = valor ;
   num_ocupadas++;
   primera_libre = (primera_libre + 1) % num_celdas_total;

   // Añade el nº de múltiplos de 5 y llama a la impresora
   if (valor%5 == 0){
      multiplos5insertados++;
      impresora.signal();
   }

   // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   ocupadas.signal();
}
// -----------------------------------------------------------------------------
bool ProdConsMu::metodo_impresora(){
   // Si al inicio del método metodo_impresora la hebra impresora detecta que ya se han
   // insertado en el buffer todos los múltiplos de 5 que se tenían que insertar, devuelve false
   if (multiplos5insertados == num_items/5)
      return false;
   // Espera si aún no se han insertado nuevos
   if (multiplos5impresos == multiplos5insertados)
      impresora.wait();
   
   // Imprime el nuevo número de insertados
   cout << "        Impresora: " << multiplos5insertados - multiplos5impresos
        << " nuevos múltiplos de 5" << endl;
   
   // Actualiza el número de impresos
   multiplos5impresos = multiplos5insertados;

   return true;
}




// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora( MRef<ProdConsMu> monitor, unsigned hebra)
{
   for( unsigned i = 0 ; i < max_items_prod_por_hebra ; i++ )
   {
      int valor = producir_dato( hebra ) ;
      monitor->escribir( valor );
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora( MRef<ProdConsMu>  monitor, unsigned hebra)
{
   for( unsigned i = 0 ; i < max_items_cons_por_hebra ; i++ )
   {
      int valor = monitor->leer();
      consumir_dato( valor ) ;
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_impresora(MRef<ProdConsMu> monitor){
   while (monitor->metodo_impresora()){};
}
// -----------------------------------------------------------------------------

int main()
{
   cout << "--------------------------------------------------------------------" << endl
        << "Problema del productor-consumidor únicos (Monitor SU, buffer LIFO). " << endl
        << "--------------------------------------------------------------------" << endl
        << flush ;

   // crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
   MRef<ProdConsMu> monitor = Create<ProdConsMu>() ;



   // LLama a cada hebra
   thread hebras_productoras[np],
          hebras_consumidoras[nc],
          hebra_impresora(funcion_hebra_impresora,monitor);

   for (unsigned i=0; i<np; i++)
      hebras_productoras[i] = thread(funcion_hebra_productora,monitor,i);
   
   for (unsigned i=0; i<nc; i++)
      hebras_consumidoras[i] = thread(funcion_hebra_consumidora,monitor,i);

   // Espera a las hebras
   for (unsigned i=0; i<np; i++)
      hebras_productoras[i].join();
   
   for (unsigned i=0; i<nc; i++)
      hebras_consumidoras[i].join();
   
   hebra_impresora.join();

   test_contadores() ;
}
