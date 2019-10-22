# file_client:
A file client that can connect a user secured by a password to a running instance of file_server and upload/download files to/from the server, delete files that have been uploaded by the user to the server, and print a list of the user’s files that are currently on the server.
The client also allows the user to get a list of all the users that are currently connected to the server, send messages to other users and get messages from them, and read messages received from other users while offline.
Run the program by: file_client [hostname [port_number]]
Upon successful connection the user of the client is prompted for a username and password, and if authentication is successful, the user of the client can use the following commands:

1. list_of_files
Prints a list of the user's files currently on the server; this list does not include files containing the messages received while offline.
2. add_file <path_of_local_file> <file_name_on_server>
Uploads the local file whose path is given in the 2nd argument to the server, under the name that is given in the 3rd argument
3. get_file <file_name_on_server> <path_to_local_file>
Downloads the file on the server whose name is given in the 2nd argument to the client, to the path that is given in the 3rd argument
4. delete_file <file_name_on_server>
Deletes from the server the file whose name is given in the 2nd argument
5. users_online
Prints the list of users currently connected to the server
6. msg <user_name_we_send_to>: <The_message>
Sends the message to the user; if the user is online – the message is printed to his console immediately. Otherwise – the message is stored on the server, and the receiving user will be able to read it when he connects.
7. read_msgs
Prints to the user’s console all the messages that were sent to him while he was offline.
8. quit
Disconnects from the server

# file_server:
A file server that supports file_client.
Run the program by: file_server <path_to_users_file> <path_to_data_directory> [port]
Upon activation, the server reads and stores the data from the ‘users’ file and prepares the users' directories. 
Then it proceeds to listen to new incoming connections, to authentication requests from those connections and to 
instructions from already connected clients.
