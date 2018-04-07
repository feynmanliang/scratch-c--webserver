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

class Request {
    public:
        std::string verb;
        std::string path;
        std::string httpVersion;
        std::map<std::string, std::string> headers;

        Request(const std::string &buffer) {
            std::vector<std::string> lines;
            boost::split(lines, buffer, boost::is_any_of("\n"));

            auto it  = lines.begin();

            // parse request line
            std::vector<std::string> request_line;
            boost::split(request_line, *it, boost::is_any_of(" "));
            this->verb = request_line.at(0);
            this->path = request_line.at(1);
            this->httpVersion = request_line.at(2);

            // parse headers
            for (++it; (it != lines.end()) && (it->size() > 1); ++it) {
                boost::sregex_token_iterator i(it->begin(), it->end(), boost::regex(": "), -1);
                boost::sregex_token_iterator j;
                if (i != j)
                    this->headers.insert(std::pair<std::string, std::string>(*i, *i++));
            }
        }

        ~Request() { }
};

class Server {
    int port;
    int sockfd;
    struct addrinfo *servinfo;

    public:
        Server(int port) {
            this->port = port;
        }

        ~Server() {
            freeaddrinfo(this->servinfo);
            close(this->sockfd);
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
            if ((status = getaddrinfo(nullptr, std::to_string(this->port).c_str(), &hints, &servinfo)) != 0) {
                fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
                exit(1);
            }

            // creates a socket
            this->sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
            if (this->sockfd < 0)
                error("ERROR opening socket");

            // allow socket to be reused immediately
            const int yes = 1;
            if (setsockopt(this->sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1)
                error("ERROR setting socket option");

            // binds socket to localhost:3000 (specified in servinfo) and starts listening
            if (bind(this->sockfd, servinfo->ai_addr, servinfo->ai_addrlen) < 0)
                error("ERROR on binding");
            listen(this->sockfd, 0); // no backlog

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
            newsockfd = accept(this->sockfd, (struct sockaddr *) &cli_addr, &clilen);
            if (newsockfd < 0)
                error("ERROR on accept");

            // read() is the generic fd syscall, recv() is specific to sockets
            memset(buffer, 0, 256);
            n = recv(newsockfd, buffer, 255, 0);
            if (n < 0) error("ERROR reading from socket");

            Request r(buffer);

            std::cout << r.verb << " " << r.path << " " <<  r.httpVersion << std::endl;
            for (const auto &kvp : r.headers) {
                std::cout << kvp.first << ": " << kvp.second << std::endl;
            }

            // sends the response
            n = write(newsockfd,"I got your message",18);
            if (n < 0) error("ERROR writing to socket");

            // close client connection
            close(newsockfd);
        }
};


int main(int argc, char *argv[]) {
    Server s(3000);
    s.start();
    return 0;
}
