#include "Task.h"
#include "Ping.h"
#include "Log.h"
#include "HttpResponse.h"

#include <sstream>
#include <errno.h>

using std::ostringstream;
extern Logger logger;
extern int errno;

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
    pingArray[i].parentTask = this;
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
      Ping *p = pingArray + i;
      body["results"][i]["ip"] = p->ip;
      double lossRate = p->status == PING_SUCCESS ? 0 : 1.0;
      body["results"][i]["lossRate"] = lossRate;
      double cost = (double)(p->recvtime - p->sendtime) / 1000.0;
      if (lossRate == 0)
      {
        body["results"][i]["avgRtt"] = cost;
      }
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