// Ramón

/*
make p3_gasolinera3_mpi_exe; mpirun -oversubscribe -np 12 p3_gasolinera3_mpi_exe
*/

#include <mpi.h>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <iostream>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int
   num_coches = 10,
   num_tipos_combustible = 3,
   num_procesos = num_coches + 2,
   id_gasolinera = num_coches,
   id_impresora = num_coches + 1;
int
   surtidores[num_tipos_combustible],
   etiq_entrar[num_tipos_combustible],
   etiq_salir = num_tipos_combustible,
   etiq_imprimir = num_tipos_combustible+1;


//-------------------------------------------------------------------------------

template< int min, int max > int aleatorio(){
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
void imprimir(const string &s){
   MPI_Ssend(s.c_str(), s.length(), MPI_CHAR, id_impresora, etiq_imprimir, MPI_COMM_WORLD);
}
//-------------------------------------------------------------------------------
void repostar(int num_coche, int tipo){
    milliseconds duration(aleatorio<20,100>());

    imprimir("Coche " + to_string(num_coche) + " de tipo " + to_string(tipo) + " empieza a repostar");

    this_thread::sleep_for(duration);

    imprimir("Coche " + to_string(num_coche) + " de tipo " + to_string(tipo) + " termina de repostar");
}
//-------------------------------------------------------------------------------
void esperar(int num_coche, int tipo){
    milliseconds duration(aleatorio<20,100>());

    imprimir("Coche " + to_string(num_coche) + " de tipo " + to_string(tipo) + " espera fuera");

    this_thread::sleep_for(duration);

    imprimir("Coche " + to_string(num_coche) + " de tipo " + to_string(tipo) + " termina de esperar");
}

//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------

void funcion_coche(int id){
   const int tipo = aleatorio<0,num_tipos_combustible-1>();

   while(true){
      imprimir("Coche " + to_string(id) + " de tipo " + to_string(tipo) + " quiere entrar a respostar");

      MPI_Ssend(&tipo, 1, MPI_INT, id_gasolinera, etiq_entrar[tipo], MPI_COMM_WORLD);
      
      repostar(id, tipo);
      
      MPI_Ssend(&tipo, 1, MPI_INT, id_gasolinera, etiq_salir, MPI_COMM_WORLD);

      esperar(id, tipo); 
   }
}

//-------------------------------------------------------------------------------

void funcion_gasolinera(){
   int tipo,
       tag_aceptable,
       esperando,
       surtidores_libres[num_tipos_combustible];
   MPI_Status estado;

   // Inicialización de surtidores libres
   for (int i=0; i<num_tipos_combustible; i++)
      surtidores_libres[i] = surtidores[i];


   while(true){
      esperando = 0;

      // Comprobamos si alguien quiere salir
      MPI_Iprobe(MPI_ANY_SOURCE, etiq_salir, MPI_COMM_WORLD, &esperando, &estado);
      if (esperando)
         tag_aceptable = etiq_salir;

      // Si no, comprobamos si alguien quiere entrar.
      // (La condición "&& esperando==false" comprueba si ya había alguien esperando a salir
      // antes de entrar en el bucle)
      for (int i=0; i<num_tipos_combustible && esperando==false; i++)
         if (surtidores_libres[i]){
            MPI_Iprobe(MPI_ANY_SOURCE, etiq_entrar[i], MPI_COMM_WORLD, &esperando, &estado);
            if (esperando)
               tag_aceptable = etiq_entrar[i];
         }



      // Si hay algún mensaje, lo recibimos
      if (esperando){
         MPI_Recv(&tipo, 1, MPI_INT, MPI_ANY_SOURCE, tag_aceptable, MPI_COMM_WORLD, &estado);

         // Mensaje de salida
         if (estado.MPI_TAG == etiq_salir)
            surtidores_libres[tipo]++;
         else
            surtidores_libres[tipo]--;
      }
      // Si no había nadie esperando, esperamos un tiempo
      else
         this_thread::sleep_for(milliseconds(20));
   }
}

//-------------------------------------------------------------------------------
/*
// Otra forma de hacerlo, recibiendo varios mensajes en cada bucle
void funcion_gasolinera(){
   int 
      tipo = 0,
      esperando = 0,
      surtidores_libres[num_tipos_combustible];
   MPI_Status
      estado;
   
   for(int i=0; i<num_tipos_combustible; i++)
      surtidores_libres[i] = surtidores[i];
   
   while(true) {
      esperando = 0;

      // PVemos si alguien quiere salir
      MPI_Iprobe(MPI_ANY_SOURCE,etiq_salir,MPI_COMM_WORLD,&esperando,&estado);

      if(esperando) {
         // Si hay alguno esperando lo liberamos
         MPI_Recv(&tipo,1,MPI_INT,MPI_ANY_SOURCE,etiq_salir,MPI_COMM_WORLD,&estado);
         surtidores_libres[tipo]++;
      }

      // En otro caso vemos si alguien quiere entrar por cada tipo de combustible
      for(int i=0; i<num_tipos_combustible; i++) {
         if(surtidores_libres[i]) {
               // Si hay surtidores disponibles, vemos si alguien quiere entrar
               MPI_Iprobe(MPI_ANY_SOURCE,etiq_entrar[i],MPI_COMM_WORLD,&esperando,&estado);

               if(esperando) {
                  // Hay al menos uno que quiere entrar
                  MPI_Recv(&tipo,1,MPI_INT,MPI_ANY_SOURCE,etiq_entrar[i],MPI_COMM_WORLD,&estado);
                  surtidores_libres[tipo]--;
               }
         }
      }

      if (! esperando)
         this_thread::sleep_for(milliseconds(20));
   }
}
*/
//-------------------------------------------------------------------------------

void funcion_impresora(){
   MPI_Status estado;
   int size;
   while (true){
      MPI_Probe(MPI_ANY_SOURCE,etiq_imprimir,MPI_COMM_WORLD,&estado);
      MPI_Get_count(&estado, MPI_CHAR, &size);

      char * buffer = new char[size + 1];

      MPI_Recv(buffer,size,MPI_CHAR,estado.MPI_SOURCE,etiq_imprimir,MPI_COMM_WORLD,&estado);

      buffer[size] = '\0';
      cout << buffer << endl;
      delete[] buffer;
   }
}

//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------

int main( int argc, char** argv )
{
   // Inicializamos vectores
   for (int i=0; i<num_tipos_combustible; i++)  // {3,2,1}
      surtidores[i] = num_tipos_combustible - i;
   for (int i=0; i<num_tipos_combustible; i++)  // {0,1,2}
      etiq_entrar[i] = i;

   


   int id_propio, num_procesos_actual;

   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );

   if ( num_procesos == num_procesos_actual ) {
      // ejecutar la función correspondiente a 'id_propio'
      if (id_propio == id_gasolinera)
         funcion_gasolinera();
      else if (id_propio == id_impresora)
         funcion_impresora();
      else
         funcion_coche(id_propio);
   }
   else {
      if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
      { cout << "el número de procesos esperados es:    " << num_procesos << endl
             << "el número de procesos en ejecución es: " << num_procesos_actual << endl
             << "(programa abortado)" << endl ;
      }
   }

   MPI_Finalize( );
   return 0;
}