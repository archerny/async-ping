#ifndef _TASK_WORKER_H_
#define _TASK_WORKER_H_

#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/buffer.h>
#include <event2/util.h>

#include <map>

#include "ThreadWorker.h"
#include "CommonList.h"
#include "HttpResponse.h"

using common::ThreadWorker;
using common::ConcurrentDequeue;
using std::map;

#define DOING_TASK_CAPACITY 1024

class HttpServer;
class Task;

void notifyEventHandler(int fd, short event, void *arg);
void socketReadHandler(int fd, short event, void *arg);

class TaskWorker : public ThreadWorker
{
public:
  TaskWorker();
  virtual ~TaskWorker();

  friend void notifyEventHandler(int fd, short event, void *arg);
  friend void socketReadHandler(int fd, short event, void *arg);

public:
  bool initWorker();
  void notify();
  virtual void *doRun();
  inline void registerMaster(HttpServer *master) { this->master = master; }
  inline bool enqueueTask(Task *task) { return taskQueue.pushHead(task); }
  inline struct event_base *getEvBase() { return base; }
  void sendToMaster(Task *response);
  void onComplete(Task *task);
  void onReceive(uint64_t timestamp, unsigned short seq, unsigned short index);
  unsigned short getShortPid() { return pid & 0xffff; }
  void setThreadId(unsigned short threadId) { this->threadId = threadId; }
  inline unsigned short getThreadId() { return threadId; }
  evutil_socket_t getRawSocket() { return rawSocketFd; }

private:
  bool initPipe();
  bool initRawSocket();
  bool closePipe();
  void doTask(Task *task);

private:
  HttpServer *master;
  map<uint64_t, Task *> doingMap;
  ConcurrentDequeue<Task *> taskQueue;  // todo queue
  int notifyFd;
  int recvFd; // recvs data from
  int doingCount;
  pid_t pid;
  unsigned short threadId;

private:
  struct event_base *base;
  struct event *notifyEvent;
  struct event *sockReadEvent;
  evutil_socket_t rawSocketFd;
};

#endif