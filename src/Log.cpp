#include "Log.h"
#include <cstdio>
#include <cstring>


using namespace common;

void logEventHandler(int fd, short event, void *arg)
{
  Logger *me = (Logger *)arg;
  char tmpBuf[2];
  ssize_t readed = read(me->recvFd, tmpBuf, 1);
  if (readed <= 0)
  {
    // read error, nothing to read, log
  }
  LogEventBody *body;
  if (me->logQueue.popTail(body))
  {
    body->dumpToFile(me->logFileFp);
    delete body;
  }
  // check rotate;
}

LogEventBody::LogEventBody()
{
  content = new char[LOG_LINE_BUF_SIZE];
  memset(content, 0, sizeof(char) * LOG_LINE_BUF_SIZE);
  time(&now);
}

LogEventBody::~LogEventBody()
{
  delete [] content;
}

void LogEventBody::dumpToFile(FILE *fp)
{
  struct tm *tmNow = localtime(&now);
  const char *levelString = getLevelString(level);

  fprintf(fp, "[%s] %04d-%02d-%02d %02d:%02d:%02d (line:%d): %s\n", 
    levelString, tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday, tmNow->tm_hour,
    tmNow->tm_min, tmNow->tm_sec, line, content);
  // flush buffer
  fflush(fp);
}

Logger::Logger()
{
  logFileFp = NULL;
  logFileSize = DEFAULT_SIZE;
  logLevel = LEVEL_INFO;
  logEvent = NULL;
  base = NULL;
}

Logger::~Logger()
{
  if (base)
  {
    event_base_free(base);
    base = NULL;
  }
  if (logEvent)
  {
    delete logEvent;
    logEvent = NULL;
  }
  if (logFileFp)
  {
    fclose(logFileFp);
    logFileFp = NULL;
  }
  // close pipe ?
}

const char *LogEventBody::getLevelString(const LogLevel &level)
{
  switch(level)
  {
    case LEVEL_ERROR:
      return "ERROR";
    case LEVEL_WARN:
      return "WARN";
    case LEVEL_INFO:
      return "INFO";
    case LEVEL_DEBUG:
      return "DEBUG";
    default:
      break;
  }
  return "UNKNOWN";
}

void Logger::log(LogLevel level, const char *srcFileName, int line, const char *fmt, ...)
{
  if (level > this->logLevel)
  {
    return; // ignore;
  }
  // process args
  LogEventBody *body = new LogEventBody();
  body->line = line;
  body->level = level;
  va_list vargs;
  va_start(vargs, fmt);
  vsnprintf(body->content, LOG_LINE_BUF_SIZE - 1, fmt, vargs);
  va_end(vargs);

  // enqueue and notify
  logQueue.pushHead(body);
  this->notify();
}

void Logger::notify()
{
  ssize_t written = write(notifyFd, "l", 1);
  if (written)
  {
    // ???
  }
}

bool Logger::initLogger(const string& fileName)
{
  logFileName = fileName;
  logFileFp = fopen(fileName.c_str(), "a+");
  if (!logFileFp)
  {
    return false;
  }
  if (!initPipe())
  {
    return false;
  }
  base  = event_base_new();
  if (!base)
  {
    return false;
  }
  logEvent = new event;
  event_assign(logEvent, base, recvFd, EV_READ | EV_PERSIST, logEventHandler, (void *)this);
  event_add(logEvent, NULL);
  return true;
}

bool Logger::initPipe()
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

bool Logger::closePipe()
{
  close(recvFd);
  close(notifyFd);
  return true;
}

void *Logger::doRun()
{
  event_base_dispatch(base);
  return NULL;
}