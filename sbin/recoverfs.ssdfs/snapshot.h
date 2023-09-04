//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/recoverfs.ssdfs/snapshot.h - snapshot related declarations.
 *
 * Copyright (c) 2023 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *              http://www.ssdfs.org/
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#ifndef _SSDFS_UTILS_RECOVERFS_SNAPSHOT_H
#define _SSDFS_UTILS_RECOVERFS_SNAPSHOT_H

enum {
	SSDFS_YEAR_GRANULARITY,
	SSDFS_MONTH_GRANULARITY,
	SSDFS_DAY_GRANULARITY,
	SSDFS_HOUR_GRANULARITY,
	SSDFS_MINUTE_GRANULARITY,
	SSDFS_CURRENT_TIMESTAMP_GRANULARITY,
};

static inline
int check_hour_granularity(struct ssdfs_time_range *snapshot)
{
	int is_hour_defined = snapshot->hour != SSDFS_ANY_HOUR;
	int is_minute_defined = snapshot->minute != SSDFS_ANY_MINUTE;

	if (is_hour_defined) {
		if (is_minute_defined)
			return SSDFS_MINUTE_GRANULARITY;
		else
			return SSDFS_HOUR_GRANULARITY;
	}

	return SSDFS_DAY_GRANULARITY;
}

static inline
int check_day_granularity(struct ssdfs_time_range *snapshot)
{
	int is_day_defined = snapshot->day != SSDFS_ANY_DAY;

	if (is_day_defined) {
		return check_hour_granularity(snapshot);
	}

	return SSDFS_MONTH_GRANULARITY;
}

static inline
int check_month_granularity(struct ssdfs_time_range *snapshot)
{
	int is_month_defined = snapshot->month != SSDFS_ANY_MONTH;

	if (is_month_defined) {
		return check_day_granularity(snapshot);
	}

	return SSDFS_YEAR_GRANULARITY;
}

static inline
int ssdfs_recoverfs_timestamp_granularity(struct ssdfs_time_range *snapshot)
{
	int is_year_defined = snapshot->year != SSDFS_ANY_YEAR;
	int is_month_defined = snapshot->month != SSDFS_ANY_MONTH;
	int is_day_defined = snapshot->day != SSDFS_ANY_DAY;
	int is_hour_defined = snapshot->hour != SSDFS_ANY_HOUR;
	int is_minute_defined = snapshot->minute != SSDFS_ANY_MINUTE;

	if (is_year_defined) {
		return check_month_granularity(snapshot);
	} else if (is_month_defined) {
		return check_day_granularity(snapshot);
	} else if (is_day_defined) {
		return check_hour_granularity(snapshot);
	} else if (is_hour_defined) {
		if (is_minute_defined)
			return SSDFS_MINUTE_GRANULARITY;
		else
			return SSDFS_HOUR_GRANULARITY;
	}

	return SSDFS_CURRENT_TIMESTAMP_GRANULARITY;
}

#define SSDFS_BASE_YEAR		(1900)

static inline
int is_year_inside_range(struct ssdfs_time_range *snapshot,
			 struct tm *current_time,
			 struct tm *time)
{
	u32 calculated_year;

	/*
	 * tm_year - The number of years since 1900.
	 *           Correct it to human-friendly format.
	 */

	if (snapshot->year == SSDFS_ANY_YEAR) {
		calculated_year = SSDFS_BASE_YEAR;
		calculated_year += current_time->tm_year;

		snapshot->year = calculated_year;
	}

	calculated_year = SSDFS_BASE_YEAR;
	calculated_year += time->tm_year;

	if (calculated_year > snapshot->year)
		return SSDFS_FALSE;

	return SSDFS_TRUE;
}

static inline
int is_month_inside_range(struct ssdfs_time_range *snapshot,
			  struct tm *current_time,
			  struct tm *time)
{
	u32 calculated_month;

	if (!is_year_inside_range(snapshot, current_time, time))
		return SSDFS_FALSE;

	/*
	 * tm_mon - The number of months since January,
	 *          in the range 0 to 11. Correct it
	 *          to human-friendly format.
	 */

	if (snapshot->month == SSDFS_ANY_MONTH) {
		calculated_month = current_time->tm_mon + 1;
		snapshot->month = calculated_month;
	}

	calculated_month = time->tm_mon + 1;

	if (calculated_month > snapshot->month)
		return SSDFS_FALSE;

	return SSDFS_TRUE;
}

static inline
int is_day_inside_range(struct ssdfs_time_range *snapshot,
			struct tm *current_time,
			struct tm *time)
{
	if (!is_month_inside_range(snapshot, current_time, time))
		return SSDFS_FALSE;

	if (snapshot->day == SSDFS_ANY_DAY)
		snapshot->day = current_time->tm_mday;

	if (time->tm_mday > snapshot->day)
		return SSDFS_FALSE;

	return SSDFS_TRUE;
}

static inline
int is_hour_inside_range(struct ssdfs_time_range *snapshot,
			 struct tm *current_time,
			 struct tm *time)
{
	if (!is_day_inside_range(snapshot, current_time, time))
		return SSDFS_FALSE;

	if (snapshot->hour == SSDFS_ANY_HOUR)
		snapshot->hour = current_time->tm_hour;

	if (time->tm_hour > snapshot->hour)
		return SSDFS_FALSE;

	return SSDFS_TRUE;
}

static inline
int is_minute_inside_range(struct ssdfs_time_range *snapshot,
			   struct tm *time)
{
	if (snapshot->year == SSDFS_ANY_YEAR) {
		return SSDFS_TRUE;
	} else if (snapshot->month == SSDFS_ANY_MONTH) {
		return SSDFS_TRUE;
	} else if (snapshot->day == SSDFS_ANY_DAY) {
		return SSDFS_TRUE;
	} else if (snapshot->hour == SSDFS_ANY_HOUR) {
		return SSDFS_TRUE;
	} else if (snapshot->minute == SSDFS_ANY_MINUTE) {
		return SSDFS_TRUE;
	}

	if (time->tm_year > snapshot->year)
		return SSDFS_FALSE;
	else if (time->tm_mon > snapshot->month)
		return SSDFS_FALSE;
	else if (time->tm_mday > snapshot->day)
		return SSDFS_FALSE;
	else if (time->tm_hour > snapshot->hour)
		return SSDFS_FALSE;
	else if (time->tm_min > snapshot->minute)
		return SSDFS_FALSE;

	return SSDFS_TRUE;
}

static
int is_timestamp_inside_range(struct ssdfs_time_range *snapshot,
			      u64 timestamp)
{
	int timestamp_granularity;
	struct tm time;
	u64 current_timestamp;
	struct tm current_time;

	current_timestamp = ssdfs_current_time_in_nanoseconds();
	ssdfs_nanoseconds_to_localtime(current_timestamp, &current_time);
	ssdfs_nanoseconds_to_localtime(timestamp, &time);

	timestamp_granularity = ssdfs_recoverfs_timestamp_granularity(snapshot);

	switch (timestamp_granularity) {
	case SSDFS_YEAR_GRANULARITY:
		return is_year_inside_range(snapshot, &current_time, &time);

	case SSDFS_MONTH_GRANULARITY:
		return is_month_inside_range(snapshot, &current_time, &time);

	case SSDFS_DAY_GRANULARITY:
		return is_day_inside_range(snapshot, &current_time, &time);

	case SSDFS_HOUR_GRANULARITY:
		return is_hour_inside_range(snapshot, &current_time, &time);

	case SSDFS_MINUTE_GRANULARITY:
		return is_minute_inside_range(snapshot, &time);

	default: /* SSDFS_CURRENT_TIMESTAMP_GRANULARITY */
		break;
	}

	return SSDFS_TRUE;
}

#endif /* _SSDFS_UTILS_RECOVERFS_SNAPSHOT_H */
