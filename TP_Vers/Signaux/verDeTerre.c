#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <commun.h>
#include <terrain.h>
#include <vers.h>
#include <signaux.h>


/* 
 * VARIABLES GLOBALES (utilisees dans les handlers)
 */

int continu;

/*
 * HANDLERS
 */

void handler_ver ( int sig ) {

     continu = 0;
}

int main( int nb_arg , char * tab_arg[] ) {

     char nomprog[128] ;
     pid_t pid_aire ;
     pid_t pid_ver ;

     /*----------*/

     /* 
      * Capture des parametres 
      */
     /* - nom du programme */
     strcpy( nomprog , tab_arg[0] );

     if( nb_arg != 2 )
     {
	  fprintf( stderr , "Usage : %s <pid aire>\n", 
		   nomprog );
	  exit(-1);
     }

     /* - parametres */
     pid_aire = atoi( tab_arg[1] ) ;


     /* Initialisation de la generation des nombres pseudo-aleatoires */
     srandom((unsigned int)getpid());

     pid_ver = getpid() ; 
     printf( "\n\n--- Debut ver [%d] ---\n\n" , pid_ver );   

     continu = 1;

     struct sigaction act;

     act.sa_handler = handler_ver;
     sigemptyset( &( act.sa_mask) );
     act.sa_flags = 0;

    sigaction( SIGUSR2, &act, NULL );

     while ( continu ) {

          sleep( random() % TEMPS_MOYEN + 1 );
          if ( continu )
               kill( pid_aire, SIGUSR1 );
     }
     
     printf( "\n\n--- Arret ver [%d] ---\n\n" , pid_ver );
  
     exit(0);
}
