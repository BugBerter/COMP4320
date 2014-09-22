#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>

#define HEADER_LENGTH 0x5

// vlength operation constants
#define VLENGTH_OPERATION 0x55

// disvowel operation constants
#define DISVOWEL_OPERATION 0xAA

struct tcpsetup {
    int sockfd;
    struct addrinfo *server;
};

struct client_request {
    uint16_t length;
    uint16_t rid;
    uint8_t operation;
    char msg[1019]; // per biaz, total message not more than 1Kb
} __attribute__((__packed__)); // stop the compiler from padding uint16_t's into 32 bit ints

struct server_response {
    uint16_t length;
    uint16_t rid;
    char msg[1020];
} __attribute__((__packed__));

struct tcpsetup* tcp_connect(char *hostname, char *port) {
    int status, sockfd;
    struct addrinfo hints, *res, *p;

    struct tcpsetup *result = malloc(sizeof(struct tcpsetup));

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(hostname, port, &hints, &res)) != 0) {
        printf("Failed to get address info\n");
        exit(1);
    }
    else
    {
        //printf("ai_flags: 0x%x\nai_family: 0x%x\nai_socktype: 0x%x\nai_protocol: 0x%x\nai_addrlen: 0x%x\nai_canonname %s\n",
        //    res->ai_flags, res->ai_family, res->ai_socktype, res->ai_protocol, res->ai_addrlen, res->ai_canonname);
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

struct client_request* prepare_request(char **argv) {
    uint8_t operation = (uint8_t)atoi(argv[3]);
    int len = strlen(argv[4]);

    struct client_request *req = malloc(sizeof(struct client_request));
    req->length = htons(HEADER_LENGTH + len);
    req->rid = htons(1);
    req->operation = operation;
    memcpy(req->msg, argv[4], len);
    
    return req;   
}

int main(int argc, char *argv[])
{

    if (argc < 5) {
        printf("The correct usage is: clientTCP <servername> <port number> <operation> <string>\n");
        exit(1);
    }

    struct timeval before, after;

    struct tcpsetup *con = tcp_connect(argv[1], argv[2]);

    struct client_request *req = prepare_request(argv);

    gettimeofday(&before, NULL);
    //printf("Start time: %ld\n", before.tv_usec);

    printf("Sending:\n\tLength: %d\n\tRID: %d\n\tOperation: %x\n\tMessage: %s\n",
        ntohs(req->length), ntohs(req->rid), req->operation, req->msg);

    send(con->sockfd, req, req->length, 0);

    struct server_response *response = malloc(sizeof(struct server_response));
    recv(con->sockfd, response, 1024, 0);

    gettimeofday(&after, NULL);
    //printf("End time: %ld\n", after.tv_usec);

    suseconds_t dt = (after.tv_usec - before.tv_usec);

    if (req->operation == 0x55) {
        uint16_t length = response->msg[1] | (response->msg[0] << 8);

        printf("Recieved:\n\tLength: %d\n\tRID: %d\n\tMessage:%d\nRound Trip Time: %ld microseconds\n",
                    ntohs(response->length), ntohs(response->rid), length, dt);
    } else if (req->operation == 0xAA) {
        printf("Recieved:\n\tLength: %d\n\tRID: %d\n\tMessage:%s\nRound Trip Time: %ld microseconds\n",
                    ntohs(response->length), ntohs(response->rid), response->msg, dt);
    }

    close(con->sockfd);
    return 0;
}