// sender.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/select.h>

#define PORT 8080
#define MAX_PACKET_SIZE 1024
#define WINDOW_SIZE 4
#define TOTAL_PACKETS 10
#define TIMEOUT 3   // seconds

typedef struct {
    int seqNo;
    char data[100];
} Packet;

int main() {
    int sockfd;
    struct sockaddr_in serverAddr;
    socklen_t addr_size = sizeof(serverAddr);
    srand(time(NULL));

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    Packet window[WINDOW_SIZE];
    int base = 0, nextSeq = 0;
    int ackNo;
    struct timeval tv;

    printf("[Sender: Ready] Starting Go-Back-N ARQ...\n");

    while (base < TOTAL_PACKETS) {

        while (nextSeq < base + WINDOW_SIZE && nextSeq < TOTAL_PACKETS) {
            Packet pkt;
            pkt.seqNo = nextSeq;
            sprintf(pkt.data, "Packet %d", nextSeq);

            printf("[Sender: Ready] Making packet %d\n", pkt.seqNo);

            if (rand() % 10 < 8) { // 80% chance to send
                sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr*)&serverAddr, addr_size);
                printf("[Sender: Sent] %s (seqNo=%d)\n", pkt.data, pkt.seqNo);
            } else {
                printf("[Sender: Lost] Simulated loss for Packet %d\n", pkt.seqNo);
            }

            window[nextSeq % WINDOW_SIZE] = pkt;
            nextSeq++;
        }

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        tv.tv_sec = TIMEOUT;
        tv.tv_usec = 0;

        int activity = select(sockfd + 1, &readfds, NULL, NULL, &tv);

        if (activity > 0 && FD_ISSET(sockfd, &readfds)) {
            recvfrom(sockfd, &ackNo, sizeof(ackNo), 0, (struct sockaddr*)&serverAddr, &addr_size);
            printf("[Sender: ACK received] ackNo=%d\n", ackNo);

            if (ackNo >= base) {
                base = ackNo + 1;
                printf("[Sender: Slide Window] base moved to %d\n", base);
            }
        } else {
            printf("[Sender: Timeout] Resending window...\n");
            for (int i = base; i < nextSeq; i++) {
                sendto(sockfd, &window[i % WINDOW_SIZE], sizeof(Packet), 0, (struct sockaddr*)&serverAddr, addr_size);
                printf("[Sender: Resent] Packet %d\n", window[i % WINDOW_SIZE].seqNo);
            }
        }
    }

    printf("[Sender: Done] All packets acknowledged.\n");
    close(sockfd);
    return 0;
}
