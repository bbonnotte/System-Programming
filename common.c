#include "common_impl.h"

int creer_socket(int port_num) 
{
   int yes=1;
   int sockfd = 0;
   struct sockaddr_in myaddr;
   socklen_t size_struct_sockaddr = sizeof(struct sockaddr_in);

   /* fonction de creation et d'attachement */
    sockfd= socket(AF_INET,SOCK_STREAM,0);

        if (-1 ==  sockfd)
        {
          ERROR_EXIT("ERROR socket");
        }

    if (-1 == setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR, &yes, sizeof(int)))
        {
        	ERROR_EXIT("ERROR setting socket options");
        }    

        memset(&myaddr,'\0',size_struct_sockaddr);
        memset(&(myaddr.sin_zero),0,8);
        myaddr.sin_family = AF_INET;
        myaddr.sin_addr.s_addr = htonl(INADDR_ANY); // Utilise mon adresse ip
        myaddr.sin_port = htons(port_num);

        // binding
     if (-1 == bind(sockfd,(struct sockaddr *)&myaddr,size_struct_sockaddr))
     	ERROR_EXIT("ERROR bind");

       // Renvoie l'adresse sur laquel la socket est attaché
    if(-1 == getsockname(sockfd,(struct sockaddr *)&myaddr,&size_struct_sockaddr)) {
            ERROR_EXIT("ERROR getsockname");
    }
   /* d'une nouvelle socket */

   /* renvoie le numero de descripteur */
   /* et modifie le parametre port_num */
    // *port_num = ntohs(myaddr.sin_port);

   return sockfd;
}

// source : http://www.binarytides.com/hostname-to-ip-address-c-sockets-linux/
// Cette fonction s'occupe de faire la conversion nom de machine a l'adresse ip
int hostname_to_ip(char * hostname , char* ip)
{
    struct hostent *he;
    struct in_addr **addr_list;
    int i;
         
    if ( (he = gethostbyname( hostname ) ) == NULL) 
    {
        // get the host info
        herror("gethostbyname");
        return 1;
    }
  
    addr_list = (struct in_addr **) he->h_addr_list;
     
    for(i = 0; addr_list[i] != NULL; i++) 
    {
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 0;
    }
     
    return 1;
}

void get_addr_info(struct sockaddr_in * server, char * name, int port)
{
  struct hostent * server_host;
  memset(server, 0, sizeof(struct sockaddr_in));
  server->sin_family = AF_INET;
  server->sin_port = htons(port);
  server_host = gethostbyname(name);
  memcpy(&(server->sin_addr), server_host->h_addr, server_host->h_length);
}


/* Vous pouvez ecrire ici toutes les fonctions */
/* qui pourraient etre utilisees par le lanceur */
/* et le processus intermediaire. N'oubliez pas */
/* de declarer le prototype de ces nouvelles */
/* fonctions dans common_impl.h */