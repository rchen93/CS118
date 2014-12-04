#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/fcntl.h>

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <fstream>
#include <sstream> 
#include <stdlib.h>
#include <iterator>

#include "common.h"

using namespace std;

int main(int argc, char** argv) {
	int sockfd, portno, n, cwnd, end, seq_num = 0, counter = 0, packet_num = 0;
	struct sockaddr_in serv_addr, client_addr;
	socklen_t len = sizeof(struct sockaddr_in);
	string filename, line; 
	char temp[100];
	message initial, current;
	vector<message> packets;
	vector<time_t> sent_times;
	unsigned char data[MAX_PACKET_SIZE - HEADER_SIZE];
	
	if (argc < 2) {
		cerr << "ERROR: Incorrect number of arguments" << endl;
		cerr << "sender <portnumber>" << endl;
		return -1;
	}

	portno = atoi(argv[1]);
	cwnd = WINDOW_SIZE;
	end = (cwnd / DATA_SIZE) - 1;

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

	temp[n] = '\0';
	for (int i = 0; i < strlen(temp); i++) {
		filename += temp[i];
	}

	cout << "Filename: " << filename << endl;

	// Create initial ACK for file request
	initial.type = false;
	initial.packet_num = packet_num; 
	initial.last_packet = true;

	ifstream request(filename.c_str(), ios::in | ios::binary);

	if (request) {
		initial.seq_num = seq_num;

		// Send ACK with initial seq number 0 confirming valid file
		sendto(sockfd, &initial, sizeof(initial), 0,
			(struct sockaddr*) &client_addr, len);

		//buffer = vector<unsigned char>((istreambuf_iterator<char>(request)), (istreambuf_iterator<char>()));
		//cout << "Buffer size: " << buffer.size() << endl;
	} else {
		initial.seq_num = -1;

		// Send ACK with initial seq number -1 confirming invalid file
		sendto(sockfd, &initial, sizeof(initial), 0,
			(struct sockaddr*) &client_addr, len);

		cerr << "ERROR: File not found" << endl;
		return -1;
	}

	current.last_packet = false;
	current.type = true;
	int data_length = 0;

	// Split data into packets
	while (true) {
		unsigned char c = request.get();
		if (request.eof()) {
			break;
		}
		current.data[counter] = c;
		data_length++;
		if (counter == DATA_SIZE - 1) {
			// cout << "Packet has MAX_PACKET_SIZE" << endl;
			current.seq_num = seq_num;
			current.packet_num = packet_num;
			current.data_length = data_length;
			packets.push_back(current);

			counter = -1;
			data_length = 0;
			seq_num += MAX_PACKET_SIZE;
			packet_num++;
		}
		counter++; 
	}

	cout << "Counter: " << counter << endl;
	if (counter != 0) {
		cout << "Last packet is smaller than max packet size" << endl;
		current.data[counter] = '\0';
		current.seq_num = seq_num;
		current.packet_num = packet_num;
		current.data_length = data_length;
		current.last_packet = true;
		packets.push_back(current);
		packet_num++;
	} else {
		packets[packets.size() - 1].data[DATA_SIZE - 1] = '\0';
		packets[packets.size() - 1].last_packet = true;
	}

	int base = 0;
	int next_packet_num = 0;

	// Send all initial packets
	cout << "Sending initial packets" << endl;

	for (next_packet_num; next_packet_num <= end && next_packet_num < packets.size(); next_packet_num++) {
		cout << "Next_packet_num " << next_packet_num << endl;
		sendto(sockfd, &packets[next_packet_num], sizeof(packets[next_packet_num]), 0,
			(struct sockaddr*) &client_addr, len);

		sent_times.push_back(time(NULL));
	}

	while (true) {
		if (base == next_packet_num)
			break;

		// Check timeouts
		double diff = difftime(time(NULL), sent_times[0]);
		cout << "Difference: " << diff << endl;
		if (diff > PACKET_TIMEOUT) {
			cout << "Timeout for packet " << base << endl;
			// Resend all packets in window
			sent_times.clear();
			for (int i = base; i < next_packet_num && i < packets.size(); i++) {
				cout << "Resending packet " << i << endl;
				sendto(sockfd, &packets[i], sizeof(packets[i]), 0,
					(struct sockaddr*) &client_addr, len);

				sent_times.push_back(time(NULL));
			}
			continue;
		}

		// Get a packet
		message ack;
		n = recvfrom(sockfd, &ack, sizeof(ack), O_NONBLOCK,
			(struct sockaddr*) &client_addr, &len);
		if (n == 0) {
			continue;	// No more messages
		}
		else if (n < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				continue;	// No messages immediately available
			}
			break;	// Error
		}


		// TODO: Check corruption and randomly drop packets

		cout << "Received ACK with seq_num: " << ack.seq_num << " and packet_num: " << ack.packet_num << endl;

		if (ack.packet_num == base) {
			cout << "Old base: " << base << endl;
			cout << "Old end: " << end << endl;
			base = ack.packet_num + 1;
			end++; 
			cout << "New base: " << base << endl;
			cout << "New end: " << end << endl;

			for (next_packet_num; next_packet_num <= end && next_packet_num < packets.size(); next_packet_num++) {
				cout << "Sending packet: " << next_packet_num << endl;

				sendto(sockfd, &packets[next_packet_num], sizeof(packets[next_packet_num]), 0,
					(struct sockaddr*) &client_addr, len);

				sent_times.erase(sent_times.begin());
				time_t curr = time(NULL);
				sent_times.push_back(curr);
			}

			if (next_packet_num >= packets.size())
				next_packet_num = packets.size();
		} 
	}
	
}