#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    int server_sock, new_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    char buffer[BUFFER_SIZE];
    int n;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port_number>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Error in socket creation");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(argv[1]));

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error in bind");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    listen(server_sock, 5);
    printf("Server listening on port %s...\n", argv[1]);

    client_len = sizeof(client_addr);
    new_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
    if (new_sock < 0) {
        perror("Error on accept");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Connection accepted from client with IP: %s and port: %d\n",
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    while (1) {
        n = read(new_sock, buffer, BUFFER_SIZE);
        if (n <= 0) {
            printf("Client disconnected.\n");
            break;
        }
        buffer[n] = '\0';
        printf("Client: %s\n", buffer);

        if (strcmp(buffer, "close") == 0) {
            printf("Received 'close' message. Closing connection.\n");
            break;
        }

        printf("Server (You): ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';

        write(new_sock, buffer, strlen(buffer));
    }

    close(new_sock);
    close(server_sock);
    return 0;
}