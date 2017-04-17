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
    // read error, nothing to read, log
  }
  Task *task;
  if (!me->taskQueue.popTail(task))
  {
    // nothing todo
    return;
  }
  if (me->doingCount >= DOING_TASK_CAPACITY)
  {
    string errorMsg = "too many doing tasks, drop this one";
    logger.log(ERROR_LOG, errorMsg.c_str());
    HttpResponse *resp = new HttpResponse(task->request, 
                  "{success: false, errorMsg: \"" + errorMsg + "\"}");
    me->sendToMaster(resp);
    delete task;
  }
  me->doingCount ++;
  task->initTask();
  me->doTask(task);
}


TaskWorker::TaskWorker()
{
  base = NULL;
  notifyEvent = NULL;
  master = NULL;
  doingCount = 0;
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

void TaskWorker::sendToMaster(HttpResponse *response)
{
  master->enqueueResponse(response);
  master->notify();
}

void TaskWorker::doTask(Task *task)
{
  for (int i = 0; i < task->totalCount; i ++)
  {
    Ping ping = (task->pingArray)[i];
    ping.worker = this;
    ping.start();
  }
}

void TaskWorker::onComplete(Task *task)
{
  doingCount --;
  HttpResponse *response = task->buildResponse();
  sendToMaster(response);
}