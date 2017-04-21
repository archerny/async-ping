#ifndef _PING_H_
#define _PING_H_

#include <string>
#include <stdint.h>

const int PING_TIMEOUT = -1;
const int PING_UNINITED = -2;
const int PING_DOING = -3;
const int PING_SEND_ERROR = -4;
const int PING_SUCCESS =-5;

class Task;
class TaskWorker;

using std::string;


/**
 * ping can complete only on these situation:
 * 1. send from raw socket error
 * 2. no response, wait until timeout (error response that can not be identified goes here)
 * 3. successfully get response
 */
class Ping
{
public:
  Ping();
  ~Ping();

public:
  static void handleTimeout(int fd, short event, void *arg);

  void start();
  bool setTimer();

public:
  unsigned short index;
  string ip;
  int timeout;
  int status;
  int64_t sendtime;
  int64_t recvtime;
  TaskWorker *worker;
  Task *parentTask;
  struct event *timeoutEvent; // timeout for this ping
  struct timeval *tv;
};

#endif