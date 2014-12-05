#ifndef PACKET_H
#define PACKET_H

const int MAX_PACKET_SIZE = 1024;
const int HEADER_SIZE = 8;
const int DATA_SIZE = MAX_PACKET_SIZE - HEADER_SIZE;
const int PACKET_TIMEOUT = 500; // 500 ms = 0.5s
const int DEFAULT = 10;


struct Packet {
	bool type;		// True for message, false for ACK
	bool last_packet;	// True if it is last packet
	int seq_num;
	int packet_num;
	int data_length;
	unsigned char data[MAX_PACKET_SIZE - HEADER_SIZE];
};

/*
	This function determines if a packet is bad by comparing a randomly
	generated probability with the threshold probability.
*/
bool isPacketBad(double threshold) {
	double prob = rand() / (double) RAND_MAX;
	return prob <= threshold;
}

/*
	This function creates a packet with given type, sequence number and
	packet number.
*/
Packet createPacket(bool msg_type, int seq, int packet) {
	Packet new_packet;

	new_packet.type = msg_type;
	new_packet.seq_num = seq;
	new_packet.packet_num = packet;

	return new_packet;
}

#endif /* PACKET_H */