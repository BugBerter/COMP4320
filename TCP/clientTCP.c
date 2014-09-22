#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdint.h>
#include <unistd.h>

#define HEADER_LENGTH 0x5

// vlength operation constants
#define VLENGTH_OPERATION 0x55

// disvowel operation constants
#define DISVOWEL_OPERATION 0xAA

struct tcpsetup
{
    int sockfd;
    struct addrinfo *server;
};

struct client_request {
    uint16_t length;
    uint16_t rid;
    char msg[968]; // per biaz, total message not more than 1Kb = 1k - 16 - 16
} __attribute__((__packed__));

struct tcpsetup* tcp_connect(char *hostname, char *port)
{
    int status, sockfd;
    struct addrinfo hints, *res, *p;

    struct tcpsetup *result = malloc(sizeof(struct tcpsetup));

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(hostname, port, &hints, &res)) != 0) {
        printf("Failed to get address info\n");
        exit(1);
    }
    else
    {
        printf("ai_flags: 0x%x\nai_family: 0x%x\nai_socktype: 0x%x\nai_protocol: 0x%x\nai_addrlen: 0x%x\nai_canonname %s\n",
            res->ai_flags, res->ai_family, res->ai_socktype, res->ai_protocol, res->ai_addrlen, res->ai_canonname);
    }

    for(p = res; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "tcp client: failed to bind socket\n");
        exit(2);
    }

    result->sockfd = sockfd;
    result->server = p;

    return result;
}

int main(int argc, char *argv[])
{

    if (argc < 5) {
        printf("The correct usage is: clientTCP <servername> <port number> <operation> <string>\n");
        exit(1);
    }

    struct tcpsetup *con = tcp_connect(argv[1], argv[2]);

    int len = strlen(argv[4]);

    struct client_request *req = malloc(sizeof(struct client_request));
    req->length = htons(HEADER_LENGTH + len);
    req->rid = htons(1234);
    memcpy(req->msg, argv[4], len);

    printf("Sending request with info:\n\tlength: %d\n\trid: %d\n\tmessage: %s\n", ntohs(req->length), ntohs(req->rid), req->msg);

    sendto(con->sockfd, (char *)req, req->length, 0, (struct sockaddr*)&con->server, sizeof con->server);

    close(con->sockfd);
    return 0;
}