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
#include "Log.h"
#include "IcmpPacket.h"

extern Logger logger;

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
    me->worker->onComplete(me->parentTask);
    cout << me->parentTask->buildResponse() << endl;
  }
  return;
}

Ping::Ping()
{
  ip.clear();
  worker = NULL;
  parentTask = NULL;
  pingEvent = NULL;
  timeoutEvent = NULL;
  tv = NULL;
}

Ping::~Ping()
{
  if (pingEvent)
  {
    delete pingEvent;
    pingEvent = NULL;
  }
  if (timeoutEvent)
  {
    delete timeoutEvent;
    timeoutEvent = NULL;
  }
  if (tv)
  {
    delete tv;
    tv = NULL;
  }
}

void Ping::start()
{
  // build icmp packet
  unsigned char type = 8;
  unsigned char code = 0;
  unsigned short id = worker->getShortPid() ;// pid
  unsigned short seq = 0;
  const string content = "Hello from xping.";

  IcmpPacket packet = IcmpPacket(type, code, id, seq, content);
  size_t size = packet.getPktSize();
  unsigned char *buf = packet.getBuf();

  // send packet
  struct sockaddr_in sa;
  inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr));
  sa.sin_family = AF_INET;
  ssize_t nsent = sendto(parentTask->rawSocketFd, buf, size, MSG_DONTWAIT, (struct sockaddr *) &sa, sizeof(struct sockaddr_in));
  if (nsent == -1)
  {
    logger.log(ERROR_LOG, "send icmp packet error, target: %s", ip.c_str());
    status = PING_SEND_ERROR;
    return;
  }

  // timeout timer
  setTimer();
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