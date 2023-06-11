#include <stdio.h>     
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>   
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define MAXPENDING 5
#define FLOWERS_COUNT 40
#define GARGENER_COUNT 2

pthread_mutex_t mutex;
int flowers[FLOWERS_COUNT];
int flowers_count = 0;

typedef struct thread_args {
    int socket;
} thread_args;

void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}

void *clientThread(void *args) {
    int server_socket;
    int client_socket;
    pthread_t threadId;
    struct sockaddr_in client_addr;
    int client_length = sizeof(client_addr);
    pthread_detach(pthread_self());
    server_socket = ((thread_args*)args)->socket;
    free(args);

    int buffer[3];
    for (;;) {
        recvfrom(server_socket, buffer, sizeof(buffer), 0, (struct sockaddr*) &client_addr, &client_length);
        //printf("[%s:%d] => data\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
        pthread_mutex_lock(&mutex);
        int client_index = buffer[0];
        if (flowers_count > 0) {
            int fl = flowers[--flowers_count];
            printf("Giving flower #%d to #%d\n", fl, client_index);
            buffer[1] = fl;
        } else {
            buffer[1] = -1;
        }
        pthread_mutex_unlock(&mutex);
        sendto(server_socket, buffer, sizeof(buffer), 0, (struct sockaddr*) &client_addr, sizeof(client_addr));
        //printf("[%s:%d] <= data\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
    }
}

void *flowersThread(void *args) {
    int server_socket;
    int client_socket;
    pthread_t threadId;
    struct sockaddr_in client_addr;
    int client_length = sizeof(client_addr);
    pthread_detach(pthread_self());
    server_socket = ((thread_args*)args)->socket;
    free(args);
    
    int buffer[3];
    for (;;) {
        recvfrom(server_socket, buffer, sizeof(buffer), 0, (struct sockaddr*) &client_addr, &client_length);
        pthread_mutex_lock(&mutex);
        int flower_index = buffer[0];
        printf("FLower #%d is dying\n", flower_index);
        flowers[flowers_count++] = flower_index;
        sleep(2);
        buffer[1] = flower_index;
        pthread_mutex_unlock(&mutex);
        sendto(server_socket, buffer, sizeof(buffer), 0, (struct sockaddr*) &client_addr, sizeof(client_addr));
        sleep(1);
    }
}

int createTCPSocket(unsigned short server_port) {
    int server_socket;
    struct sockaddr_in server_addr;

    if ((server_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) DieWithError("socket() failed");
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;              
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    server_addr.sin_port = htons(server_port);

    if (bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) DieWithError("bind() failed");
    printf("Open socket on %s:%d\n", inet_ntoa(server_addr.sin_addr), server_port);
    return server_socket;
}

int main(int argc, char *argv[])
{
    unsigned short server_port;
    unsigned short flowers_port;
    int server_socket;
    int flowers_socket;
    pthread_t threadId;
    pthread_mutex_init(&mutex, NULL);
    if (argc != 3)
    {
        fprintf(stderr, "Usage:  %s <Port for clients> <Port for flowers>\n", argv[0]);
        exit(1);
    }

    server_port = atoi(argv[1]);
    flowers_port = atoi(argv[2]);

    server_socket = createTCPSocket(server_port);
    flowers_socket = createTCPSocket(flowers_port);


    for (int i = 0; i < GARGENER_COUNT; i++) {
        thread_args *args = (thread_args*) malloc(sizeof(thread_args));
        args->socket = server_socket;
        if (pthread_create(&threadId, NULL, clientThread, (void*) args) != 0) DieWithError("pthread_create() failed");
    }
    

    thread_args *args1 = (thread_args*) malloc(sizeof(thread_args));
    args1->socket = flowers_socket;
    if (pthread_create(&threadId, NULL, flowersThread, (void*) args1) != 0) DieWithError("pthread_create() failed");

    for (;;) {
        sleep(1);
    }
    pthread_mutex_destroy(&mutex);
    return 0;
}