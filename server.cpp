#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <iostream>
#include <string>
#include <sstream>

using namespace std;

#define BUF_SIZE 1024

char buf[BUF_SIZE+1]; /* message buffer */

void *handle_client(void *arg);

void error(const char *msg)
{
  perror(msg);
  exit(1);
}

void string_return(int m_sockfd)
{
  int n;
  char buf[BUF_SIZE] = {};
  
  again:
    while ((n = recv(m_sockfd, buf, BUF_SIZE, 0)) > 0)
      send(m_sockfd, buf, n, 0);

    if(n < 0 && errno==EINTR) {  // Interrupt occured
      if(errno == EINTR) {
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

  pthread_detach(pthread_self());
  string_return(child_fd);  //Do the load balancing.
  close(child_fd);

  return NULL;
}

int main(int argc, char **argv)
{
  int parentfd;                    /* parent socket */
  int *childfd;                    /* child socket */
  int portno;                      /* port txo listen on */
  int clilen;                      /* byte size of client's address */
  struct sockaddr_in serveraddr;   /* server's addr */
  struct sockaddr_in clientaddr;   /* client addr */
  struct hostent *hostp;           /* client host info */
  char *hostaddrp;                 /* dotted decimal host addr string */
  int optval;                      /* flag value for setsockopt */
  int n;                           /* message byte size */
  int tid;                         /* thread ID */

  pthread_t thread;

  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  portno = atoi(argv[1]);
  parentfd = socket(AF_INET, SOCK_STREAM, 0);

  if (parentfd < 0)
    error("ERROR opening socket");

  optval = 1;
  setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR,
         (const void *)&optval, sizeof(int));

  bzero((char *)&serveraddr, sizeof(serveraddr));

  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  if (bind(parentfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) {
    error("ERROR on binding");
  }

  if (listen(parentfd, 10000) < 0) /* allow 5 requests to queue up */
    error("ERROR on listen");


  clilen = sizeof(clientaddr);
  while (true) {
    childfd = new int;

    cout << "Waiting for connections...\n";
    *childfd = accept(parentfd, (struct sockaddr *)&clientaddr,
		      (socklen_t *)&clilen);

    if (*childfd < 0 ) {
      if(errno!=EINTR) {
        error("ERROR on accept");
      } else {
        continue;
      }
    }

    cout << "Connected!\n";
    if ((tid = pthread_create(&thread, NULL, &handle_client, childfd)) < 0) {
      fprintf(stderr, "thread create failed (%d)\n", tid);
      delete childfd;
    }
  }
}
