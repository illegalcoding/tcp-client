#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <thread>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sched.h>
#include <chrono>
#define DEFAULT_BUFLEN 512
int do_exit = 0;
void sender(int send_result, int client_socket, char* sendbuf) {
  while(!do_exit) {
    std::cin.getline(sendbuf, DEFAULT_BUFLEN);
    send_result = send(client_socket, sendbuf, strlen(sendbuf), 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  return;
}
// @TODO: fix program not detecting CLOSE_WAIT
// When the socket goes CLOSE_WAIT, the program doesn't detect it, and just keeps going along, thinking the connection is open.
void listener(int return_value, int sender_socket, char* recvbuf, int recvbuflen, char* clientaddr) {
  while(!do_exit) {  
    try {
      return_value = recv(sender_socket, recvbuf, recvbuflen, 0);
      if (return_value == -1) {
	printf("\nConnection lost");
	// Theoretically, we shouldn't need to close here, since after the join it gets closed anyway.
	//close(sender_socket);
	do_exit = 1;
	//close(sender_socket);
	//exit(1);
      }
      // printf("\nBytes received: %d\n", return_value);
      
      if (return_value > 0) {
	for (int i = 0; i < DEFAULT_BUFLEN; i++) {
	  if (recvbuf[i] == '\0') {
	    // std::cout << std::endl;
	    memset(recvbuf, 0, DEFAULT_BUFLEN);
	    break;
	  }
	  else {
	    std::cout << recvbuf[i];
	  }
	}
	
      }
      
    }
    
    catch (const std::exception& e) {

    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  
  }
  return;
}
void usage() {
  fprintf(stderr, "usage:\ttcp-client address port\n");
}
static void sighandler(int signum)
{

  fprintf(stderr, "Signal caught, press <ENTER> to exit.");
  do_exit = 1;
  
}
int main(int argc, char **argv) {
	if (argc < 3) {
		usage();
		return 1;
	}
	signal(SIGINT, sighandler);
	int return_value;
	struct addrinfo *result = NULL,  *ptr = NULL, hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	return_value = getaddrinfo(argv[1], argv[2], &hints, &result);
	if (return_value != 0) {
		printf("getaddrinfo failed: %d\n", return_value);
		return 1;
	}
	int connect_socket;
	ptr = result;
	connect_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
	return_value = connect(connect_socket, ptr->ai_addr, (int)ptr->ai_addrlen);
	struct sockaddr_in client_addr;
	socklen_t addrlen = sizeof(client_addr);
	return_value = getpeername(connect_socket, (struct sockaddr*)&client_addr, &addrlen);
	char addrbuf[DEFAULT_BUFLEN];
	memset(addrbuf, 0, DEFAULT_BUFLEN);
	std::cout << "connected to " << inet_ntop(client_addr.sin_family, &client_addr.sin_addr, addrbuf, DEFAULT_BUFLEN) << std::endl;
	freeaddrinfo(result);
	int recvbuflen = DEFAULT_BUFLEN;
	char sendbuf[DEFAULT_BUFLEN];
	char recvbuf[DEFAULT_BUFLEN];
	memset(&recvbuf, 0, DEFAULT_BUFLEN);
	int send_result;
	memset(&sendbuf, 0, DEFAULT_BUFLEN);
	std::thread t_listener(&listener, return_value, connect_socket, recvbuf, recvbuflen, addrbuf);
	std::thread t_sender(&sender, send_result, connect_socket, sendbuf);
	t_listener.join();
	t_sender.join();
	close(connect_socket);
	return 0;
}
