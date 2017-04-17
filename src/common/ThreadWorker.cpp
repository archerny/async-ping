#include "ThreadWorker.h"

using namespace common;


// ISO C++ forbids taking the address of a bound member function to
// form a pointer to member function.
static void *commonThreadFunc(void *data)
{
  ThreadWorker *worker = (ThreadWorker *)data;
  worker->doRun();
  return NULL;
}

ThreadWorker::ThreadWorker()
{
  // thread is not started, so end is true
  end = true;
  threadInst = NULL;
  threadAttr = new pthread_attr_t;
  pthread_attr_init(threadAttr);
  // default set to joinable
  pthread_attr_setdetachstate(threadAttr, PTHREAD_CREATE_JOINABLE);
  // set default stack size
  size_t stackSize = 1024 * 1024 * 16; // 16MB
  pthread_attr_setstacksize(threadAttr, stackSize);
}

ThreadWorker::~ThreadWorker()
{
  if (threadInst) {
    end = true;
    // wait for the thread to exit
    int status = pthread_join(*threadInst, NULL);
    if (status) {
      // join thread error, log it
    }
    delete threadInst;
  }
  // release thread attr
  pthread_attr_destroy(threadAttr);
  delete threadAttr;
}

bool ThreadWorker::endThread()
{
  // let thread itself exit gracefully
  return end = true;
}

void ThreadWorker::join()
{
  if (threadInst == NULL)
  {
    return;
  }
  int status = pthread_join(*threadInst, NULL);
  if (status)
  {
    // log join error
  }
  else
  {
    // join thread success
    delete threadInst;
    threadInst = NULL;
  }
}

bool ThreadWorker::runIt()
{
  if (!end) {
    // already running
    return true;
  }
  threadInst = new pthread_t;
  int status = pthread_create(threadInst, threadAttr, commonThreadFunc, (void *)this);
  if (status)
  {
    // pthread create error, log it 
    return false;
  }
  else 
  {
    // create thread successfully, log it
    end = false;
    return true;
  }
}