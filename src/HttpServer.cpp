#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <unistd.h>

#include "Common.h"
#include "HttpServer.h"
#include "HttpUtil.h"
#include "TaskWorker.h"

using common::ThreadWorker;
using common::Common;

// general callback
static void generalHttpCallback(struct evhttp_request *req, void *arg)
{
  evhttp_send_error(req, 404, "Not Found");
}

static void timeoutResponse(evutil_socket_t fd, short event, void *arg)
{
  // ConcurrentDequeue<HttpResponse *> *responseQueue = (ConcurrentDequeue<HttpResponse *> *)arg;
  // HttpResponse *response = NULL;
  // if (responseQueue->popTail(response))
  // {
  //   response->doResponse();
  //   delete response;
  // }
}

void tryRespEventHandler(int fd, short event, void *arg)
{
  HttpServer *me = (HttpServer *)arg;
  char tmpBuf[2];
  ssize_t readed = read(me->recvFd, tmpBuf, 1);
  if (readed <= 0)
  {
    // read error, nothing to read, log
  }
  HttpResponse *response = NULL;
  if (me->responseQueue.popTail(response))
  {
    response->doResponse();
    delete response;
  }
}

HttpServer::HttpServer(unsigned short port)
{
  base = NULL;
  http = NULL;
  handle = NULL;
  timeoutEvent = NULL;
  tryRespEvent = NULL;
  tv = NULL;
  bindPort = port;
  exitStatus = 0;
}

HttpServer::~HttpServer()
{
  if (timeoutEvent) {
    delete timeoutEvent;
    timeoutEvent = NULL;
  }
  if (tryRespEvent)
  {
    delete tryRespEvent;
    tryRespEvent = NULL;
  }
  if (tv) {
    delete tv;
    tv = NULL;
  }
  if (base)
  {
    event_base_free(base);
    base = NULL;
  }
}

bool HttpServer::initServer()
{
  if (!initPipe())
  {
    return false;
  }
  base = event_base_new();
  if (!base)
  {
    // init event base error, log it
    return false;
  }
  http = evhttp_new(base);
  if (!http)
  {
    // init http struct error, log it
    return false;
  }
  // start to register handlers
  // evhttp_set_cb(http, "/dump", dump_request_cb, NULL);
  doRegister();
  evhttp_set_gencb(http, generalHttpCallback, NULL);
  handle = evhttp_bind_socket_with_handle(http, "0.0.0.0", bindPort);
  if (!handle)
  {
    // bind listening socket error, log it
    return false;
  }
  // log, init server success;
  setTimerTask();
  setNotifyEvent();
  return true;
}

void *HttpServer::doRun()
{
  std::cout << "start do run, " << bindPort << std::endl;
  if (!initServer())
  {
    // init server error, failed to start the http server
    exitStatus = 1;
    pthread_exit((void *)&exitStatus);
  }
  else
  {
    event_base_dispatch(base);
  }
  return NULL;
}

bool HttpServer::doRegister()
{
  map<string, void *>::iterator itFront, itEnd;
  itFront = handlers.begin();
  itEnd = handlers.end();
  while(itFront != itEnd)
  {
    string path = itFront->first;
    void (*reqhandler)(struct evhttp_request *, void *)  = (void (*)(struct evhttp_request *, void *))itFront->second;
    evhttp_set_cb(http, path.c_str(), reqhandler, (void *)this);
    itFront ++;
  }
  return true;
}

bool HttpServer::registerHandler(const string& path, void (*handler)(struct evhttp_request *, void *))
{
  map<string, void *>::iterator it = handlers.find(path);
  if (it != handlers.end()) {
    // already registered, reject and log
    return false;
  }
  handlers.insert(pair<string, void*>(path, (void *)handler));
  return true;
}

bool HttpServer::setTimerTask()
{
  timeoutEvent = new event;
  tv = new timeval;
  int flags = EV_PERSIST;
  
  // init event
  event_assign(timeoutEvent, base, -1, flags, timeoutResponse, (void *) &responseQueue);
  evutil_timerclear(tv);
  tv->tv_sec = 60;  // every 60 second do this work
  // tv->tv_usec = 20;  // 20 usec loop consumes 3% cpu of single core on pc (CORE i5)
  event_add(timeoutEvent, tv);
  return true;
}

bool HttpServer::setNotifyEvent()
{
  tryRespEvent = new event;
  event_assign(tryRespEvent, base, recvFd, EV_READ | EV_PERSIST, tryRespEventHandler, (void *)this);
  event_add(tryRespEvent, NULL);
  return true;
}

void HttpServer::registerWorker(TaskWorker *worker)
{
  if (worker)
  {
    workers.push_back(worker);
  }
}

bool HttpServer::initPipe()
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

bool HttpServer::closePipe()
{
  close(recvFd);
  close(notifyFd);
  return true;
}

void HttpServer::notify()
{
  // simple notify
  ssize_t written = write(notifyFd, "c", 1);
  if (written <= 0) {
    // write error, log
  }
}

bool HttpServer::sendToWorker(Task *task)
{
  int size = workers.size();
  int randedIndex = Common::getRandIntvalue(0, size);
  TaskWorker *worker = workers[randedIndex];
  worker->enqueueTask(task);
  worker->notify();
  return true;
}