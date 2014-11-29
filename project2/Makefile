all: receivermake sendermake
receivermake : receiver.cpp
	g++ -o receiver receiver.cpp
sendermake : sender.cpp
	g++ -o sender sender.cpp
clean: receiverclean senderclean receivedclean
receiverclean :
	rm receiver
senderclean :
	rm sender
receivedclean :
	rm received_*