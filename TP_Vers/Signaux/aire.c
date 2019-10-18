#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <commun.h>
#include <terrain.h>
#include <vers.h>
#include <jeu.h>
#include <signaux.h>

#define MAX_VERS 26

/* 
 * VARIABLES GLOBALES (utilisees dans les handlers)
 */
char Nom_Prog[256] ;

int fd, nbVers, continu;

int verrou ( int fd, short l_type, short l_whence, long l_start, long l_len, short l_pid, int ope );
int verrou9cases( int fd, int x, int y, off_t fdEmp, int poser );

/*
 * HANDLER
 */

void handler_alarm( int sig ) {

    continu = 0;
}

void handler ( int sig, siginfo_t *info, void* useless ) {

    static int tab[MAX_VERS], init = 0;
    static ver_t ver[MAX_VERS];

    int i, indLibre, nbVoisin, nbLigne, nbCol, no_err;
    coord_t *voisin = NULL;
    off_t fdEmp;

    if ( !init ) {

        for ( i = 0 ; i < MAX_VERS ; i++ )
            tab[i] = -1;

        init = 1;
    }

    for ( i = 0 ; i < MAX_VERS && tab[i] != info->si_pid ; i++ );

    if ( i == MAX_VERS ) {

        for ( i = 0 ; i < MAX_VERS && tab[i] != -1 ; i++ );

        if ( i == MAX_VERS ) {

            printf("Erreur : la nombre max de ver à été atteint : %d\n", MAX_VERS);
            kill( info->si_pid, SIGUSR2 );
            exit(-1);
        }

        tab[i] = info->si_pid;

        if ( verrou( fd, F_WRLCK, 0, 0, 0, getpid(), F_SETLKW) )
		    exit(-1);

        if ( terrain_dim_lire(fd, &nbLigne, &nbCol) )
            exit(-2);

        if ( jeu_ver_initialiser( fd, nbLigne, nbCol, &(ver[i])) == -1 ) {

            kill( info->si_pid, SIGUSR2 );
            nbVers--;
        }

        terrain_marque_ecrire( fd, ver[i].tete, (case_t)(i + 'A')); /* jeu_ver_initialiser le fait deja mais ne marche pas, je suis obligé de le refaire ici */

        if ( verrou( fd, F_UNLCK, 0, 0, 0, getpid(), F_SETLKW) ) /* pose un verrou sur les 8 cases autours ( plus la case du milieu ) */
            exit(-4);

        nbVers++;

        if ( verrou( fd, F_RDLCK, 0, 0, 0, getpid(), F_SETLKW) )
			exit(-5);

        if( (no_err = terrain_afficher(fd) ))
            exit(-6);

        if ( verrou( fd, F_UNLCK, 0, 0, 0, getpid(), F_SETLKW) )
            exit(-7);
    } else {

        verrou9cases( fd, ver[i].tete.x, ver[i].tete.y, fdEmp, 1 );

        if ( ( no_err = terrain_voisins_rechercher( fd, ver[i].tete, &voisin, &nbVoisin )) )
            exit(-8);

        if ( !( no_err = terrain_case_libre_rechercher( fd, voisin, nbVoisin, &indLibre ) ) && indLibre != -1 ) {

            if( (no_err = terrain_marque_ecrire( fd, ver[i].tete, (case_t)(i + 'a') ) ) )
			    exit(-9);

            if( (no_err = terrain_marque_ecrire( fd, voisin[indLibre], (case_t)(i + 'A') ) ) )
			    exit(-9);

            verrou9cases( fd, ver[i].tete.x, ver[i].tete.y, fdEmp, 0 );

            ver[i].tete = voisin[indLibre];

            if ( verrou( fd, F_RDLCK, 0, 0, 0, getpid(), F_SETLKW) )
				exit(-10);

			if( (no_err = terrain_afficher(fd) ))
				exit(-11);

			if ( verrou( fd, F_UNLCK, 0, 0, 0, getpid(), F_SETLKW) )
				exit(-12);

        } else {

            verrou9cases( fd, ver[i].tete.x, ver[i].tete.y, fdEmp, 0 );
            kill( info->si_pid, SIGUSR2 );
            nbVers--;
            if ( nbVers == 0 )
                printf("Le vers %c a gagné !!!!! Bravo à lui !\n", i + 'A');
        }
    }
}

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

    char fich_terrain[128] ;
    pid_t pid_aire ; 

    /*----------*/

    /* 
    * Capture des parametres 
    */
    strcpy( Nom_Prog , tab_arg[0] );

    if( nb_arg != 2 ) {

        fprintf( stderr , "Usage : %s <fichier terrain>\n", Nom_Prog );
        exit(-1);
    }

    strcpy( fich_terrain , tab_arg[1] );
    pid_aire = getpid() ; 

    /* Affichage du pid pour les processus verDeTerre */
    printf( "\n\t-----------------.\n") ; 
    printf( "--- pid %s = %d ---\n" , Nom_Prog , pid_aire ) ; 
    printf(   "\t-----------------.\n\n") ; 

    /* Initialisation de la generation des nombres pseudo-aleatoires */
    srandom((unsigned int)pid_aire);

    /*----------*/

    fd = open( fich_terrain, O_RDWR );

    printf("\n\t----- %s : Debut du jeu -----\n\n" , Nom_Prog );

    nbVers = 0;

    struct sigaction act;

    act.sa_sigaction = handler;
    sigemptyset( &( act.sa_mask) );
    act.sa_flags = SA_SIGINFO;

    sigaction( SIGUSR1, &act, NULL );

    act.sa_handler = handler_alarm;
    sigemptyset( &( act.sa_mask) );
    act.sa_flags = 0;

    sigaction( SIGALRM, &act, NULL );

    pause();
        
    while ( nbVers ) {

        pause();
    }

    printf("alarm\n");

    continu = 1;

    while ( continu ) {

        pause();
        alarm(10);
    }

    printf("\n\n\t----- %s : Fin du jeu -----\n\n" , Nom_Prog );

    exit(0);
}

