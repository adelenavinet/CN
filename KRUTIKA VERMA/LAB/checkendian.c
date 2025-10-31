#include <stdio.h>
#include <stdint.h>

int checkEndianness() {
    unsigned int num = 1;
    char *ptr = (char *)&num;
    if (*ptr == 1) {
        return 1;
    } else {
        return 0;
    }
}
void printBytes(unsigned int num) {
    printf("Byte content of 0x%X:\n", num);
    char *ptr = (char *)&num;
    for (size_t i = 0; i < sizeof(unsigned int); i++) {
        printf("Byte %zu: 0x%02X\n", i, (unsigned char)ptr[i]);
    }
}
uint32_t convertEndianness(uint32_t val) {
    return ((val << 24) & 0xFF000000) |
           ((val << 8) & 0x00FF0000) |
           ((val >> 8) & 0x0000FF00) |
           ((val >> 24) & 0x000000FF);
}
int main() {
    if (checkEndianness()) {
        printf("Host machine is Little Endian.\n");
    } else {
        printf("Host machine is Big Endian.\n");
    }
    unsigned int number;
    printf("\nEnter an unsigned integer number: ");
    scanf("%x", &number);
    printBytes(number);
    uint32_t converted_number = convertEndianness(number);
    printf("\nNumber after endianness conversion: 0x%X\n", converted_number);
    printBytes(converted_number);
    return 0;
}
