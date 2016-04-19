#include "common_impl.h"

int main(int argc, char **argv)
{   

  char **newargv;

  int yes=1;
  int sockfd = 0;
  int port_num ;//= malloc(sizeof(int *));
  int i ;
  int size_hostname;
  char buffer[BUFFERSIZE];
  char *distant_host;
  struct sockaddr_in dest_addr; // Info of dsmexec 
  socklen_t addrlen = sizeof(struct sockaddr_in);
  distant_host = malloc(128*sizeof(char*));

  /* processus intermediaire pour "nettoyer" */
  /* la liste des arguments qu'on va passer */
  /* a la commande a executer vraiment */

  newargv = malloc((argc-1)*sizeof(char *));

  for( i=0; i< argc-1; i++) {
    newargv[i]= malloc(128*sizeof(char ));
  }

  newargv[0] = "ssh";

  for(i = 1 ; i< argc ; i++) {
    newargv[i] = argv[i + 1];

  }

  /* Affichage du tableau contenant les nouveaux arguments */
  // for ( i = 0; i < argc -1; i ++)
  // {
  //  printf("newargv[%d] vaut %s\n",i,newargv[i]);
  // }

  // Derniere element doit valoir null pour que le execvp s'effectue
  newargv[argc ] = NULL;

  /* creation d'une socket pour se connecter au */
  /* au lanceur et envoyer/recevoir les infos */
  /* necessaires pour la phase dsm_init */   

  // Get address information of dsmexec
  memset(&dest_addr,0,sizeof(struct sockaddr_in));
  memset(&(dest_addr.sin_zero),0,8);
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(atoi(newargv[argc -3])); // port envoye

  // Enregistre l'adresse ip dans dest_addr
  dest_addr.sin_addr.s_addr = inet_addr(newargv[argc-2]);

  sockfd =socket(AF_INET,SOCK_STREAM,0);

  if (-1 ==  sockfd)
  {
    ERROR_EXIT("ERROR socket");
  }

  if (-1 == setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR, &yes, sizeof(int)))
  {
    ERROR_EXIT("ERROR setting socket options");
  }    

  // connect a la socket d'ecoute

  if(-1 == connect(sockfd,(struct sockaddr *)&dest_addr,addrlen))
    ERROR_EXIT("ERROR connect");

  /* renvoie le numero de descripteur */
  /* et modifie le parametre port_num */
  port_num = atoi(newargv[argc -3]);

  /* Envoi du nom de machine au lanceur */

  // On copie le nom du processus distant dans distant_host
  memset(distant_host,0,128*sizeof(char));
  if(-1 == gethostname(distant_host,128*sizeof(char))) {
    ERROR_EXIT("ERROR dsmwrap gethostname");
  }

  // Taille
  size_hostname = strlen(distant_host);
  memset(buffer,0,BUFFERSIZE);
  sprintf(buffer,"%d",size_hostname); 

  if( -1 == write(sockfd,buffer,BUFFERSIZE)) {
    ERROR_EXIT("ERROR dsmwrap write taille nom machine");
  }

  // Nom de la machine 
  memset(buffer,0,BUFFERSIZE);
  sprintf(buffer,"%s",distant_host); 

  if (-1 == write(sockfd,buffer,BUFFERSIZE)) {
    ERROR_EXIT("ERROR dsmwrap write nom processus distant");
  }

  /* Envoi du pid au lanceur */
  // On recupere le pid du processus distant
  memset(buffer,0,BUFFERSIZE);
  sprintf(buffer,"%d",getpid()); 

  if(-1 == write(sockfd,buffer,BUFFERSIZE)) {
    ERROR_EXIT("ERROR dsmwrap write pid");
  }

  /* Creation de la socket d'ecoute pour les */
  /* connexions avec les autres processus dsm */
  if( -1 == creer_socket(port_num)) {
    ERROR_EXIT("ERROR dsmwrap socket dsm_proc \n");
  }

  /* Envoi du numero de port au lanceur */
  /* pour qu'il le propage à tous les autres */
  /* processus dsm */
  memset(buffer,0,BUFFERSIZE);
  sprintf(buffer,"%s",newargv[argc -3]);

  if(-1 == write(sockfd,buffer,BUFFERSIZE)) {
    ERROR_EXIT("ERROR dsmwrap write port");
  }

  printf("omae wa mô shinde iru, ca veut dire : je suis dans ls dsmwrap \n");

  fflush(stdout);


  /* on execute la bonne commande */

  if(-1 == execvp(newargv[0],newargv))
    ERROR_EXIT("ERROR dsmwrap exec");

  return 0;
}
