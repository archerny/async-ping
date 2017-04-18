#ifndef _TASK_H_
#define _TASK_H_

#include <string>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>

#include "JsonValue.h"
#include "HttpResponse.h"

using std::string;
using common::JsonValue;

#define IPHDR 20
#define MAX_DATA_SIZE (IP_MAXPACKET - IPHDR - ICMP_MINLEN)
#define MAX_PING_PER_TASK 65536   //reach max of unsigned short

class Ping;

class Task
{
public:
  Task(JsonValue &jv);
  Task(int errorCode);
  ~Task();

public:
  static void handleSocketRead(int fd, short event, void *arg);
  string buildResponse();
  void fillHttpResponse(HttpResponse *resp);
  bool initTask(struct event_base *base);
  inline bool isComplete() { return doneCount >= totalCount; }

  bool prepareError;
  int preErrorCode;

  int totalCount;
  int doneCount;
  Ping *pingArray;
  evutil_socket_t rawSocketFd;
  struct event *sockReadEvent;
};

#endif