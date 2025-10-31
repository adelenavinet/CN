#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char message[BUFFER_SIZE] = {0};

    // 1. Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // 2. Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    printf("Connected to server. Type 'logout' to exit.\n");

    // 3. Communication loop
    while (1) {
        printf("Enter message: ");
        fgets(message, BUFFER_SIZE, stdin);

        // Remove trailing newline character
        message[strcspn(message, "\n")] = 0;

        // Send message to server
        send(sock, message, strlen(message), 0);

        // Check if user wants to logout
        if (strcmp(message, "logout") == 0) {
            printf("Logging out...\n");
            break;
        }

        // Clear buffer and receive response from server
        memset(buffer, 0, BUFFER_SIZE);
        int read_size = read(sock, buffer, BUFFER_SIZE - 1);
        if (read_size > 0) {
             printf("Server echo: %s\n", buffer);
        } else if (read_size == 0) {
            printf("Server closed the connection\n");
            break;
        } else {
            perror("read failed");
            break;
        }
    }

    // 4. Close socket
    close(sock);
    return 0;
}