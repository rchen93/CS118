/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   This version runs forever, forking off a separate 
   process for each connection
*/
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <signal.h>	/* signal name macros, and the kill() prototype */

#include <string>
#include <assert.h>
#include <iostream>
#include <fstream>

/* FUNCTION PROTOTYPES */
void dostuff(int);
std::string parseMessage (const std::string&);
std::string getFileType (const std::string&); 
int writeHTML(const std::string&, int); 

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void error(char *msg)
{
    perror(msg);
    exit(1);
}

/* TESTING */

// TODO: Fill out test
void shouldParseMessage() {

}

// TODO: Fill out rest of extensions
void shouldGetFileType() {
  // HTML
  std::string fp1 = "hello.html";
  std::string ext1 = getFileType("hello.html");
  assert("html" == ext1);

  // JPEG

  // JPG

  // GIF

  // Invalid
}

void testSuite() {
  shouldParseMessage();
  shouldGetFileType();
}

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno, pid;
     socklen_t clilen;
     struct sockaddr_in serv_addr, cli_addr;
     struct sigaction sa;          // for signal SIGCHLD

     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     
     listen(sockfd,5);
     
     clilen = sizeof(cli_addr);
     
     /****** Kill Zombie Processes ******/
     sa.sa_handler = sigchld_handler; // reap all dead processes
     sigemptyset(&sa.sa_mask);
     sa.sa_flags = SA_RESTART;
     if (sigaction(SIGCHLD, &sa, NULL) == -1) {
         perror("sigaction");
         exit(1);
     }
     /*********************************/
     
     while (1) {
         newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
         
         if (newsockfd < 0) 
             error("ERROR on accept");
         
         pid = fork(); //create a new process
         if (pid < 0)
             error("ERROR on fork");
         
         if (pid == 0)  { // fork() returns a value of 0 to the child process
             close(sockfd);
             testSuite();        // uncomment for tests
             dostuff(newsockfd);
             exit(0);
         }
         else //returns the process ID of the child process to the parent
             close(newsockfd); // parent doesn't need this 
     } /* end of while */
     return 0; /* we never get here */
}

/********HELPER FUNCTIONS***************/

/*
  This function takes in the full message as the input and parses the 
  GET header and returns the file being requested as a string.
*/
std::string parseMessage (const std::string& message) {
  // std::cout << "Inside parseMessage" << std::endl;
  // std::cout << message << std::endl;
  int start = message.find("GET /") + 5;
  // std::cout << "Start" << start << std::endl;
  int end = message.find("HTTP/");
  // std::cout << "End" << end << std::endl;
  return message.substr(start, end-start-1);
}

/*
  This function takes in a string containing the filepath and returns
  the file type. If it is not HTML, JPEG, JPG, or GIF, then it will
  return an empty string.
*/
std::string getFileType (const std::string& filepath) {
  // std::cout << filepath << std::endl;

  if (filepath.find(".html") != std::string::npos) 
    return "html";
  else if (filepath.find(".jpg") != std::string::npos ||
           filepath.find(".jpeg") != std::string::npos) 
    return "jpeg";
  else if (filepath.find(".gif") != std::string::npos)
    return "gif";

  return "";
}

int writeHTML(const std::string& filepath, int sock) {
  std::string response, line; 

  // std::cout << "File: " << filepath << std::endl;

  std::ifstream request(filepath.c_str());

  // std::cout << request << std::endl;

  if (request.is_open()) {
    while(std::getline(request, line)) {
      response += line;
      std::cout << "Line: " << line << std::endl;
    }

  request.close();  
  }

  // std::cout << "Response: " << response << std::endl;
  return write(sock, response.c_str(), response.length()); 
}

/******** DOSTUFF() *********************
 There is a separate instance of this function 
 for each connection.  It handles all communication
 once a connnection has been established.
 *****************************************/
void dostuff (int sock)
{
   int n;
   char buffer[256];  
   std::string message;
   std::string response;

   // Reads 255 bytes of the entire input at a time
   while (1) {
      n = read(sock, buffer, 255);
      if (n < 0) error("ERROR reading from socket");
      std::string temp = std::string(buffer);
      message += temp;
      // std::cout << temp << std::endl;

      // When the two-carriage return is detected, stop reading
      if (message.find("\r\n\r\n") != std::string::npos)
        break;
      bzero(buffer, 256);
   }

   std::cout << "Here is the message: " << message << std::endl;
   std::string file = parseMessage(message);
   std::string fileType = getFileType(file);

   if (fileType == "html") {
      n = writeHTML(file, sock);
   }

   else {
       n = write(sock,response.c_str(), response.length());
   }

   if (n < 0) error("ERROR writing to socket");
}
