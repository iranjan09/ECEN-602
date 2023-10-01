#include "common.h"

int create_socket(bool isIPv4)
{
    int socket_fd = -1;
    if (isIPv4 ==  true)
    {
        socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    }
    else
    {
        socket_fd = socket(AF_INET6, SOCK_STREAM, 0);
    }
    if(socket_fd == -1)
    {
        perror("Socket Create Faild");
        exit(-1);
    }
    else{
        printf("Socket is Created Successfully....\n");
    }
    return socket_fd;
}

void set_server_address(struct sockaddr_in *server_address, char * ip, int port)
{
    bzero(server_address, sizeof(*server_address));
    (*server_address).sin_family = AF_INET;
    (*server_address).sin_addr.s_addr = inet_addr(ip);
    (*server_address).sin_port = htons(port);
}

void set_server_address_ipv6(struct sockaddr_in6 *server_address, char * ip, int port)
{
    bzero(server_address, sizeof(*server_address));
    (*server_address).sin6_family = AF_INET6;
    if (inet_pton(AF_INET6, ip, &(*server_address).sin6_addr) <= 0)
        printf("inet_pton error for %s", ip);
    (*server_address).sin6_port = htons(port);
}

void bind_server(int socket_fd, struct sockaddr_in server_address)
{
    int val = bind(socket_fd, (struct sockaddr *) &server_address, sizeof(server_address));
    if(val != 0)
    {
        perror("Socket Bind Failed");
        exit(-1);
    }
    else
    {
        printf("Socket is binded Successfully....\n");
    }
}

void bind_server_ipv6(int socket_fd, struct sockaddr_in6 server_address)
{
    int val = bind(socket_fd, (struct sockaddr *) &server_address, sizeof(server_address));
    if(val != 0)
    {
        perror("Socket Bind Failed");
        exit(-1);
    }
    else
    {
        printf("Socket is binded Successfully....\n");
    }
}

void start_listening(int socket_fd)
{
    int val = listen(socket_fd, QUEUE_SIZE);
    if(val != 0)
    {
        perror("Listening Failed");
        exit(-1);
    }
    else
    {
        printf("Server Listening Started....\n");
    }
}

int accept_connection(struct sockaddr_in * client_addresses, int client_count, int socket_fd)
{
    socklen_t len = (socklen_t)sizeof(client_addresses[client_count]);
    int new_socket_fd = accept(socket_fd, (struct sockaddr *)&client_addresses[client_count], &len);
    if(new_socket_fd < 0)
    {
        perror("Accept connection error: ");
        exit(-1);
    }
    return new_socket_fd;
}

int check_name(char * name, int client_count, int max_client, struct user_data * clients){
    int i;
    if(client_count == max_client)
    {
        return -1;
    }
    for(i=0; i <client_count; i++)
    {
        if(strcmp(name, clients[i].user_name) == 0)
        {
            return -2;
        }
    }
    return 0;
}

void send_ack_message(int new_client_fd, int client_count, struct user_data * clients){
    int i;
	sbcp_message_t ack_message;
    sbcp_header_t ack_header;
    sbcp_attribute_t ack_attribute;

    ack_header.uiVrsn = 3;
    ack_header.uiType = 7;

    ack_attribute.uiType = 4;

    char msg[512];
    char client_num[50];
    sprintf(client_num, "%d", client_count);
    strcpy(msg, client_num);
    strcat(msg," ");
    for(i=0; i < client_count-1; i++)
    {
        strcat(msg,clients[i].user_name);
        if(i < (client_count-2))
        {
            strcat(msg, ",");
        }
    }
    ack_attribute.uiLength = strlen(msg)+1;
    strcpy(ack_attribute.acPayload, msg);
    ack_message.sMsgHeader = ack_header;
    ack_message.sMsgAttribute = ack_attribute;

    send(new_client_fd,(void *) &ack_message,sizeof(ack_message),0);
}

void send_nack_message(int new_client_fd, int client_count, struct user_data * clients, int code){
	sbcp_message_t nack_message;
    sbcp_header_t nack_header;
    sbcp_attribute_t nack_attribute;

    char msg[130];

    nack_header.uiVrsn = 3;
    nack_header.uiType = 5;

    nack_attribute.uiType = 1;

    if(code == -1)
    {
        strcpy(msg,"Max clients are already present. Wait till chatroom is free\n");
    }
    else if (code == -2)
    {
        strcpy(msg,"Duplicate username entered. Please enter unique username\n");
    }

    nack_attribute.uiLength = strlen(msg);
    strcpy(nack_attribute.acPayload, msg);

    nack_message.sMsgHeader = nack_header;
    nack_message.sMsgAttribute = nack_attribute;

    send(new_client_fd,(void *) &nack_message,sizeof(nack_message),0);

    close(new_client_fd);

}

int join_message_process(int new_client_fd, int *client_count, int max_client, struct user_data * clients){
	sbcp_message_t join_message;
    sbcp_attribute_t join_message_attr;
    recv(new_client_fd,(struct message *) &join_message,sizeof(join_message),0);
    join_message_attr = join_message.sMsgAttribute;
    char name[16];
    int ret = 0;
    strcpy(name, join_message_attr.acPayload);
    ret = check_name(name, *client_count, max_client, clients);
    if(ret < 0)
    {
        printf("Sending NAK.\n");
        send_nack_message(new_client_fd, *client_count, clients,ret);
        return -1;
    }
    else
    {
        strcpy(clients[*client_count].user_name, name);
        clients[*client_count].socket_fd = new_client_fd;
        clients[*client_count].user_number = *client_count;
        *client_count = (*client_count) + 1;
        send_ack_message(new_client_fd, *client_count, clients);
    }
    return 0;
}

void remove_client(struct user_data * clients, int socket_fd, int client_count)
{
	int index = 0;
	for(index=0; index < client_count; index++)
	{
		if(clients[index].socket_fd == socket_fd)
		{
			break;
		}
	}
	while(index+1 < client_count)
	{
		clients[index] = clients[index+1];
        index++;
	}
}

void broadcast_message(int listening_fd, int socket_fd, struct user_data * clients, int max_fd, fd_set * set1, int *client_count){

    sbcp_message_t message_from_client;
    sbcp_message_t message_to_client;

    int index, j;
	int recv_bytes = recv(socket_fd, (sbcp_message_t *) &message_from_client, sizeof(sbcp_message_t), 0);
	if(recv_bytes <= 0)
	{
		if(recv_bytes == 0)
		{
			printf("Connection closed by by socket %d", socket_fd);
			sbcp_message_t * hung_message;

			for(index=0; index < (*client_count); index++)
			{
				if(clients[index].socket_fd == socket_fd){
					hung_message = get_hung_message(clients[index].user_name);
					break;
				}
			}
			for(j = 0; j <=max_fd; j++)
			{
				if(FD_ISSET(j, set1))
				{
					if(j != listening_fd && j != socket_fd)
					{
						if(send(j, (void *) hung_message, sizeof(sbcp_message_t), 0) == -1){
							perror("Sending Message Error: ");
							exit(-1);
						}
					}
				}

			}
		}
		else
		{
			perror("Error receiving message");
			exit(-1);
		}
		close(socket_fd);
		FD_CLR(socket_fd, set1);
		remove_client(clients, socket_fd, *client_count);
		(*client_count) = (*client_count) - 1;
	}
	else
	{
        // Messages

		message_to_client.sMsgHeader.uiType = 3;
		message_to_client.sMsgHeader.uiVrsn = 3;
		message_to_client.sMsgAttribute.uiType= 4;

		for(index=0; index <= (*client_count); index++)
		{
			if(clients[index].socket_fd == socket_fd)
			{
				strcpy(message_to_client.sMsgAttribute.acPayload, clients[index].user_name);
                if (message_from_client.sMsgHeader.uiType == IDLE)
                {
                    strcat(message_to_client.sMsgAttribute.acPayload, " is idle");
                }
                else
                {
                    strcat(message_to_client.sMsgAttribute.acPayload, ": ");
                    strcat(message_to_client.sMsgAttribute.acPayload, message_from_client.sMsgAttribute.acPayload);
                }
                strcat(message_to_client.sMsgAttribute.acPayload,"\n");
                break;
			}
		}
		for(index=0; index <= max_fd; index++)
		{
			if (FD_ISSET(index, set1)) {
				if(index != listening_fd && index!=socket_fd)
				{
					if(send(index, (void *) &message_to_client, sizeof(sbcp_message_t), 0) == -1){
						perror("Sending Message Error: ");
						exit(-1);
					}
				}
			}

		}
	}

}

sbcp_message_t * get_join_message(char * new_user_name){
	sbcp_message_t * message_to_client = malloc(sizeof(sbcp_message_t));
	(*message_to_client).sMsgHeader.uiType = 3;
	(*message_to_client).sMsgHeader.uiVrsn = 3;
	(*message_to_client).sMsgAttribute.uiType= 4;
	strcat((*message_to_client).sMsgAttribute.acPayload, new_user_name);
	strcat((*message_to_client).sMsgAttribute.acPayload, " ");
	strcat((*message_to_client).sMsgAttribute.acPayload, "joined the chat.\n");
	return message_to_client;
}

sbcp_message_t * get_hung_message(char * new_user_name){
	sbcp_message_t *message_to_client = malloc(sizeof(sbcp_message_t));
	(*message_to_client).sMsgHeader.uiType = 3;
	(*message_to_client).sMsgHeader.uiVrsn = 3;
	(*message_to_client).sMsgAttribute.uiType= 4;
	strcat((*message_to_client).sMsgAttribute.acPayload, new_user_name);
	//strcat((*message_to_client).sMsgAttribute.acPayload, " ");
	strcat((*message_to_client).sMsgAttribute.acPayload, " left the chat.\n");
	return message_to_client;
}








