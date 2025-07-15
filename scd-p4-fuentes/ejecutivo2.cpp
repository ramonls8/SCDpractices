// Ramón

/*
   Datos de las tareas:
      Ta.  T    C
      ------------
      A  500   100
      B  500   150
      C  1000  200
      D  2000  240
      ------------
   
   El ciclo principal dura Tm = mcm(500,500,1000,2000) = 2000 ms

   Planificación:   ( Ts = 500 ms )
   | A B C   | A B D   | A B C   | A B     |

   Preguntas:
   1) Ciclo 1 = Ciclo 3 = A+B+C = 100+150+200 = 450 ms
      Ciclo 2 =         = A+B+D = 100+150+240 = 490 ms
      Ciclo 4 =         = A+B   = 100+150     = 250 ms
      El tiempo de espera mínimo que queda en un ciclo secundario es 10 ms en el
      caso del segundo ciclo, donde se ejecuta A + B + D = 100ms + 150ms + 240ms = 490ms,
      quedando 10 ms para llegar a los 500 ms de Ts.
   
   2) Si D tuviera un tiempo de cómputo de 250 ms, solo afectaría al segundo ciclo,
      que es el único en el que aparece D. El tiempo que tardarían las tareas sería
      A + B + D = 100ms + 150ms + 250ms = 500ms, coincidiendo con Ts, de forma que
      sería planificable si no hay retrasos inesperados y el tiempo de espera en
      este ciclo secundario sería de 0 ms.
*/

// -----------------------------------------------------------------------------

#include <string>
#include <iostream> // cout, cerr
#include <thread>
#include <chrono>   // utilidades de tiempo
#include <ratio>    // std::ratio_divide

using namespace std ;
using namespace std::chrono ;
using namespace std::this_thread ;

// tipo para duraciones en segundos y milisegundos, en coma flotante:
//typedef duration<float,ratio<1,1>>    seconds_f ;
typedef duration<float,ratio<1,1000>> milliseconds_f ;

// -----------------------------------------------------------------------------
// tarea genérica: duerme durante un intervalo de tiempo (de determinada duración)

void Tarea( const std::string & nombre, milliseconds tcomputo )
{
   cout << "   Comienza tarea " << nombre << " (C == " << tcomputo.count() << " ms.) ... " ;
   sleep_for( tcomputo );
   cout << "fin." << endl ;
}

// -----------------------------------------------------------------------------
// tareas concretas del problema:

void TareaA() { Tarea( "A", milliseconds(100) );  }
void TareaB() { Tarea( "B", milliseconds(150) );  }
void TareaC() { Tarea( "C", milliseconds(200) );  }
void TareaD() { Tarea( "D", milliseconds(240) );  }

// -----------------------------------------------------------------------------
// implementación del ejecutivo cíclico:

int main( int argc, char *argv[] )
{
   // Ts = duración del ciclo secundario (en unidades de milisegundos, enteros)
   const milliseconds Ts_ms( 500 );

   // ini_sec = instante de inicio de la iteración actual del ciclo secundario
   time_point<steady_clock> ini_sec = steady_clock::now();

   while( true ) // ciclo principal
   {
      cout << endl
           << "---------------------------------------" << endl
           << "Comienza iteración del ciclo principal." << endl ;

      for( int i = 1 ; i <= 4 ; i++ ) // ciclo secundario (4 iteraciones)
      {
         cout << endl << "Comienza iteración " << i << " del ciclo secundario." << endl ;

         switch( i )
         {
            case 1 : TareaA(); TareaB(); TareaC();           break ;
            case 2 : TareaA(); TareaB(); TareaD();           break ;
            case 3 : TareaA(); TareaB(); TareaC();           break ;
            case 4 : TareaA(); TareaB();                     break ;
         }

         // calcular el siguiente instante de inicio del ciclo secundario
         ini_sec += Ts_ms ;

         // esperar hasta el inicio de la siguiente iteración del ciclo secundario
         sleep_until( ini_sec );

         // Retraso respecto del instante final esperado
         time_point<steady_clock> fin_sec = steady_clock::now();
         
         cout << "El ciclo secundario se ha retrasado "
              << milliseconds_f(fin_sec - ini_sec).count() << " milisegundos" << endl;
      }
   }
}
