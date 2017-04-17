#ifndef _PING_H_
#define _PING_H_

#include <string>

const int PING_TIMEOUT = -1;
const int PING_UNINITED = -2;
const int PING_NOT_DONE = -3;

class Task;
class TaskWorker;

using std::string;

class Ping
{
public:
  static void handleRequest();
  static void handleReceive();
  static void handleTimeout(int fd, short event, void *arg);

  void start();
  bool setTimer();
  bool cancelTimer();
  bool cancelEvent();

public:
  unsigned short index;
  string ip;
  int timeout;
  int status;
  int round;
  TaskWorker *worker;
  Task *parentTask;
  struct event *pingEvent;
  struct event *timeoutEvent; // timeout for this ping
  struct timeval *tv;
  // struct sockaddr_in sa; // internet address
};

#endif