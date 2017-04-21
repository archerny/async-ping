#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>
#include <string>
#include <stdlib.h>
#include <unistd.h>

#include "Common.h"
#include "HttpServer.h"
#include "TaskWorker.h"
#include "Log.h"

using common::ThreadWorker;
using common::Common;

extern Logger logger;

// general callback returns 404
static void generalHttpCallback(struct evhttp_request *req, void *arg)
{
  evhttp_send_error(req, 404, "Not Found");
}

static void timeoutResponse(evutil_socket_t fd, short event, void *arg)
{
  //
}

void tryRespEventHandler(int fd, short event, void *arg)
{
  HttpServer *me = (HttpServer *)arg;
  char tmpBuf[2];
  ssize_t readed = read(me->recvFd, tmpBuf, 1);
  if (readed <= 0)
  {
    logger.log(WARN_LOG, "notified but nothing to read");
    // do not return. maybe some task is in the queue
  }
  Task *taskResp = NULL;
  if (me->responseQueue.popTail(taskResp))
  {
    struct evhttp_request *req;
    map<Task *, struct evhttp_request *>::iterator it = me->doingRequest.find(taskResp);
    if (it == me->doingRequest.end())
    {
      logger.log(ERROR_LOG, "index the evhttp_request by task pointer error, impossible");
      delete taskResp;  // in case of mem leak
      return;
    }
    req = it->second;
    me->doingRequest.erase(it);

    HttpResponse response = HttpResponse(req);
    taskResp->fillHttpResponse(&response);
    response.doResponse();
    delete taskResp;
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
    logger.log(ERROR_LOG, "init msg queue pipe error");
    return false;
  }
  base = event_base_new();
  if (!base)
  {
    logger.log(ERROR_LOG, "new event base error in http server thread");
    return false;
  }
  http = evhttp_new(base);
  if (!http)
  {
    logger.log(ERROR_LOG, "new http struct error");
    return false;
  }
  // register handler
  doRegister();
  evhttp_set_gencb(http, generalHttpCallback, NULL);
  handle = evhttp_bind_socket_with_handle(http, "0.0.0.0", bindPort);
  if (!handle)
  {
    logger.log(ERROR_LOG, "bind to local addr (0.0.0.0 : %d) error, http server will not serve", bindPort);
    return false;
  }
  
  logger.log(INFO_LOG, "init http server success, bind addr (0.0.0.0 : %d)", bindPort);
  setTimerTask();
  setNotifyEvent();
  return true;
}

void *HttpServer::doRun()
{
  if (!initServer())
  {
    logger.log(ERROR_LOG, "init http server error, exits thread");
    pthread_exit(NULL);
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
    logger.log(ERROR_LOG, "duplicated reister, url: %s", path.c_str());
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
  tv->tv_sec = 60;
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
    logger.log(ERROR_LOG, "create pipe error");
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
  ssize_t written = write(notifyFd, "c", 1);
  if (written <= 0) {
    logger.log(ERROR_LOG, "write to notify pipe error");
  }
}

bool HttpServer::sendToWorker(Task *task)
{
  int size = workers.size();
  static int randedIndex = Common::getRandIntvalue(0, size);
  TaskWorker *worker = workers[randedIndex];
  worker->enqueueTask(task);
  worker->notify();
  randedIndex = (randedIndex + 1) % WORKER_NUM; // dispatch strategy
  return true;
}