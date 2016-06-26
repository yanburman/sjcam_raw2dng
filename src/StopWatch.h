#ifndef __STOP_WATCH_H
#define __STOP_WATCH_H

#if defined(_WIN32) || defined(_WIN64)
#include <time.h>
#include <windows.h>
typedef unsigned __int64 uint64_t;

struct timezone {
  int tz_minuteswest; /* minutes W of Greenwich */
  int tz_dsttime; /* type of dst correction */
};

int gettimeofday(struct timeval *tv, struct timezone *tz);
#else
#include <stdint.h>
#include <sys/time.h>
#endif

class StopWatch
{
  public:
  StopWatch()
  {
    reset();
  }

  void reset()
  {
    elapsed.tv_sec = 0;
    elapsed.tv_usec = 0;
  }

  uint64_t elapsed_usec();

  void elapsed_timeval(struct timeval *res)
  {
    *res = elapsed;
  }

  void run()
  {
    gettimeofday(&start, NULL);
  }

  void stop();

  protected:
  struct timeval elapsed;
  struct timeval start;
};

#endif // __STOP_WATCH_H
