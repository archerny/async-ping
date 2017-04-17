#ifndef _HTTP_RESPONSE_H_
#define _HTTP_RESPONSE_H_

#include <string>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>

#include "JsonValue.h"

using common::JsonValue;

// only responses 200
class HttpResponse
{
public:
  HttpResponse(struct evhttp_request *req, const string& content);
  HttpResponse(struct evhttp_request *req, const JsonValue& json);
  HttpResponse(struct evhttp_request *req);
  HttpResponse(struct evhttp_request *req, int code);
  ~HttpResponse();

  void doResponse();
private:
  struct evhttp_request *request;
  struct evbuffer *evb;
  int responseCode;
};

#endif