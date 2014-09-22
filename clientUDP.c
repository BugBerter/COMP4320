#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdint.h>
#include <unistd.h>

#define PORT "10010"
#define BACKLOG 10

// vlength operation constants
#define VLENGTH_OPERATION 0x55

// disvowel operation constants
#define DISVOWEL_OPERATION 0xAA

struct udpsetup
{
    int sockfd;
    struct addrinfo *server;
};

struct client_request
{
    uint16_t request_length;
    uint16_t request_id;
    uint8_t  operation;
    uint16_t message_length;
    char *message;
};

struct server_response {
    uint16_t length;
    uint16_t rid;
    char msg[968]; // per biaz, total message not more than 1Kb
} __attribute__((__packed__));

// handles calling socket and bind, returns the sock file descriptor
// ready to be read()
struct udpsetup* udp_connect();

// implements the protocol for this lab
struct client_request* read_request(int sockfd);

int main(int argc, char const *argv[])
{
    struct udpsetup *con = udp_connect();

    sendto(con->sockfd, MSG, MSG_LEN, 0, (struct sockaddr*)&con->server, sizeof con->server);

    close(con->sockfd);
    return 0;
}

struct udpsetup* udp_connect(char *hostname)
{
    int status, sockfd;
    struct addrinfo hints, *res, *p;

    struct udpsetup *result = malloc(sizeof(struct udpsetup));

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(hostname, PORT, &hints, &res)) != 0) {
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

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "talker: failed to bind socket\n");
        exit(2);
    }

    result->sockfd = sockfd;
    result->server = p;

    return result;
}

struct client_request* read_request(int sockfd)
{
    uint16_t length, request_id, message_length;
    uint8_t operation;
    struct client_request *request = malloc(sizeof(struct client_request));

    // get header information:
    read(sockfd, &length, 2);
    read(sockfd, &request_id, 2);
    read(sockfd, &operation, 1);

    // no need to ntoh_(operaton) because it's only a single byte
    // no need to ntohs(request_id) because we only send it back to the client
    length = ntohs(length);

    // read the message contents:
    // there are always 5 bytes in protocol header
    message_length = length - 5;
    char *message;
    message = malloc(message_length + 1); // 1 extra so we can insert the null byte

    read(sockfd, message, message_length);
    message[message_length] = '\0'; // so we can treat the message as a normal string

    // populate our struct
    request->request_length = length;
    request->request_id = request_id;
    request->operation = operation;
    request->message_length = message_length;
    request->message = message;

    return request;
}