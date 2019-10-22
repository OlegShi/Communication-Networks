#include "common.c"

int connectToServer(int argc, char* argv[]){
	
	int sockfd;
	struct addrinfo hints, *result, *srvIter;
	char hostname[CHUNK];
	char port[CHUNK];
	
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	if(argc==1){
		snprintf(hostname, CHUNK, "%s", "localhost");
		snprintf(port, CHUNK, "%d", DEFAULT_PORT);
	}
	else if (argc==2){
		snprintf(hostname, CHUNK, "%s", argv[1]);
		snprintf(port, CHUNK, "%d", DEFAULT_PORT);
	}
	else{
		snprintf(hostname, CHUNK, "%s", argv[1]);
		snprintf(port, CHUNK, "%s", argv[2]);
	}
	
	if ((getaddrinfo(hostname, port, &hints, &result)) != 0) {
		printf("ERROR: getaddrinfo failed\n");
		return ERROR_STATUS;
	}
	
	
	
	// loop through all the results and connect to the first we can
	for(srvIter = result; srvIter != NULL; srvIter = srvIter->ai_next) {
		if ((sockfd = socket(srvIter->ai_family, srvIter->ai_socktype, srvIter->ai_protocol)) == -1) {
			printf("ERROR: socket failed\n");
			continue;
		}

		if (connect(sockfd, srvIter->ai_addr, srvIter->ai_addrlen) == -1) {
			printf("ERROR: connect failed\n");
			close(sockfd);
			continue;
		}

		break; // if we get here, we must have connected successfully
	}

	if (srvIter == NULL) {
		// looped off the end of the list with no connection
		printf("ERROR: failed to connect\n");
		return ERROR_STATUS;
	}

	freeaddrinfo(result); // all done with this structure
	
	return sockfd;
	
}

int getInputFromStdin(char* buf, int len){
	char *pos;
	if(fgets(buf, len, stdin)==NULL){
		printf("Error: fgets failed to get input from stdin\n");
		return -1;
	}
	if ((pos=strchr(buf, '\n')) != NULL){
    	*pos = '\0';//fgets reads the input with the newline character, here we remove it.
	}
	else{//this means the user entered a string that is too long
		buf[MAX_ID_CHAR]='\0';
	}
	return 0;
}

void communicateWithServer(int sockfd){
	
	message m;
	char username[MAX_ID_CHAR+1];
	char password[MAX_ID_CHAR+1];
	char inputBuf[MAX_DATA_SIZE];
	char inputParam1[MAX_DATA_SIZE];
	char inputParam2[MAX_DATA_SIZE];
	char inputParam3[MAX_DATA_SIZE];
	int numInputParams;
	FILE* fp;

	if(receiveMessage(sockfd, &m) == ERROR_STATUS){
		return;
	}
	if(m.header.opcode == WELCOME){
		printf("%s\n", m.data);
	}
	else{
		printf("Error: did not receive expected hello message from server\n");
		return;
	}
	printf("User: ");
	if(getInputFromStdin(username, MAX_ID_CHAR+1)){
		printf("Error: failed to get username\n");
		return;
	}
	printf("Password: ");
	if(getInputFromStdin(password, MAX_ID_CHAR+1)){
		printf("Error: failed to get password\n");
		return;
	}

	m.header.opcode = USER_NAME;
	snprintf(m.data, MAX_ID_CHAR+1, "%s", username);
    m.data[MAX_ID_CHAR] = '\0';
	m.header.length = strnlen(m.data, MAX_DATA_SIZE);
	if(m.header.length < MAX_DATA_SIZE){
		m.header.length++;
	}
	if (sendMessage(sockfd, &m)) {
		return;
	}
	
	m.header.opcode = USER_PASSWORD;
	snprintf(m.data, MAX_ID_CHAR+1, "%s", password);
    m.data[MAX_ID_CHAR] = '\0';
	m.header.length = strnlen(m.data, MAX_DATA_SIZE);
	if(m.header.length < MAX_DATA_SIZE){
		m.header.length++;
	}
	if (sendMessage(sockfd, &m)) {
		return;
	}
	
	if(receiveMessage(sockfd, &m) == ERROR_STATUS){
		return;
	}
	if(m.header.opcode == AUTHORIZATION_SUCCESS){
		printf("%s\n", m.data);
	}
	else{
		printf("Error: authentication failed\n");
		return;
	}
	while(1){	
		
		if(getInputFromStdin(inputBuf, MAX_DATA_SIZE)){
			printf("Error: failed to get user input\n");
			continue;
		}
		numInputParams = sscanf(inputBuf, "%s %s %s", inputParam1, inputParam2, inputParam3);
		if(numInputParams < 1){
			printf("Error: failed to get user input\n");
			continue;
		}
		if(!strncmp(inputParam1, "quit", 5)){
			m.header.opcode = QUIT;
			m.header.length = 0;
			sendMessage(sockfd, &m);
			break;	
		}
		else if(!strncmp(inputParam1, "delete_file", 12)){
			if(numInputParams < 2){
				printf("usage: delete_file <name_of_file>\n");
				continue;
			}
			m.header.opcode = DELETE_FILE;
			m.header.length = strlen(inputParam2)+1;
			snprintf(m.data, m.header.length, "%s", inputParam2);
			sendMessage(sockfd, &m);
			
			if(receiveMessage(sockfd, &m) == ERROR_STATUS){
				printf("Error: did not receive confirmation message from server\n");
				return;
			}
			if(m.header.opcode == DELETE_FILE){
				printf("%s\n", m.data);
			}
			else{
				printf("Error: did not receive expected reply from server\n");
				return;
			}
			continue;	
		}
		else if(!strncmp(inputParam1, "list_of_files", 14)){
			
			m.header.opcode = LIST_OF_FILES;
			m.header.length = 0;
			sendMessage(sockfd, &m);
			while(1){
				if(receiveMessage(sockfd, &m) == ERROR_STATUS){
					printf("Error: did not receive expected message from server\n");
					return;
				}
				if(m.header.opcode == LIST_OF_FILES){
					printf("%s", m.data);
				}
				else if(m.header.opcode == END_LIST_OF_FILES){
					break;
				}
				else{
					printf("Error: did not receive expected message from server\n");
					return;
				}
			}
			continue;
		}
		else if(!strncmp(inputParam1, "add_file", 9)){
			if(numInputParams < 3){
				printf("usage: add_file <path_to_local_file> <new_file_name_on_server>\n");
				continue;
			}
			fp = fopen(inputParam2, "r");
			if (fp == NULL ) {
				printf("No such file exists!\n");
				continue;
			}
			fclose(fp);
			m.header.opcode = ADD_FILE;
			m.header.length = strlen(inputParam3)+1;
			snprintf(m.data, m.header.length, "%s", inputParam3);
			sendMessage(sockfd, &m);
			if(sendFile(sockfd, inputParam2)){
				printf("Error: failed to send file\n");
				return;
			}
			
			if(receiveMessage(sockfd, &m) == ERROR_STATUS){
				printf("Error: did not receive expected message from server\n");
				return;
			}
			if(m.header.opcode == ADD_FILE){
				printf("%s\n", m.data);
			}
			else{
				printf("Error: did not receive confirmation from server that file was added\n");
				return;
			}
			continue;	
		}
		else if(!strncmp(inputParam1, "get_file", 9)){
			if(numInputParams < 3){
				printf("usage: get_file <file_name_on_server> <local_path_to_save_file>\n");
				continue;
			}
			m.header.opcode = GETFILE;
			m.header.length = strlen(inputParam2)+1;
			snprintf(m.data, m.header.length, "%s", inputParam2);
			sendMessage(sockfd, &m);
			if(receiveFile(sockfd, inputParam3)){
				printf("Error: failed to receive file\n");
				return;
			}
			continue;	
		}
		else{
			printf("unsupported command: '%s'\n", inputParam1);
			continue;
		}
	}
}


int main(int argc, char* argv[]){
	
	int sockfd;
	
	if((sockfd=connectToServer(argc, argv))==ERROR_STATUS){
		return ERROR_STATUS;
	}
	
	communicateWithServer(sockfd);
	
	return 0;	
	
}





