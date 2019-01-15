#ifdef TIVAboard

#include "rtc_func.h"
#include <RTC_Library.h>

// Define variables and constants
time_t myEpochRTC;
tm myTimeRTC;

DateTime myRTC;

void initTime()
{
    myRTC.begin();
    myRTC.setTimeZone(tz_PST);

    myRTC.setTime(1545504658);
}


extern "C" int _gettimeofday(struct timeval* tv, void * tzone) {
    myEpochRTC = myRTC.getTime();
    tv->tv_sec = myEpochRTC;
    tv->tv_usec = myEpochRTC * 1E6;
    return 0; // return non-zero for error
} // end _gettimeofday()

#endif
