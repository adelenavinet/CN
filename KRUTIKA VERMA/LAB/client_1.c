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

void wait_for_ack_0(int socket_fd, struct sockaddr_in server_address, socklen_t address_length);
void wait_for_ack_1(int socket_fd, struct sockaddr_in server_address, socklen_t address_length);

typedef void (*State)(int, struct sockaddr_in, socklen_t);
State current_state;

int calculate_checksum(struct Packet *p) {
    int sum = p->sequence;
    for (int i = 0; i < strlen(p->content); i++)
        sum += p->content[i];
    return sum;
}

void transmit_and_receive(int socket_fd, struct sockaddr_in server_address, socklen_t address_length, int sequence_number) {
    struct Packet outgoing_packet, incoming_ack;
    sprintf(outgoing_packet.content, "Message containing Sequence = %d", sequence_number);
    outgoing_packet.sequence = sequence_number;
    outgoing_packet.check_sum = calculate_checksum(&outgoing_packet);
    printf("\nSending frame with Seq. No. = %d\n", outgoing_packet.sequence);
    sendto(socket_fd, &outgoing_packet, sizeof(outgoing_packet), 0, (struct sockaddr *)&server_address, address_length);
    int bytes_received = recvfrom(socket_fd, &incoming_ack, sizeof(incoming_ack), 0, (struct sockaddr *)&server_address, &address_length);
    if (bytes_received > 0 && incoming_ack.sequence == sequence_number && incoming_ack.check_sum == calculate_checksum(&incoming_ack)) {
        printf("ACK%d received. Waiting for ACK%d now.\n", sequence_number, 1 - sequence_number);
        current_state = (sequence_number == 0) ? wait_for_ack_1 : wait_for_ack_0;
    } else {
        printf("Received invalid or corrupted ACK! Retrying...\n");
    }
}

void wait_for_ack_0(int socket_fd, struct sockaddr_in server_address, socklen_t address_length) {
    transmit_and_receive(socket_fd, server_address, address_length, 0);
}

void wait_for_ack_1(int socket_fd, struct sockaddr_in server_address, socklen_t address_length) {
    transmit_and_receive(socket_fd, server_address, address_length, 1);
}

int main() {
    int socket_fd;
    struct sockaddr_in server_address;
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8080);
    server_address.sin_addr.s_addr = INADDR_ANY;
    socklen_t address_length = sizeof(server_address);
    current_state = wait_for_ack_0;
    int packet_count;
    printf("Number of packets to send: ");
    scanf("%d", &packet_count);
    for (int i = 0; i < packet_count; i++) {
        current_state(socket_fd, server_address, address_length);
    }
    close(socket_fd);
    return 0;
}