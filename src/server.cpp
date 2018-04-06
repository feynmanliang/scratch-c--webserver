#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#define PORTNO "3000"

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd;
     socklen_t clilen;
     char buffer[256];
     struct sockaddr_storage cli_addr;
     int n;

      // `getaddrinfo`, does DNS resolution and sets up `addrinfo` struct
      // since we're building a server we use NULL pointer for hostname and
      // rely on AI_PASSIVE to fill it in
     int status;
     struct addrinfo hints, *servinfo;
     memset(&hints, 0, sizeof hints);
     hints.ai_family = AF_UNSPEC;     // IPv4 or IPv6 both work
     hints.ai_socktype = SOCK_STREAM; // TCP stream socket
     hints.ai_flags = AI_PASSIVE; // returned socket address structure intended for bind; fill in hostname
     if ((status = getaddrinfo(NULL, PORTNO, &hints, &servinfo)) != 0) {
         fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
         exit(1);
     }

     // creates a socket
     sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
     if (sockfd < 0)
        error("ERROR opening socket");

     // allow socket to be reused immediately
     int yes = 1;
     if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1)
         error("ERROR setting socket option");

     // binds socket to localhost:3000 (specified in servinfo) and starts listening
     if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) < 0)
              error("ERROR on binding");
     listen(sockfd, 0); // no backlog

     clilen = sizeof cli_addr;
     newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
     if (newsockfd < 0)
          error("ERROR on accept");
     memset(buffer, 0, 256);
     n = read(newsockfd,buffer,255);
     if (n < 0) error("ERROR reading from socket");
     printf("Here is the message: %s\n",buffer);
     n = write(newsockfd,"I got your message",18);
     if (n < 0) error("ERROR writing to socket");
     close(newsockfd);
     close(sockfd);

     freeaddrinfo(servinfo);
     return 0;
}
