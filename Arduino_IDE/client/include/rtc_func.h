#ifndef RTC_FUNC_H
#define RTC_FUNC_H
#include <sys/time.h>

extern "C" int _gettimeofday(struct timeval*, void*);

void initTime();

#endif
