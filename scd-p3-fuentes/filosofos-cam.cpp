// Ramón

/*
make filosofos-cam_mpi_exe; mpirun -oversubscribe -np 11 filosofos-cam_mpi_exe
*/
// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
// -----------------------------------------------------------------------------


#include <mpi.h>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <iostream>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int
   num_filosofos = 5 ,              // número de filósofos 
   num_filo_ten  = 2*num_filosofos, // número de filósofos y tenedores 
   num_procesos  = num_filo_ten+1,  // número de procesos total (por ahora solo hay filo y ten)
   id_camarero   = num_procesos-1,
   etiq_tenedor  = 0,
   etiq_sentarse = 1,
   etiq_levantarse = 2;


//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

// ---------------------------------------------------------------------

void funcion_filosofos( int id )
{
   int id_ten_izq = (id+1)              % num_filo_ten, //id. tenedor izq.
       id_ten_der = (id+num_filo_ten-1) % num_filo_ten; //id. tenedor der.

   int valor;

   while ( true ){
      // Se sienta
      MPI_Ssend(&valor, 1, MPI_INT, id_camarero, etiq_sentarse, MPI_COMM_WORLD);
      cout <<"Filósofo "<<id<<" se sienta"<<endl;


      // Solicita tenedor izq y der
      cout <<"Filósofo " <<id << " solicita ten. izq." <<id_ten_izq <<endl;
      MPI_Ssend(&valor, 1, MPI_INT, id_ten_izq, etiq_tenedor, MPI_COMM_WORLD);
      cout <<"Filósofo " <<id <<" solicita ten. der." <<id_ten_der <<endl;
      MPI_Ssend(&valor, 1, MPI_INT, id_ten_der, etiq_tenedor, MPI_COMM_WORLD);


      // Come
      cout <<"Filósofo " <<id <<" comienza a comer" <<endl ;
      sleep_for( milliseconds( aleatorio<10,100>() ) );


      // Suelta tenedor izq y der
      cout <<"Filósofo " <<id <<" suelta ten. izq. " <<id_ten_izq <<endl;
      MPI_Ssend(&valor, 1, MPI_INT, id_ten_izq, etiq_tenedor, MPI_COMM_WORLD);
      cout<< "Filósofo " <<id <<" suelta ten. der. " <<id_ten_der <<endl;
      MPI_Ssend(&valor, 1, MPI_INT, id_ten_der, etiq_tenedor, MPI_COMM_WORLD);


      // Se levanta
      MPI_Ssend(&valor, 1, MPI_INT, id_camarero, etiq_levantarse, MPI_COMM_WORLD);
      cout <<"Filósofo "<<id<<" se levanta"<<endl;

      // Piensa
      cout << "Filosofo " << id << " comienza a pensar" << endl;
      sleep_for( milliseconds( aleatorio<10,100>() ) );
   }
}
// ---------------------------------------------------------------------

void funcion_tenedores( int id )
{
   int valor, id_filosofo ;  // valor recibido, identificador del filósofo
   MPI_Status estado ;       // metadatos de las dos recepciones

   while ( true ){
      // ...... recibir petición de cualquier filósofo
      MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_tenedor, MPI_COMM_WORLD, &estado);
      // ...... guardar en 'id_filosofo' el id. del emisor
      id_filosofo = estado.MPI_SOURCE;
      cout <<"Ten. " <<id <<" ha sido cogido por filo. " <<id_filosofo <<endl;

      // ...... recibir liberación de filósofo 'id_filosofo'
      MPI_Recv(&valor, 1, MPI_INT, id_filosofo, etiq_tenedor, MPI_COMM_WORLD, &estado);
      cout <<"Ten. "<< id<< " ha sido liberado por filo. " <<id_filosofo <<endl ;
   }
}
// ---------------------------------------------------------------------

void funcion_camarero()
{
   int valor, tag_aceptable;
   int libres = num_filosofos-1;
   MPI_Status estado ;       // metadatos de las dos recepciones

   while ( true ){
      if (libres == 0)  tag_aceptable = etiq_levantarse;
      else              tag_aceptable = MPI_ANY_TAG;

      MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, tag_aceptable, MPI_COMM_WORLD, &estado);

      switch(estado.MPI_TAG){
         case etiq_levantarse:
            libres++;
            break;
         case etiq_sentarse:
            libres--;
            break;
      }
   }
}

// ---------------------------------------------------------------------

int main( int argc, char** argv )
{
   int id_propio, num_procesos_actual ;

   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );


   if ( num_procesos == num_procesos_actual )
   {
      // ejecutar la función correspondiente a 'id_propio'
      if ( id_propio == id_camarero)
         funcion_camarero();
      else if ( id_propio % 2 == 0 )          // si es par
         funcion_filosofos( id_propio ); //   es un filósofo
      else                               // si es impar
         funcion_tenedores( id_propio ); //   es un tenedor
   }
   else
   {
      if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
      { cout << "el número de procesos esperados es:    " << num_procesos << endl
             << "el número de procesos en ejecución es: " << num_procesos_actual << endl
             << "(programa abortado)" << endl ;
      }
   }

   MPI_Finalize( );
   return 0;
}

// ---------------------------------------------------------------------
