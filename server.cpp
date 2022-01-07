#include <cstdio>
#include <iostream>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>

using std::cout;


constexpr int BUF_SIZE = 1024;
constexpr int MAX_CONN = 10000;

void *handle_client(void *arg);


void error(const char *msg)
{
  perror(msg);
  exit(1);
}


// Echos back the messages received from clients
void echoBack(int sockfd)
{
  int n;
  char buf[BUF_SIZE] = {};
  
  again:
    while ((n = recv(sockfd, buf, BUF_SIZE, 0)) > 0)
      send(sockfd, buf, n, 0);

    if (n < 0) {
      if(errno == EINTR) { // Interrupt occured
        goto again;
      }
      else {
        error("Echo error : ");
      }
    }
}


void *handle_client(void *arg)
{
  int child_fd = *(int *)arg;
  delete (int *)arg;

  // marks this thread as unjoinable
  pthread_detach(pthread_self());
  echoBack(child_fd);
  close(child_fd);

  return NULL;
}


int main(int argc, char **argv)
{
  struct sockaddr_in server_addr, client_addr; 
  int portno;


  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  portno = atoi(argv[1]);
  int parentfd = socket(AF_INET, SOCK_STREAM, 0);
  
  if (parentfd < 0)
    error("ERROR opening socket");

  int optval = SOL_SOCKET;
  setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));

  memset((char *)&server_addr, 0, sizeof(server_addr));

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons((unsigned short)portno);

  if (bind(parentfd, (struct sockaddr *) &server_addr, 
	   sizeof(server_addr)) < 0) {
    error("ERROR on binding");
  }

  if (listen(parentfd, MAX_CONN) < 0)
    error("ERROR on listen");


  int clilen = sizeof(client_addr);
  pthread_t thread;

  while (true) {
    int *childfd = new int; // Use Dynamic allocation to avoid race condition

    cout << "Waiting for connections...\n";
    *childfd = accept(parentfd, (struct sockaddr *)&client_addr,
		      (socklen_t *)&clilen);

    if (*childfd < 0 ) {
      if (errno != EINTR) {
        error("ERROR on accept");
      } else {
        continue;
      }
    }

    cout << "Connected!\n";
    if (int tid = pthread_create(&thread, NULL, &handle_client, childfd); tid < 0) {
      fprintf(stderr, "thread create failed (%d)\n", tid);
      delete childfd;
    }
  }
}
