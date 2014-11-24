#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <iostream>
#include <vector>
#include <string>
#include <cstring>

using namespace std;

struct message {
	bool type;		// True for msg, false for ACK
	int seq_num;
	bool last_packet;	// True if it is last packet
	string body;
};

int main(int argc, char** argv) {
	int sockfd, portno, n, seq;
	struct sockaddr_in serv_addr;
	socklen_t len = sizeof(struct serv_addr);
	string hostname, filename;
	vector<string> messages;
	message msg;

	if (argc < 4) {
		cerr << "Incorrect number of arguments" << endl;
		return -1;
	}

	hostname = argv[2];
	portno = atoi(argv[3]);
	filename = argv[4];

	// Set socket and populate server address
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		cerr << "ERROR: opening socket" << endl;
	}
	bzero((char*) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);

	// Send initial request for the file
	n = sendto(sockfd, filename.c_str(), filename.size(), 0,
		(struct sockaddr*) &serv_addr, len);

	// Receive ACK from server
	while (recvfrom(sockfd, &msg, sizeof(msg), 0,
		(struct sockaddr*) &serv_addr, &len) == -1);

	seq = msg.seq_num;

	while (true) {
		n = recvfrom(sockfd, &msg, 1000, 0,
			(struct sockaddr*) &serv_addr, &len);
		if (n == -1) continue;

		messages.push_back(msg.body);

		msg.message_type = false;
		bzero(&msg.body, 1000);
		sendto(sockfd, &msg, n, 0, 
			(struct sockaddr*) &serv_addr, sizeof(serv_addr));

		if (msg.last_packet) break;
	}

}