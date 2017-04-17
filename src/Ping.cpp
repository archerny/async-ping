#include "Ping.h"
#include <arpa/inet.h>  // header file needed when translate ip to inet address
#include <sys/socket.h> // header file needed when sendto

#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>

#include <iostream>
using std::cout;
using std::endl;

#include "Task.h"
#include "TaskWorker.h"

void Ping::handleReceive()
{
  return;
}

void Ping::handleRequest()
{
  return;
}

void Ping::handleTimeout(int fd, short event, void *arg)
{
  Ping *me = (Ping *)arg;
  me->status = PING_TIMEOUT;
  me->parentTask->doneCount ++;
  // delete event that waiting for ping response
  me->cancelEvent();
  if (me->parentTask->isComplete())
  {
    // HttpResponse *response = me->parentTask->buildResponse();
    me->worker->sendToMaster(new HttpResponse(me->parentTask->request, 415));
    delete me->parentTask;
  }
  return;
}

void Ping::start()
{
  setTimer();
  cout << "start ping for " << ip << endl;
  return;
}

bool Ping::setTimer()
{
  timeoutEvent = new event;
  tv = new timeval;

  // init event, flag set 0, becacuse this timer will just be used once
  event_assign(timeoutEvent, worker->getEvBase(), -1, 0, Ping::handleTimeout, (void *) this);
  evutil_timerclear(tv);
  tv->tv_sec = timeout;
  event_add(timeoutEvent, tv);
  return true;
}

bool Ping::cancelTimer()
{
  return true;
}

bool Ping::cancelEvent()
{
  return true;
}