#include <iostream>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <csignal>
#include <vector>
#include <mutex>
#include <queue>

using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using std::mutex;
using std::queue;

static void *handle_client(void *arg);
static void init();
void error(const char *msg);

static constexpr int BUF_SIZE = 1024;
static constexpr int MAX_CONN = 10000;
static mutex conns_lock;
static constexpr int MAX_TERM_TIME = 5; // the maximum shutdown duration (second) when server received SIGTERM
static bool shutdown_event;

static queue<int> conns_ids;    // free idx in conns
static vector<pthread_t *> conns(MAX_CONN); // store connected threads

enum class State { kConnected, kShutdown };

class Conn {
public:
  Conn(pthread_t *thread, int child_fd): thread_(thread), child_fd_(child_fd) {
    std::unique_lock<mutex> lock(conns_lock, std::defer_lock);

    lock.lock();
    if (conns_ids.empty())
      error("conns_idx is empty()");

    conns_id_ = conns_ids.front();
    conns_ids.pop();
    if (conns[conns_id_])
      error("conns[conns_id] is not NULL");

    conns[conns_id_] = thread_;
    lock.unlock();
 
    state = State::kConnected;
  }

  ~Conn() {
    std::unique_lock<mutex> lock(conns_lock);
    close(child_fd_);
    conns[conns_id_] = nullptr;
    delete thread_;
    conns_ids.push(conns_id_);
  }

  int getChildFD() const {
    return child_fd_;
  }

  void setState(State s) {
    state = s;
  }

  State getState() const {
    return state;
  }

private:
  int conns_id_;
  int child_fd_;
  pthread_t *thread_;
  State state;
};




void error(const char *msg)
{
  perror(msg);
  exit(1);
}


static void sigtermHandler(int sigNum)
{
  cerr << "Received SIGTERM\nTry to cancel all connection...\n";

  shutdown_event = true;
  
  // make sure each connections is disconnected before we leave.
  for (int i = 0; i < MAX_TERM_TIME; ++i) {
    sleep(1);
    int j;
    for (j = 0; j < MAX_CONN; ++j)
      if (conns[j])
        break;

    if (j == MAX_CONN)
      goto shutdown;
  }


// Force shutdown
forece_shutdown:
  // TODO: It might cause starvation if there are plenty of clients attemp to connect.
  //       However, the number of concurrent connections is limited by MAX_CONNS.
  //       So, it is far away to cause starvation.
  {
    cerr << "Time's up. Force shutdown ..." << endl;
    std::unique_lock<mutex> lock(conns_lock, std::defer_lock);

    lock.lock();
    for (int i = 0; i < MAX_CONN; ++i) {
      if (conns[i] && pthread_cancel(*conns[i]) < 0)
          error("pthread_cacnel");
    }
    lock.unlock();

    cerr << "shutdown" << endl;
  }

shutdown:
  exit(1);
}

// Echos back the messages received from clients
static void echoBack(Conn *conn)
{
  int n;
  char buf[BUF_SIZE] = {};
  const int sockfd = conn->getChildFD();
  
  again:
    //pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    while ((n = recv(sockfd, buf, BUF_SIZE, MSG_DONTWAIT)) > 0) {
      sendAgain:
        if (int tmp = send(sockfd, buf, n, 0); tmp < 0) {
          if (errno == EINTR) {
            goto sendAgain;
          } else if (errno == ECONNRESET) {
            goto disconnect;
          } else {
            error("Echo error : ");
          }
        }


    }


    if (shutdown_event && conn->getState() != State::kShutdown) {
      char msg[0x100];
      snprintf(msg, sizeof(msg), "Server will shutdown in %d seconds !\n", MAX_TERM_TIME);

      sendAgain2:
      if (int tmp = send(sockfd, msg, strlen(msg), 0); tmp < 0) {
        if (errno == EINTR) {
          goto sendAgain2;
        } else if (errno == ECONNRESET) {
          goto disconnect;
        } else {
          error("Echo error : ");
        }
      }
      conn->setState(State::kShutdown);
    }

    //pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    if (n < 0) {
      if(errno == EINTR || errno == EAGAIN) {
        goto again;
      } else if (errno == ECONNRESET) { // client disconnect
        goto disconnect;
      } else {
        error("Echo error : ");
      }
    }
    

  disconnect:
    return;
}

static void *handle_client(void *arg)
{
  Conn *conn = static_cast<Conn *>(arg);
  // Avoid thread occupies SIGTERM handler and make sure async signal safe
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGTERM);
  pthread_sigmask(SIG_BLOCK, &mask, NULL);

  // marks this thread as unjoinable
  pthread_detach(pthread_self());

  echoBack(conn);

  return NULL;
}

static void init()
{
  // set signal handler
  if (signal(SIGTERM, sigtermHandler) == SIG_ERR)
    error("setting signal failed\n");

  // init conns meta
  for (int i = 0; i < MAX_CONN; ++i)
    conns_ids.push(i);
}


int main(int argc, char **argv)
{
  struct sockaddr_in server_addr, client_addr; 
  int portno;


  if (argc != 2) {
    cerr << "usage: " << argv[0] << " <port>\n";
    exit(1);
  }

  init();

  portno = atoi(argv[1]);
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  
  if (server_fd < 0)
    error("ERROR opening socket");

  int optval = SOL_SOCKET;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));

  memset((char *)&server_addr, 0, sizeof(server_addr));

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons((unsigned short)portno);

  if (bind(server_fd, (struct sockaddr *) &server_addr, 
	   sizeof(server_addr)) < 0) {
    error("ERROR on binding");
  }

  if (listen(server_fd, MAX_CONN) < 0)
    error("ERROR on listen");

  int clilen = sizeof(client_addr);

  while (true) {
    cout << "Waiting for connections...\n";
    int child_fd = accept(server_fd, (struct sockaddr *)&client_addr,
		                      (socklen_t *)&clilen);

    if (child_fd < 0 ) {
      if (errno != EINTR) {
        error("ERROR on accept");
      } else {
        continue;
      }
    }

    pthread_t *thread = new pthread_t;
    Conn *conn = new Conn(thread, child_fd); // Use Dynamic allocation to avoid race condition

    cout << "Connected!\n";
    if (int tid = pthread_create(thread, NULL, &handle_client, conn); tid < 0) {
      cerr << "thread create failed (" << tid << ")\n";
      delete conn;
    }
  }
}
