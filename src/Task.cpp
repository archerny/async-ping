#include "Task.h"
#include "Ping.h"
#include "Log.h"
#include "HttpResponse.h"

#include <netdb.h>  // needed when get protocal
#include <sys/types.h>
#include <sys/socket.h>
#include <sstream>

using std::ostringstream;
extern Logger logger;

Task::Task(struct evhttp_request *srcReq, JsonValue &jv)
{
  pingArray = NULL;
  prepareError = false;
  request = srcReq;
  Json::Value& body = jv.getValue();
  if (!body.isObject())
  {
    prepareError = true;
    logger.log(ERROR_LOG, "parse task body error, body is not an object");
    return;
  }
  int timeout = body.get("timeout", 5).asInt();
  if (timeout <= 0)
  {
    prepareError = true;
    logger.log(ERROR_LOG, "parse task body error, timeout invalid, (%d)", timeout);
    return;
  }
  if (body["servers"].isNull() || !body["servers"].isArray())
  {
    prepareError = true;
    logger.log(ERROR_LOG, "parse task body error, servers invalid, null or not array");
    return;
  }
  int size = body["servers"].size();
  if (size > MAX_PING_PER_TASK)
  {
    prepareError = true;
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

HttpResponse *Task::buildResponse()
{
  JsonValue jv;
  Json::Value &body = jv.getValue()[(unsigned int)0];

  if (prepareError)
  {
    body["success"] = false;
    body["errMsg"] = "holy fucking bull shit";
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
  HttpResponse *resp = new HttpResponse(request, sout.str());
  return resp;
}

bool Task::initTask()
{
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
  return true;
}
