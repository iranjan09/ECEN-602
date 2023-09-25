#include "common.h"

// Read data from a file descriptor
static ssize_t read_data(int fd, char *ptr, size_t maxlen) {
    ssize_t n, rc;
    char c;

    for (n = 1; n < maxlen; n++) {
        if ((rc = read(fd, &c, 1)) == 1) {
            *ptr++ = c;
            if (c == '\n') {
                break;  // Newline is stored
            }
        } else if (rc == 0) {
            if (n == 1) {
                return 0;  // No data read
            } else {
                break;  // EOF, some data read
            }
        } else {
            if (errno == EINTR) {
                continue;  // Interrupted by signal
            }
            return -1;  // Error
        }
    }

    *ptr = '\0';  // Null terminate like fgets()
    return n;
}

// Write data to a file descriptor
ssize_t write_data(int fd, const void *buf_ptr, size_t n) {
    const char *ptr = buf_ptr;
    ssize_t total_bytes_written = 0;

    while (total_bytes_written < n) {
        ssize_t bytes_written = write(fd, ptr + total_bytes_written, n - total_bytes_written);
        if (bytes_written < 0) {
            if (errno == EINTR) {
                continue;  // Interrupted by signal, try writing again
            } else {
                return -1;  // Error
            }
        } else if (bytes_written == 0) {
            return total_bytes_written;  // All data written
        }

        total_bytes_written += bytes_written;
    }

    return total_bytes_written;
}

// Send data over a socket
int send_data(int sockfd, const char *data) {
    size_t len = strlen(data);
    ssize_t ret;

    ret = write_data(sockfd, data, len);

    if (ret < 0) {
        perror("Send failed");
    }

    return ret;
}

// Receive data from a socket
int receive_data(int sockfd, char *buffer, int buf_size) {
    ssize_t ret = read_data(sockfd, buffer, buf_size);

    if (ret < 0) {
        perror("Receive failed");
    }

    return ret;
}
