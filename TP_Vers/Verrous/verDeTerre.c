#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

#include <commun.h>
#include <terrain.h>
#include <vers.h>
#include <ver.h>
#include <jeu.h>

int verrou ( int fd, short l_type, short l_whence, long l_start, long l_len, short l_pid, int ope ) {

	struct flock verrou;

	verrou.l_type = l_type;
	verrou.l_whence = l_whence;
	verrou.l_start = l_start;
	verrou.l_len = l_len;
	verrou.l_pid = l_pid;

	return fcntl( fd, ope, &verrou );
}

int verrou9cases( int fd, int x, int y, off_t fdEmp, int poser ) {

	if ( poser ) {

		terrain_xy2pos( fd, x - 1, y - 1, &fdEmp);

		if ( verrou( fd, F_WRLCK, 0, fdEmp, CASE_TAILLE * 3, getpid(), F_SETLKW) )
			return -1;

		terrain_xy2pos( fd, x - 1, y, &fdEmp);

		if ( verrou( fd, F_WRLCK, 0, fdEmp, CASE_TAILLE * 3, getpid(), F_SETLKW) )
			return -2;

		terrain_xy2pos( fd, x - 1, y + 1, &fdEmp);

		if ( verrou( fd, F_WRLCK, 0, fdEmp, CASE_TAILLE * 3, getpid(), F_SETLKW) )
			return -3;
	} else {

		terrain_xy2pos( fd, x - 1, y - 1, &fdEmp);

		if ( verrou( fd, F_UNLCK, 0, fdEmp, CASE_TAILLE * 3, getpid(), F_SETLKW) )
			return -4;

		terrain_xy2pos( fd, x - 1, y, &fdEmp);

		if ( verrou( fd, F_UNLCK, 0, fdEmp, CASE_TAILLE * 3, getpid(), F_SETLKW) )
			return -5;

		terrain_xy2pos( fd, x - 1, y + 1, &fdEmp);

		if ( verrou( fd, F_UNLCK, 0, fdEmp, CASE_TAILLE * 3, getpid(), F_SETLKW) )
			return -6;
	}

	return 0;
}

int main( int nb_arg , char * tab_arg[] ) {

	/* Parametres */
	char fich_terrain[128] ;
	case_t marque = CASE_LIBRE ;
	char nomprog[128] ;

	coord_t *voisin = NULL;
	int no_err, fd, nbLigne, nbCol, nbVoisin, indLibre;
	off_t fdEmp;

	ver_t ver;

	/*----------*/

	/* Capture des parametres */
	if( nb_arg != 3 ) {

		fprintf( stderr , "Usage : %s <fichier terrain> <marque>\n", 
		tab_arg[0] );
		exit(-1);
	}

	if( strlen(tab_arg[2]) !=1 ) {

		fprintf( stderr , "%s : erreur marque <%s> incorrecte \n",
		tab_arg[0] , tab_arg[2] );
		exit(-1) ;
	}

	strcpy( nomprog , tab_arg[0] );
	strcpy( fich_terrain , tab_arg[1] );
	marque = tab_arg[2][0] ;


	/* Initialisation de la generation des nombres pseudo-aleatoires */
	srandom((unsigned int)getpid());

	printf( "\n\n%s : ----- Debut du ver %c (%d) -----\n\n ", nomprog, marque, getpid() );

	fd = open( fich_terrain, O_RDWR );

	if ( verrou( fd, F_RDLCK, 0, 0, sizeof(int) * 2, getpid(), F_SETLKW) )
		return -1;

	if ( terrain_dim_lire(fd, &nbLigne, &nbCol) )
		return -2;

	if ( verrou( fd, F_UNLCK, 0, 0, sizeof(int) * 2, getpid(), F_SETLKW) )
		return -3;

	if ( jeu_ver_initialiser( fd, nbLigne, nbCol, &ver) )
		return -4;

	/* meme si le fichier ne vas etre utilisé que en lecture dans un premier temps, je fais le choix de mettre un verrou en ecriture puisque apres cette lecture si une case est libre on va ecrire dans cette case et au lieu de mettre 3 verrou en lecture puis 1 verrour en ecriture je bloque directement en ecriture*/

	verrou9cases( fd, ver.tete.x, ver.tete.y, fdEmp, 1 );
	
	if ( ( no_err = terrain_voisins_rechercher( fd, ver.tete, &voisin, &nbVoisin )) )
		return -7;

	while ( !( no_err = terrain_case_libre_rechercher( fd, voisin, nbVoisin, &indLibre ) ) && indLibre != -1 ) {

		if( (no_err = terrain_marque_ecrire( fd, voisin[indLibre], marque ) ) )
			return -8;

		verrou9cases( fd, ver.tete.x, ver.tete.y, fdEmp, 0 );

		ver.tete = voisin[indLibre]; 

		sleep( random() % TEMPS_MOYEN + 1 );

		verrou9cases( fd, ver.tete.x, ver.tete.y, fdEmp, 1 );
		
		if ( ( no_err = terrain_voisins_rechercher( fd, ver.tete, &voisin, &nbVoisin )) )
			return -15;
	}

	verrou9cases( fd, ver.tete.x, ver.tete.y, fdEmp, 0 );

	printf( "\n\n%s : ----- Fin du ver %c (%d) -----\n\n ", nomprog, marque, getpid() );

	exit(0);
}