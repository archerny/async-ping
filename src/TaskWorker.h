#ifndef _TASK_WORKER_H_
#define _TASK_WORKER_H_

#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/buffer.h>
#include <event2/util.h>

#include "ThreadWorker.h"
#include "CommonList.h"
#include "HttpResponse.h"

using common::ThreadWorker;
using common::ConcurrentDequeue;

#define DOING_TASK_CAPACITY 1024

class HttpServer;
class Task;

void notifyEventHandler(int fd, short event, void *arg);

class TaskWorker : public ThreadWorker
{
public:
  TaskWorker();
  virtual ~TaskWorker();

  friend void notifyEventHandler(int fd, short event, void *arg);

public:
  bool initWorker();
  void notify();
  virtual void *doRun();
  inline void registerMaster(HttpServer *master) { this->master = master; }
  inline bool enqueueTask(Task *task) { return taskQueue.pushHead(task); }
  inline struct event_base *getEvBase() { return base; }
  void sendToMaster(HttpResponse *response);

private:
  bool initPipe();
  bool closePipe();
  void doTask(Task *task);
  void onComplete(Task *task);

private:
  HttpServer *master;
  ConcurrentDequeue<Task *> taskQueue;
  int notifyFd;
  int recvFd; // recvs data from
  int doingCount;

private:
  struct event_base *base;
  struct event *notifyEvent;
};

#endif