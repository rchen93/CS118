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

#include "packet.h"

using namespace std;

/*
  This function returns the current time as a string, which is used
  for the Date: header field.
*/
string getCurrentTime() {
  time_t raw_current_time = time(0);
  string curr_time(ctime(&raw_current_time));
  return curr_time;
}

int main(int argc, char** argv) {
	int sockfd, portno, n, curr_seq_num = 0, expected_packet_num = 0;
	struct sockaddr_in serv_addr;
	socklen_t len = sizeof(serv_addr);
	string hostname, filename;
	vector<string> packets;
	Packet msg, initial, ack, last;
	double loss_threshold, corrupt_threshold;

	if (argc < 6) {
		cerr << "ERROR: Incorrect number of arguments" << endl;
		cerr << "receiver <sender_hostname> <sender_portnumber> <filename> <packet_loss_probability> <packet_corruption_probability>" << endl;
		exit(1);
	}

	hostname = argv[1];
	portno = atoi(argv[2]);
	filename = argv[3];
	loss_threshold = atof(argv[4]);
	corrupt_threshold = atof(argv[5]);

	if (loss_threshold < 0.0 || loss_threshold > 1.0 || corrupt_threshold < 0.0 || corrupt_threshold > 1.0) {
		cerr << "ERROR: Probabilities should be between 0.0 and 1.0" << endl;
		exit(1);
	}

	// Set up socket and connection info
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		cerr << "ERROR: opening socket" << endl;
		exit(1);
	}

	bzero((char*) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	inet_aton(hostname.c_str(), &serv_addr.sin_addr);
	serv_addr.sin_port = htons(portno);

	// Send initial request for the file
	cout << "TIMESTAMP: " << getCurrentTime() << "EVENT: " << "Sending initial request for file " << filename.c_str() << endl << endl;
	n = sendto(sockfd, filename.c_str(), filename.size(), 0,
		(struct sockaddr*) &serv_addr, len);

	// Receive ACK from server
	cout << "TIMESTAMP: " << getCurrentTime() << "EVENT: " << "Receiving ACK for initial request" << endl << endl;
	while (recvfrom(sockfd, &initial, sizeof(initial), 0,
		(struct sockaddr*) &serv_addr, &len) == -1);

	if (initial.seq_num == -1) {
		cerr << "ERROR: File not found" << endl;
		exit(1);
	}

	while (true) {
		bzero(&msg, sizeof(Packet));
		n = recvfrom(sockfd, &msg, sizeof(msg), 0,
			(struct sockaddr*) &serv_addr, &len);
		if (n <= 0) {
			continue;
		}

		// Reliability simulation
		// Packet Loss
		if (isPacketBad(loss_threshold)) {
			cout << "TIMESTAMP: " << getCurrentTime() << "EVENT: " << "Packet with sequence number " << msg.seq_num << " and packet number " << msg.packet_num << " has been lost!" << endl << endl;
			continue;
		}

		// Packet Corruption
		if (isPacketBad(corrupt_threshold)) {
			cout << "TIMESTAMP: " << getCurrentTime() << "EVENT: " << "Packet with sequence number " << msg.seq_num << " and packet number " << msg.packet_num << " has been corrupted!" << endl << endl;
			continue;
		}

		cout << "TIMESTAMP: " << getCurrentTime() << "EVENT: " << "Received packet with sequence number " << msg.seq_num << " and packet number " << msg.packet_num << endl << endl;
		
		// Packet is received in order
		if (msg.packet_num == expected_packet_num) {
			cout << "TIMESTAMP: " << getCurrentTime() << "EVENT: " << "In order packet received" << endl << endl;

			// Extract 
			string data;
			for (unsigned int i = 0; i < msg.data_length; i++) {
				data += msg.data[i];
			}
			packets.push_back(data);

			// Make ACK packet
			curr_seq_num += msg.data_length;
			ack = createPacket(false, curr_seq_num, expected_packet_num);

			// Send ACK packet
			cout << "TIMESTAMP: " << getCurrentTime() << "EVENT: " << "Sending ACK with sequence number " << ack.seq_num << " and packet number " << ack.packet_num << endl << endl; 
			sendto(sockfd, &ack, sizeof(ack), 0, 
				(struct sockaddr*) &serv_addr, sizeof(serv_addr));

			// Update packet number
			expected_packet_num++;

			if (msg.last_packet) {
				cout << "TIMESTAMP: " << getCurrentTime() << "EVENT: " << "Last packet received" << endl << endl;
				break;
			}
		}
		// Out-of-order packet
		else {
			cout << "TIMESTAMP: " << getCurrentTime() << "EVENT: " << "Out of order packet received" << endl << endl;
			cout << "TIMESTAMP: " << getCurrentTime() << "EVENT: " << "Expected packet number " << expected_packet_num << " Got " << msg.packet_num << endl << endl;

			ack = createPacket(false, curr_seq_num, expected_packet_num - 1);

			cout << "TIMESTAMP: " << getCurrentTime() << "EVENT: " << "Sending ACK with sequence number " << ack.seq_num << " and packet number " << ack.packet_num << endl << endl; 

			// Resend ACK for most recently received in-order packet
			sendto(sockfd, &ack, sizeof(ack), 0, 
				(struct sockaddr*) &serv_addr, sizeof(serv_addr));
		}
	}

	// Send repeated ACK's to ensure that the server does not have last ACK dropped
	// Continue to resend last packet even after client closes
	last = createPacket(false, curr_seq_num, expected_packet_num - 1);
	for (int i = 0; i < DEFAULT; i++) {
		sendto(sockfd, &last, sizeof(last), 0,
			(struct sockaddr*) &serv_addr, sizeof(serv_addr));
	}

	// Write packet contents to file
	ofstream output;
	string output_filename = "received_" + filename;

	output.open(output_filename.c_str(), ios::out | ios::binary);
	if (!output.is_open()) {
		cerr << "ERROR: Cannot open output file" << endl;
		exit(1);
	}

	for (int i = 0; i < packets.size(); i++) {
		output << packets[i];
	}
	output.close();
}