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

    if ( !init ) { /* initialisation */

        for ( i = 0 ; i < MAX_VERS ; i++ )
            tab[i] = -1;

        init = 1;
    }

    for ( i = 0 ; i < MAX_VERS && tab[i] != info->si_pid ; i++ ); /* recherche du vers qui a envoyé le signal */

    if ( i == MAX_VERS ) { /* le vers n'est pas connue, c'est la premiere fois qu'il joue */

        for ( i = 0 ; i < MAX_VERS && tab[i] != -1 ; i++ ); /* recherche d'une case libre */

        if ( i == MAX_VERS ) {

            printf("Erreur : la nombre max de ver à été atteint : %d\n", MAX_VERS);
            kill( info->si_pid, SIGUSR2 );
        }

        tab[i] = info->si_pid;

        if ( terrain_dim_lire(fd, &nbLigne, &nbCol) )
            exit(-2);

        if ( jeu_ver_initialiser( fd, nbLigne, nbCol, &(ver[i])) == -1 ) {

            kill( info->si_pid, SIGUSR2 );
            nbVers--;
        }

        terrain_marque_ecrire( fd, ver[i].tete, (case_t)(i + 'A')); /* jeu_ver_initialiser le fait deja mais ne marche pas, je suis obligé de le refaire ici */

        nbVers++;

        if( (no_err = terrain_afficher(fd) ))
            exit(-6);

    } else { /* le vers est connue, on le fait jouer normalement */

        if ( ( no_err = terrain_voisins_rechercher( fd, ver[i].tete, &voisin, &nbVoisin )) )
            exit(-8);

        if ( !( no_err = terrain_case_libre_rechercher( fd, voisin, nbVoisin, &indLibre ) ) && indLibre != -1 ) {
            /* si c'est different de -1, alors au moins un emplacement autour de lui est libre */

            if( (no_err = terrain_marque_ecrire( fd, ver[i].tete, (case_t)(i + 'a') ) ) )
			    exit(-9);

            if( (no_err = terrain_marque_ecrire( fd, voisin[indLibre], (case_t)(i + 'A') ) ) )
			    exit(-9);

            ver[i].tete = voisin[indLibre];

			if( (no_err = terrain_afficher(fd) ))
				exit(-11);

        } else { /* sinon le vers ne peux plus se deplacer, il faut lui demander d'arreter de jouer */

            kill( info->si_pid, SIGUSR2 );
            nbVers--;
            if ( nbVers == 0 )
                printf("Le vers %c a gagné !!!!! Bravo à lui !\n", i + 'A');
        }
    }
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

    pause(); /* attente du premier vers */
        
    while ( nbVers ) { /* tant qu'il reste des vers a jouer */

        pause(); /* on attend des signal */
    }

    printf("alarm\n");

    continu = 1;
    alarm(10); 

    while ( continu ) { /* attente des processus qui continuerais eventuellement de se connecter */

        pause();
        alarm(0); // arret de l'ancienne alarm
        alarm(10); // remise de l'alarm
    }

    printf("\n\n\t----- %s : Fin du jeu -----\n\n" , Nom_Prog );

    exit(0);
}