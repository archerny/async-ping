#include <stdlib.h>
#include <unistd.h>
#include <iostream>

#include "TaskWorker.h"
#include "Task.h"
#include "HttpServer.h"
#include "Ping.h"

extern Logger logger;

void notifyEventHandler(int fd, short event, void *arg)
{
  TaskWorker *me = (TaskWorker *)arg;
  char tmpBuf[2];
  ssize_t readed = read(me->recvFd, tmpBuf, 1);
  if (readed <= 0) {
    logger.log(ERROR_LOG, "task worker read pipe error, try pop one from queue");
  }
  Task *task;
  if (!me->taskQueue.popTail(task))
  {
    logger.log(ERROR_LOG, "task worker notified, but nothing to do, return");
    return;
  }
  if (me->doingCount >= DOING_TASK_CAPACITY)
  {
    logger.log(ERROR_LOG, "too many doing tasks, drop this one");
    task->prepareError = true;
    task->preErrorCode = 1005;
    me->sendToMaster(task);
    return;
  }
  if (task->initTask(me->base))
  {
    // really doing this task
    me->doingCount ++;
    me->doTask(task);
  }
  else
  {
    logger.log(ERROR_LOG, "init task error, failed to execute");
    task->prepareError = true;
    task->preErrorCode  = 1006;
    me->sendToMaster(task);
    return;
  }
}


TaskWorker::TaskWorker()
{
  base = NULL;
  notifyEvent = NULL;
  master = NULL;
  doingCount = 0;
  pid = getpid();
}

TaskWorker::~TaskWorker()
{
  // close the pipes inited
  if (notifyEvent) {
    delete notifyEvent;
    notifyEvent = NULL;
  }
  if (base)
  {
    event_base_free(base);
    base = NULL;
  }
}

void TaskWorker::notify()
{
  // just a simple notify
  ssize_t written = write(notifyFd, "c", 1);
  if (written <= 0) {
    // write error, log
  }
}

bool TaskWorker::initWorker()
{
  if (!initPipe()) {
    return false;
  }
  base = event_base_new();
  if (!base)
  {
    // init event base error, log it
    return false;
  }
  notifyEvent = new event;
  event_assign(notifyEvent, base, recvFd, EV_READ | EV_PERSIST, notifyEventHandler, (void *)this);
  event_add(notifyEvent, NULL);
  return true;
}

bool TaskWorker::initPipe()
{
  int fds[2];
  if (pipe(fds))
  {
    // create pipe error, log
    return false;
  }
  recvFd = fds[0];
  notifyFd = fds[1];
  return true;
}

bool TaskWorker::closePipe()
{
  close(recvFd);
  close(notifyFd);
  return true;
}

void *TaskWorker::doRun()
{
  std::cout << "start to run the taskWorker" << std::endl;
  if (!initWorker())
  {
    // init worker error, failed to run the task queue
    pthread_exit(NULL);
  }
  else
  {
    // start the event loop, and wait for loop exit
    event_base_dispatch(base);
  }
  return NULL;
}

void TaskWorker::sendToMaster(Task *response)
{
  master->enqueueResponse(response);
  master->notify();
}

void TaskWorker::doTask(Task *task)
{
  for (int i = 0; i < task->totalCount; i ++)
  {
    Ping* ping = task->pingArray + i;
    ping->worker = this;
    ping->start();
  }
}

void TaskWorker::onComplete(Task *task)
{
  doingCount --;
  sendToMaster(task);
}