#include "common.c"

/*functions declaration*/
UserID* processUsersFile(char* filePath, int* numberOfUsers, char* dirPath);
int activateServer(char *dirPath, int numberOfUsers, int port, UserID *listOfUsers);
int serverProtocol(int clientSockfd, char *dirPath, int numberOfUsers, UserID *listOfUsers);
int createSubDirectories(char *dirPath, int numberOfUsers, UserID *listOfUsers);
//int send_message(int s, message* msg);
//authorizeUser(clientSockfd, numberOfUsers, listOfUsers);
//int receive_message(int s, message* message)
//int receive_buffer(int s, void* buf, int len)
//int listOfFiles(int clientSockfd, char *dirPath, char *userName)
//int deleteFile(int clientSockfd, char *dirPath, char *userName)


int countFilesInDir(char* dirPath){
	int file_count = 0;
	DIR * dirp;
	struct dirent * entry;

	dirp = opendir(dirPath);
	if (dirp == NULL) {
		printf ("Cannot open directory '%s'\n", dirPath);
		return -1;
	}
	while ((entry = readdir(dirp)) != NULL) {
		if (entry->d_name[0] == '.')
			continue;
		file_count++;
	}
	closedir(dirp);
	return file_count;
}



/* creates new UserID */
UserID createUser(char * userName, char * password) {
	UserID newUser = (UserID) (malloc(sizeof(struct user_id)));
	if(newUser==NULL) {
		printf("ERROR: allocation memory\n");
		return NULL;
	}
	strcpy(newUser->password, password);
	strcpy(newUser->userName, userName);
	return newUser;
}
/* creates array of UserID struct from the file located in the "filePath" arg */
/* stores the actual number of users in "numberOfUsers" arg and returns the array of UserID*/
UserID* processUsersFile(char* filePath, int* numberOfUsers, char* dirPath) {

	UserID arrayOfUsers[MAX_CLIENTS];
	char userName[MAX_ID_CHAR];
	char password[MAX_ID_CHAR];
	char buffer[CHUNK];
	int index = 0;
	UserID* ret;
	int i;

	FILE* fp = fopen(filePath, "r");
	if (fp == NULL ) {
		printf("ERROR: opening file\n");
		return NULL ;
	}

	while (fgets(buffer, CHUNK-1, fp) != NULL ) {
		if (sscanf(buffer, "%s%s", userName, password) != 2) {
			printf("ERROR: reading users file\n");
			fclose(fp);
			return NULL ;
		}
	    arrayOfUsers[index] = createUser(userName, password);
	    if(arrayOfUsers[index] == NULL) {
			fclose(fp);
	 	    return NULL;
	    }
		

	    index++;
	}
	ret = (UserID*)(malloc(index * sizeof(UserID)));
	if (ret == NULL) {
		printf("ERROR: allocation memory\n");
		fclose(fp);
		return NULL;
	}
	for (i = 0; i < index; i++) {
		ret[i] = arrayOfUsers[i];
	}
	fclose(fp);
	*numberOfUsers = index;
	return ret;

}

int activateServer(char *dirPath, int numberOfUsers, int port, UserID *listOfUsers) {

	int sockfd, newsockfd, status, enable=1;
	struct sockaddr_in serverAddr, clientAddr;
	socklen_t addr_size = sizeof(clientAddr);

	/*---- Create the socket. The three arguments are: ----*/
	/* 1) Internet domain (PF_INET for IPv4) 2) Stream socket (SOCK_STREAM for tcp)
	 * 3) Default protocol */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		printf("Could not create socket: %s\n", strerror(errno));
		return ERROR_STATUS;
	}
	
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
		printf("setsockopt(SO_REUSEADDR) failed\n");
		return ERROR_STATUS;
	}
	
	/*---- Configure settings of the server address struct ----*/
	serverAddr.sin_family = AF_INET;
	/* Set port number, using htons function to use proper byte order */
	serverAddr.sin_port = htons(port);
	/* Set IP address to localhost
	   binds the socket to all available interfaces.*/
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY );

	/*---- Do we need this?-----*/
	/* Set all bits of the padding field to 0 */
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

	/*---- Bind the address struct to the socket ----*/
	status = bind(sockfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
	if (status < 0) {
		printf("Could not bind socket: %s\n", strerror(errno));
		close(sockfd);
		return ERROR_STATUS;
	}
	/*---- Listen on the socket, with 5 max connection requests queued ----*/
	status = listen(sockfd, 5);
	if (status < 0)
	{
		printf("listen() failed: %s\n", strerror(errno));
		close(sockfd);
		return ERROR_STATUS;
	}
	/*---- Accept call creates a new socket for the incoming connection ----*/
	while (1) {
		newsockfd = accept(sockfd, (struct sockaddr*) &clientAddr, &addr_size);
		if (newsockfd < 0) {
			printf("accept() failed: %s\n", strerror(errno));
			continue;
		}
		if (serverProtocol(newsockfd, dirPath, numberOfUsers, listOfUsers)) {
			close(newsockfd);
			//return ERROR_STATUS;
		}

		close(newsockfd);
	}
	return 0;

}

int createSubDirectories(char *dirPath, int numberOfUsers, UserID *listOfUsers) {
    char userDirPath[CHUNK] = {0};
    int i;
	DIR* dir = opendir(dirPath);
	for(i = 0; i < numberOfUsers; ++i) {
	    snprintf(userDirPath, sizeof(userDirPath), "%s%s%s", dirPath, "/", listOfUsers[i]->userName);	
		dir = opendir(userDirPath);
		if (dir){
			/* Directory exists. */
			closedir(dir);
		}
		else if (ENOENT == errno){
			/* Directory does not exist. */
			if(mkdir(userDirPath, 0777) < 0 )
				return ERROR_STATUS;
		}
		else{
			return ERROR_STATUS;
		}
		if((listOfUsers[i]->numberOfFiles = countFilesInDir(userDirPath)) == -1){
			return ERROR_STATUS;
		}
		memset(userDirPath, 0, sizeof(userDirPath));
	}
	return 0;
}

int authorizeUser(int clientSockfd, int numberOfUsers, UserID *listOfUsers, message *m, message *s, int *indexOfUser, char* dirpath){
	int i;
	message t;
	char buff[MAX_DATA_SIZE];
	char userDirPath[CHUNK];
	
	/* reads user name */
	if (receiveMessage(clientSockfd, m) == ERROR_STATUS)
		return ERROR_STATUS;
	/* reads password */
	if (receiveMessage(clientSockfd, s) == ERROR_STATUS)
		return ERROR_STATUS;
	if ((m->header.opcode != USER_NAME) || s->header.opcode != USER_PASSWORD ){
		printf("ERROR : the message does not match the protocol\n");
		return ERROR_STATUS ;
	}
	for(i = 0; i < numberOfUsers; ++i) {
		if ( (strcmp(m->data, listOfUsers[i]->userName) == 0) && (strcmp(s->data, listOfUsers[i]->password)) == 0 ) {
			*indexOfUser = i;
			snprintf(userDirPath, CHUNK, "%s%s%s", dirpath, "/", listOfUsers[i]->userName);
			if((listOfUsers[i]->numberOfFiles = countFilesInDir(userDirPath)) == -1){
				return ERROR_STATUS;
			}
			sprintf(buff, "%s %s, you have %d files stored.", "Hi", listOfUsers[i]->userName, listOfUsers[i]->numberOfFiles);
			t.header.opcode = AUTHORIZATION_SUCCESS;
			t.header.length = strnlen(buff, MAX_DATA_SIZE)+1;
			if(t.header.length < MAX_DATA_SIZE){
				t.header.length++;
			}
			strncpy(t.data, buff, t.header.length);
			sendMessage(clientSockfd, &t);
			return 0;
		}
	}
	return ERROR_STATUS;
}

int listOfFiles(int clientSockfd, char *dirPath, char *userName) {
	int nameLen;
	message m;
	m.header.opcode = LIST_OF_FILES;
	m.data[0] = 0;
    char usersDirPath[CHUNK] = {0};
    snprintf(usersDirPath, sizeof(usersDirPath), "%s%s%s", dirPath, "/", userName);

	DIR* d = opendir(usersDirPath);
	struct dirent *dir;
	int currentIndex = 0;
	if (d != NULL) {
		while ((dir = readdir(d)) != NULL) {
			// skip
			if (dir->d_name[0] == '.')
				continue;
			nameLen = strnlen(dir->d_name, MAX_FILE_SIZE);
			if (currentIndex + nameLen + 2 >= MAX_DATA_SIZE) // one for \n, one for \0
			{
				m.header.length = currentIndex+1;
				m.data[currentIndex] = '\0';
				if (sendMessage(clientSockfd, &m)) {
					closedir(d);
					return ERROR_STATUS;
				}
				currentIndex = 0;
			}
			strncpy(m.data + currentIndex, dir->d_name, nameLen);
			m.data[currentIndex + nameLen] = '\n';
			currentIndex += (nameLen + 1);
		}
		if (currentIndex != 0) {
			m.header.length = currentIndex+1;
			m.data[currentIndex] = '\0';
			if (sendMessage(clientSockfd, &m)) {
				closedir(d);
				return ERROR_STATUS;
			}
		}
	}
	else {
		printf("ERROR : opening user's directory %s\n", strerror(errno));
		return ERROR_STATUS;
	}

	m.header.opcode = END_LIST_OF_FILES;
	m.header.length = 0;
	if (sendMessage(clientSockfd, &m)) {
		return ERROR_STATUS;
	}
	return 0;

}

int deleteFile(int clientSockfd, char *dirPath, UserID user, message t) {
	message m;
	m.header.opcode = DELETE_FILE;
	m.data[0] = 0;
    char fileName[CHUNK] = {0};
    snprintf(fileName, sizeof(fileName), "%s%s%s%s%s", dirPath, "/", user->userName,"/", t.data);
    /* faild to remove */
    if (remove(fileName) < 0) {
    	snprintf(m.data, MAX_DATA_SIZE, "%s", "No such file exists!");
    	m.data[20] = '\0';
    	m.header.length = strnlen(m.data, MAX_DATA_SIZE)+1;
		if (sendMessage(clientSockfd, &m)) {
			return ERROR_STATUS;
		}
		return 0;
    }
    /* removed */
    else {
		snprintf(m.data, MAX_DATA_SIZE, "%s", "File removed");
    	m.data[12] = '\0';
    	m.header.length = strnlen(m.data, MAX_DATA_SIZE)+1;
		if (sendMessage(clientSockfd, &m)) {
			return ERROR_STATUS;
		}
		return 0;
    }

}

int serverProtocol(int clientSockfd, char *dirPath, int numberOfUsers, UserID *listOfUsers) {
	int status = 0;
	int indexOfUser;
	message m, s;
	char pathBuf[MAX_DATA_SIZE];
	
	m.header.opcode = WELCOME;
	m.header.length = strnlen(GREETING_MESSAGE, MAX_DATA_SIZE)+1;
	strncpy(m.data, GREETING_MESSAGE, m.header.length);
	if (sendMessage(clientSockfd, &m))
		return ERROR_STATUS;
	if (authorizeUser(clientSockfd, numberOfUsers, listOfUsers, &m , &s, &indexOfUser, dirPath)) {
		printf("authentication failure: %s\n", strerror(errno));
		return ERROR_STATUS;
	}

	while (m.header.opcode != QUIT && status == 0) {
		// read the message header
		status = receiveMessage(clientSockfd, &m);
		if (status == ERROR_STATUS) {
			printf("ERROR : unexpected message from client\n");
			continue;
		}
		switch (m.header.opcode) {
			case LIST_OF_FILES:
				status = listOfFiles(clientSockfd, dirPath, listOfUsers[indexOfUser]->userName);
				break;
			case DELETE_FILE:
				status = deleteFile(clientSockfd, dirPath, listOfUsers[indexOfUser], m);
				break;
			case ADD_FILE:
				if(snprintf(pathBuf, MAX_DATA_SIZE, "%s%s%s%s%s", dirPath, "/", listOfUsers[indexOfUser]->userName, "/", m.data) < 0){
					printf("ERROR : sprintf encountered an error\n");
					status = ERROR_STATUS;
					break;
				}
				status = receiveFile(clientSockfd, pathBuf);
				if(status != ERROR_STATUS){
					snprintf(m.data, MAX_DATA_SIZE, "%s", "File added");
					m.data[10] = '\0';
				}
				else {
					snprintf(m.data, MAX_DATA_SIZE, "%s", "A problem has occured. The file was not added.");
					m.data[46] = '\0';
				}
				m.header.length = strnlen(m.data, MAX_DATA_SIZE)+1;
				m.header.opcode = ADD_FILE;
				if (sendMessage(clientSockfd, &m)) {
					status = ERROR_STATUS;
				}
				break;
			case GETFILE:
				if(snprintf(pathBuf, MAX_DATA_SIZE, "%s%s%s%s%s", dirPath, "/", listOfUsers[indexOfUser]->userName, "/", m.data) < 0){
					printf("ERROR : sprintf encountered an error\n");
					status = ERROR_STATUS;
					break;
				}
				status = sendFile(clientSockfd, pathBuf);
				break;
			case QUIT:
				break;
		}
		//if (status == ERROR_STATUS) {
		//	break;
		//}
	}
	return status;
}

int main(int argc, char* argv[]) {
	char *usersFilePath;
	char *dirPath;
	int port = DEFAULT_PORT;
	UserID *listOfUsers;
	int numberOfUsers;

	if (argc < 3 ) {
		printf("ERROR: number of arguments does not match\n");
		return ERROR_STATUS;
	}

	usersFilePath = argv[1];
	dirPath = argv[2];

	//in case port number provided
	if ( argc == 4) {
		port = atoi(argv[3]);
	}

	if((listOfUsers = processUsersFile(usersFilePath, &numberOfUsers, dirPath)) == NULL)
		return ERROR_STATUS;
	if(createSubDirectories(dirPath, numberOfUsers, listOfUsers) == ERROR_STATUS)
		return ERROR_STATUS;
	if(activateServer(dirPath, numberOfUsers, port, listOfUsers) == ERROR_STATUS)
		return ERROR_STATUS;

	return 0;

}




