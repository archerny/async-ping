#include "Ping.h"
#include <arpa/inet.h>  // header file needed when translate ip to inet address
#include <sys/socket.h> // header file needed when sendto
#include <errno.h>

#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>

#include "Task.h"
#include "TaskWorker.h"
#include "Log.h"
#include "IcmpPacket.h"
#include "Common.h"

using common::Common;

extern Logger logger;
extern int errno;

void Ping::handleTimeout(int fd, short event, void *arg)
{
  Ping *me = (Ping *)arg;
  if (me->status != PING_UNINITED && me->status != PING_DOING)
  {
    logger.log(ERROR_LOG, "duplicated ping done, definitely bug there");
    return;
  }
  me->status = PING_TIMEOUT;
  me->parentTask->doneCount ++;
  if (me->parentTask->isComplete())
  {
    me->worker->onComplete(me->parentTask);
  }
  return;
}

Ping::Ping()
{
  ip.clear();
  worker = NULL;
  parentTask = NULL;
  timeoutEvent = NULL;
  tv = NULL;
}

Ping::~Ping()
{
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

  evutil_socket_t rawSocketFd = worker->getRawSocket();
  char srcBuf[EXTRA_PROTO_SIZE];  //#define EXTRA_PROTO_SIZE 12
  unsigned short *tid = reinterpret_cast<unsigned short *>(srcBuf);
  *tid = worker->getThreadId();
  uint64_t *timestamp = reinterpret_cast<uint64_t *>(srcBuf + 2);
  *timestamp = parentTask->timestamp;
  unsigned short *idx = reinterpret_cast<unsigned short *>(srcBuf + 10);
  *idx = index;

  IcmpPacket packet = IcmpPacket(type, code, id, seq, EXTRA_PROTO_SIZE, srcBuf);
  size_t size = packet.getPktSize();
  unsigned char *buf = packet.getBuf();

  // send packet
  struct sockaddr_in sa;
  //inet_pton() returns 1 on success (network address was successfully converted).
  int convertRet = inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr));
  if (convertRet != 1)
  {
    logger.log(ERROR_LOG, "convert ip address via inet_pton error, src address: %s, error #%d", ip.c_str(), errno);
    if (status == PING_UNINITED || status == PING_DOING)
    {
      status = PING_SEND_ERROR;
      parentTask->doneCount ++;
      if (parentTask->isComplete())
      {
        worker->onComplete(parentTask);
      }
    }
    else
    {
      logger.log(ERROR_LOG, "duplicated ping done, definitely bug there");
    }
    return;
  }
  sa.sin_family = AF_INET;
  sendtime = Common::currentTime();
  socklen_t socketAddrLen = sizeof(struct sockaddr_in);
  ssize_t nsent = sendto(rawSocketFd, buf, size, MSG_DONTWAIT, (struct sockaddr *) &sa, socketAddrLen);
  if (nsent == -1)
  {
    logger.log(ERROR_LOG, "send icmp packet error, target: %s, error #%d", ip.c_str(), errno);
    if (status == PING_UNINITED || status == PING_DOING)
    {
      status = PING_SEND_ERROR;
      // this ping is done when send error occurs
      parentTask->doneCount ++;
      if (parentTask->isComplete())
      {
        worker->onComplete(parentTask);
      }
    }
    else
    {
      logger.log(ERROR_LOG, "duplicated ping done, definitely bug there");
    }
    return;
  }
  status = PING_DOING;
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
