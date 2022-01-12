#include <iostream>
#include <pthread.h>
#include <vector>
#include <string>
#include <ctime>
#include <chrono>
#include <cstring>
#include <sys/socket.h>  
#include <netinet/in.h>
#include <stdio.h>  
#include <unistd.h>  
#include <sys/types.h>  
#include <arpa/inet.h>  
#include <cmath> 


using namespace std;

void *echo(void *);

constexpr int test_times = 3;
constexpr int thread_nums = 10000;
constexpr int portno = 1234;
char host[] = "127.0.0.1";

void *echo(void *result)
{
  struct sockaddr_in server;	  
  char msg[] = "hello world";
  char buf[32];
  int n;
 
  //pthread_detach(pthread_self()); 

  auto start = std::chrono::high_resolution_clock::now();
  int sock = socket(AF_INET, SOCK_STREAM, 0); 
	    
  server.sin_family = AF_INET;
  server.sin_port = htons(portno);  
	        
  inet_pton(AF_INET, host, &server.sin_addr.s_addr);  
		  
  connect(sock, (struct sockaddr *)&server, sizeof(server));  
		    
  memset(buf, 0, sizeof(buf));
  n = send(sock, msg, sizeof(msg), 0);  
  n = read(sock, buf, sizeof(buf));  
  close(sock);
		        
  //printf("\t[Info] Receive %d bytes: %s\n", n, buf);  
  auto finish = std::chrono::high_resolution_clock::now();
		  
  *(int *)(result) = std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count();
  pthread_exit(result);
}


long long int measure(const int thread_nums)
{
  vector<pthread_t> threads(thread_nums);
  for (int i = 0; i < thread_nums; ++i) {
    int *res = new int(0);
    pthread_create(&threads[i], NULL, echo, (void *)(res));
  }

  uint64_t total = 0;
  int fail = 0;
  for (int i = 0; i < thread_nums; ++i) {
    void *tmp = nullptr;
    pthread_join(threads[i], &tmp);
    if (!tmp) {
	++fail;
        continue;
    }

    total += *(int *)tmp;
    delete (int *)tmp;
  }

  return total;

/*  cout << "thread num: " << (thread_nums-fail) << endl;
  cout << "total time   = " << total << "ns" << endl;
  cout << "average time = " << total/(thread_nums-fail) << "ns" << endl << endl;
  */
}

int main(int argc, char *argv[])
{
  int level = log10(thread_nums);
  vector<long long int> sum(level);
  int cnt = 0;



  for (int i = 1; i <= thread_nums; i *= 10) {
    for (int j = 0; j < level; ++j) {
      sum[cnt] += measure(i);
    }

    sum[cnt] /= level;
    sum[cnt++] /= i;
  }

  for (int i = 0; i < level; ++i)
    cout << sum[i] << " ns" << endl;

  return 0;
}
