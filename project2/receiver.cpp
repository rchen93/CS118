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

using namespace std;

const int MAX_PACKET_SIZE = 1000;
const int RESEND = 10;

struct message {
	bool type;		// True for msg, false for ACK
	int seq_num;
	int packet_num;
	bool last_packet;	// True if it is last packet
	char body[1000];
};

int main(int argc, char** argv) {
	int sockfd, portno, n, seq, packet;
	struct sockaddr_in serv_addr;
	socklen_t len = sizeof(struct sockaddr_in);
	string hostname, filename;
	vector<string> messages;
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
	cout << "Sending initial request for file" << endl;
	n = sendto(sockfd, filename.c_str(), filename.size(), 0,
		(struct sockaddr*) &serv_addr, len);

	// Receive ACK from server
	cout << "Receiving ACK" << endl;
	while (recvfrom(sockfd, &msg, sizeof(msg), 0,
		(struct sockaddr*) &serv_addr, &len) == -1);
	seq = msg.seq_num;
	packet = msg.packet_num;
	cout << "ACK's sequence number: " << seq << endl;
	cout << "ACK's packet number: " << packet << endl;

	int expected_packet_num = packet + 1;
	seq++;
	while (true) {
		bzero(&msg, sizeof(message));
		n = recvfrom(sockfd, &msg, sizeof(msg), 0,
			(struct sockaddr*) &serv_addr, &len);

		if (n <= 0) {
			continue;
		}
		cout << "Received packet with Sequence Number: " << msg.seq_num << endl;
		cout << "Received packet " << msg.packet_num << endl;
		
		// Packet is received in order
		message ack;
		if (msg.packet_num == expected_packet_num) {
			cout << "Correct Packet #" << endl;

			// Extract 
			messages.push_back(msg.body);

			// Make ACK packet
			ack.type = false;
			ack.packet_num = expected_packet_num;
			seq += strlen(msg.body);
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

	// Write packet contents to file
	ofstream output;
	string output_filename = "received_" + filename;
	output.open(output_filename.c_str(), ios::out | ios::binary);
	for (int i = 0; i < messages.size(); i++) {
		output << messages[i];
	}
	output.close();

	// cout << endl;
	// cout << "Packets received" << endl;
	// for (int i = 0; i < messages.size(); i++) {
	// 	cout << "Packet " << i+1 << endl;
	// 	cout << messages[i] << endl << endl;
	// }

}