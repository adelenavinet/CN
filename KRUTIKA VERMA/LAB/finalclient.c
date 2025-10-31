#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// Frame structure
struct Frame {
    int seq; // sequence number (0 or 1)
    char data[1024];
    int checksum;
};

// Function declarations of FSM states
void wait_ack0(int sockfd, struct sockaddr_in servaddr, socklen_t len);
void wait_ack1(int sockfd, struct sockaddr_in servaddr, socklen_t len);

// Function pointer
typedef void (*StateFn)(int, struct sockaddr_in, socklen_t);

// FSM state pointer
StateFn state_fn;

// Computing Checksum
int compute_checksum(struct Frame *f) {
    int sum = f->seq;
    for (int i = 0; i < strlen(f->data); i++)
        sum += f->data[i];
    return sum;
}

// FSM State: wait_ack0
void wait_ack0(int sockfd, struct sockaddr_in servaddr, socklen_t len) {
    struct Frame frame, ack;
    strcpy(frame.data, "Message containing Sequence = 0");
    frame.seq = 0;
    frame.checksum = compute_checksum(&frame);

    printf("\nAarav is sending frame with Seq. No. = %d\n", frame.seq);
    sendto(sockfd, &frame, sizeof(frame), 0,
           (struct sockaddr *)&servaddr, len);

    int n = recvfrom(sockfd, &ack, sizeof(ack), 0,
                     (struct sockaddr *)&servaddr, &len);
    if (n > 0 && ack.seq == 0 && ack.checksum == compute_checksum(&ack)) {
        printf("ACK0 received, now waiting for ACK1\n");
        state_fn = wait_ack1;
    } else {
        printf("Received Invalid/Corrupted ACK! Retrying...\n");
    }
}

// FSM State: wait_ack1
void wait_ack1(int sockfd, struct sockaddr_in servaddr, socklen_t len) {
    struct Frame frame, ack;
    strcpy(frame.data, "Message containing Sequence = 1");
    frame.seq = 1;
    frame.checksum = compute_checksum(&frame);

    printf("\nAarav is sending frame with Seq. No. = %d\n", frame.seq);
    sendto(sockfd, &frame, sizeof(frame), 0,
           (struct sockaddr *)&servaddr, len);

    int n = recvfrom(sockfd, &ack, sizeof(ack), 0,
                     (struct sockaddr *)&servaddr, &len);
    if (n > 0 && ack.seq == 1 && ack.checksum == compute_checksum(&ack)) {
        printf("ACK1 received, now waiting for ACK1\n");
        state_fn = wait_ack0;
    } else {
        printf("Received Invalid/Corrupted ACK! Retrying...\n");
    }
}

int main() {
    int sockfd;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(8080);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    socklen_t len = sizeof(servaddr);

    // Start FSM in wait_ack0 state
    state_fn = wait_ack0;

    int n;
    printf("Enter the number of packets to send to server: ");
    scanf("%d", &n);
    // Run finite-state machine for user-input transmissions
    for (int i = 0; i < n; i++) {
        state_fn(sockfd, servaddr, len);
    }

    close(sockfd);
    return 0;
}

