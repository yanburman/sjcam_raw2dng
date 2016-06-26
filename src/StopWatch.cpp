
#include <stdlib.h>

#include "StopWatch.h"

#if defined(_WIN32) || defined(_WIN64)

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS 11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS 11644473600000000ULL
#endif

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
  FILETIME ft;
  unsigned __int64 tmpres = 0;
  static int tzflag = 0;

  if (NULL != tv) {
    GetSystemTimeAsFileTime(&ft);

    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;

    tmpres /= 10; /*convert into microseconds*/
    /*converting file time to unix epoch*/
    tmpres -= DELTA_EPOCH_IN_MICROSECS;
    tv->tv_sec = (long)(tmpres / 1000000UL);
    tv->tv_usec = (long)(tmpres % 1000000UL);
  }

  if (NULL != tz) {
    if (!tzflag) {
      _tzset();
      tzflag++;
    }
    tz->tz_minuteswest = _timezone / 60;
    tz->tz_dsttime = _daylight;
  }

  return 0;
}
#endif

static struct timeval _utime_gap(struct timeval a, struct timeval b)
{
  struct timeval ret;
  if (b.tv_usec >= a.tv_usec) {
    ret.tv_usec = b.tv_usec - a.tv_usec;
    ret.tv_sec = b.tv_sec - a.tv_sec;
  } else {
    ret.tv_usec = 1000000 + b.tv_usec - a.tv_usec;
    ret.tv_sec = b.tv_sec - a.tv_sec - 1;
  }
  return ret;
}

uint64_t StopWatch::elapsed_usec()
{
  return (uint64_t)elapsed.tv_sec * 1000000 + elapsed.tv_usec;
}

void StopWatch::stop()
{
  struct timeval end, gap;
  gettimeofday(&end, NULL);
  gap = _utime_gap(start, end);
  elapsed.tv_sec += gap.tv_sec;
  elapsed.tv_usec += gap.tv_usec;
  if (elapsed.tv_usec >= 1000000) {
    elapsed.tv_usec -= 1000000;
    elapsed.tv_sec++;
  }
}
