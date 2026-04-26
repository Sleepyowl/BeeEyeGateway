#pragma once

struct rtc_time;

#pragma once

struct rtc_time;

/**
 * @brief Synchronize RTC time using NTP (UTC).
 *
 * Attempts to obtain the current time from an NTP server and update
 * the RTC accordingly.
 *
 * @return 0 on success, negative error code on failure.
 */
int rtc_synchronize_time(void);

/**
 * @brief Get current UTC time from RTC.
 *
 * Reads the current time from the RTC and returns it as UTC.
 *
 * @param[out] rtc_tm Pointer to structure that will be filled with UTC time.
 *
 * @return 0 on success, negative error code (-errno) on failure.
 */
int rtc_get_time_utc(struct rtc_time *rtc_tm);

/**
 * @brief Get local time using EU daylight saving rules.
 *
 * Converts RTC UTC time to local time using European DST rules
 * (summer/winter time).
 *
 * @param[out] rtc_tm Pointer to structure that will be filled with local time.
 * @param[in] tz_min Time zone offset from UTC in minutes (standard time, without DST).
 *
 * @return 0 on success, negative error code (-errno) on failure.
 */
int rtc_get_time_eu_local(struct rtc_time *rtc_tm, int tz_min);
