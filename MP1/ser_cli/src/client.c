#include "common.h"

int server_sock_init(struct sockaddr_in *sockaddr, char *ip_addr, int port) {

	memset (sockaddr, 0, sizeof(*sockaddr));
	sockaddr->sin_family = AF_INET;
	sockaddr->sin_port = htons (port);

    if(inet_pton(AF_INET, ip_addr, &(sockaddr->sin_addr)) < 0) {
		perror("inet_pton error");
		return -1;
	}

    return 0;
}

int main(int argc, char *argv[]) {

	int sockfd = -1;
	int ret;
	char ip_addr[16], send_buf[BUFFER_SIZE], recv_buf[BUFFER_SIZE+1];
	struct sockaddr_in serv_sockaddr;
    	int port = -1;
	int t_flag = 0; // Set this flag to close the socket after sending to server for test case 4

    // Input validation
	if (argc != 3) {
		printf("Usage: %s <ip_addr> <port>\n", argv[0]);
		return -1;
	}

	strcpy(ip_addr, argv[1]);
	port = atoi(argv[2]);
  
	printf ("CLIENT: Server Port number is %s \n", argv[2]);
	printf ("CLIENT: Server IPv4 Address is %s \n", argv[1]);
	
    // Unable to create socket, exit
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("ERROR: Socket creation failed");
		exit(1);
	}
	
	if(server_sock_init(&serv_sockaddr, ip_addr, port) < 0) {
		perror("ERROR: Server socket init failed");
		exit(1);
	}
	
	// Connect to server
	if(connect(sockfd, (struct sockaddr*)&serv_sockaddr, sizeof(serv_sockaddr)) < 0) {
		perror("ERROR: Connection to server failed");
		exit(1);
	}

	printf("CLIENT: Connected to Server\n");

	// Client communication loop
	while(1) {
	  
		memset (send_buf, 0, BUFFER_SIZE);
		memset (recv_buf, 0, BUFFER_SIZE + 1);
		// Read input from user
		fgets(send_buf, BUFFER_SIZE, stdin);
		
		if (strlen (send_buf) == 0) {
			printf("CLIENT: Close the connection to Server because No data/EOF\n");
			close(sockfd);
			exit(1);
		}
		
		// Send line to server
		printf ("CLIENT: Sending Data: %s\n", send_buf);
		if(send_data(sockfd, send_buf) < 0) {
			perror ("ERROR: Client failed to send data to server");
			break;
		}

		if (t_flag == 1) {
		    close(sockfd);
		    exit(0);	
		}
		
		//Receive data from receiver
		ret = receive_data(sockfd, recv_buf, BUFFER_SIZE);
        if (ret <= 0) {
            if (ret == 0) {
                printf("CLIENT: Server disconnected.\n");
            } else {
                perror("ERROR: Reading from socket failed");
            }
            close(sockfd);
            exit(1);
        }
		else
		{	
        recv_buf [ret] = '\0';
		}
		ret = 0;
		printf ("CLIENT: Received Data: %s\n", recv_buf);
  		

	}

	// Close socket
	close(sockfd);

	return 0;
}
