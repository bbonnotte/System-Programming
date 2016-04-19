#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <poll.h>


/* autres includes (eventuellement) */

#define MAX_MACHINE 20
#define BUFFERSIZE 256
#define ERROR_EXIT(str) {perror(str);exit(EXIT_FAILURE);}

/* definition du type des infos */
/* de connexion des processus dsm */
struct dsm_proc_conn  {
   int rank; // rang de la machine
   int sockfd; // Chaque machine parle au serveur avec une socket qui lui ai propre
   int port;
   char *hostname; // Nom de la machine 
   int size_hostname; // taille de la chaine de caractere
};
typedef struct dsm_proc_conn dsm_proc_conn_t; 

/* definition du type des infos */
/* d'identification des processus dsm */
struct dsm_proc {   
  pid_t pid;
  dsm_proc_conn_t connect_info;
};
typedef struct dsm_proc dsm_proc_t;

int creer_socket(int port_num);
int hostname_to_ip(char * hostname , char* ip);


