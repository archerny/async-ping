#include "HttpHandler.h"
#include "HttpServer.h"
#include "HttpResponse.h"
#include "Task.h"
#include "Log.h"

extern Logger logger;

void HttpHandler::handlePingReq(struct evhttp_request *req, void *arg)
{
  HttpServer *server = (HttpServer *)arg;
  Task *task;
  JsonValue jv;
  // checkMethod
  // If the HTTP method used is not acceptable, then according to the specification, you should return a 405 Method Not Allowed.
  // For example, Allow: POST, or if there are multiple choices, Allow: POST, PUT
  int cmdType = evhttp_request_get_command(req);
  if (cmdType != EVHTTP_REQ_POST)
  {
    logger.log(ERROR_LOG, "invalid request, method not supported, drop");
    task = new Task(405);
    goto finally;
  }
  // get request data
  if (!recvReqInput(req, jv))
  {
    logger.log(ERROR_LOG, "invalid request, payload data format not json, drop");
    task = new Task(415);
    goto finally;
  }
  task = new Task(jv);

finally:
  map<Task *, struct evhttp_request *>::iterator it = server->doingRequest.find(task);
  if  (it != server->doingRequest.end())
  {
    logger.log(ERROR_LOG, "what the fuck, impossible");
    delete task;
    return;
  }
  server->doingRequest.insert(pair<Task *, struct evhttp_request *>(task, req));

  if (task->prepareError)
  {
    // no need to execute this, return direct
    server->enqueueResponse(task);
    server->notify();
  }
  else
  {
    // valid request task. go! bite it
    server->sendToWorker(task);
  }
}

void HttpHandler::handleSshReq(struct evhttp_request *req, void *arg)
{
  return;
}

void HttpHandler::handleHttpReq(struct evhttp_request *req, void *arg)
{
  return;
}

void HttpHandler::hadnleSnmpReq(struct evhttp_request *req, void *arg)
{
  return;
}

bool HttpHandler::recvReqInput(struct evhttp_request *req, string &retData)
{
  struct evbuffer *buf = evhttp_request_get_input_buffer(req);
  char recvbuf[RECV_BUF_SIZE];
  retData.clear();
  while (evbuffer_get_length(buf))
  {
    int nrecvd = evbuffer_remove(buf, recvbuf, RECV_BUF_SIZE - 1);
    if (nrecvd > 0)
    {
      recvbuf[nrecvd] = '\0';
      retData += recvbuf;
    }
  }
  return true;
}

bool HttpHandler::recvReqInput(struct evhttp_request *req, JsonValue &jv)
{
  struct evbuffer *buf = evhttp_request_get_input_buffer(req);
  char recvbuf[RECV_BUF_SIZE];
  string retData;
  while (evbuffer_get_length(buf))
  {
    int nrecvd = evbuffer_remove(buf, recvbuf, RECV_BUF_SIZE - 1);
    if (nrecvd > 0)
    {
      recvbuf[nrecvd] = '\0';
      retData += recvbuf;
    }
  }
  if (jv.parse(retData))
  {
    return true;
  }
  else
  {
    return false;
  }
}