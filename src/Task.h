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
#define MAX_PING_PER_TASK 65536 //reach max of unsigned short
#define EXTRA_PROTO_SIZE 12     //12 bytes, threadId(2), taskTimestamp(8), subIndex(2)

class Ping;

class Task
{
public:
  Task(JsonValue &jv);
  Task(int errorCode);
  ~Task();

public:
  string buildResponse();
  void fillHttpResponse(HttpResponse *resp);
  inline bool isComplete() { return doneCount >= totalCount; }
  inline void setTimestamp(uint64_t t) { this->timestamp = t; }

  bool prepareError;
  int preErrorCode;

  uint64_t timestamp;
  int totalCount;
  // important: assert that doneCount should not be counted more than once, in any case
  int doneCount;
  Ping *pingArray;
};

#endif