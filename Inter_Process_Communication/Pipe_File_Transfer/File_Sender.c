#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define FIFO_NAME "file_fifo"
#define CHUNK 1024

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    char *input_file = argv[1];

    // Open input file
    FILE *fp = fopen(input_file, "rb");
    if (!fp) {
        perror("Error opening input file");
        return 1;
    }

    // Get total file size
    struct stat st;
    if (stat(input_file, &st) < 0) {
        perror("Error getting file size");
        return 1;
    }
    long file_size = st.st_size;
    printf("File size: %ld bytes\n", file_size);

    // Create FIFO if not exists
    mkfifo(FIFO_NAME, 0666);

    // Open FIFO for writing
    int fifo_fd = open(FIFO_NAME, O_WRONLY);
    if (fifo_fd < 0) {
        perror("Error opening FIFO");
        return 1;
    }

    // Send file size first
    write(fifo_fd, &file_size, sizeof(long));

    char buffer[CHUNK];
    long sent = 0;
    size_t n;

    // Send file in chunks
    while ((n = fread(buffer, 1, CHUNK, fp)) > 0) {
        write(fifo_fd, buffer, n);
        sent += n;

        printf("\rSent: %ld / %ld bytes", sent, file_size);
        fflush(stdout);
    }

    printf("\nTransfer completed.\n");

    fclose(fp);
    close(fifo_fd);
    return 0;
}
