#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <stdlib.h>
#include <fstream> 
#include <sstream>

#include "common.h"

using namespace std;

const int RESEND = 10;

int main(int argc, char** argv) {
	int sockfd, portno, n, seq = 0;
	struct sockaddr_in serv_addr;
	socklen_t len = sizeof(struct sockaddr_in);
	string hostname, filename;
	vector<pair<string,int> > messages;
	message msg;

	if (argc < 4) {
		cerr << "Incorrect number of arguments" << endl;
		cerr << "receiver <sender_hostname> <sender_portnumber> <filename>" << endl;
		return -1;
	}

	hostname = argv[1];
	portno = atoi(argv[2]);
	filename = argv[3];

	// Set socket and populate server address
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		cerr << "ERROR: opening socket" << endl;
	}
	bzero((char*) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);

	// Send initial request for the file
	cout << "Sending initial request for file: " << filename.c_str() << endl;
	n = sendto(sockfd, filename.c_str(), filename.size(), 0,
		(struct sockaddr*) &serv_addr, len);

	// Receive ACK from server
	cout << "Receiving ACK for initial request" << endl;
	message initial;
	while (recvfrom(sockfd, &initial, sizeof(initial), 0,
		(struct sockaddr*) &serv_addr, &len) == -1);

	if (initial.seq_num == -1) {
		cerr << "ERROR: File not found" << endl;
		return -1;
	}

	int expected_packet_num = 0;

	while (true) {
		bzero(&msg, sizeof(message));
		n = recvfrom(sockfd, &msg, sizeof(msg), 0,
			(struct sockaddr*) &serv_addr, &len);

		if (n <= 0) {
			continue;
		}
		cout << "Received packet with sequence Number: " << msg.seq_num << endl;
		cout << "Received packet " << msg.packet_num << endl;
		
		// Packet is received in order
		message ack;
		if (msg.packet_num == expected_packet_num) {
			cout << "Correct Packet #" << endl;

			// Extract 
			string data;
			for (unsigned int i = 0; i < msg.data_length; i++) {
				data += msg.data[i];
			}
			messages.push_back(make_pair(data, msg.data_length));

			// Make ACK packet
			ack.type = false;
			ack.packet_num = expected_packet_num;
			seq += msg.data_length;
			ack.seq_num = seq;

			// Send ACK packet
			sendto(sockfd, &ack, sizeof(ack), 0, 
				(struct sockaddr*) &serv_addr, sizeof(serv_addr));

			// Update packet number
			expected_packet_num++;

			if (msg.last_packet) {
				cout << "Last packet received" << endl;
				break;
			}

		}
		// Out-of-order packet
		// Resend ACK for most recently received in-order packet
		else {
			ack.type = false;
			ack.seq_num = seq;
			ack.packet_num = expected_packet_num;

			sendto(sockfd, &ack, sizeof(ack), 0, 
				(struct sockaddr*) &serv_addr, sizeof(serv_addr));
		}
	}

	// Send repeated ACK's to ensure that the server does not have last ACK dropped
	// Continue to resend last packet even after client closes
	message last;
	last.seq_num = seq;
	for (int i = 0; i < RESEND; i++) {
		sendto(sockfd, &last, sizeof(last), 0,
			(struct sockaddr*) &serv_addr, sizeof(serv_addr));
	}

	// cout << endl;
	// cout << "Packets received" << endl;
	// for (int i = 0; i < messages.size(); i++) {
	// 	cout << "Packet " << i+1 << endl;
	// 	cout << messages[i].first << endl << endl;
	// }

	// Write packet contents to file
	ofstream output;
	stringstream output_buffer; 
	string output_filename = "received_" + filename;
	output.open(output_filename.c_str(), ios::out | ios::binary);

	if (!output.is_open()) {
		cerr << "Could not open file" << endl;
	}
	
	for (int i = 0; i < messages.size(); i++) {
		output << messages[i].first;
	}
	output << output_buffer.rdbuf();
	output.close();

}