#ifndef _THREAD_WORKER_H_
#define _THREAD_WORKER_H_

#include <pthread.h>

namespace common
{

class ThreadWorker
{
public:
  ThreadWorker();
  virtual ~ThreadWorker();

  virtual void *doRun() = 0;
  
  bool runIt();

  bool endThread();
  void join();
  

private:
  pthread_t *threadInst;
  pthread_attr_t *threadAttr;

protected:
  bool end;
};
}

#endif