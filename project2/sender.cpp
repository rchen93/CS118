#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <fstream>
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
	string hostname, filename, line;
	char temp[100];
	message msg;
	vector<message> packets;
	
	if (argc < 3) {
		cerr << "ERROR: Incorrect number of arguments" << endl;
		return -1;
	}

	hostname = argv[1];
	portno = atoi(argv[2]);

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

	for (int i = 0; i < strlen(temp) - 1; i++) {
		filename += temp[i];
	}

	// Create initial ACK for file request
	msg.type = false;
	msg.seq_num = 0;
	msg.last_packet = false;

	// Send ACK with initial seq number 0
	sendto(sockfd, &msg, sizeof(message), 0,
		(struct sockaddr*) &serv_addr, len);

	ifstream file(filename.c_str());
	file.get();
	if (file.is_open()) {
		// Split data into packets
		int i = 0;
		while (!file.eof()) {
			line += file.get();
			if (i == 999) {
				// Populate new packet
				message current;
				current.type = true;
				current.seq_num = packets.size();
				current.last_packet = false;
				current.body = line;
				packets.push_back(current);
				cout << "Number of packets: " << packets.size() << endl;

				line = "";
				i = -1;
			}

			i++;
		}

		file.close();
		packets[packets.size()-1].last_packet = true;
	} else {
		cerr << "ERROR: File not found" << endl;
	}
}