#ifndef _FS_CPP_
#define _FS_CPP_

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include "fs_crypt.h"
#include "fs_param.h"
#include "fs_server.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

using std::cin;
using std::cout;
using std::endl;
using std::string;
using std::vector;
using std::unordered_map;
using std::stringstream;

static const int MAX_MESSAGE_SIZE = 1024;

int port;
unordered_map<string, string> users;

int handle_connection(int connectionfd);

int handle_message(int connectionfd, string &userName, const char *msg, const unsigned int msg_size);

int handle_fs_session(const char *username, const char *password,
                      unsigned int sequence);

int handle_fs_create(const char *username, const char *password,
                     unsigned int session, unsigned int sequence,
                     const char *pathname, char type);

int handle_fs_delete(const char *username, const char *password,
                     unsigned int session, unsigned int sequence,
                     const char *pathname);

int handle_fs_read(const char *username, const char *password,
                   unsigned int session, unsigned int sequence,
                   const char *pathname, unsigned int offset);

int handle_fs_write(const char *username, const char *password,
                    unsigned int session, unsigned int sequence,
                    const char *pathname, unsigned int offset,
                    const char *data);

int main(int argc, char* argv[]){
    // Handle input parameters
    if(argc == 2){
        port = atoi(argv[1]);
    }
    else if(argc == 1){
        port = -1; // system assign;
    }
    else{
        cout << "usage: ./fs <port> < passwords" << endl;
        return 1;
    }

    // Read user names and passwords
    string userInput, passwordInput;
    while(cin >> userInput >> passwordInput){
        users[userInput] = passwordInput;
    }

    // Create socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("Error opening stream socket");
		return 1;
	}

	// Set the "reuse port" socket option
	int yesval = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yesval, sizeof(yesval)) == -1) {
		perror("Error setting socket options");
		return 1;
	}

	// Create a sockaddr_in struct for the proper port and bind() to it.
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = (port >= 0) ? htons(port) : 0;

	if (bind(sockfd, (sockaddr *) &addr, sizeof(addr)) == -1) {
		perror("Error binding stream socket");
		return 1;
	}

	// Detect which port was chosen
    if(port == -1){
        socklen_t length = sizeof(addr);
        if (getsockname(sockfd, (sockaddr *) &addr, &length) == -1) {
            perror("Error getting port of socket");
            return 1;
        }
        // Use ntohs to convert from network byte order to host byte order.
        port = ntohs(addr.sin_port);
    }

	// Begin listening for incoming connections.
	listen(sockfd, 10);

	// Serve incoming connections one by one forever.
	while (true) {
		int connectionfd = accept(sockfd, 0, 0);
		if (connectionfd == -1) {
			perror("Error accepting connection");
			return 1;
		}

		if (handle_connection(connectionfd) == -1) {
			return 1;
		}
	}

    return 0;
}

int handle_connection(int connectionfd){
    // Receive message from client.
	char msg[MAX_MESSAGE_SIZE];
	memset(msg, 0, sizeof(msg));

	for (int i = 0; i < MAX_MESSAGE_SIZE; i++) {
		// Receive exactly one byte
		int rval = recv(connectionfd, msg + i, 1, MSG_WAITALL);
		if (rval == -1) {
			perror("Error reading stream message");
			return -1;
		}
		// Stop if we received a null character
		if (msg[i] == '\0') {
			break;
		}
	}

	stringstream ss;
	ss.clear();
	ss.str(msg);
	int msg_size;
	string userName;
	ss >> userName;
	ss >> msg_size;

	// Print out the message
	cout << connectionfd << " " << userName << " " << msg_size << endl;
	
	memset(msg, 0, sizeof(msg));
	for (int i = 0; i < msg_size; i++) {
		// Receive exactly one byte
		int rval = recv(connectionfd, msg + i, 1, 0);
		if (rval == -1) {
			perror("Error reading stream message");
			return -1;
		}
	}
	
	if(handle_message(connectionfd, userName, msg, msg_size) == -1){
        perror("Error handling the message");
        return -1;
    }

	// Send response code to client
	int response = 0;
	response = htons(response);
    if (send(connectionfd, &response, sizeof(response), 0) == -1) {
        perror("Error sending response to client");
        return -1;
    }

	// Close connection
	close(connectionfd);

    return 0;
}

int handle_message(int connectionfd, string &userName, const char *msg, const unsigned int msg_size){
	auto it = users.find(userName);
	if(it == users.end()){
		perror("Error unknown user");
		return -1;
	}

	const char* userNamePtr = userName.c_str();
	const char* passwordPtr = users[userName].c_str();
	char de_msg[msg_size];

	if(fs_decrypt(passwordPtr, (void*)msg, msg_size, (void*)de_msg) == -1){
		perror("Error decrypting the message");
		return -1;
	}

	stringstream ss;
    ss.clear();
    ss << de_msg;

	string commandType, pathname;
	unsigned int session, sequence, offset;
	char type;
	ss >> commandType;
	ss >> session;
	ss >> sequence;

	if(commandType == "FS_SESSION"){
		if(handle_fs_session(userNamePtr, passwordPtr, sequence) == -1){
			perror("Error handling fs session");
			return -1;
		}
	}
	else if(commandType == "FS_CREATE"){
		ss >> pathname;
		ss >> type;
		if(handle_fs_create(userNamePtr, passwordPtr, session, sequence, pathname.c_str(), type) == -1){
			perror("Error handling fs create");
			return -1;
		}
	}
	else if(commandType == "FS_READBLOCK"){
		ss >> pathname;
		ss >> offset;
		if(handle_fs_read(userNamePtr, passwordPtr, session, sequence, pathname.c_str(), offset) == -1){
			perror("Error handling fs readblock");
			return -1;
		}
	}
	else if(commandType == "FS_WRITEBLOCK"){
		ss >> pathname;
		ss >> offset;

		unsigned int nullPos;
		for(nullPos = 0; nullPos < msg_size; nullPos++){
			if(de_msg[nullPos] == '\0'){
				break;
			}
		}

		vector<char> data;
		
		for(unsigned int i = nullPos + 1; i < msg_size; i++){
			data.push_back(de_msg[i]);
		}

		cout << "HERE " << data.size() << endl;

		if(handle_fs_write(userNamePtr, passwordPtr, session, sequence, pathname.c_str(), offset, data.data()) == -1){
			perror("Error handling fs writeblock");
			return -1;
		}
	}
	else if(commandType == "FS_DELETE"){
		ss >> pathname;
		if(handle_fs_delete(userNamePtr, passwordPtr, session, sequence, pathname.c_str()) == -1){
			perror("Error handling fs delete");
			return -1;
		}
	}
	else{
		perror("Error unknown command");
		return -1;
	}

    return 0;
}

int handle_fs_session(const char *username, const char *password,
                      unsigned int sequence){
	cout << "SESSION " << username << " " << password << " " << sequence << endl;
    return 0;
}

int handle_fs_create(const char *username, const char *password,
                     unsigned int session, unsigned int sequence,
                     const char *pathname, char type){
	cout << "CREATE " << username << " " << password << " " << session << " " << sequence << " " << pathname << " " << type << endl;
    return 0;
}

int handle_fs_delete(const char *username, const char *password,
                     unsigned int session, unsigned int sequence,
                     const char *pathname){
	cout << "DELETE " << username << " " << password << " " << session << " " << sequence << " " << pathname << endl;
    return 0;
}

int handle_fs_read(const char *username, const char *password,
                   unsigned int session, unsigned int sequence,
                   const char *pathname, unsigned int offset){
    cout << "READ " << username << " " << password << " " << session << " " << sequence << " " << pathname << " " << offset << endl;
    return 0;
}

int handle_fs_write(const char *username, const char *password,
                    unsigned int session, unsigned int sequence,
                    const char *pathname, unsigned int offset,
                    const char *data){
    cout << "WRITE " << username << " " << password << " " << session << " " << sequence << " " << pathname << " " << offset << endl;
	for(unsigned int i = 0; i < FS_BLOCKSIZE; i++){
		cout << data[i];
	}
	cout<<endl;
    return 0;
}

#endif