#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <fstream>
#include <sstream> 
#include <stdlib.h>

using namespace std;

const int MAX_PACKET_SIZE = 1000;

struct message {
	bool type;
	int seq_num;
	bool last_packet;
	string body;
};

int main(int argc, char** argv) {
	int sockfd, portno, n;
	struct sockaddr_in serv_addr, client_addr;
	socklen_t len = sizeof(struct sockaddr_in);
	string filename, line, buffer;
	char temp[100];
	message msg;
	vector<message> packets;
	
	if (argc < 2) {
		cerr << "ERROR: Incorrect number of arguments" << endl;
		cerr << "sender <portnumber>" << endl;
		return -1;
	}

	portno = atoi(argv[1]);

	// Set socket and populate server address
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	bzero((char*) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;			// Server binds to any/all interfaces

	// Bind server to socket
	if (bind(sockfd, (struct sockaddr*) &serv_addr, len) == -1) {
		cerr << "ERROR: Failed to bind socket" << endl; 
		return -1;
	}

	// Get file request
	do {
		n = recvfrom(sockfd, temp, 100, 0, 
			(struct sockaddr*) &client_addr, &len);
	} while (n == -1);

	for (int i = 0; i < strlen(temp); i++) {
		filename += temp[i];
	}

	cout << "Filename: " << filename << endl;

	// Create initial ACK for file request
	msg.type = false;
	msg.seq_num = 0;
	msg.last_packet = false;

	// Send ACK with initial seq number 0
	sendto(sockfd, &msg, sizeof(message), 0,
		(struct sockaddr*) &serv_addr, len);

	std::ostringstream response;
	std::ifstream request(filename.c_str());

	if (request) {
	  response << request.rdbuf();
	  request.close();  
	  buffer = response.str(); 
	} else {
		cerr << "ERROR: File not found" << endl;
		return -1;
	}

	int counter = 0;

	// Split data into packets
	// cout << "In file" << endl;
	for (int i = 0; i < buffer.size(); i++) {
		line += buffer[i];
		if (i == MAX_PACKET_SIZE - 2) {
			line += '\0'; 
			// Populate new packet
			message current;
			current.type = true;
			current.seq_num = packets.size();
			current.last_packet = false;
			current.body = line;
			packets.push_back(current);

			line = "";
			counter = -1;
		} 

		counter++;
	}

	if (line != "") {
			message current;
			current.type = true;
			current.seq_num = packets.size();
			current.last_packet = false;
			current.body = line;
			packets.push_back(current);
	}

	// cout << "Closing file" << endl;
	packets[packets.size()-1].last_packet = true;

	cout << "Number of packets: " << packets.size() << endl;
	for (int i = 0; i < packets.size(); i++) {
		cout << "Data: " << packets[i].body << endl;
		message recv;
		sendto(sockfd, &(packets[i]), sizeof(message), 0,
			(struct sockaddr*) &client_addr, len);

		while (recvfrom(sockfd, &recv, sizeof(message), 0,
			(struct sockaddr*) &client_addr, &len) == -1);
	}
}