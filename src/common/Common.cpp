#include <sys/types.h>
#include <unistd.h>

#include "Common.h"

using namespace common;

int Common::getRandIntvalue(int start, int end)
{
  if (end <= start)
  {
    return start;
  }
  static int randInit = 0;
  if (!randInit)
  {
    randInit = 1;
    unsigned seed = (unsigned)getpid() + (unsigned)time(0);
    srand(seed);
  }
  int delta = end - start;
  // RAND_MAX + 1 make the "end" exclusive
  int r = (int)(rand() / (double)((unsigned)RAND_MAX + 1) * (delta));
  return start + r;
}