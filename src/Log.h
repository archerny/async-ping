#ifndef _LOG_H_
#define _LOG_H_

#include <string>
#include <iostream>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "ThreadWorker.h"
#include "CommonList.h"

using std::string;
using common::ThreadWorker;
using common::ConcurrentDequeue;

enum LogLevel
{
  LEVEL_ERROR = 10,
  LEVEL_WARN = 20,
  LEVEL_INFO = 30,
  LEVEL_DEBUG = 40
};

#define DEFAULT_SIZE (256 * 1024 * 1024)
#define LOG_LINE_BUF_SIZE 8193

#define INFO_LOG LEVEL_INFO, __FILE__, __LINE__
#define WARN_LOG LEVEL_WARN, __FILE__, __LINE__
#define ERROR_LOG LEVEL_ERROR, __FILE__, __LINE__
#define DEBUG_LOG LEVEL_DEBUG, __FILE__, __LINE__

#define LOG Logger::doLog

void logEventHandler(int fd, short event, void *arg);

class LogEventBody
{
public:
  char *content;
  LogLevel level;
  int line;
  time_t now;

  LogEventBody();
  ~LogEventBody();
  const char *getLevelString(const LogLevel &level);
  void dumpToFile(FILE *fp);
};

class Logger : public ThreadWorker
{
public:
  Logger();
  ~Logger();
  bool initLogger(const string& fileName);
  virtual void *doRun();
  // send log to log queue
  void log(LogLevel level, const char *srcFileName, int line, const char *fmt, ...);
  // recv log
  friend void logEventHandler(int fd, short event, void *arg);
  void notify();
  
private:
  const char *getLevelString(const LogLevel &level);
  void rotateLog() {};
  void setLogSize() {};
  void setLogLevel() {};

  bool initPipe();
  bool closePipe();

private:
  FILE *logFileFp;
  string logFileName;
  size_t logFileSize;
  LogLevel logLevel;
  int recvFd;
  int notifyFd;
  ConcurrentDequeue<LogEventBody *> logQueue;

private:
  struct event_base *base;
  struct event *logEvent;
};

#endif