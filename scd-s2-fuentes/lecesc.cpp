// Ramón

#include <iostream>
#include <thread>
#include <chrono> // duraciones (duration), unidades de tiempo
#include "scd.h"

using namespace std;
using namespace scd;

// numero de escritores y lectores
const int
   num_esc_total = 4,
   num_lec_total = 5;

mutex salida;


// *****************************************************************************
// Clase monitor LecEsc
class LecEsc : public HoareMonitor {
   private:
      bool escrib = false;                   // true si un escritor está escribiendo, false si no
      unsigned n_lec = 0;                    // Nº de lectores leyendo actualmente

      CondVar lectura;                       // Espera de los lectores
      CondVar escritura;                     // Espera de los escritores

   public:
      LecEsc();                              // Constructor
      void ini_lectura();
      void fin_lectura();
      void ini_escritura();
      void fin_escritura();
};
//-------------------------------------------------------------------------
LecEsc::LecEsc(){
   lectura = newCondVar();
   escritura = newCondVar();
}
//-------------------------------------------------------------------------
void LecEsc::ini_lectura(){
   if (escrib)          // Espera a que termine el escritor
      lectura.wait();
   n_lec++;             // Se añade como lector
   lectura.signal();    // Despierta a otros lectores
}
//-------------------------------------------------------------------------
void LecEsc::fin_lectura(){
   n_lec--;
   if (n_lec == 0)      // Si no hay lectores, deja escribir
      escritura.signal();
}
//-------------------------------------------------------------------------
void LecEsc::ini_escritura(){
   if (n_lec > 0)
      escritura.wait(); // Espera a los lectores
   escrib = true;
}
//-------------------------------------------------------------------------
void LecEsc::fin_escritura(){
   escrib = false;
   
   // Prioridad a los lectores
   if (!lectura.empty())
      lectura.signal();
   else
      escritura.signal();
}


// *****************************************************************************


//------------------------------------------------------------
void leer(int i){
    chrono::milliseconds duration(aleatorio<50,100>());
    salida.lock();
    cout << "Lector " << i << " leyendo (" << duration.count() << " ms)" << endl;
    salida.unlock();
    this_thread::sleep_for(duration);
}
//------------------------------------------------------------
void escribir(int i){
    chrono::milliseconds duration(aleatorio<50,100>());
    salida.lock();
    cout << "Escritor " << i << " escribiendo (" << duration.count() << " ms)" << endl;
    salida.unlock();
    this_thread::sleep_for(duration);
}
//------------------------------------------------------------
void esperar_escritor(int i){
    chrono::milliseconds duration(aleatorio<50,100>());
    salida.lock();
    cout << "        Escritor " << i << " descanso (" << duration.count() << " ms)" << endl;
    salida.unlock();
    this_thread::sleep_for(duration);
}
//------------------------------------------------------------
void esperar_lector(int i){
    chrono::milliseconds duration(aleatorio<50,100>());
    salida.lock();
    cout << "        Lector " << i << " descanso (" << duration.count() << " ms)" << endl;
    salida.unlock();
    this_thread::sleep_for(duration);
}
//----------------------------------------------------------------------


//----------------------------------------------------------------------
// función que ejecuta la hebra del escritor
void funcion_hebra_escritor(MRef<LecEsc> monitor, int i){
   while (true){
      monitor->ini_escritura();
      escribir(i);
      monitor->fin_escritura();
      esperar_escritor(i);
   }
}
//----------------------------------------------------------------------
// función que ejecuta la hebra del lector
void  funcion_hebra_lector(MRef<LecEsc> monitor, int i){
   while(true){
      monitor->ini_lectura();
      leer(i);
      monitor->fin_lectura();
      esperar_lector(i);
   }
}
//----------------------------------------------------------------------


int main()
{
   // Crea el monitor
   MRef<LecEsc> monitor = Create<LecEsc>();
   

   // Crea las hebras escritores y escritores
   thread escritores[num_esc_total];
   thread lectores[num_lec_total];

   for (int i=0; i<num_esc_total; i++)
      escritores[i] = thread(funcion_hebra_escritor,monitor,i);
   for (int i=0; i<num_lec_total; i++)
      lectores[i] = thread(funcion_hebra_lector,monitor,i);


   // Espera a las hebras
   for (int i=0; i<num_esc_total; i++)
      escritores[i].join();
   for (int i=0; i<num_lec_total; i++)
      lectores[i].join();
}
