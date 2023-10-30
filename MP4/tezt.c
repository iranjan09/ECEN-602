
#include "include.h"

char *get_filename(char *str) {
    const char *separator = "/";
    const char *last_slash = strrchr(str, separator[0]);
    
    if (last_slash != NULL) {
        return strdup(last_slash + 1);
    } else {
        return strdup(str); // If there's no slash, the entire string is treated as the filename.
    }
}

int main(int argc, char *argv[]) {
  	
	
	if (argc != 4) {
        printf("Client ERROR: Please provide Proxy Server IP address, port number and URL\n");
		printf("Usage: ./client <Proxy IP> <Port> <URL>");
        return 1;
    }
	char *server_proxy = argv[1], *file;
    char hostname[256]; 
    uint16_t port_number;
    char path[512];
    char *url = argv[3];
	port_number = atoi(argv[2]);
	struct sockaddr_in proxy;
	int ret = 0, msglen = 0;
	int sockfd = -1;
	FILE *fp;
	char buf[BUFFER_SIZE];
	char request[200];
	
	
    if (argc != 4) {
        printf("Client ERROR: Please provide Proxy Server IP address, port number and URL\n");
		printf("Usage: ./client <Proxy IP> <Port> <URL>");
        return 1;
    }

    // Parse the URL to extract the hostname, port, and path


    if (strstr(url, "https://") == url) {
        url += 8; // Skip "https://"
    } else if (strstr(url, "http://") == url) {
        url += 7; // Skip "http://"
    }

    if (sscanf(url, "%255[^/]/%255s", hostname, path) < 2) {
        sscanf(url, "%255[^/]", hostname);
        path[0] = '\0'; // No path specified
    }

    // Create the GET request
     // Adjust the size as needed
    int request_size = snprintf(request, sizeof(request), "GET /%s HTTP/1.0\r\nHost:%s\r\n\r\n", path, hostname);

    if (request_size < 0 || request_size >= sizeof(request)) {
        printf("Client ERROR: Request buffer overflow\n");
        return 1;
    }

    printf("Proxy Address: %s\nProxy Port: %d\nURL: %s\n", server_proxy, port_number, url);
    printf("Hostname: %s\n Path: %s\n", hostname, path);
    printf("GET Request:\n%s", request);
	
	memset(&proxy, 0, sizeof(struct sockaddr_in));
    proxy.sin_family = AF_INET;
    proxy.sin_port = htons(port_number);
	ret = inet_pton (AF_INET, server_proxy, &(proxy.sin_addr));
	
    if (ret < 0) {
       perror ("Client ERROR: IP conversion failed:");
	   exit(EXIT_FAILURE);
	}
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
    if(sockfd < 0) {
       perror("Client ERROR: Socket Creation failed:");
       exit(EXIT_FAILURE);
    }
    /* Connect to server */
    ret = connect (sockfd, (struct sockaddr *)&proxy, sizeof(proxy));
	
    if (ret < 0) {
        perror ("ERROR: Socket Connection failed:");
		exit(EXIT_FAILURE);
	}

	if((send(sockfd, request, strlen(request), 0)) == -1){
        perror("Client Error: Sending Failed:");
        exit(EXIT_FAILURE);
    }
	memset(buf, 0 , BUFFER_SIZE);
	file = get_filename(path);
	fp = fopen(file, "w");
	
	if ((msglen = recv(sockfd, buf, BUFFER_SIZE, 0)) <= 0) {
          perror("Client Error: Unable to receive from proxy:");
    } else if (strncmp(buf, "404", 3) == 0) {
        // Handle 404 error
        printf("Client Error: Received 404 error: %s", buf);
        remove(file);
    } else {
		//Extract the data to a temp_buf
        char *temp_buf = strstr(buf, "\r\n\r\n");

        if (temp_buf != NULL) {
            // Write the response body to the file
            fwrite(temp_buf + 4, 1, strlen(temp_buf) - 4, fp);
            memset(buf, 0, BUFFER_SIZE);

            while ((msglen = recv(sockfd, buf, BUFFER_SIZE, 0)) > 0) {
				//Write to buffer if more data coming
                fwrite(buf, 1, msglen, fp);
                memset(buf, 0, BUFFER_SIZE);
            }

            if (msglen < 0) {
                perror("Receive Error:");
            }
        }
    }

    fclose(fp);
    close(sockfd);
    return 0;
}
		




