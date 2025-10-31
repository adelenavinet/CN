// receiver.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080
#define MAX_PACKET_SIZE 1024
#define TOTAL_PACKETS 10

typedef struct {
    int seqNo;
    char data[100];
} Packet;

int main() {
    int sockfd;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addr_size = sizeof(clientAddr);
    srand(time(NULL));

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    printf("[Receiver: Ready] Waiting for packets...\n");

    int expectedSeq = 0;
    Packet pkt;
    int ackNo;

    while (expectedSeq < TOTAL_PACKETS) {
        int n = recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr*)&clientAddr, &addr_size);

        if (rand() % 10 < 2) { // 20% corruption
            printf("[Receiver: Corrupted] Packet %d discarded\n", pkt.seqNo);
            continue;
        }

        if (pkt.seqNo == expectedSeq) {
            printf("[Receiver: Ready] In-order packet %d received → Delivering message\n", pkt.seqNo);
            expectedSeq++;
        } else {
            printf("[Receiver: Discard] Out-of-order packet %d (expected %d) discarded\n", pkt.seqNo, expectedSeq);
        }

        ackNo = expectedSeq - 1;
        sendto(sockfd, &ackNo, sizeof(ackNo), 0, (struct sockaddr*)&clientAddr, addr_size);
        printf("[Receiver: Sent ACK] ackNo=%d\n", ackNo);
    }

    printf("[Receiver: Done] All packets received.\n");
    close(sockfd);
    return 0;
}
