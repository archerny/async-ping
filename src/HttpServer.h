#ifndef _HTTP_SERVER_H_
#define _HTTP_SERVER_H_

#include <string>
#include <map>
#include <vector>

#include "Log.h"
#include "ThreadWorker.h"
#include "CommonList.h"
#include "HttpResponse.h"
#include "Task.h"

using common::ThreadWorker;
using common::ConcurrentDequeue;
using std::string;
using std::map;
using std::pair;
using std::vector;

extern Logger logger;

class TaskWorker;
class Task;

void tryRespEventHandler(int fd, short event, void *arg);

class HttpServer : public ThreadWorker
{

public:
  HttpServer(unsigned short port);
  virtual ~HttpServer();

public:
  // thread entrance
  bool initServer();
  virtual void *doRun();

  friend void tryRespEventHandler(int fd, short event, void *arg);

public:
  bool registerHandler(const string &path, void (*handler)(struct evhttp_request *, void *));
  void registerWorker(TaskWorker *worker);
  inline bool enqueueResponse(Task *resp) { return responseQueue.pushHead(resp); }
  bool sendToWorker(Task *task);
  void notify();
  // not thread safe
  map<Task *, struct evhttp_request *> doingRequest;

private:
  bool setTimerTask();
  bool setNotifyEvent();
  bool doRegister();
  bool initPipe();
  bool closePipe();
  map<string, void *> handlers;

private:
  // httpserver info
  unsigned short bindPort;
  int notifyFd;
  int recvFd;
  // index for find task's original task
  ConcurrentDequeue<Task *> responseQueue;
  vector<TaskWorker *> workers;

private:
  // libevent structs
  struct event_base *base;
  struct evhttp *http;
  struct evhttp_bound_socket *handle;
  struct event *timeoutEvent;
  struct event *tryRespEvent;
  struct timeval *tv;
};

#endif