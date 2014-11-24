#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <iostream>
#include <string>
#include <cstring>
#include <vector>

using namespace std;

struct message {
	bool type;
	int seq_num;
	bool last_packet;
	string body;
};

int main(int argc, char** argv) {
	
	if (argc < 3) {
		cout << "Incorrect number of arguments" << endl;
		return -1;
	}
}