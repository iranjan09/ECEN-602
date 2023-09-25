#include "common.h"

#define CHAR_PER_READ 1

static int read_cnt;
static char *read_ptr;
static char read_buf[BUFFER_SIZE];

//Read/Write functions from W. Richard Stevens, Bill Fenner, and Andrew M. Rudoff, Unix Network
//Programming, Volume 1, The Sockets Networking API, 3rd Edition, Addison- Wesley,
//2004, Chapter 3.


static ssize_t my_read(int fd, char *ptr) {
    if (read_cnt <= 0) {
    again:
        if ((read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0) {
            if (errno == EINTR)
                goto again;
            return (-1);
        } else if (read_cnt == 0)
            return (0);
        read_ptr = read_buf;
    }
    read_cnt--;
    *ptr = *read_ptr++;
    return (1);
}

ssize_t readLine(int fd, void *vptr, size_t maxlen) {
    ssize_t n, rc;
    char c, *ptr;

    ptr = vptr;
    for (n = 1; n < maxlen; n++) {
        if ((rc = my_read(fd, &c)) == 1) {
            *ptr++ = c;
            if (c == '\n')
                break; 
        } else if (rc == 0) {
            *ptr = 0;
            return (n - 1); 
        } else
            return (-1); 
    }
    *ptr = 0; 
    return (n);
}

ssize_t writen(int fd, void *buf_ptr, size_t bufSize) {
  
  ssize_t bytes_left = bufSize; 
  ssize_t bytes_written;
  char *ptr;
  
  ptr = buf_ptr;

  while (bytes_left > 0) {
    if ((bytes_written = write(fd, ptr, bytes_left)) <= 0) {
      if (bytes_written < 0 && errno == EINTR) {
        bytes_written = 0; 
      }
      else {
        return -1; 
      }
    }

    bytes_left -= bytes_written;
    ptr += bytes_written;
  }

  return bufSize;
}

int send_data(int sockfd, char *data) {

	int len, ret;

	// Get data length
	len = strlen(data);

	// Send data
	ret = writen(sockfd, data, len);

	// Error handling
	if (ret < 0) {
		perror("Send failed");
	}

	return ret;

}

int receive_data(int sockfd, char *buffer, int buf_size) {

	int ret;

	// Receive data
	ret = readLine(sockfd, buffer, buf_size);

	// Error handling
	if (ret < 0) {
		perror("Receive failed");
	}
	return ret;

}
