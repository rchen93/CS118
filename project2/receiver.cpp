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
	string body;
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

	cout << "Sending initial request" << endl;
	// Send initial request for the file
	n = sendto(sockfd, filename.c_str(), filename.size(), 0,
		(struct sockaddr*) &serv_addr, len);

	cout << "Receiving ACK" << endl;
	// Receive ACK from server
	while (recvfrom(sockfd, &msg, sizeof(msg), 0,
		(struct sockaddr*) &serv_addr, &len) == -1);

	seq = msg.seq_num;
	cout << "Seq: " << seq << endl;

	while (true) {
		cout << "Inside while loop" << endl; 
		bzero(&msg, sizeof(message));
		cout << "Zeroed out message" << endl; 
		n = recvfrom(sockfd, &msg, sizeof(message), 0,
			(struct sockaddr*) &serv_addr, &len);
		if (n == -1) {
			cout << "n is -1" << endl;
			continue;
		}
		cout << "Received packet: " << msg.seq_num << endl;
		cout << msg.body << endl;
		messages.push_back(msg.body);

		msg.type = false;
		bzero(&msg.body, MAX_PACKET_SIZE);
		sendto(sockfd, &msg, n, 0, 
			(struct sockaddr*) &serv_addr, sizeof(serv_addr));

		if (msg.last_packet) break;
	}

}