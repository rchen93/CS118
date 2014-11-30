#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <stdlib.h> 

using namespace std;

const int MAX_PACKET_SIZE = 1000;

struct message {
	bool type;		// True for msg, false for ACK
	int seq_num;
	bool last_packet;	// True if it is last packet
	char body[1000];
};

int main(int argc, char** argv) {
	int sockfd, portno, n, seq;
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
	cout << "Seq: " << seq << endl;

	int expected_seq_num = seq + 1;
	while (true) {
		bzero(&msg, sizeof(message));
		n = recvfrom(sockfd, &msg, sizeof(msg), 0,
			(struct sockaddr*) &serv_addr, &len);

		if (n <= 0) {
			continue;
		}
		cout << "Received packet: " << msg.seq_num << endl;
		
		// Packet is received in order
		message ack;
		if (msg.seq_num == expected_seq_num) {
			cout << "Correct Sequence #" << endl;
			messages.push_back(msg.body);

			// Build ACK
			ack.type = false;
			ack.seq_num = expected_seq_num;
			sendto(sockfd, &ack, sizeof(ack), 0, 
				(struct sockaddr*) &serv_addr, sizeof(serv_addr));

			// Update sequence number
			expected_seq_num += sizeof(msg.body);

			if (msg.last_packet) {
				cout << "Last packet received" << endl;
				break;
			}

		}
		// Duplicate ACK
		else {
			ack.type = false;
			ack.seq_num = expected_seq_num;
			sendto(sockfd, &ack, sizeof(ack), 0, 
				(struct sockaddr*) &serv_addr, sizeof(serv_addr));
		}
	}

	for (int i = 0; i < messages.size(); i++) {
		cout << "Packet " << i << endl;
		cout << messages[i] << endl << endl;
	}

}