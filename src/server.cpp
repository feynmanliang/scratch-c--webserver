#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

void error(const char *msg) {
    perror(msg);
    exit(1);
}

struct Request {
    std::string verb;
    std::string path;
    std::string httpVersion;
};

class Server {
    int port;
    int sockfd;
    struct addrinfo *servinfo;

    public:
        Server(int p) {
            port = p;
        }

        ~Server() {
            freeaddrinfo(servinfo);
            close(sockfd);
        };

        /**
         * Sets up a TCP listener on specified port and returns the file descriptor.
         */
        void start() {
            struct addrinfo hints;

            // `getaddrinfo`, does DNS resolution and sets up `addrinfo` struct
            // since we're building a server we use NULL pointer for hostname and
            // rely on AI_PASSIVE to fill it in
            int status;
            memset(&hints, 0, sizeof hints);
            hints.ai_family = AF_UNSPEC;     // IPv4 or IPv6 both work
            hints.ai_socktype = SOCK_STREAM; // TCP stream socket
            hints.ai_flags = AI_PASSIVE; // returned socket address structure intended for bind; fill in hostname
            char portstr[8];
            sprintf(portstr, "%i", port);
            if ((status = getaddrinfo(NULL, portstr, &hints, &servinfo)) != 0) {
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

            this->handle_message();
        }

        /**
         * Handles a single message and exits
         */
        void handle_message() {
            int newsockfd;
            socklen_t clilen;
            char buffer[256];
            int n;
            struct sockaddr_storage cli_addr;

            // accept the client connection
            clilen = sizeof cli_addr;
            newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
            if (newsockfd < 0)
                error("ERROR on accept");

            // read() is the generic fd syscall, recv() is specific to sockets
            memset(buffer, 0, 256);
            n = recv(newsockfd, buffer, 255, 0);
            if (n < 0) error("ERROR reading from socket");

            Request r = this->parse_request(std::string(buffer, 255));

            std::cout << r.verb << " " << r.path << " " <<  r.httpVersion << std::endl;

            // sends the response
            n = write(newsockfd,"I got your message",18);
            if (n < 0) error("ERROR writing to socket");

            // close client connection
            close(newsockfd);
        }

        Request parse_request(std::string buffer) {
            Request r;
            std::vector<std::string> lines;
            boost::split(lines, buffer, boost::is_any_of("\n"));

            std::vector<std::string> request_line;
            boost::split(request_line, lines.at(0), boost::is_any_of(" "));
            r.verb = request_line.at(0);
            r.path = request_line.at(1);
            r.httpVersion = request_line.at(2);
            return r;
        }
};


int main(int argc, char *argv[]) {
    Server s(3000);
    s.start();
    return 0;
}
