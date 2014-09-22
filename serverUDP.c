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

// expects a 16 bit integer
#define LOW_BITS(x) (char)(x & 0xFF)
#define HIGH_BITS(x) (char)(x >> 8)

#define HEADER_RESPONSE_LENGTH 0x4 // in bytes
// vlength operation constants
#define VLENGTH_OPERATION 0x55

// disvowel operation constants
#define DISVOWEL_OPERATION 0xAA

struct client_request
{
    uint16_t request_length;
    uint16_t request_id;
    uint8_t  operation;
    uint16_t message_length;
    char *message;
};

// biaz recommend for sending:
//      store data into struct in network byte order (htons(..), etc.)
// then take the server_response and cast to (char *)
struct server_response {
    uint16_t length;
    uint16_t rid;
    char msg[968]; // per biaz, total message not more than 1Kb
} __attribute__((__packed__));

// handles calling socket and bind, returns the sock file descriptor
// ready to be read()
int udp_connect();

uint16_t num_vowels(char *msg);

char *disvowel(char *msg, uint16_t msg_length);

// implements the protocol for this lab
struct client_request* read_request(int sockfd);

int main(int argc, char const *argv[])
{
    int sockfd = udp_connect();
    struct client_request *request;
    uint16_t message_length;

    struct server_response *response;

    for (;;)
    {
        printf("Waiting to read client request\n");

        request = read_request(sockfd);

        if (request->operation == VLENGTH_OPERATION)
        {
            uint16_t vowels = htons(num_vowels(request->message));

            response = malloc(sizeof(struct server_response));
            response->length = htons(0x0006);
            response->rid = request->request_id; // already network endian
            memcpy(&response->msg, &vowels, 2);

            write(sockfd, (char *)response, 6);
        }
        else if (request->operation == DISVOWEL_OPERATION)
        {
            // + 1 for the null byte
            char *disvoweled = disvowel(request->message, message_length + 1);

            int c = 0,
                disvoweled_length = strlen(disvoweled),     // -1 for the null byte
                message_length = HEADER_RESPONSE_LENGTH + disvoweled_length - 1;

            response->length = htons(message_length);
            response->rid = request->request_id;
            memcpy(&response->msg, disvoweled, disvoweled_length);

            write(sockfd, (char *)response, message_length);
        }
        else
        {
            printf("Invalid operation: %X\n", request->operation);
        }

        // throw it away because we're done
        free(request);
    }

    close(sockfd);

    return 0;
}


int udp_connect()
{
    int status, sockfd;
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
        printf("Failed to get address info\n");
        exit(1);
    }
    else
    {
        printf("ai_flags: 0x%x\nai_family: 0x%x\nai_socktype: 0x%x\nai_protocol: 0x%x\nai_addrlen: 0x%x\nai_canonname %s\n",
            res->ai_flags, res->ai_family, res->ai_socktype, res->ai_protocol, res->ai_addrlen, res->ai_canonname);
    }

    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        close(sockfd);
        exit(1);
    }
    else
    {
        printf("Set up the socket file descriptor correctly\n");
    }

    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1)
    {
        close(sockfd);
        perror("server: bind");
        exit(1);
    }
    else
    {
        printf("bound the socket\n");
    }

    return sockfd;
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

uint16_t num_vowels(char *msg)
{
    uint16_t i, count;
    // we can always access msg[0] because we malloc message_length + 1 and insert
    // a null byte; in the worst case, i = 0 and msg[i] = '\0' and we skip right out.
    for (i = 0; msg[i] != '\0'; ++i)
    {
        switch(msg[i])
        {
            case 'a':
            case 'A':
            case 'e':
            case 'E':
            case 'i':
            case 'I':
            case 'o':
            case 'O':
            case 'u':
            case 'U':
                ++count;
            default:
                break;
        }
    }

    return count;
}

char *disvowel(char *msg, uint16_t msg_length)
{
    char *ret;
    uint16_t c, i;
    c = 0;

    // since char = 1 byte we don't need to multiply by sizeof(char)
    ret = malloc(msg_length - num_vowels(msg));
    for (i = 0; msg[i] != '\0'; ++i)
    {
        switch(msg[i])
        {
            case 'a':
            case 'A':
            case 'e':
            case 'E':
            case 'i':
            case 'I':
            case 'o':
            case 'O':
            case 'u':
            case 'U':
                 break;
            default:
                ret[c++] = msg[i];
                break;
        }
    }

    ret[c] = '\0';
    return ret;
}