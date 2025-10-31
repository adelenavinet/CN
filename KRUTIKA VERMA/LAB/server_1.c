#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

struct Packet {
    int sequence; 
    char content[1024];
    int check_sum;
};

int calculate_checksum(struct Packet *p) {
    int sum = p->sequence;
    for (int i = 0; i < strlen(p->content); i++)
        sum += p->content[i];
    return sum;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    int socket_fd, expected_seq = 1;
    struct sockaddr_in server_address, client_address;
    socklen_t client_len = sizeof(client_address);
    struct Packet incoming_packet, acknowledgement;
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(argv[1]));
    server_address.sin_addr.s_addr = INADDR_ANY;
    if (bind(socket_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Binding failed");
        exit(1);
    }
    printf("Server is ready, listening on port %s...\n", argv[1]);
    while (1) {
        int bytes_received = recvfrom(socket_fd, &incoming_packet, sizeof(incoming_packet), 0, (struct sockaddr*)&client_address, &client_len);
        if (bytes_received < 0) continue;
        printf("\nReceived frame: seq=%d, data='%s'\n", incoming_packet.sequence, incoming_packet.content);
        if (incoming_packet.check_sum == calculate_checksum(&incoming_packet) && incoming_packet.sequence == expected_seq) {
            printf("Valid frame %d received. Data delivered.\n", incoming_packet.sequence);
            acknowledgement.sequence = expected_seq;
            expected_seq = 1 - expected_seq;
        } else {
            const char* message = (incoming_packet.check_sum != calculate_checksum(&incoming_packet)) ? "Corrupted" : "Duplicate";
            printf("%s frame %d detected. Resending ACK for previous frame (%d).\n", message, incoming_packet.sequence, 1 - expected_seq);
            acknowledgement.sequence = 1 - expected_seq;
        }
        strcpy(acknowledgement.content, "ACK");
        acknowledgement.check_sum = calculate_checksum(&acknowledgement);
        sendto(socket_fd, &acknowledgement, sizeof(acknowledgement), 0, (struct sockaddr*)&client_address, client_len);
    }
    close(socket_fd);
    return 0;
}