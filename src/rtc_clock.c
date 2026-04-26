#include "rtc_clock.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/net/sntp.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/timeutil.h>

#include <time.h>


LOG_MODULE_DECLARE(app);

static bool is_dst_eu(const struct rtc_time *t);
static void utc_to_local(struct rtc_time *t, int timezone_offset_min);
static int get_time_from_ntp(struct rtc_time *rtc_tm);

static const struct device *dev_rtc = DEVICE_DT_GET(DT_NODELABEL(rv3028));

int rtc_get_time_utc(struct rtc_time *rtc_tm) {
    int ret = 0;
    if (!device_is_ready(dev_rtc)) {
        return -ENODEV;
    }

    ret = rtc_get_time(dev_rtc, rtc_tm);
    return ret;
}

int rtc_get_time_eu_local(struct rtc_time *rtc_tm, int tz_min) {
    int ret = 0;
    ret = rtc_get_time_utc(rtc_tm);
    if (!ret) {
        utc_to_local(rtc_tm, tz_min);
    }
    return ret;
}

int rtc_synchronize_time(void) {
    int ret = 0;
    if (!device_is_ready(dev_rtc)) {
        return -ENODEV;
    }
    struct rtc_time tm;
    ret = get_time_from_ntp(&tm);
    if (!ret) {
        ret = rtc_set_time(dev_rtc, &tm);
    }
    return ret;
}


static bool is_dst_eu(const struct rtc_time *t)
{
    int y = t->tm_year + 1900;
    int m = t->tm_mon + 1;
    int d = t->tm_mday;

    if (m < 3 || m > 10) {
        return false;
    }

    if (m > 3 && m < 10) {
        return true;
    }

    /* weekday of 31st of month */
    struct rtc_time last = {
        .tm_year = y - 1900,
        .tm_mon  = m - 1,
        .tm_mday = 31,
    };

    mktime((struct tm *)&last);  // fills tm_wday

    int last_sunday = 31 - last.tm_wday;

    if (m == 3) {
        return d >= last_sunday;
    } else {
        return d < last_sunday;
    }
}

static void utc_to_local(struct rtc_time *t, int timezone_offset_min)
{
    time_t ts = mktime((struct tm *)t);

    int offset = timezone_offset_min * 60;

    if (is_dst_eu(t)) {
        offset += 3600; // DST +1h
    }

    ts += offset;

    struct tm *tmp = localtime(&ts);
    *t = *(struct rtc_time *)tmp;
}

static int get_time_from_ntp(struct rtc_time *rtc_tm)
{
    struct sntp_time sntp_time;
    int ret;

    ret = sntp_simple("pool.ntp.org", 3000, &sntp_time);
    if (ret < 0) {
        LOG_ERR("NTP failed: %d", ret);
        return ret;
    }

    struct timespec ts = {
        .tv_sec = sntp_time.seconds,
        .tv_nsec = ((uint64_t)sntp_time.fraction * 1000000000ULL) >> 32
    };

    /* Zephyr-native */
    sys_clock_settime(CLOCK_REALTIME, &ts);

    struct tm tm_utc;
    gmtime_r(&ts.tv_sec, &tm_utc);
    
    rtc_tm->tm_sec  = tm_utc.tm_sec;
    rtc_tm->tm_min  = tm_utc.tm_min;
    rtc_tm->tm_hour = tm_utc.tm_hour;
    rtc_tm->tm_mday = tm_utc.tm_mday;
    rtc_tm->tm_mon  = tm_utc.tm_mon;
    rtc_tm->tm_year = tm_utc.tm_year;
    rtc_tm->tm_wday = tm_utc.tm_wday;
    rtc_tm->tm_yday = tm_utc.tm_yday;
    rtc_tm->tm_isdst = 0;    

    return 0;
}