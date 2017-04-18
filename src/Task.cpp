#include "Task.h"
#include "Ping.h"
#include "Log.h"
#include "HttpResponse.h"

#include <sstream>

using std::ostringstream;
extern Logger logger;

// already knows that the task is error
Task::Task(int errorCode)
{
  prepareError = true;
  preErrorCode = errorCode;
  totalCount = 0;
  doneCount = 0;
  pingArray = NULL;
}

// task parse from json
Task::Task(JsonValue &jv)
{
  pingArray = NULL;
  prepareError = false;
  preErrorCode = 0;
  totalCount = 0;
  doneCount = 0;
  Json::Value& body = jv.getValue();
  if (!body.isObject())
  {
    prepareError = true;
    preErrorCode = 1001;
    logger.log(ERROR_LOG, "parse task body error, body is not an object");
    return;
  }
  int timeout = body.get("timeout", 5).asInt();
  if (timeout <= 0)
  {
    prepareError = true;
    preErrorCode = 1002;
    logger.log(ERROR_LOG, "parse task body error, timeout invalid, (%d)", timeout);
    return;
  }
  if (body["servers"].isNull() || !body["servers"].isArray())
  {
    prepareError = true;
    preErrorCode = 1003;
    logger.log(ERROR_LOG, "parse task body error, servers invalid, null or not array");
    return;
  }
  int size = body["servers"].size();
  if (size > MAX_PING_PER_TASK)
  {
    prepareError = true;
    preErrorCode = 1004;
    logger.log(ERROR_LOG, "parse task body error, servers overflow, too much servers in one request");
    return;
  }
  totalCount = size;
  doneCount = 0;

  pingArray = new Ping[size];
  for (unsigned short i = 0; i < size; i ++)
  {
    pingArray[i].index = i;
    pingArray[i].ip = body["servers"][i].asString();
    pingArray[i].timeout = timeout;
    pingArray[i].status = PING_UNINITED;
    pingArray[i].round = 0;
    pingArray[i].parentTask = this;
    pingArray[i].pingEvent = NULL;
  }
}

Task::~Task()
{
  if (pingArray)
  {
    delete [] pingArray;
    pingArray = NULL;
  }
}

string Task::buildResponse()
{
  JsonValue jv;
  Json::Value &body = jv.getValue()[(unsigned int)0];

  if (prepareError)
  {
    body["success"] = false;
    string errorMsg;  // process error
    switch (preErrorCode)
    {
      case 1001:
        errorMsg = "payload invalid, json body is not an object";
        break;
      case 1002:
        errorMsg = "payload invalid, timeout invalid";
        break;
      case 1003:
        errorMsg = "payload invalid, servers invalid, null or not array";
        break;
      case 1004:
        errorMsg = "payload invalid, servers overflow, too much servers in one request (capacity: 65536)";
        break;
      case 1005:
        errorMsg = "too many tasks working, drop this task";
        break;
      case 1006:
        errorMsg = "init task error, casuse varies, please check log";
        break;
      default:
        errorMsg = "unknown error";
        break;
    }
    body["errMsg"] = errorMsg;
  }
  else
  {
    body["success"] = true;
    body["errMsg"] = "";
    for (int i = 0; i < totalCount; i++)
    {  
      body["results"][i]["ip"] = pingArray[i].ip;
      body["results"][i]["lossRate"] = 1.0;
      body["results"][i]["avgRtt"] = 5.0 + i;
    }
  }
  ostringstream sout;
  sout << body;
  return sout.str();
}

void Task::fillHttpResponse(HttpResponse *response)
{
  if (preErrorCode > 0 && preErrorCode < 1000)
  {
    response->fill(preErrorCode); // http error
  }
  else
  {
    response->fill(buildResponse());
  }
}

bool Task::initTask(struct event_base * base)
{
  // build raw socket
  struct protoent *proto = NULL;
  if (!(proto = getprotobyname("icmp")))
  {
    logger.log(ERROR_LOG, "proto icmp not supported on this host, drop ping task.");
    return false;
  }
  if ((rawSocketFd = socket(AF_INET, SOCK_RAW, proto->p_proto)) == -1)
  {
    logger.log(ERROR_LOG, "create raw socket error, drop ping task");
    return false;
  }
  evutil_make_socket_nonblocking(rawSocketFd);

  // register fd read callback
  sockReadEvent = new event;
  event_assign(sockReadEvent, base, rawSocketFd, EV_READ | EV_PERSIST, Task::handleSocketRead, (void *)this);
  event_add(sockReadEvent, NULL);
  return true;
}

void Task::handleSocketRead(int fd, short event, void *arg)
{
  Task *me = (Task *)arg;

  unsigned char recvPacket[MAX_DATA_SIZE];
  struct ip *ipPacket = (struct ip *)recvPacket;
  
  socklen_t slen = sizeof(struct sockaddr);
  struct sockaddr_in remote;
  int nrecv = recvfrom(me->rawSocketFd, recvPacket, sizeof(recvPacket), MSG_DONTWAIT,  (struct sockaddr *)&remote, &slen);
  if (nrecv == -1)
  {
    logger.log(ERROR_LOG, "recv from raw socket error");
    return;
  }
  // IHL: 4 bits Internet Header Length is the length of the internet header in 32 bit words,
  // and thus points to the beginning of the data. Note that the minimum value for a correct
  // header is 5.
  int hlen = ipPacket->ip_hl * 4;
  if (hlen < 20 || nrecv < hlen + ICMP_MINLEN)
  {
    logger.log(ERROR_LOG, "recv packet too short, drop it");
    return;
  }
  struct icmphdr *icmpHeader = (struct icmphdr *)(recvPacket + hlen);
  unsigned short id  = icmpHeader->un.echo.id;
  unsigned short seq = icmpHeader->un.echo.sequence;
  char charIpBuf[20] = {0};
  if (inet_ntop(AF_INET, &(remote.sin_addr), charIpBuf, 20) == NULL)
  {
    logger.log(ERROR_LOG, "recv remote addr convert to ip string, error");
    return;
  }
  std::string ipStr = charIpBuf;
  std::cout << ipStr << std::endl;
  std::cout << id << std::endl;
  std::cout << seq << std::endl;
}

