#ifndef _COMMON_H_
#define _COMMON_H_

#include <cstdlib>
#include <ctime>
#include <sys/time.h>
#include <stdint.h>

#define WORKER_NUM 1

namespace common
{

class Common
{
public:
  static int getRandIntvalue(int start, int end);

  static uint64_t currentTime();
};

}

#endif