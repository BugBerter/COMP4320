#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#define DEBUG

#ifdef DEBUG
#define PRINTD printf
#else
#define PRINTD(format, args...) ((void)0)
#endif

#define MAX_PACKET_SIZE 1024
#define URL_ARRAY_LENGTH (size_t)(MAX_PACKET_SIZE - 6 + 1)

struct hostnames {
    uint16_t numhosts;
    char **hostnames;
};

struct client_request {
    uint16_t length;
    uint8_t checksum;
    uint8_t groupid;
    uint8_t rid;
    uint8_t delim;
    uint8_t computed_checksum;
    uint16_t computed_length;
    struct hostnames* hostnames;
    struct sockaddr* addr;
    socklen_t sock_len;
} __attribute__((__packed__));

struct packet {
    uint16_t length;
    uint8_t checksum;
    uint8_t groupid;
    uint8_t rid;
    uint8_t delim;
    char urls[URL_ARRAY_LENGTH];
} __attribute__((__packed__));

// biaz recommend for sending:
//      store data into struct in network byte order (htons(..), etc.)
// then take the server_response and cast to (char *)
struct server_response {
    uint16_t length;
    uint8_t checksum;
    uint8_t groupid;
    uint8_t rid;
    char msg[1019]; // per biaz, total message not more than 1Kb = 1024
} __attribute__((__packed__));

int udp_connect(char *port)
{
    int status, sockfd;
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, port, &hints, &res)) != 0) {
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
    PRINTD("Set up the socket file descriptor correctly\n");

    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1)
    {
        close(sockfd);
        perror("server: bind");
        exit(1);
    }
    PRINTD("bound the socket\n");

    return sockfd;
}

uint8_t checksum(char *str, size_t len) {
    // used to take any overflow from the first byte into the second
    const uint16_t CARRY_MASK = 0xFF00;

    size_t i;
    uint16_t sum = 0x0000;
    for (i = 0; i < len; ++i) {
        sum += str[i];
        sum += (sum & CARRY_MASK) >> 8;
    }
    // take the complement per the algorithm
    return (uint8_t)~sum;
}

struct hostnames *spliton(char *str, size_t len, char delim) {
    int count = 1;
    int i;
    // TODO: potential for segfault if the url list
    // is exactly the length of the urls array
    for (i = 0; i >= len || str[i] != '\0'; ++i) {
        if (str[i] == delim) ++count;
    }
    PRINTD("Counted %d delims\n", count);

    char *delimstr = malloc(2);
    delimstr[0] = delim;
    delimstr[1] = '\0';

    // see http://www.cplusplus.com/reference/cstring/strtok/ for details on strtok
    char **result = calloc(count, sizeof(char *));
    char *s = strtok(str, delimstr);
    i = 0;
    while (s != NULL) {
        if (i >= count) {
            break;
        }

        PRINTD("strtok returned on the %dth iteration: %s\n", i, s);
        result[i++] = s;
        s = strtok(NULL, delimstr);
    }

    struct hostnames *hostnames = malloc(sizeof(struct hostnames));
    hostnames->numhosts = count;
    hostnames->hostnames = result;

    PRINTD("Num hostnames: %d\n", hostnames->numhosts);
    #ifdef DEBUG
        if (hostnames->numhosts > 0){
            PRINTD("%s\n", hostnames->hostnames[0]);   
        }
    #endif

    return hostnames;
}

struct client_request *read_request(int sockfd)
{
    PRINTD("Waiting to read client request\n");

    struct packet *p = calloc(1, sizeof(struct packet));
    struct sockaddr *their_addr = malloc(sizeof(struct sockaddr));
    struct client_request *request = malloc(sizeof(struct client_request));

    // change to recvfrom and return the struct
    socklen_t addr_len = sizeof their_addr;
    ssize_t bytes_recieved = recvfrom(sockfd, p, sizeof(struct packet), 0, (struct sockaddr *)their_addr, &addr_len);

    PRINTD("Recieved from client:\n");
    PRINTD("\t%c%s\n", p->delim, p->urls);

    request->checksum = p->checksum;
    request->groupid = p->groupid;
    request->rid = p->rid;
    request->delim = p->delim;
    request->length = ntohs(p->length);
    request->computed_length = bytes_recieved;
    request->hostnames = spliton(p->urls, URL_ARRAY_LENGTH, request->delim);
    request->addr = their_addr;
    request->sock_len = addr_len;

    p->checksum = 0;
    request->computed_checksum = checksum((char *)p, sizeof(struct packet));

    PRINTD("Checksum from client: %d\nComputed checksum: %d\n", request->checksum, request->computed_checksum);

    return request;
}


uint32_t get_ip_addr(struct addrinfo *info) {
    struct sockaddr *addr = info->ai_addr;
    PRINTD("Got ai_addr\n");
    if (addr->sa_family != AF_INET) {
        PRINTD("ai_addr->sa_family != AF_INET\n");
        return htonl(0xFFFFFFFF);
    } else {
        struct sockaddr_in addr_in = *(struct sockaddr_in *)addr;
        PRINTD("Getting sin_addr from addr_in\n");
        PRINTD("%x %x %x\n", addr_in.sin_family, addr_in.sin_port, addr_in.sin_addr.s_addr);

        return htonl(addr_in.sin_addr.s_addr);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Correct usage is UDPServer port#\n");
        exit(1);
    }

    int sockfd = udp_connect(argv[1]);
    uint16_t message_length;
    struct server_response *response;
    struct client_request *request;

    for (;;)
    {
        request = read_request(sockfd);
        if ((request->checksum ^ request->computed_checksum) != 0x00)
        {
            char *buf = malloc(5 * sizeof(uint8_t));
            // ignore the actual fields here, we're matching the byte pattern:
            //   checksum, GID, RequestID, 0x00, 0x00
            buf[1] = request->groupid;
            buf[2] = request->rid;
            buf[3] = 0x00;
            buf[4] = 0x00;
            buf[0] = checksum(buf, 5);
            PRINTD("Checksums neq, sending: %x %x %x %x %x\n", buf[0], buf[1], buf[2], buf[3], buf[4]);

            sendto(sockfd, response, 5, 0, request->addr, request->sock_len);

        } else if (request->computed_length != request->length){
            response = malloc(sizeof(struct server_response));
            // ignore the actual fields here, we're matching the byte pattern:
            //   checksum, 127, 127, 0x00, 0x00
            buf[1] = 127;
            buf[2] = 127;
            buf[3] = 0x00;
            buf[4] = 0x00;
            buf[0] = checksum(buf, 5);
            PRINTD("Checksums neq, sending: %x %x %x %x %x\n", buf[0], buf[1], buf[2], buf[3], buf[4]);
            
            sendto(sockfd, response, 5, 0, request->addr, request->sock_len);
        } else {

            uint32_t ip;
            int status, i, pos, j = 0;
            struct addrinfo *hints, *res;
            hints = malloc(sizeof(struct addrinfo));

            response = calloc(1, sizeof(struct server_response));

            hints->ai_family = AF_INET;
            hints->ai_socktype = SOCK_DGRAM;
            hints->ai_flags = AI_PASSIVE;

            PRINTD("Numhosts = %d\n", request->hostnames->numhosts);
            for (i = 0; i < request->hostnames->numhosts; ++i) {
                getaddrinfo(request->hostnames->hostnames[i], "http", hints, &res);
                ip = get_ip_addr(res);
                PRINTD("Got IP address of %d from hostname %s\n", ip, request->hostnames->hostnames[i]);

                response->msg[j] = (uint8_t)((ip >> 24) & 0xFF);
                response->msg[j + 1] = (uint8_t)((ip >> 16) & 0xFF);
                response->msg[j + 2] = (uint8_t)((ip >> 8) & 0xFF);
                response->msg[j + 3] = (uint8_t)(ip & 0xFF);

                PRINTD("%02x %02x %02x %02x\n", (uint8_t)((ip >> 24) & 0xFF), (uint8_t)((ip >> 16) & 0xFF), (uint8_t)((ip >> 8) & 0xFF), (uint8_t)(ip & 0xFF));
                j += 4;
            }

            response->length = htons(5 + j);
            response->groupid = request->groupid;
            response->rid = request->rid;
            response->checksum = checksum((char *)response, sizeof(struct server_response));

            PRINTD("Sending: %x %x %x %x ", response->length, response->checksum, response->groupid, response->rid);
            #ifdef DEBUG
                for (i = 0; i < 1019; i++) {
                    printf("%02x", response->msg[i]);   
                }
                printf("\n");
            #endif

            sendto(sockfd, response, response->length, 0, request->addr, request->sock_len);
        }

        free(request->hostnames);
        free(request->addr);
        free(request);
    }

    close(sockfd);

    return 0;
}