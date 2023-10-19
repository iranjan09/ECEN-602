#include "include.h"

void handle_zombie(int sigtemp_fd)
{
	//wait on zombie process
    wait(NULL);
}

// From UNP, vol 1, 1st Ed, p. 499]
FILE * temp_netascii_file(FILE *fd, char * file_net){
    srand(time(0));
    int n = rand();
    sprintf(file_net, "temp_file_%d.txt", n);
    FILE * temp_fd = fopen(file_net, "w");
    int c;
    int nextChar = -1;
    while(1){
        c = getc(fd);
        if(nextChar >= 0){
            fputc(nextChar, temp_fd);
            nextChar = -1;
        }
        if(c == EOF){
            if(ferror(temp_fd)){
                perror("Error writing to Temp File");
            }
            break;
        }
        else if(c == '\n'){
            c = '\r';
            nextChar = '\n';
        }
        else if(c == '\r'){
            nextChar = '\0';
        }
        else{
            nextChar = -1;
        }
        fputc(c,temp_fd);
    }
    fclose(temp_fd);
    return fopen(file_net, "r");
}

ssize_t transmitData(int server_fd, mess_recv_t * msg, size_t len, struct sockaddr_in *addr, socklen_t socklen){
	//Utilize sendto function
	ssize_t sent = sendto(server_fd, msg, len, 0, (struct sockaddr *) addr, socklen);
	if(sent < 0)
	{
		perror("Data sending to client failed:");
	}
	return sent;
}

ssize_t send_tftp_ack(int sock_fd, struct sockaddr_in * addr, socklen_t socklen, uint16_t block_num)
{
	mess_recv_t ack;
	//Create ack packet
	ack.opCode = htons(ACK_EVENT);
	ack.tftp_ack.block_num = htons(block_num);
	
	return transmitData(sock_fd, &ack, sizeof(ack.tftp_ack), addr, socklen);
}

ssize_t send_tftp_data(int sock_fd, struct sockaddr_in *addr, socklen_t socklen, uint16_t block_num, int mode, uint8_t *data, ssize_t data_length) {
    
    //TFTP data message
    mess_recv_t data_message;
	
	char msg[MAX_BUF];	
	int tot = 0;
	int extra = 0;//for future
		
	memcpy(msg, data, data_length);
	tot = extra + data_length;
	//Create data packet
    data_message.opCode = htons(DATA_EVENT);
    data_message.tftp_data.block_num = htons(block_num);
	//Copy data
    memcpy(data_message.tftp_data.data, msg, tot);
	return transmitData(sock_fd, &data_message, tot+4, addr, socklen);
}

ssize_t send_tftp_error(int sock_fd, int ec, char * error, struct sockaddr_in * addr, socklen_t socketlen)
{
	mess_recv_t err_msg;
	if(strlen(error) >= MAX_BUF){
		return 0;
	}
	//Create error packet
	err_msg.tftp_err.opCode = htons(ERR_EVENT);
	err_msg.tftp_err.err_code = htons(ec);
	//Copy error data
	strcpy((char *)err_msg.tftp_err.err_data, error);
	
	return transmitData(sock_fd, &err_msg, strlen(error)+5, addr, socketlen);
}


ssize_t tftp_recv(int sockfd, struct sockaddr_in *cli_addr, socklen_t * socklen, mess_recv_t *recv_buf){
	//Receive from the client
	ssize_t msglen = recvfrom(sockfd, recv_buf, sizeof(*recv_buf), 0, (struct sockaddr *) cli_addr, socklen);
	//Incorrect message received
	if(msglen < 0 && errno !=EAGAIN){
		perror("Error in tftp receive: ");
	}
	return msglen;
}

void handle_tftp_request(struct sockaddr_in *cli_addr, socklen_t socket_len, mess_recv_t *buf, ssize_t len, uint16_t opCode)
{
    ssize_t read_len, recvlen;
    char *filename, *null_terminator_position, *transfer_mode;
	FILE *file_descriptor, *temporary_file_fd;
    struct timeval timeout;//for timeout
	uint16_t block_number = 0;// which ack seq at
	int mode = 0, retryAttempts, flag = 0;    
    char temp_file[50];
	int server_fd;
	uint8_t data_buffer[512];//data to send
	mess_recv_t client_msg;
	
	//Create child socket
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("socket creation failed. Exit!");
        exit(EXIT_FAILURE);
    }
	//Initialize timer values
    timeout.tv_usec = 0;
    timeout.tv_sec = TIMEOUT;

	//Add timer to socket
    if (setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Setting socket timeoit failed");
        exit(EXIT_FAILURE);
    }

    printf("Client %s:%u Connected to TFTP server\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
	
	// Cast the buffer to a the request structure to get filename
    filename = (char *)buf->tftp_req.file_mode;

    // Locate the null terminator (0) in the trailing buffer to separate filename and mode
    null_terminator_position = &filename[len - 3];
	
	// Check if the null terminator was found
	if (*null_terminator_position != '\0') {
        printf("Requested invalid file name or mode incorrect\n");
		//Send error to client
		send_tftp_error(server_fd, UNDEFINED, "Filename invalid or transfermode incorrect", cli_addr, socket_len);
        exit(EXIT_FAILURE);
    }
	
	//Find the transfer mode
	transfer_mode = strchr(filename, '\0') + 1;
 
	//Got ahead error
    if (transfer_mode > null_terminator_position) {
        perror("Transfer mode not given");
		send_tftp_error(server_fd, UNDEFINED, "Filename invalid or transfermode incorrect", cli_addr, socket_len);
        exit(EXIT_FAILURE);
    }
	
	//Set the mode based on the string
	if (strcasecmp(transfer_mode, "netascii") == 0)
        mode = NETASCII;
    else if (strcasecmp(transfer_mode, "octet") == 0)
        mode = OCTET;
	else
		mode = 0;

    // Print the parsed filename and mode
    printf("Requested file: %s, Mode: %s\n", filename, transfer_mode);
	//Mode does not match NETASCII/OCTET mode
	if (mode == 0) {
		printf("Client %s:%u sent invalid mode\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
		//send to client the error
		send_tftp_error(server_fd, UNDEFINED, "Transfer mode invalid", cli_addr, socket_len);
		exit(EXIT_FAILURE);
	}
	//open the file for read/write
	file_descriptor = fopen(filename, opCode == RRQ_EVENT ? "r" : "w");
	// file not getting created!!
    if (file_descriptor == NULL)
    {
        printf("File does not exist\n");
        send_tftp_error(server_fd, FILE_NOT_FOUND, "File not found", cli_addr, socket_len);
        exit(EXIT_FAILURE);
    }

	printf("File name %s in  transfer mode %d %s request by %s:%u! \n", filename, mode,
            opCode == RRQ_EVENT ? "reading" : "writing", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));

    if (opCode == RRQ_EVENT)
    {
		//Need to convert the file for NETASCII
        if (mode == NETASCII)
        {
            temporary_file_fd = temp_netascii_file(file_descriptor,temp_file);
			//Close the file to assign
            fclose (file_descriptor);
            file_descriptor = temporary_file_fd;
        }
        while (!flag)
        {
			//Start reading
            read_len = fread(data_buffer, 1, sizeof(data_buffer), file_descriptor);
			//Move to next block to read
            block_number++;

            if (read_len < MAX_BUF)
            {
                flag = 1;  //Done reading
            }
		
			//Try for 10 attempts
			retryAttempts = RETRY_LIMIT;
            while (retryAttempts)
            {
				//Send data to client
                if (send_tftp_data(server_fd, cli_addr, socket_len, block_number, mode, data_buffer, read_len) < 0)
                {
					//Failed
                    printf("Client %s:%u TFTP session terminated\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
                    exit(EXIT_FAILURE);
                }
				
				//ACK/ERROR receive
                recvlen = tftp_recv(server_fd, cli_addr, &socket_len, &client_msg);

				//Frame size invalid
                if (recvlen >= 0 && recvlen < MIN_FRAME_LEN)
                {
					printf("Invalid request from %s:%d. Try again!\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
					//Send error to client
					send_tftp_error(server_fd, UNDEFINED, "Frame size is invalid", cli_addr, socket_len);
                    exit(EXIT_FAILURE);
                }

                if (recvlen >= MIN_FRAME_LEN)
                {
                    break;
                }
				//Erroneous packet
                if (errno != EAGAIN)
                {
                    printf("Client %s:%u TFTP session terminated.\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
                    exit(EXIT_FAILURE);
                }
				
				
                printf("Client %s:%u not responding. Trying again.\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
				//Decrement retries
                retryAttempts--;
            }

			//10 attempts failed close child process
            if (retryAttempts == 0)
            {
                printf("Retries failed. Transfer ended.\n");
                exit(EXIT_FAILURE);
            }
			
			//Client send error dump it and exit
            if (ntohs(client_msg.opCode) == ERR_EVENT)
            {
                printf("Client %s:%u sent error event message %u:%s\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port), ntohs(client_msg.tftp_err.err_code),
                        client_msg.tftp_err.err_data);
                exit(EXIT_FAILURE);
            }

			//Unknown Opcode from client
            if (ntohs(client_msg.opCode) != ACK_EVENT)
            {
                printf("Client %s:%u sent unknown opcode\n",inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
                send_tftp_error(server_fd, UNDEFINED, "Unknown Opcode. try again", cli_addr, socket_len);
                exit(EXIT_FAILURE);
            }
			
			//Ack sequence number not expected
            if (ntohs(client_msg.tftp_ack.block_num) != block_number) 
            {
                printf("Client %s:%u ACK number sent is high\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
				//Send error to client about invalid ACK
                send_tftp_error(server_fd, UNDEFINED, "ACK Invalid", cli_addr, socket_len);
                exit(EXIT_FAILURE);
            }
        }
    } else if (opCode == WRQ_EVENT) {
		//reinitialize the variables for WRQ
		block_number = 0;
        flag = 0;
        recvlen = 0;
        mess_recv_t read_write_msg;
         
		//Send Ack to client
        if (send_tftp_ack(server_fd, cli_addr, socket_len, block_number) < 0)
        { 
			//Client terminated session exit child process
            printf("Client %s:%u session ended\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
            exit(EXIT_FAILURE);
        }

        while (!flag)
        {
			//Try for 10 attempts
            retryAttempts = RETRY_LIMIT;
            while(retryAttempts)
            {
				//Client req handle
                recvlen = tftp_recv(server_fd, cli_addr, &socket_len, &read_write_msg);
				
				//Frame size of invalid length received
                if (recvlen >= 0 && recvlen < MIN_FRAME_LEN)
                {
					printf("Invalid request from client %s:%d. Try again!\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
					//Send to Client also
					send_tftp_error(server_fd, UNDEFINED, "Frame size is invalid", cli_addr, socket_len);
                    exit(EXIT_FAILURE);
                }

                if (recvlen >= MIN_FRAME_LEN)
                {
                    break;
                }

                if (errno != EAGAIN)
                {
                    printf("Client %s:%u TFTP session terminated\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
                    exit(EXIT_FAILURE);
                }
				
				//Send ack to client
                recvlen = send_tftp_ack(server_fd, cli_addr, socket_len, block_number);
				
				//Client responded in negative
                if (recvlen < 0)
                {
                    printf("Client %s:%u TFTP session terminated\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
                    exit(EXIT_FAILURE);
                }
				//Retrying again
                retryAttempts--;
            }
			
			//Tried 10 times exit child process if not successful
            if (retryAttempts == 0)
            {
                printf("Retries failed. Transfer ended.\n");
                exit(EXIT_FAILURE);
            }
			
			//Move to next block
            block_number++;

            if (recvlen < sizeof(read_write_msg.tftp_data))				
            {
                flag = 1; //End transmission last block
            }

			//Out of sequence ack received
            if (ntohs(read_write_msg.tftp_ack.block_num) != block_number)
            {
                printf("Client %s:%u ACK number sent is high\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
				//send error to client
                send_tftp_error(server_fd, UNDEFINED, "ACK Invalid", cli_addr, socket_len);
                exit(EXIT_FAILURE);
            }
			
			//Opcode not matching expected, close child
            if (ntohs(read_write_msg.opCode) != DATA_EVENT)
            {
               printf("Client %s:%u sent unknown opcode\n",inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
                send_tftp_error(server_fd, UNDEFINED, "Unknown Opcode. try again", cli_addr, socket_len);
                exit(EXIT_FAILURE);
            }
			
			//Client sent error message, dump and exit
            if (ntohs(read_write_msg.opCode) == ERR_EVENT)
            {
                printf("Client %s:%u sent error event message %u:%s\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port), 
																		ntohs(read_write_msg.tftp_err.err_code),
																		read_write_msg.tftp_err.err_data);
                exit(EXIT_FAILURE);
            }
			
			// Write to the file
            recvlen = fwrite(read_write_msg.tftp_data.data, 1, recvlen - MIN_FRAME_LEN, file_descriptor);
			
			//Write failed
            if (recvlen < 0)
            {
                perror("Server:Write error-)");
                exit(EXIT_FAILURE);
            }
			
			//After writing send ack
            recvlen = send_tftp_ack(server_fd, cli_addr, socket_len, block_number);
			
			//Client did not send ack
            if (recvlen < 0)
            {
				printf("Client %s:%u TFTP session terminated\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
                exit(EXIT_FAILURE);
            }
        }
    }
		
	//Done transferring, do cleanup
	printf("TFTP data sent successfully to %s:%u!\n", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
    fclose(file_descriptor);
    if(temp_file != NULL){
        remove(temp_file);
    }
    close(server_fd);
    exit(0);

}	


int main(int argc, char *argv[]) {
	
	if (argc < 2) {
        fprintf(stderr, "Usage: %s [directory] [port]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
	
	int sockfd;
    ssize_t recvlen;
    struct sockaddr_in serv_addr, cli_addr;
    
	uint16_t opCode, portno = 0;
	socklen_t socklen = sizeof(serv_addr);
	mess_recv_t recv_buf;


	if (argc > 2) {
		if (sscanf(argv[2], "%hu", &portno)) {
            portno = htons(portno);
        }
        else
        {
            fprintf(stderr, "Server: Error Port Number Invalid\n");
            exit(-1);
        }
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
        perror("SERVER: Error opening socket");
        exit(EXIT_FAILURE);
    }

    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = portno;


    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("SERVER: Error on binding");
        exit(EXIT_FAILURE);
    }

	signal(SIGCHLD, (void *)handle_zombie);
	
    printf("SERVER: TFTP Server Initialized, waiting for clients\n");
    
    while (1) {
        recvlen = tftp_recv(sockfd, &cli_addr, &socklen, &recv_buf);
        if (recvlen < 0) {
            continue;
        }

        if (recvlen < MIN_FRAME_LEN) {
            printf("Invalid request from %s:%d. Try again!\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
            send_tftp_error(sockfd, UNDEFINED, "Frame size is invalid", &cli_addr, socklen);
            continue;
        }
				  
		opCode = ntohs(recv_buf.opCode);

        if (opCode == RRQ_EVENT || opCode == WRQ_EVENT) {
            
            if (fork() == 0) {
				//Let Child proceess handle
                handle_tftp_request(&cli_addr, socklen, &recv_buf, recvlen, opCode);
                exit(EXIT_FAILURE);
            }
        } else {
            printf("Client %s:%d sent invalid request opcode\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
            send_tftp_error(sockfd, UNDEFINED, "Invalid opcode", &cli_addr, socklen);
        }
    }

    close(sockfd);
    printf("Closing Server socket.\n");
    return 0;
}
