#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAXLINE 1024

// Frame structure
struct Frame {
    int seq;
    char data[1024];
    int checksum;
};

// Computing Checksum-
int compute_checksum(struct Frame *f) {
    int sum = f->seq;
    for (int i = 0; i < strlen(f->data); i++)
        sum += f->data[i];
    return sum;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len = sizeof(cliaddr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { perror("socket"); exit(1); }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[1]));
    servaddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        exit(1);
    }

    printf("This is the Server, listening on port %s...\n", argv[1]);

    int expected_seq = 1;
    struct Frame frame, ack;

    while (1) {
        int n = recvfrom(sockfd, &frame, sizeof(frame), 0,
                         (struct sockaddr*)&cliaddr, &len);
        if (n < 0) { perror("recvfrom"); continue; }

        printf("\nReceived Frame: seq=%d, data=%s, checksum=%d\n",
               frame.seq, frame.data, frame.checksum);

        // Check corruption
	// If frame's checksum is not equal to the computed checksum
        if (frame.checksum != compute_checksum(&frame)) {
            printf("Corrupted Frame! Resending last ACK%d\n", (expected_seq+1)%2);
            ack.seq = (expected_seq+1)%2;
            strcpy(ack.data, "ACK");
            ack.checksum = compute_checksum(&ack);
            sendto(sockfd, &ack, sizeof(ack), 0,
                   (struct sockaddr*)&cliaddr, len);
            continue;
        }

        if (frame.seq == expected_seq) {
            printf("Accepted Frame %d\n", frame.seq);
            printf("Delivered: %s\n", frame.data);

            ack.seq = expected_seq;
            strcpy(ack.data, "ACK");
            ack.checksum = compute_checksum(&ack);
            sendto(sockfd, &ack, sizeof(ack), 0,
                   (struct sockaddr*)&cliaddr, len);

            expected_seq = 1 - expected_seq;
        } else {
            printf("Duplicate Frame %d! Resending ACK%d\n", frame.seq, 1-expected_seq);
            ack.seq = 1 - expected_seq;
            strcpy(ack.data, "ACK");
            ack.checksum = compute_checksum(&ack);
            sendto(sockfd, &ack, sizeof(ack), 0,
                   (struct sockaddr*)&cliaddr, len);
        }
    }

    close(sockfd);
    return 0;
}

