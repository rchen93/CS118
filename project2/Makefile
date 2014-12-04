all: receivermake sendermake
receivermake : receiver.cpp 
	g++ -o receiver receiver.cpp -I.
sendermake : sender.cpp 
	g++ -o sender sender.cpp -I.
clean: receiverclean senderclean receivedclean
receiverclean :
	rm receiver
senderclean :
	rm sender
receivedclean :
	rm received_*