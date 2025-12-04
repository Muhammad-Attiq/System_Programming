#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define FIFO_NAME "file_fifo"
#define CHUNK 1024

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <output_file>\n", argv[0]);
        return 1;
    }

    char *output_file = argv[1];

    // Create FIFO if needed
    mkfifo(FIFO_NAME, 0666);

    // Open FIFO for reading
    int fifo_fd = open(FIFO_NAME, O_RDONLY);
    if (fifo_fd < 0) {
        perror("Error opening FIFO");
        return 1;
    }

    // Read expected file size
    long file_size;
    read(fifo_fd, &file_size, sizeof(long));

    printf("Expected file size: %ld bytes\n", file_size);

    // Open output file
    FILE *fp = fopen(output_file, "wb");
    if (!fp) {
        perror("Error creating output file");
        close(fifo_fd);
        return 1;
    }

    char buffer[CHUNK];
    long received = 0;
    ssize_t n;

    // Receive data until EOF
    while ((n = read(fifo_fd, buffer, CHUNK)) > 0) {
        fwrite(buffer, 1, n, fp);
        received += n;

        printf("\rReceived: %ld / %ld bytes", received, file_size);
        fflush(stdout);

        if (received >= file_size) break;
    }

    printf("\nTransfer complete.\n");

    fclose(fp);
    close(fifo_fd);

    // Integrity check
    if (received == file_size)
        printf("Integrity check PASSED.\n");
    else
        printf("Integrity check FAILED! Received %ld bytes.\n", received);

    return 0;
}
