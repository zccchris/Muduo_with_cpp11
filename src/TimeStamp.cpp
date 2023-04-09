#include "TimeStamp.h"
#include <time.h>
using namespace std;

TimeStamp::TimeStamp(int64_t microSecondSinceEpoch) : microSecondSinceEpoch_(microSecondSinceEpoch) {}

TimeStamp TimeStamp::now(){ //���ص�ǰʱ�� ����һ��static����
    return TimeStamp(time(NULL));
}

string TimeStamp::toString() const{ //��int64_t��ʱ��ת��Ϊ�ַ���

    char buf[128] = { 0 };
    tm* tm_time = localtime(&microSecondSinceEpoch_);  //localtime�������صĽṹ������tm������ʾ��
    snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d",
        tm_time->tm_year + 1900,
        tm_time->tm_mon + 1,
        tm_time->tm_mday,
        tm_time->tm_hour,
        tm_time->tm_min,
        tm_time->tm_sec);
    return buf;
}
/**
struct tm
{
  int tm_sec;			// Seconds.	[0-60] (1 leap second)
  int tm_min;			// Minutes.	[0-59]
  int tm_hour;			// Hours.	[0-23]
  int tm_mday;			// Day.		[1-31]
  int tm_mon;			// Month.	[0-11]
  int tm_year;			// Year	- 1900.
  int tm_wday;			// Day of week.	[0-6]
  int tm_yday;			// Days in year.[0-365]
  int tm_isdst;			// DST.		[-1/0/1]
# ifdef	__USE_MISC
  long int tm_gmtoff;		// Seconds east of UTC.
  const char *tm_zone;		// Timezone abbreviation.
# else
  long int __tm_gmtoff;		// Seconds east of UTC.
  const char *__tm_zone;	// Timezone abbreviation.
# endif
};
*/

/*
#include <iostream>
int main()
{
    cout << TimeStamp::now().toString() << endl;
    return 0;
}*/