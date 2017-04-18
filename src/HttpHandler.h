#ifndef _HTTP_HANDLER_H_
#define _HTTP_HANDLER_H_

#include <event2/http.h>

#include "JsonValue.h"

using common::JsonValue;

#define RECV_BUF_SIZE 4097

class HttpHandler
{
public:
  static void handlePingReq(struct evhttp_request *req, void *arg);

  static void handleSshReq(struct evhttp_request *req, void *arg);

  static void handleHttpReq(struct evhttp_request *req, void *arg);

private:
  // util functions for handlers
  static bool recvReqInput(struct evhttp_request *req, string& str);

  static bool recvReqInput(struct evhttp_request *req, JsonValue &jv);
};

#endif