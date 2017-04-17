#include "HttpHandler.h"
#include "HttpUtil.h"
#include "HttpServer.h"
#include "HttpResponse.h"
#include "Task.h"
#include <iostream>

void HttpHandler::handlePingReq(struct evhttp_request *req, void *arg)
{
  HttpServer *server = (HttpServer *)arg;
  // checkMethod
  // If the HTTP method used is not acceptable, then according to the specification, you should return a 405 Method Not Allowed.
  // For example, Allow: POST, or if there are multiple choices, Allow: POST, PUT
  int cmdType = evhttp_request_get_command(req);
  if (cmdType != EVHTTP_REQ_POST)
  {
    // log the error
    HttpResponse *resp = new HttpResponse(req, 405);
    server->enqueueResponse(resp);
    server->notify();
    return;
  }
  // get request data
  JsonValue jv;
  if (!recvReqInput(req, jv))
  {
    // log the error, data format not supported
    // maybe I should return 415(Unsupported Media Type)
    HttpResponse *resp = new HttpResponse(req, 415);
    server->enqueueResponse(resp);
    server->notify();
    return;
  }
  Task *task = new Task(req, jv);
  server->sendToWorker(task);
  std::cout << "test ping handler processed" << std::endl;
  return;
}

void HttpHandler::handleSshReq(struct evhttp_request *req, void *arg)
{
  return;
}

void HttpHandler::handleHttpReq(struct evhttp_request *req, void *arg)
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