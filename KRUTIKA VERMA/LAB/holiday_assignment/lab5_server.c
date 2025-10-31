#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// This function will be called when a child process terminates
void handle_sigchld(int sig) {
    // waitpid() with WNOHANG will reap all terminated children without blocking
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

// Function to handle communication with a single client
void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    int read_size;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Get client IP and port for logging
    getpeername(client_socket, (struct sockaddr*)&client_addr, &addr_len);
    printf("Handler assigned for client %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // Loop to receive messages from the client
    while ((read_size = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        // Null-terminate the received string
        buffer[read_size] = '\0';

        // Remove trailing newline character if present
        if (buffer[read_size - 1] == '\n') {
            buffer[read_size - 1] = '\0';
        }

        printf("Client (%s:%d) sent: %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buffer);

        // Check for "logout" message
        if (strcmp(buffer, "logout") == 0) {
            printf("Client %s:%d sent logout. Closing connection.\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            break; // Exit the loop to close connection
        }

        // Echo the message back to the client
        send(client_socket, buffer, strlen(buffer), 0);
    }

    if (read_size == 0) {
        printf("Client %s:%d disconnected.\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    } else if (read_size == -1) {
        perror("recv failed");
    }

    close(client_socket);
    exit(0); // Terminate the child process
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    pid_t child_pid;

    // Set up the signal handler to reap zombie processes
    struct sigaction sa;
    sa.sa_handler = &handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, 0) == -1) {
        perror("sigaction");
        exit(1);
    }

    // 1. Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Optional: Allow reuse of port
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // 2. Bind socket to port
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Listen on all available interfaces
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // 3. Listen for connections
    if (listen(server_fd, 5) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    // 4. Accept connections in a loop
    while (1) {
        // Accept a new connection
        if ((client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len)) < 0) {
            perror("accept failed");
            continue; // Continue to the next iteration
        }

        printf("Connection accepted from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // 5. Fork a child process to handle the client
        if ((child_pid = fork()) == 0) {
            // This is the child process
            close(server_fd); // Child doesn't need the listener socket
            handle_client(client_socket);
        } else if (child_pid > 0) {
            // This is the parent process
            close(client_socket); // Parent doesn't need the client-specific socket
        } else {
            perror("fork failed");
        }
    }

    close(server_fd);
    return 0;
}