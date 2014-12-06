#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <sys/time.h>		// gettimeofday()

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <fstream>
#include <sstream> 
#include <stdlib.h>
#include <time.h>

#include "packet.h"

using namespace std;

/*
	This function determines if the packet times out. 
*/
bool isTimeout(timeval curr, timeval old) {
	timeval diff;

	timersub(&curr, &old, &diff);

	// cout << "Diff sec: " << diff.tv_sec << " Diff u_sec: " << diff.tv_usec << " ms: " << (diff.tv_usec/ 1000) << endl;

	return ((diff.tv_sec != 0) || (diff.tv_usec > (PACKET_TIMEOUT * 1000)));
}

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
	int sockfd, portno, n, cwnd, end, seq_num = 0, counter = 0, packet_num = 0;
	struct sockaddr_in serv_addr, client_addr;
	socklen_t len = sizeof(client_addr);
	string filename, line; 
	char temp[100];
	Packet initial, current;
	vector<Packet> packets;
	vector<timeval> sent_times;
	double loss_threshold, corrupt_threshold;
	timeval curr;
	
	if (argc < 5) {
		cerr << "ERROR: Incorrect number of arguments" << endl;
		cerr << "sender <portnumber> <window_size> <packet_loss_probability> <packet_corruption_probability>" << endl;
		exit(1);
	}

	portno = atoi(argv[1]);
	cwnd = atoi(argv[2]);
	end = (cwnd / MAX_PACKET_SIZE) - 1;

	loss_threshold = atof(argv[3]);
	corrupt_threshold = atof(argv[4]);

	if (loss_threshold < 0.0 || loss_threshold > 1.0 || corrupt_threshold < 0.0 || corrupt_threshold > 1.0) {
		cerr << "ERROR: Probabilities should be between 0.0 and 1.0" << endl;
		exit(1);
	}

	// Set socket and populate server address
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	bzero((char*) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;			// Server binds to any/all interfaces

	// Bind server to socket
	if (bind(sockfd, (struct sockaddr*) &serv_addr, len) == -1) {
		cerr << "ERROR: Failed to bind socket" << endl; 
		exit(1);
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

	cout << "TIMESTAMP: " << getCurrentTime() << "EVENT: " << "Requested File " << filename << endl << endl;

	ifstream request(filename.c_str(), ios::in | ios::binary);

	if (request) {
		initial = createPacket(false, seq_num, packet_num);
		initial.last_packet = true;

		// Send ACK with initial seq number 0 confirming valid file
		sendto(sockfd, &initial, sizeof(initial), 0,
			(struct sockaddr*) &client_addr, len);

	} else {
		initial = createPacket(false, -1, packet_num);
		initial.last_packet = true;

		// Send ACK with initial seq number -1 confirming invalid file
		sendto(sockfd, &initial, sizeof(initial), 0,
			(struct sockaddr*) &client_addr, len);

		cerr << "ERROR: File not found" << endl;
		exit(1);
	}

	current = createPacket(true, seq_num, packet_num);

	int data_length = 0;

	// Split data into packets
	while (true) {
		unsigned char c = request.get();
		if (request.eof()) {
			break;
		}
		current.data[counter] = c;
		data_length++;

		if (counter == MAX_PACKET_SIZE - 1) {
			current.seq_num = seq_num;
			current.packet_num = packet_num;

			current.last_packet = false;
			current.data_length = data_length;

			packets.push_back(current);

			counter = -1;
			data_length = 0;
			seq_num += MAX_PACKET_SIZE;
			packet_num++;
		}
		counter++; 
	}

	if (counter != 0) {
		current.data[counter] = '\0';
		current.seq_num = seq_num;
		current.packet_num = packet_num;
		current.data_length = data_length;
		current.last_packet = true;
		packets.push_back(current);
		packet_num++;
	} else {
		packets[packets.size() - 1].last_packet = true;
	}

	int base = 0;
	int next_packet_num = 0;

	// Send all initial packets
	// cout << "Original end: " << end << endl; 
	cout << "TIMESTAMP: " << getCurrentTime() << "EVENT: " << "Sending initial packets up to window" << endl;

	for (next_packet_num; next_packet_num <= end && next_packet_num < packets.size(); next_packet_num++) {
		cout << "TIMESTAMP: " << getCurrentTime() << "EVENT: " << "Sending packet with sequence number ";
		cout << packets[next_packet_num].seq_num << endl; //<< " and packet number " << packets[next_packet_num].packet_num << endl;
		
		sendto(sockfd, &packets[next_packet_num], sizeof(packets[next_packet_num]), 0,
			(struct sockaddr*) &client_addr, len);

		gettimeofday(&curr, NULL);
		sent_times.push_back(curr);
	}

	while (true) {
		gettimeofday(&curr, NULL);
		timeval old = sent_times[0];

		// Check timeouts
		if (isTimeout(curr, old)) {
			cout << "TIMESTAMP: " << getCurrentTime() << "EVENT: " << "Timeout for packet " << base << endl;
			cout << "TIMESTAMP: " << getCurrentTime() << "EVENT: Retransmission" << endl;

			// Resend all packets in window
			sent_times.clear();
			// cout << "Base: " << base << " Next_packet_num: " << next_packet_num << endl;
			for (int i = base; i < next_packet_num && i < packets.size(); i++) {
				cout << "TIMESTAMP: " << getCurrentTime() << "EVENT: " << "Resending packet with sequence number ";
				cout << packets[i].seq_num << endl; // << " and packet number " << packets[i].packet_num << endl;

				sendto(sockfd, &packets[i], sizeof(packets[i]), 0,
					(struct sockaddr*) &client_addr, len);

				gettimeofday(&curr, NULL);
				sent_times.push_back(curr);
			}
			cout << endl;
			continue;
		}

		// Get ACK
		Packet ack;
		n = recvfrom(sockfd, &ack, sizeof(ack), MSG_DONTWAIT,
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

		// Reliability simulation
		// Packet Loss
		if (isPacketBad(loss_threshold)) {
			cout << "TIMESTAMP: " << getCurrentTime() << "EVENT: " << "ACK " << ack.seq_num << " has been lost!" << endl << endl;
			// cout << " and packet number " << ack.packet_num << " has been lost!" << endl << endl;
			continue;
		}

		// Packet Corruption
		if (isPacketBad(corrupt_threshold)) {
			cout << "TIMESTAMP: " << getCurrentTime() << "EVENT: " << "ACK " << ack.seq_num << " has been corrupted!" << endl << endl;
			// cout << " and packet number " << ack.packet_num << " has been corrupted!" << endl << endl;
			continue;
		}

		cout << "TIMESTAMP: " << getCurrentTime() << "EVENT: " << "Received ACK " << ack.seq_num  << endl << endl;
		//cout << " and packet number " << ack.packet_num << endl << endl;

		// All ACKS received
		if (ack.packet_num == packets.size() - 1)
				break;

		// Slide the window upon successful cum ACK
		if (ack.packet_num >= base) {

			//cout << "Old base: " << base << endl;
			//cout << "Old end: " << end << endl;
			base = ack.packet_num + 1;
			end = base + (cwnd/MAX_PACKET_SIZE) - 1; 
			//cout << "New base: " << base << endl;
			//cout << "New end: " << end << endl;

			for (next_packet_num; next_packet_num <= end && next_packet_num < packets.size(); next_packet_num++) {
				cout << "TIMESTAMP: " << "EVENT: " << getCurrentTime() << "Sending packet with sequence number ";
				cout << packets[next_packet_num].seq_num << endl << endl;
				// cout << " and packet number " << packets[next_packet_num].packet_num << endl << endl;

				sendto(sockfd, &packets[next_packet_num], sizeof(packets[next_packet_num]), 0,
					(struct sockaddr*) &client_addr, len);

				sent_times.erase(sent_times.begin());
				gettimeofday(&curr, NULL);
				sent_times.push_back(curr);
			}
		}

	}
	
}