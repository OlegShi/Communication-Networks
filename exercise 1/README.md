# file_client:
A file client that can connect a user secured by a password to a running instance of file_server and upload/download files 
to/from the server, delete files that have been uploaded by the user to the server, and print a list of the user's files that are 
currently on the server.
Run the program by: file_client [hostname [port_number]]

Upon successful connection the user of the client is prompted for a username and password, and if authentication is successful, 
the user of the client can use the following commands:

1. list_of_files:
Prints a list of the user's files currently on the server
2. add_file <path_of_local_file> <file_name_on_server>:
Uploads the local file whose path is given in the 2nd argument to the server, under the name that is given in the 3rd argument
3. get_file <file_name_on_server> <path_to_local_file>:
Downloads the file on the server whose name is given in the 2nd argument to the client, to the path that is given in the 3rd argument
4. delete_file <file_name_on_server>:
Deletes from the server the file whose name is given in the 2nd argument
5. quit:
Disconnects from the server

# file_server:
A file server that supports file_client.
Run the program by: file_server <path_to_users_file> <path_to_data_directory> [port]
Upon activation, the server reads and stores the data from the users file and prepares the users' 
directories. Then it proceeds to listen to incoming connections from clients. After a client has disconnected, 
the server proceeds to listen to the next incoming client connection.

# Communication protocol:
Definitions:\
After connection is established, all communication between client and server is done by the "message" struct:\
typedef struct\
{\
message_header header;\
char data[MAX_DATA_SIZE];\
}message;

MAX_DATA_SIZE is defined by:\
#define CHUNK 4096\
#define HEADER_SIZE (sizeof(message_header))\
#define MAX_DATA_SIZE (CHUNK - HEADER_SIZE)\
The struct "message_header" is defined by:\
typedef struct\
{\
short opcode;\
short length;\
}message_header;

The possible values of the field "opcode" of the struct "message_header" are defined in the following enum:\
typedef enum\
{\
WELCOME = 0x00,\
LIST_OF_FILES = 0x01,\
DELETE_FILE = 0x02,\
ADD_FILE = 0x03,\
GETFILE = 0x04,\
QUIT = 0x05,\
USER_NAME = 0x06,\
USER_PASSWORD = 0x07,\
AUTHORIZATION_SUCCESS = 0x08,\
END_LIST_OF_FILES = 0x09,\
FILE_END = 0x0A,\
FILE_CONTENT = 0x0B\
} opcode;
