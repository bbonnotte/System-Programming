#include "common_impl.h"

/* variables globales */

/* un tableau gerant les infos d'identification */
/* des processus dsm */
      dsm_proc_t *proc_array = NULL; 

/* le nombre de processus effectivement crees */
      volatile int nb_machine_creat = 0;

void usage(void)
{
  fprintf(stdout,"Usage : dsmexec machine_file executable arg1 arg2 ...\n");
  fflush(stdout);
  exit(EXIT_FAILURE);
}

// Cette fonction est à modifier
void sigchld_handler(int sig)
{
/* on traite les fils qui se terminent */

  pid_t pid;

  if( sig == SIGCHLD) {
    while ((pid = waitpid(-1, NULL, WNOHANG)) != -1)
    {
      if (pid > 0)
        nb_machine_creat--;
      sleep(1);
    }
  }
  
/* pour eviter les zombies */
}

// recupere la taille du tableau

int length(const char** array) {
  int count = 0;
  while(array[count]) 
    count++;
  return count;
}


// Compte le nombre de processus

int count_lines(FILE* fp) {

  int nb_machine =0;
  int ch = 0;


  while(!feof(fp)) {

    ch =fgetc(fp);

    if(ch == '\n'){
      nb_machine ++;
    }
  }

  rewind(fp);   // retourne au debut du fichier
  nb_machine++;
  return nb_machine;
}

// Recupere le nom de machine dans le fichier machine_file
void read_machine( FILE* fp, char** machine) {
  int i;
  
  char line[BUFFERSIZE];
  int nb_machine = count_lines(fp);

  for(i = 0; i<nb_machine; i++) {
   memset(line,0,BUFFERSIZE);
   memset(machine[i],0,BUFFERSIZE);
   fgets(line,BUFFERSIZE,fp);
       
    // Supprimer le \n
   strtok(line,"\n");

   strcpy(machine[i],line);
   printf("Les noms de machine est ->%s\n",machine[i]);

 }

}


int main(int argc, char *argv[])
{
  if (argc < 3){
    usage();
  } else {       
    int i,j; // Compteurs de boucles
    int sockfd;
    int ** fd_out; // Tableau pipe sortie standard
    int ** fd_err; // Tableau pipe error
    int nb_machine = 0; // nb_machine contenut dans machine_file.txt 
    int inactifs = 0 ; // Pour vérifier si les extrémités des tubes sont inactifs
    //int port_num ; 
    pid_t pid;
    char hostname[128];
    char ip[100];
    char buffer[BUFFERSIZE]; // Buffer dans lequelle sont enregistrés les informations 
    char **newargv; // Tableau contenant les données que l'on va envoyé au processus intermédiaire dsmwrap
    char ** machine;
    char *port = "8080"; // Port temporaire fixé pour réalisé nos opérations
    struct sockaddr_in client_addr; // Structure contenant les informations du client i.e dsmwrap
    struct sigaction action; // Structure necessaire a la gestion des zombies ou erreurs
    struct pollfd *fds; // Pointeur sur la structure pollfd necessaire pour réaliser le multiplexing 
    socklen_t size_struct_sockaddr = sizeof(struct sockaddr_in); // Taille de la structure
    FILE * fp; // Fichier contenant les noms de machines
    nfds_t nfds;

     /* Mise en place d'un traitant pour recuperer les fils zombies*/      
     /* XXX.sa_handler = sigchld_handler; */
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = sigchld_handler;
    action.sa_flags = SA_RESTART;

     /* lecture du fichier de machines */
    fp = fopen("/net/malt/t/aguisset/Documents/Sys/Phase1/machine_file.txt","r");

    if (NULL == fp) {
      ERROR_EXIT("ERROR fopen");
    }

     /* 1- on recupere le nombre de processus a lancer */
    nb_machine = count_lines(fp);

    printf("nb_machine est : %d\n",nb_machine);

     // Allocation de chaque machine

    newargv = malloc(nb_machine*sizeof(char *));
    machine = malloc(nb_machine*sizeof(char *));
    fd_out = malloc(nb_machine*sizeof(int *));
    fd_err = malloc(nb_machine*sizeof(int*));

    proc_array = malloc(nb_machine *sizeof(dsm_proc_t));
    fds = malloc(2*nb_machine*sizeof(struct pollfd)); // Car 1 paire de tubes

     // Necessaire pour allouer chaque case de memoire correspondant a la machine
    for(i = 0; i<nb_machine;i++) {
      machine[i] = malloc(128*sizeof(char));
      newargv[i] =  malloc(128*sizeof(char));
    }

    /* 2- on recupere les noms des machines : le nom de */

    /* la machine est un des elements d'identification */
    read_machine(fp,machine);

     /* creation de la socket d'ecoute */
    sockfd = creer_socket(atoi(port));

          /* + ecoute effective */ 
    if(-1 == listen(sockfd,nb_machine)) {
      ERROR_EXIT("ERROR listen");
    }

    // Communication client-server necessite de connaitre l'adresse ip du serv
    memset(hostname,'\0',128);
    if(-1 ==  gethostname(hostname,128*sizeof(char))) {
      ERROR_EXIT("ERROR gethostname");
    }
    
    // Adresse ip à partir du hostname
    hostname_to_ip(hostname,ip);

     /* creation des fils */
    for(i = 0; i < nb_machine ; i++) {

          /* creation du tube pour rediriger stdout */
        fd_out[i] = malloc(sizeof(int) * 2);
        if(-1 == pipe(fd_out[i])) {
          ERROR_EXIT("ERROR pipe out");
        }


        	/* creation du tube pour rediriger stderr */
        fd_err[i] = malloc(sizeof(int) * 2);
        
        if ( -1 == pipe(fd_err[i])) {
          ERROR_EXIT("ERROR pipe err");
        }

         pid = fork();

        if(pid == -1){ 
          ERROR_EXIT("fork");
        }
        if (pid == 0) { /* fils */	


        	   /* redirection stdout */	 
            close(fd_out[i][0]);
            dup2(fd_out[i][1], STDOUT_FILENO);
            close(fd_out[i][1]);

        	   /* redirection stderr */	 
            close(fd_err[i][0]);
            dup2(fd_err[i][1], STDOUT_FILENO);
            close(fd_err[i][1]);

        	   /* Creation du tableau d'arguments pour le ssh */ 

            newargv[0] = "ssh";

              // nom de la machine distante
            strcpy(newargv[1],machine[i]);

              // dsmwrap
            newargv[2] = "/net/malt/t/aguisset/Documents/Sys/Phase1/bin/dsmwrap";

              //executable (on suppose qu'on veut executer truc)
            newargv[3] = "/net/malt/t/aguisset/Documents/Sys/Phase1/bin/truc";

              // Port de dsmexec (on suppose a 8080)
            newargv[4] = port;

              // Adresse ip (recuperer grace a gethostname)
            newargv[5] = ip;

            newargv[6] = NULL;

             /* jump to new prog : */

            if( -1 == execvp(newargv[0],newargv) )
            {
              ERROR_EXIT("ERROR execvp");
            } 

        	} else  if(pid > 0) { /* pere */		      
        	   /* fermeture des extremites des tubes non utiles */
            if(-1 == close(fd_out[i][1])) {
              ERROR_EXIT("ERROR close fd_out");
            }
            if(-1 == close(fd_err[i][1]) ) {
              ERROR_EXIT("ERROR close fd_in");
            }

            proc_array[i].connect_info.rank = i +1;

             // On incremente le nombre de machine crée, il est égale au nombre de machine 
            nb_machine_creat++;	      
          }   
    }


    for(i = 0; i < nb_machine ; i++){

      /* on accepte les connexions des processus dsm */

      proc_array[i].connect_info.sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &size_struct_sockaddr);

      if(-1 == proc_array[i].connect_info.sockfd) {
      ERROR_EXIT("ERROR accept");
      }

      /*  On recupere le nom de la machine distante */
      //gethostname(hostname, 128*sizeof(char));
      /* 1- d'abord la taille de la chaine */
      memset(buffer,'\0',BUFFERSIZE);

      if(-1 == read(proc_array[i].connect_info.sockfd,buffer,BUFFERSIZE))
      ERROR_EXIT("ERROR dsmexec read");

      proc_array[i].connect_info.size_hostname = atoi(buffer);

      printf("[Proc %d] taille de la chaine machine : %d \n", i, proc_array[i].connect_info.size_hostname);

      /* 2- puis la chaine elle-meme */

      memset(buffer,'\0',BUFFERSIZE);
      proc_array[i].connect_info.hostname = malloc(proc_array[i].connect_info.size_hostname*sizeof(char));

      if(-1 == read(proc_array[i].connect_info.sockfd,buffer,BUFFERSIZE))
      ERROR_EXIT("ERROR dsmexec read");

      strncpy( proc_array[i].connect_info.hostname,buffer,proc_array[i].connect_info.size_hostname);

      printf("Proc %d]nom de la machine : %s \n", i,proc_array[i].connect_info.hostname);

      /* On recupere le pid du processus distant  */

      memset(buffer,'\0',BUFFERSIZE);

      if(-1 == read(proc_array[i].connect_info.sockfd,buffer,BUFFERSIZE))
      ERROR_EXIT("ERROR dsmexec read");

      proc_array[i].pid = (pid_t)atoi(buffer);
      printf("[Proc %d] A pour pid %d\n",i,proc_array[i].pid);

      /* On recupere le numero de port de la socket */
      /* d'ecoute des processus distants */
      memset(buffer,'\0',BUFFERSIZE);

      if(-1 == read(proc_array[i].connect_info.sockfd,buffer,BUFFERSIZE))
      ERROR_EXIT("ERROR dsmexec read");

      proc_array[i].connect_info.port = atoi(buffer);
      printf(" [Proc %d] A pour n° de port %d\n",i,proc_array[i].connect_info.port);

    }

       /* envoi du nombre de processus aux processus dsm*/
    memset(buffer,'\0',BUFFERSIZE);
    sprintf(buffer,"%d\n",nb_machine_creat);

    for(i = 0; i< nb_machine; i++ ) {
      if (-1 == write(proc_array[i].connect_info.sockfd,buffer,BUFFERSIZE)) {
        ERROR_EXIT("ERROR dsmexec write nb processus dsm");
      }
    }

       /* envoi des rangs aux processus dsm */

    for(i = 0; i< nb_machine; i++ ) {
    memset(buffer,'\0',BUFFERSIZE);

          // Recupération du rang de chaque processus
    sprintf(buffer,"%d\n",proc_array[i].connect_info.rank);

    if (-1 == write(proc_array[i].connect_info.sockfd,buffer,BUFFERSIZE)) {
    ERROR_EXIT("ERROR dsmexec write rank processus dsm");
    }

    }

       /* envoi des infos de connexion aux processus */

    for(i = 0; i<nb_machine_creat; i++) { // Premier boucle pour la socket du destinataire
      for(j=0; j< nb_machine_creat; j++) { // boucle pour l'emetteur (c'est son hostname que l'on veut)
      
        // Voir si possible d'envoyer une structure directement

        // Nom de la machine
        if(-1 == write(proc_array[i].connect_info.sockfd,proc_array[j].connect_info.hostname,BUFFERSIZE)){
          ERROR_EXIT("ERROR write dsmexec write du nom de machine aux processus");
        }

        // port
        memset(buffer,0,BUFFERSIZE);
        sprintf(buffer,"%d",proc_array[j].connect_info.port);

        if( -1 == write(proc_array[i].connect_info.sockfd,buffer,BUFFERSIZE)) 
          ERROR_EXIT("ERROR dsmexec write du port de la machine distante");
        }
    }

       /* gestion des E/S : on recupere les caracteres */
       /* sur les tubes de redirection de stdout/stderr */     

        // Initialisation de la structure poll

      for(i = 0 ; i< 2*nb_machine; i++) {
        if( i< nb_machine) {
          fds[i].fd = fd_out[i][0];
          fds[i].events = POLLIN | POLLHUP; // Des données peuvent être lue
        }
        else{ // i> nb_machine

            fds[i].fd = fd_err[i-nb_machine][0]; // On le remet à l'indice zero
            fds[i].events = POLLIN | POLLHUP ;
        }
          
        }

        // while(inactifs != 2*nb_machine)
        while(inactifs != 2*nb_machine)
        {
              /*je recupere les infos sur les tubes de redirection
              jusqu'à ce qu'ils soient inactifs (ie fermes par les
              processus dsm ecrivains de l'autre cote ...)
              */

          // Pour simplifier on crée un seul tableau contenant fd_out et fd_err
          // sinon créer deux tableaux fds_stdout et fds_stderr et concaténer
          nfds = 2*nb_machine_creat;

          if (-1 == poll(fds,nfds,0)) {
            ERROR_EXIT("ERROR dsmexec poll");
          }

          else{

            for(i = 0 ; i< 2*nb_machine; i++) {

              if( i< nb_machine) {
                if(fds[i].revents & POLLIN) { 

                  memset(buffer,0, BUFFERSIZE);

                  if(-1 == read(fds[i].fd,buffer,BUFFERSIZE))
                    ERROR_EXIT("ERROR dsmexec read on pipe");

                  printf("LE PIPE ERR TE DIT : %s\n",buffer);
                  break;
                }
                
                //Si revents vaut POLLHUP alors tube fermé
                // if (fds[i].revents & POLLHUP) {
                //   // Fermeture de notre côté du tube
                //   if (-1 == close(fd_out[i][0])) {
                //     ERROR_EXIT("close tube_stderr POLLHUP");
                //   }
                //   inactifs++;
                // }
                
              }
              else{ // i> nb_machine
                if(fds[i].revents & POLLIN){
                  memset(buffer,0,BUFFERSIZE);

                  fds[i].fd = fd_err[i-nb_machine][0]; // On le remet à l'indice zero
                  fds[i].events = POLLIN;

                  if( -1 == read(fds[i].fd,buffer,BUFFERSIZE)) 
                    ERROR_EXIT("ERROR dsmexec read on pipe stderr");

                  printf("LE PIPE  TE DIT : %s\n",buffer);
                  break;
                }
                
                // if (fds[i].revents & POLLHUP) {
                //   // Fermeture de notre côté du tube
                //   if (close(-1 == fd_err[i][0])) {
                //     ERROR_EXIT("close POLLHUP");
                //   }
                //   inactifs++;
                // }
                
              }
            }
          }
        
      }

    /* on attend les processus fils */
     sigaction(SIGCHLD, &action, NULL);
     /* on ferme les descripteurs proprement */

    for(i = 0; i<2* nb_machine; i++) {
      close(fds[i].fd);
    }

    /* on ferme la socket d'ecoute */
    close(sockfd);
    
    fclose(fp);
    free(proc_array);
    //free(buffer);
    free(newargv);
    free(fd_out);
    free(fd_err);  
  }   

  exit(EXIT_SUCCESS);  
}

