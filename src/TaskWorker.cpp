#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <errno.h>
#include <inttypes.h>

#include "TaskWorker.h"
#include "Task.h"
#include "HttpServer.h"
#include "Ping.h"
#include "Common.h"
#include "Log.h"

using common::Common;

extern Logger logger;
extern int errno;

void notifyEventHandler(int fd, short event, void *arg)
{
  TaskWorker *me = (TaskWorker *)arg;
  char tmpBuf[2];
  ssize_t readed = read(me->recvFd, tmpBuf, 1);
  if (readed <= 0)
  {
    logger.log(ERROR_LOG, "task worker read pipe error, try pop one from queue");
  }
  Task *task;
  if (!me->taskQueue.popTail(task))
  {
    logger.log(ERROR_LOG, "task worker notified, but nothing to do, return");
    return;
  }
  if (me->doingCount >= DOING_TASK_CAPACITY)
  {
    logger.log(ERROR_LOG, "too many doing tasks, drop this one");
    task->prepareError = true;
    task->preErrorCode = 1005;
    me->sendToMaster(task);
    return;
  }
  uint64_t now = Common::currentTime();
  task->setTimestamp(now);
  me->doingMap.insert(pair<uint64_t, Task *>(now, task));
  me->doingCount++;
  me->doTask(task);
}

// process socket event
void socketReadHandler(int fd, short event, void *arg)
{
  TaskWorker *worker = (TaskWorker *)arg;

  unsigned char recvPacket[MAX_DATA_SIZE];
  struct ip *ipPacket = (struct ip *)recvPacket;

  socklen_t slen = sizeof(struct sockaddr);
  struct sockaddr_in remote;
  int nrecv = recvfrom(worker->rawSocketFd, recvPacket, sizeof(recvPacket), MSG_DONTWAIT, (struct sockaddr *)&remote, &slen);
  if (nrecv == -1)
  {
    logger.log(ERROR_LOG, "recv from raw socket error, error #%d", errno);
    return;
  }
  // IHL: 4 bits Internet Header Length is the length of the internet header in 32 bit words,
  // and thus points to the beginning of the data. Note that the minimum value for a correct
  // header is 5.
  int hlen = ipPacket->ip_hl * 4;
  if (hlen < 20 || nrecv < hlen + ICMP_MINLEN + EXTRA_PROTO_SIZE)
  {
    logger.log(ERROR_LOG, "recv packet too short, drop it");
    return;
  }
  // check thread id
  unsigned short recvThreadId = *(unsigned short *)(recvPacket + hlen + ICMP_MINLEN);
  if (recvThreadId != worker->threadId)
  {
    logger.log(DEBUG_LOG, "recv packet not mine, my tid #%d, packet tid #%d", worker->threadId, recvThreadId);
    return;
  }
  // task timestamp
  uint64_t taskTimestamp = *(uint64_t *)(recvPacket + hlen + ICMP_MINLEN + 2);
  // Ping index in one task
  unsigned short pingIndex = *(unsigned short *)(recvPacket + hlen + ICMP_MINLEN + 10);
  // get pid and sequence
  struct icmphdr *icmpHeader = (struct icmphdr *)(recvPacket + hlen);
  unsigned short id = icmpHeader->un.echo.id;
  unsigned short seq = icmpHeader->un.echo.sequence;
  if (icmpHeader->type != ICMP_ECHOREPLY)
  {
    logger.log(WARN_LOG, "recv icmp packet, not icmp reply");
    return;
  }

  if (id != worker->getShortPid())
  {
    logger.log(WARN_LOG, "recv icmp packet, but not mine, recvd id: %d", id);
    return;
  }
  char charIpBuf[20] = {0};
  if (inet_ntop(AF_INET, &(remote.sin_addr), charIpBuf, 20) == NULL)
  {
    logger.log(ERROR_LOG, "recv remote addr convert to ip string, error");
    return;
  }
  worker->onReceive(taskTimestamp, seq, pingIndex);
  std::string ipStr = charIpBuf;
  std::cout << "ping response ip: " << ipStr << std::endl;
  std::cout << "ping response id: " << id << std::endl;
  std::cout << "ping response seq: " << seq << std::endl;
  std::cout << "ping response timestamp: " << taskTimestamp << std::endl;
  std::cout << "ping response index: " << pingIndex << std::endl;
  std::cout << "ping response thread id: " << recvThreadId << std::endl;
  return;
}

TaskWorker::TaskWorker()
{
  base = NULL;
  notifyEvent = NULL;
  master = NULL;
  doingCount = 0;
  pid = getpid();
}

TaskWorker::~TaskWorker()
{
  // close the pipes inited
  if (notifyEvent)
  {
    delete notifyEvent;
    notifyEvent = NULL;
  }
  if (base)
  {
    event_base_free(base);
    base = NULL;
  }
}

void TaskWorker::notify()
{
  // just a simple notify
  ssize_t written = write(notifyFd, "c", 1);
  if (written <= 0)
  {
    // write error, log
  }
}

bool TaskWorker::initWorker()
{
  if (!initPipe())
  {
    logger.log(ERROR_LOG, "create pipe error, exit program");
    return false;
  }
  base = event_base_new();
  if (!base)
  {
    // init event base error, log it
    return false;
  }
  if (!initRawSocket())
  {
    logger.log(ERROR_LOG, "create raw socket error, error #%d, exit program", errno);
    return false;
  }
  notifyEvent = new event;
  event_assign(notifyEvent, base, recvFd, EV_READ | EV_PERSIST, notifyEventHandler, (void *)this);
  event_add(notifyEvent, NULL);
  return true;
}

bool TaskWorker::initPipe()
{
  int fds[2];
  if (pipe(fds))
  {
    // create pipe error, log
    return false;
  }
  recvFd = fds[0];
  notifyFd = fds[1];
  return true;
}

bool TaskWorker::closePipe()
{
  close(recvFd);
  close(notifyFd);
  return true;
}

void *TaskWorker::doRun()
{
  logger.log(INFO_LOG, "start to run the task worker #%d", threadId);
  if (!initWorker())
  {
    // init worker error, failed to run the task queue
    pthread_exit(NULL);
  }
  else
  {
    // start the event loop, and wait for loop exit
    event_base_dispatch(base);
  }
  return NULL;
}

void TaskWorker::sendToMaster(Task *response)
{
  master->enqueueResponse(response);
  master->notify();
}

void TaskWorker::doTask(Task *task)
{
  for (int i = 0; i < task->totalCount; i++)
  {
    Ping *ping = task->pingArray + i;
    ping->worker = this;
    ping->start();
  }
}

void TaskWorker::onComplete(Task *task)
{
  doingCount--;
  int64_t timestamp = task->timestamp;
  map<uint64_t, Task *>::iterator iter = doingMap.find(timestamp);
  if (iter == doingMap.end())
  {
    logger.log(ERROR_LOG, "no such task in doing map when complete, impossible");
    sendToMaster(task); // maybe we can still track the http request
  }
  else
  {
    doingMap.erase(iter);
    sendToMaster(task);
  }
}

bool TaskWorker::initRawSocket()
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
    logger.log(ERROR_LOG, "create raw socket error, error #%d", errno);
    return false;
  }
  // set sendbuf and recvbuf, essential for nonblocking socket to avoid ENOBUFS(105 No buffer space available)
  int optval = 0;
  socklen_t socklen = sizeof(struct sockaddr);
  if (getsockopt(rawSocketFd, SOL_SOCKET, SO_RCVBUF, &optval, &socklen))
  {
    logger.log(ERROR_LOG, "get raw socket RX original buffer error");
    return false;
  }
  logger.log(INFO_LOG, "raw socket RX original buffer size: %d", optval);

  optval = 0;
  socklen = sizeof(struct sockaddr);
  if (getsockopt(rawSocketFd, SOL_SOCKET, SO_SNDBUF, &optval, &socklen))
  {
    logger.log(ERROR_LOG, "get raw socket TX original buffer error");
    return false;
  }
  logger.log(INFO_LOG, "raw socket TX original buffer size: %d", optval);

  int bufsize = 2048 * 1024;
  if (setsockopt(rawSocketFd, SOL_SOCKET, SO_RCVBUFFORCE, &bufsize, sizeof(bufsize)))
  {
    logger.log(ERROR_LOG, "set socket read buf size error");
    return false;
  }
  if (setsockopt(rawSocketFd, SOL_SOCKET, SO_SNDBUFFORCE, &bufsize, sizeof(bufsize)))
  {
    logger.log(ERROR_LOG, "set socket send buf error");
    return false;
  }

  optval = 0;
  socklen = sizeof(struct sockaddr);
  if (getsockopt(rawSocketFd, SOL_SOCKET, SO_RCVBUF, &optval, &socklen))
  {
    logger.log(ERROR_LOG, "get raw socket RX new buffer error");
    return false;
  }
  logger.log(INFO_LOG, "raw socket RX new buffer size: %d", optval);

  optval = 0;
  socklen = sizeof(struct sockaddr);
  if (getsockopt(rawSocketFd, SOL_SOCKET, SO_SNDBUF, &optval, &socklen))
  {
    logger.log(ERROR_LOG, "get raw socket TX new buffer error");
    return false;
  }
  logger.log(INFO_LOG, "raw socket TX new buffer size: %d", optval);

  // end of set net buf
  evutil_make_socket_nonblocking(rawSocketFd);
  logger.log(INFO_LOG, "create raw socket for thread (%d) success", threadId);

  // register fd read callback
  sockReadEvent = new event;
  event_assign(sockReadEvent, base, rawSocketFd, EV_READ | EV_PERSIST, socketReadHandler, (void *)this);
  event_add(sockReadEvent, NULL);
  return true;
}

void TaskWorker::onReceive(uint64_t timestamp, unsigned short seq, unsigned short index)
{
  map<uint64_t, Task *>::iterator iter = doingMap.find(timestamp);
  if (iter == doingMap.end())
  {
    logger.log(ERROR_LOG, "task worker recvd but cannot find this task");
    return;
  }
  Task *task = iter->second;
  if (index > task->totalCount)
  {
    logger.log(ERROR_LOG, "index overflow, no such index: %d", index);
    return;
  }
  Ping *p = task->pingArray + index;
  if (p->status != PING_UNINITED && p->status != PING_DOING)
  {
    logger.log(ERROR_LOG, "duplicated ping done, definitely bug there");
    return;
  }
  task->doneCount ++;
  p->status = PING_SUCCESS;
  p->recvtime = Common::currentTime();
  // delete the corresponding timeout event
  event_del(p->timeoutEvent);
  if (task->doneCount >= task->totalCount)
  {
    onComplete(task);
  }
}