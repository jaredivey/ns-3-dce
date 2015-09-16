#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include "test-macros.h"

// 
// http://linuxjm.sourceforge.jp/html/LDP_man-pages/man7/epoll.7.html
// 
#define BUF_LEN ((size_t) 1024 * 1000)
#define MAX_EVENTS 10
static char* readBuf[BUF_LEN];
static char* writeBuf[BUF_LEN];

static void *
server1 (void *arg)
{
  struct epoll_event ev, events[MAX_EVENTS];
  int conn_sock, nfds, epollfd;
  /* Set up listening socket, 'listen_sock' (socket(),
     bind(), listen()) */
  int sock, sockin, sockin2;
  struct sockaddr_in addr, local;
  int res;
  int on = 1, n;

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT (sock >= 0);

  res = inet_aton ("127.0.0.1", &(addr.sin_addr));
  TEST_ASSERT_EQUAL (res, 1);

  addr.sin_family = AF_INET;
  addr.sin_port = htons (1234);
  res = setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

  res = bind (sock, (struct sockaddr *) &addr, sizeof(addr));
  printf ("Server1: bind -> %d, errno:%d\n ", res, errno);
  TEST_ASSERT_EQUAL (res, 0);

  res = listen (sock, 10);
  TEST_ASSERT_EQUAL (res, 0);

  // epoll
  epollfd = epoll_create(10);
  TEST_ASSERT_UNEQUAL (epollfd, -1);

  ev.events = EPOLLIN;
  ev.data.fd = sock;
  res = epoll_ctl (epollfd, EPOLL_CTL_ADD, sock, &ev);
  TEST_ASSERT_UNEQUAL (res, -1);

  for (;;)
    {
      printf ("epoll wait\n");
      nfds = epoll_wait (epollfd, events, MAX_EVENTS, -1);
      TEST_ASSERT_UNEQUAL (res, -1);

      printf ("epoll wake\n");
      for (n = 0; n < nfds; ++n)
        {
          if (events[n].data.fd == sock)
            {
              conn_sock = accept (sock, NULL, NULL);
              TEST_ASSERT_UNEQUAL (conn_sock, -1);

              // setnonblocking(conn_sock);
              ev.events = EPOLLIN | EPOLLET;
              ev.data.fd = conn_sock;
              res = epoll_ctl (epollfd, EPOLL_CTL_ADD, conn_sock, &ev);
              TEST_ASSERT_UNEQUAL (conn_sock, -1);
            }
          else
            {
              res = read (events[n].data.fd, readBuf, BUF_LEN);
              TEST_ASSERT_EQUAL (res, 64);
            }
        }
    }
  return arg;
}

static void *
client1 (void *arg)
{
  int sock, sock2;
  struct sockaddr_in addr;
  int res;

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT (sock >= 0);
  sock2 = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT (sock2 >= 0);

  res = inet_aton ("127.0.0.1", &(addr.sin_addr));
  TEST_ASSERT_EQUAL (res, 1);

  addr.sin_family = AF_INET;
  addr.sin_port = htons (1234);

  sleep (1);

  res = connect (sock, (struct sockaddr *) &addr, sizeof(addr));
  TEST_ASSERT_EQUAL (res, 0);
  // sleep (1);
  // res = connect (sock2, (struct sockaddr *) &addr, sizeof(addr));
  // TEST_ASSERT_EQUAL (res, 0);

  // Can I Write ?
  res = write (sock, writeBuf, 64);
  TEST_ASSERT_EQUAL (res, 64);

  sleep (5);
  // Read ?
  //  TEST_ASSERT (test_poll_read (sock, 10, true));

  // res = read (sock2, readBuf, BUF_LEN);
  printf ("Client1: write -> %d \n ",res);

  close (sock);
  close (sock2);

  printf ("Client1: end\n ");

  return arg;
}

static void
launch (void *(*clientStart)(void *), void *(*serverStart)(void *))
{
  int status;
  pthread_t theClient;
  pthread_t theServer;

  printf ("launch\n ");

  status = pthread_create (&theServer, NULL, serverStart, 0);
  TEST_ASSERT_EQUAL (status, 0);

  status = pthread_create (&theClient, NULL, clientStart, 0);
  TEST_ASSERT_EQUAL (status, 0);

  void *threadResult = 0;

  status = pthread_join (theClient, &threadResult);
  TEST_ASSERT_EQUAL (status, 0);

  status = pthread_join (theServer, &threadResult);
  TEST_ASSERT_EQUAL (status, 0);

  fflush (stdout);
  fflush (stderr);
}

int
main (int argc, char *argv[])
{

  launch (server1, client1);
}
