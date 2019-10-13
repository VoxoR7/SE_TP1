#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

#include <commun.h>
#include <terrain.h>

int verrou ( int fd, short l_type, short l_whence, long l_start, long l_len, short l_pid, int ope ) {

	struct flock verrou;

	verrou.l_type = l_type;
	verrou.l_whence = l_whence;
	verrou.l_start = l_start;
	verrou.l_len = l_len;
	verrou.l_pid = l_pid;

	return fcntl( fd, ope, &verrou );
}

/*--------------------* 
 * Main demon 
 *--------------------*/
int main( int nb_arg , char * tab_arg[] ) {

	/* Parametres */
	char fich_terrain[128] ;
	char nomprog[256] ;

	int fd;
	struct stat stat2;
	time_t dernierAff = 0;
	int no_err;

	/*----------*/

  /* Capture des parametres */
	if( nb_arg != 2 ) {

		fprintf( stderr , "Usage : %s <fichier terrain>\n", tab_arg[0] );
		exit(-1);
	}

	strcpy( nomprog , tab_arg[0] );
	strcpy( fich_terrain , tab_arg[1] );


	printf("\n%s : ----- Debut de l'affichage du terrain ----- \n", nomprog );

	fd = open( fich_terrain, O_RDWR );

	while ( 1 ) {

		stat( fich_terrain, &stat2);

		printf("dernierAff : %ld, st_mtime : %ld\n", dernierAff, stat2.st_mtime);

		if ( dernierAff < stat2.st_mtime ) {

			if ( verrou( fd, F_RDLCK, 0, 0, 0, getpid(), F_SETLKW) )
				return -1;

			if( (no_err = terrain_afficher(fd) ))
				return -2;

			if ( verrou( fd, F_UNLCK, 0, 0, 0, getpid(), F_SETLKW) )
				return -1;

			dernierAff = stat2.st_mtime;
		}

		sleep(1);
	}

	printf("\n%s : --- Arret de l'affichage du terrain ---\n", nomprog );

	exit(0);
}
