#include <assert.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>

#define MAX_FILES_PER_CLIENT 15
#define MAX_CLIENTS 15
#define MAX_ID_CHAR 25
#define MAX_FILE_SIZE 512
#define DEFAULT_PORT 1377
#define ERROR_STATUS 1
#define CHUNK 4096
#define HEADER_SIZE (sizeof(message_header))
#define MAX_DATA_SIZE (CHUNK - HEADER_SIZE)
#define GREETING_MESSAGE "Welcome! Please log in."



/* structs representing protocol */
#pragma pack(push, 1)
typedef struct
{
	short opcode;
	short length;//the number of bytes in the "data" field, relevant to the containing message
}message_header;

typedef struct
{
	message_header header;
	char data[MAX_DATA_SIZE];
}message;
#pragma pack(pop)

/* represents opcodes */
typedef enum
{
	WELCOME = 0x00,
	LIST_OF_FILES = 0x01,
	DELETE_FILE = 0x02,
	ADD_FILE = 0x03,
	GETFILE = 0x04,
	QUIT = 0x05,
	USER_NAME = 0x06,
	USER_PASSWORD = 0x07,
	AUTHORIZATION_SUCCESS = 0x08,
	END_LIST_OF_FILES = 0x09,
	FILE_END = 0x0A,
	FILE_CONTENT = 0x0B

} opcode;

/* Type for defining user */
typedef struct user_id {

	char userName[MAX_ID_CHAR];
	char password[MAX_ID_CHAR];
	int numberOfFiles;

}*UserID;

int sendMessage(int sockfd, message* msg){
	int len = HEADER_SIZE + msg->header.length;
	int total = 0;
	int bytesLeftToRead = len;
	int n;
	message networkMessage;
	networkMessage.header.opcode = htons(msg->header.opcode);
	networkMessage.header.length = htons(msg->header.length);
	memcpy(networkMessage.data, msg->data, msg->header.length);
	while(total < len) {
		n = send(sockfd, &networkMessage+total, bytesLeftToRead, 0);
		if (n < 0) {
			printf("ERROR: failed to send message %s\n", strerror(errno));
			return ERROR_STATUS;
		}
		total += n;
		bytesLeftToRead -= n;
	}
	return 0;
}

int receiveBuffer(int clientSockfd, void* buf, int len){
	int total = 0;
	int bytesleft = len;
	int n;
	while(total < len) {
		n = recv(clientSockfd, (void*)((long)buf+total), bytesleft, 0);
		if (n < 0) {
			printf("ERROR : failed received message %s\n", strerror(errno));
			return ERROR_STATUS;
		} else if (n == 0) {
			printf("ERROR : problem with the client %s\n", strerror(errno));
			return ERROR_STATUS;
		}
		total += n;
		bytesleft -= n;
	}
	return 0;
}

int receiveMessage(int clientSockfd, message* message){
	int status = receiveBuffer(clientSockfd, &message->header, HEADER_SIZE);
	if (status != 0)
		return status;
	message->header.opcode = ntohs(message->header.opcode);
	message->header.length = ntohs(message->header.length);
	// make sure length is valid
	if (message->header.length > MAX_DATA_SIZE) {
		printf("ERROR : received message too big\n");
		return ERROR_STATUS;
	}
	return receiveBuffer(clientSockfd, message->data, message->header.length);
}

int sendFile(int sockfd, char *fileSourcePath){
	
	message m;
	size_t bytesRead;
	FILE* fp = fopen(fileSourcePath, "r");
	
	if (fp == NULL ) {
		printf("ERROR: opening file\n");
		return ERROR_STATUS ;
	}
	m.header.opcode = FILE_CONTENT;
	do{
		bytesRead = fread(m.data, sizeof(char), MAX_DATA_SIZE, fp);
		m.header.length = bytesRead;
		if (sendMessage(sockfd, &m)) {
			fclose(fp);
			return ERROR_STATUS;
		}
	}while(bytesRead == MAX_DATA_SIZE);
	
	if(!feof(fp)){
		printf("ERROR: file send stopped before reaching end-of-file\n");
		fclose(fp);
		return ERROR_STATUS ;
	}
	
	fclose(fp);
	m.header.opcode = FILE_END;
	m.header.length = 0;
	if (sendMessage(sockfd, &m)) {
		return ERROR_STATUS;
	}
	
	return 0;
}

int receiveFile(int sockfd, char *fileDestPath){
	
	message m;
	FILE* fp = fopen(fileDestPath, "w");
	
	if (fp == NULL ) {
		printf("ERROR: opening file\n");
		return ERROR_STATUS;
	}
	
	do{
		if (receiveMessage(sockfd, &m)) {
			fclose(fp);
			return ERROR_STATUS;
		}
		if(m.header.opcode != FILE_CONTENT && m.header.opcode != FILE_END){
			printf("ERROR: unexpected message header\n");
			fclose(fp);
			return ERROR_STATUS;
		}
		if(fwrite(m.data, sizeof(char), m.header.length, fp) != m.header.length){
			printf("ERROR: writing to file\n");
			fclose(fp);
			return ERROR_STATUS;
		}
	}while(m.header.opcode == FILE_CONTENT);
	fclose(fp);
	
	return 0;
}


