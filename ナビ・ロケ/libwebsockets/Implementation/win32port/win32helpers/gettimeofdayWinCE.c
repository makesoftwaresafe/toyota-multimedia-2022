#include <windows.h>

#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64

struct timezone 
{
    int  tzMinutesWest;	/* minutes W of Greenwich */
    int  tzDstTime;	/* type of dst correction */
};
struct timeval 
{
    long    tvSec;         /* seconds */
    long    tvUsec;        /* and microseconds */
};

int gettimeofday(struct timeval *aTv, struct timezone *aTz)
{
    FILETIME ft;
    unsigned __int64 tmpRes = 0;
    TIME_ZONE_INFORMATION tzInfo;

    if (NULL != aTv) 
    {
        GetSystemTimeAsFileTime(&ft);

        tmpRes |= ft.dwHighDateTime;
        tmpRes <<= 32;
        tmpRes |= ft.dwLowDateTime;

        /*converting file time to unix epoch*/
        tmpRes /= 10;  /*convert into microseconds*/
        tmpRes -= DELTA_EPOCH_IN_MICROSECS;
        aTv->tvSec = (long)(tmpRes / 1000000UL);
        aTv->tvUsec = (long)(tmpRes % 1000000UL);
    }

    if (NULL != aTz)
    {   
        GetTimeZoneInformation(&tzInfo);

        aTz->tzMinutesWest = tzInfo.Bias;
        if (tzInfo.StandardDate.wMonth != 0)
        {
            aTz->tzMinutesWest += tzInfo.StandardBias * 60;
        }

        if (tzInfo.DaylightDate.wMonth != 0)
        {
            aTz->tzDstTime = 1;
        }
        else
        {
            aTz->tzDstTime = 0;
        }
    }

    return 0;
}
