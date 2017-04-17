#include <iostream>
#include <string>
#include <unistd.h>

#include "Log.h"
#include "HttpServer.h"
#include "HttpHandler.h"
#include "TaskWorker.h"

using namespace std;
using std::vector;

#define WORKER_NUM 1

//// global logger
Logger logger;

int main(int argc, char **argv)
{
  // init logger;
  bool logInited = logger.initLogger("./xping.log");
  if (!logInited)
  {
    std::cout << "init logger failed" << std::endl;
    exit(-1);
  }
  logger.runIt();
  logger.log(INFO_LOG, "async logger started");
  logger.log(INFO_LOG, "xping try starting ...");

  // init server
  HttpServer *server = new HttpServer(9201);
  server->registerHandler("/net/ping", HttpHandler::handlePingReq);

  // init workers
  vector<TaskWorker *> workers;
  for (int i = 0; i < WORKER_NUM; i ++)
  {
    TaskWorker *worker = new TaskWorker();
    workers.push_back(worker);
    worker->registerMaster(server);
    server->registerWorker(worker);
    worker->runIt();
  }

  usleep(1000 * 1000);  // 1 second
  server->runIt();
  logger.log(INFO_LOG, "http server and worker threads launched, main thread waiting"); 

  // wait server to die
  server->join();
  delete server;
  logger.log(INFO_LOG, "http server exited");

  // wait worker to die
  for (vector<TaskWorker *>::iterator it = workers.begin(); it != workers.end(); it++)
  {
    (*it)->join();
    delete (*it);
  }
  logger.log(INFO_LOG, "all workers exited");

  logger.join();
  return 0;
}
