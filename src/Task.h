#ifndef _TASK_H_
#define _TASK_H_

#include <string>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>

#include "JsonValue.h"
#include "HttpResponse.h"

using std::string;
using common::JsonValue;

#define MAX_PING_PER_TASK 65536   //reach max of unsigned short

class Ping;

class Task
{
public:
  Task(struct evhttp_request *srcReq, JsonValue &jv);
  ~Task();

  HttpResponse *buildResponse();
  bool initTask();
  inline bool isComplete() { return doneCount >= totalCount; }

  int totalCount;
  int doneCount;
  bool prepareError;
  Ping *pingArray;
  struct evhttp_request *request;
  evutil_socket_t rawSocketFd;
};

#endif