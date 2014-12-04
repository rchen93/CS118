#ifndef COMMON_H
#define COMMON_H

const int MAX_PACKET_SIZE = 1024;
const int HEADER_SIZE = 8;
const int DATA_SIZE = MAX_PACKET_SIZE - HEADER_SIZE;
const int WINDOW_SIZE = DATA_SIZE * 5;
const double PACKET_TIMEOUT = 0.1;

struct message {
	bool type;		// True for packet, false for ACK
	bool last_packet;	// True if it is last packet
	int seq_num;
	int packet_num;
	int data_length;
	unsigned char data[MAX_PACKET_SIZE - HEADER_SIZE];
};

#endif /* COMMON_H */