/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/snapshotfs.ssdfs/options.c - parsing command line options functionality.
 *
 * Copyright (c) 2021-2026 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *              http://www.ssdfs.org/
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#include <sys/types.h>
#include <getopt.h>

#include "snapshotfs.h"

/************************************************************************
 *                    Options parsing functionality                     *
 ************************************************************************/

static void print_version(void)
{
	SSDFS_INFO("snapshotfs.ssdfs, part of %s\n", SSDFS_UTILS_VERSION);
}

void print_usage(void)
{
	SSDFS_SNAPSHOTFS_INFO(SSDFS_TRUE, "snapshot SSDFS file system\n\n");
	SSDFS_INFO("Usage: snapshotfs.ssdfs <options> [<folder> | <file>]\n");
	SSDFS_INFO("Options:\n");
	SSDFS_INFO("\t [-c|--create name=value, "
		   "mode=value (READ_ONLY|READ_WRITE), "
		   "type=value (PERIODIC|ONE_TIME), "
		   "expiration=value (WEEK|MONTH|YEAR|NEVER), "
		   "frequency=value (SYNCFS|HOUR|DAY|WEEK|MONTH), "
		   "snapshots-threshold=value]\t\t  create snapshot.\n");
	SSDFS_INFO("\t [-d|--debug]\t\t  show debug output.\n");
	SSDFS_INFO("\t [-h|--help]\t\t  display help message and exit.\n");
	SSDFS_INFO("\t [-l|--list minute=value, hour=value, "
		   "day=value, month=value, year=value, "
		   "mode=value (READ_ONLY|READ_WRITE), "
		   "type=value (PERIODIC|ONE_TIME), "
		   "max-number=value]\t\t  show list of snapshots.\n");
	SSDFS_INFO("\t [-m|--modify minute=value, hour=value, "
		   "day=value, month=value, year=value, "
		   "name=value, id=value, "
		   "mode=value (READ_ONLY|READ_WRITE), "
		   "type=value (PERIODIC|ONE_TIME), "
		   "expiration=value (WEEK|MONTH|YEAR|NEVER), "
		   "frequency=value (SYNCFS|HOUR|DAY|WEEK), "
		   "snapshots-threshold=value]\t\t  change snapshot's properties.\n");
	SSDFS_INFO("\t [-r|--remove name=value, "
		   "id=value]\t\t  delete snapshot.\n");
	SSDFS_INFO("\t [-R|--remove-range minute=value, hour=value, "
		   "day=value, month=value, "
		   "year=value]\t\t  delete range of snapshots.\n");
	SSDFS_INFO("\t [-s|--show-details name=value, "
		   "id=value]\t\t  show snapshot's details.\n");
	SSDFS_INFO("\t [-V|--version]\t\t  print version and exit.\n");
}

static inline
int convert_string2mode(const char *str)
{
	if (strcmp(str, SSDFS_READ_ONLY_MODE_STR) == 0)
		return SSDFS_READ_ONLY_SNAPSHOT;
	else if (strcmp(str, SSDFS_READ_WRITE_MODE_STR) == 0)
		return SSDFS_READ_WRITE_SNAPSHOT;
	else
		return SSDFS_UNKNOWN_SNAPSHOT_MODE;
}

static inline
void check_mode(int mode)
{
	switch (mode) {
	case SSDFS_READ_ONLY_SNAPSHOT:
	case SSDFS_READ_WRITE_SNAPSHOT:
		/* valid mode */
		break;

	default:
		print_usage();
		exit(EXIT_FAILURE);
	}
}

static inline
int convert_string2type(const char *str)
{
	if (strcmp(str, SSDFS_ONE_TIME_TYPE_STR) == 0)
		return SSDFS_ONE_TIME_SNAPSHOT;
	else if (strcmp(str, SSDFS_PERIODIC_TYPE_STR) == 0)
		return SSDFS_PERIODIC_SNAPSHOT;
	else
		return SSDFS_UNKNOWN_SNAPSHOT_TYPE;
}

static inline
void check_type(int type)
{
	switch (type) {
	case SSDFS_ONE_TIME_SNAPSHOT:
	case SSDFS_PERIODIC_SNAPSHOT:
		/* valid type */
		break;

	default:
		print_usage();
		exit(EXIT_FAILURE);
	}
}

static inline
int convert_string2expiration(const char *str)
{
	if (strcmp(str, SSDFS_WEEK_EXPIRATION_POINT_STR) == 0)
		return SSDFS_EXPIRATION_IN_WEEK;
	else if (strcmp(str, SSDFS_MONTH_EXPIRATION_POINT_STR) == 0)
		return SSDFS_EXPIRATION_IN_MONTH;
	else if (strcmp(str, SSDFS_YEAR_EXPIRATION_POINT_STR) == 0)
		return SSDFS_EXPIRATION_IN_YEAR;
	else if (strcmp(str, SSDFS_NEVER_EXPIRED_STR) == 0)
		return SSDFS_NEVER_EXPIRED;
	else
		return SSDFS_UNKNOWN_EXPIRATION_POINT;
}

static inline
void check_expiration(int expiration)
{
	switch (expiration) {
	case SSDFS_EXPIRATION_IN_WEEK:
	case SSDFS_EXPIRATION_IN_MONTH:
	case SSDFS_EXPIRATION_IN_YEAR:
	case SSDFS_NEVER_EXPIRED:
		/* valid expiration */
		break;

	default:
		print_usage();
		exit(EXIT_FAILURE);
	}
}

static inline
int convert_string2frequency(const char *str)
{
	if (strcmp(str, SSDFS_SYNCFS_FREQUENCY_STR) == 0)
		return SSDFS_SYNCFS_FREQUENCY;
	else if (strcmp(str, SSDFS_HOUR_FREQUENCY_STR) == 0)
		return SSDFS_HOUR_FREQUENCY;
	else if (strcmp(str, SSDFS_DAY_FREQUENCY_STR) == 0)
		return SSDFS_DAY_FREQUENCY;
	else if (strcmp(str, SSDFS_WEEK_FREQUENCY_STR) == 0)
		return SSDFS_WEEK_FREQUENCY;
	else if (strcmp(str, SSDFS_MONTH_FREQUENCY_STR) == 0)
		return SSDFS_MONTH_FREQUENCY;
	else
		return SSDFS_UNKNOWN_FREQUENCY;
}

static inline
void check_frequency(int frequency)
{
	switch (frequency) {
	case SSDFS_SYNCFS_FREQUENCY:
	case SSDFS_HOUR_FREQUENCY:
	case SSDFS_DAY_FREQUENCY:
	case SSDFS_WEEK_FREQUENCY:
	case SSDFS_MONTH_FREQUENCY:
		/* valid frequency */
		break;

	default:
		print_usage();
		exit(EXIT_FAILURE);
	}
}

static inline
void check_minute(int minute)
{
	if (minute < 0 || minute > 60) {
		print_usage();
		exit(EXIT_FAILURE);
	}
}

static inline
void check_hour(int hour)
{
	if (hour < 0 || hour > 24) {
		print_usage();
		exit(EXIT_FAILURE);
	}
}

static inline
void check_day(int day)
{
	if (day <= 0 || day > 31) {
		print_usage();
		exit(EXIT_FAILURE);
	}
}

static inline
void check_month(int month)
{
	if (month <= 0 || month > 12) {
		print_usage();
		exit(EXIT_FAILURE);
	}
}

static inline
void check_year(int year)
{
	if (year < 1970) {
		print_usage();
		exit(EXIT_FAILURE);
	}
}

static inline
int is_operation_unknown(struct ssdfs_snapshot_options *options)
{
	return options->operation == SSDFS_UNKNOWN_OPERATION;
}

void parse_options(int argc, char *argv[],
		   struct ssdfs_snapshot_options *options)
{
	int c;
	int oi = 1;
	char *p;
	char sopts[] = "c:dhl:m:r:R:s:V";
	static const struct option lopts[] = {
		{"create", 1, NULL, 'c'},
		{"debug", 0, NULL, 'd'},
		{"help", 0, NULL, 'h'},
		{"list", 1, NULL, 'l'},
		{"modify", 1, NULL, 'm'},
		{"remove", 1, NULL, 'r'},
		{"remove-range", 1, NULL, 'R'},
		{"show-details", 1, NULL, 's'},
		{"version", 0, NULL, 'V'},
		{ }
	};
	enum {
		SNAPSHOT_CREATE_NAME_OPT = 0,
		SNAPSHOT_CREATE_MODE_OPT,
		SNAPSHOT_CREATE_TYPE_OPT,
		SNAPSHOT_CREATE_EXPIRATION_OPT,
		SNAPSHOT_CREATE_FREQUENCY_OPT,
		SNAPSHOT_CREATE_EXISTING_NUMBER_OPTS,
	};
	char *const snapshot_create_tokens[] = {
		[SNAPSHOT_CREATE_NAME_OPT]		= "name",
		[SNAPSHOT_CREATE_MODE_OPT]		= "mode",
		[SNAPSHOT_CREATE_TYPE_OPT]		= "type",
		[SNAPSHOT_CREATE_EXPIRATION_OPT]	= "expiration",
		[SNAPSHOT_CREATE_FREQUENCY_OPT]		= "frequency",
		[SNAPSHOT_CREATE_EXISTING_NUMBER_OPTS]	= "snapshots-threshold",
		NULL
	};
	enum {
		SNAPSHOT_LIST_RANGE_MINUTE_OPT = 0,
		SNAPSHOT_LIST_RANGE_HOUR_OPT,
		SNAPSHOT_LIST_RANGE_DAY_OPT,
		SNAPSHOT_LIST_RANGE_MONTH_OPT,
		SNAPSHOT_LIST_RANGE_YEAR_OPT,
		SNAPSHOT_LIST_MODE_OPT,
		SNAPSHOT_LIST_TYPE_OPT,
		SNAPSHOT_LIST_MAX_NUMBER_OPT,
	};
	char *const snapshot_list_tokens[] = {
		[SNAPSHOT_LIST_RANGE_MINUTE_OPT]	= "minute",
		[SNAPSHOT_LIST_RANGE_HOUR_OPT]		= "hour",
		[SNAPSHOT_LIST_RANGE_DAY_OPT]		= "day",
		[SNAPSHOT_LIST_RANGE_MONTH_OPT]		= "month",
		[SNAPSHOT_LIST_RANGE_YEAR_OPT]		= "year",
		[SNAPSHOT_LIST_MODE_OPT]		= "mode",
		[SNAPSHOT_LIST_TYPE_OPT]		= "type",
		[SNAPSHOT_LIST_MAX_NUMBER_OPT]		= "max-number",
		NULL
	};
	enum {
		SNAPSHOT_MODIFY_MINUTE_OPT = 0,
		SNAPSHOT_MODIFY_HOUR_OPT,
		SNAPSHOT_MODIFY_DAY_OPT,
		SNAPSHOT_MODIFY_MONTH_OPT,
		SNAPSHOT_MODIFY_YEAR_OPT,
		SNAPSHOT_MODIFY_NAME_OPT,
		SNAPSHOT_MODIFY_ID_OPT,
		SNAPSHOT_MODIFY_MODE_OPT,
		SNAPSHOT_MODIFY_TYPE_OPT,
		SNAPSHOT_MODIFY_EXPIRATION_OPT,
		SNAPSHOT_MODIFY_FREQUENCY_OPT,
		SNAPSHOT_MODIFY_EXISTING_NUMBER_OPTS,
	};
	char *const snapshot_modify_tokens[] = {
		[SNAPSHOT_MODIFY_MINUTE_OPT]		= "minute",
		[SNAPSHOT_MODIFY_HOUR_OPT]		= "hour",
		[SNAPSHOT_MODIFY_DAY_OPT]		= "day",
		[SNAPSHOT_MODIFY_MONTH_OPT]		= "month",
		[SNAPSHOT_MODIFY_YEAR_OPT]		= "year",
		[SNAPSHOT_MODIFY_NAME_OPT]		= "name",
		[SNAPSHOT_MODIFY_ID_OPT]		= "id",
		[SNAPSHOT_MODIFY_MODE_OPT]		= "mode",
		[SNAPSHOT_MODIFY_TYPE_OPT]		= "type",
		[SNAPSHOT_MODIFY_EXPIRATION_OPT]	= "expiration",
		[SNAPSHOT_MODIFY_FREQUENCY_OPT]		= "frequency",
		[SNAPSHOT_MODIFY_EXISTING_NUMBER_OPTS]	= "snapshots-threshold",
		NULL
	};
	enum {
		SNAPSHOT_REMOVE_NAME_OPT = 0,
		SNAPSHOT_REMOVE_ID_OPT,
	};
	char *const snapshot_remove_tokens[] = {
		[SNAPSHOT_REMOVE_NAME_OPT]		= "name",
		[SNAPSHOT_REMOVE_ID_OPT]		= "id",
		NULL
	};
	enum {
		SNAPSHOT_REMOVE_RANGE_MINUTE_OPT = 0,
		SNAPSHOT_REMOVE_RANGE_HOUR_OPT,
		SNAPSHOT_REMOVE_RANGE_DAY_OPT,
		SNAPSHOT_REMOVE_RANGE_MONTH_OPT,
		SNAPSHOT_REMOVE_RANGE_YEAR_OPT,
	};
	char *const snapshot_remove_range_tokens[] = {
		[SNAPSHOT_REMOVE_RANGE_MINUTE_OPT]	= "minute",
		[SNAPSHOT_REMOVE_RANGE_HOUR_OPT]	= "hour",
		[SNAPSHOT_REMOVE_RANGE_DAY_OPT]		= "day",
		[SNAPSHOT_REMOVE_RANGE_MONTH_OPT]	= "month",
		[SNAPSHOT_REMOVE_RANGE_YEAR_OPT]	= "year",
		NULL
	};
	enum {
		SNAPSHOT_SHOW_DETAILS_NAME_OPT = 0,
		SNAPSHOT_SHOW_DETAILS_ID_OPT,
	};
	char *const snapshot_show_details_tokens[] = {
		[SNAPSHOT_SHOW_DETAILS_NAME_OPT]	= "name",
		[SNAPSHOT_SHOW_DETAILS_ID_OPT]		= "id",
		NULL
	};

	options->operation = SSDFS_UNKNOWN_OPERATION;
	options->show_debug = SSDFS_FALSE;
	memset(options->name_buf, 0, sizeof(SSDFS_MAX_NAME_LEN));
	memset(options->uuid_buf, 0, sizeof(SSDFS_UUID_SIZE));

	while ((c = getopt_long(argc, argv, sopts, lopts, &oi)) != EOF) {
		switch (c) {
		case 'c':
			if (!is_operation_unknown(options)) {
				print_usage();
				exit(EXIT_FAILURE);
			} else {
				options->operation =
					SSDFS_CREATE_SNAPSHOT;
			}

			p = optarg;
			while (*p != '\0') {
				char *value;
				struct ssdfs_snapshot_create_options *create;

				create = &options->create;

				switch (getsubopt(&p, snapshot_create_tokens,
						  &value)) {
				case SNAPSHOT_CREATE_NAME_OPT:
					strncpy(options->name_buf, value,
						sizeof(options->name_buf) - 1);
					create->name = options->name_buf;
					break;
				case SNAPSHOT_CREATE_MODE_OPT:
					create->mode =
						convert_string2mode(value);
					check_mode(create->mode);
					break;
				case SNAPSHOT_CREATE_TYPE_OPT:
					create->type =
						convert_string2type(value);
					check_type(create->type);
					break;
				case SNAPSHOT_CREATE_EXPIRATION_OPT:
					create->expiration =
					    convert_string2expiration(value);
					check_expiration(create->expiration);
					break;
				case SNAPSHOT_CREATE_FREQUENCY_OPT:
					create->frequency =
					    convert_string2frequency(value);
					check_frequency(create->frequency);
					break;
				case SNAPSHOT_CREATE_EXISTING_NUMBER_OPTS:
					create->snapshots_threshold =
								atoi(value);
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
			break;
		case 'd':
			options->show_debug = SSDFS_TRUE;
			break;
		case 'h':
			print_usage();
			exit(EXIT_SUCCESS);
		case 'l':
			if (!is_operation_unknown(options)) {
				print_usage();
				exit(EXIT_FAILURE);
			} else {
				options->operation =
					SSDFS_LIST_SNAPSHOTS;
			}

			p = optarg;
			while (*p != '\0') {
				char *value;
				struct ssdfs_snapshot_list_options *list;
				struct ssdfs_time_range *range;

				list = &options->list;

				switch (getsubopt(&p, snapshot_list_tokens,
						  &value)) {
				case SNAPSHOT_LIST_RANGE_MINUTE_OPT:
					range = &list->time_range;
					range->minute = atoi(value);
					check_minute(range->minute);
					break;
				case SNAPSHOT_LIST_RANGE_HOUR_OPT:
					range = &list->time_range;
					range->hour = atoi(value);
					check_hour(range->hour);
					break;
				case SNAPSHOT_LIST_RANGE_DAY_OPT:
					range = &list->time_range;
					range->day = atoi(value);
					check_day(range->day);
					break;
				case SNAPSHOT_LIST_RANGE_MONTH_OPT:
					range = &list->time_range;
					range->month = atoi(value);
					check_month(range->month);
					break;
				case SNAPSHOT_LIST_RANGE_YEAR_OPT:
					range = &list->time_range;
					range->year = atoi(value);
					check_year(range->year);
					break;
				case SNAPSHOT_LIST_MODE_OPT:
					list->mode = convert_string2mode(value);
					check_mode(list->mode);
					break;
				case SNAPSHOT_LIST_TYPE_OPT:
					list->type = convert_string2type(value);
					check_type(list->type);
					break;
				case SNAPSHOT_LIST_MAX_NUMBER_OPT:
					list->max_number = atoi(value);
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
			break;
		case 'm':
			if (!is_operation_unknown(options)) {
				print_usage();
				exit(EXIT_FAILURE);
			} else {
				options->operation =
					SSDFS_MODIFY_SNAPSHOT;
			}

			p = optarg;
			while (*p != '\0') {
				char *value;
				struct ssdfs_snapshot_modify_options *modify;
				struct ssdfs_time_range *range;

				modify = &options->modify;

				switch (getsubopt(&p, snapshot_modify_tokens,
						  &value)) {
				case SNAPSHOT_MODIFY_MINUTE_OPT:
					range = &modify->time_range;
					range->minute = atoi(value);
					check_minute(range->minute);
					break;
				case SNAPSHOT_MODIFY_HOUR_OPT:
					range = &modify->time_range;
					range->hour = atoi(value);
					check_hour(range->hour);
					break;
				case SNAPSHOT_MODIFY_DAY_OPT:
					range = &modify->time_range;
					range->day = atoi(value);
					check_day(range->day);
					break;
				case SNAPSHOT_MODIFY_MONTH_OPT:
					range = &modify->time_range;
					range->month = atoi(value);
					check_month(range->month);
					break;
				case SNAPSHOT_MODIFY_YEAR_OPT:
					range = &modify->time_range;
					range->year = atoi(value);
					check_year(range->year);
					break;
				case SNAPSHOT_MODIFY_NAME_OPT:
					strncpy(options->name_buf, value,
						sizeof(options->name_buf) - 1);
					modify->name = options->name_buf;
					break;
				case SNAPSHOT_MODIFY_ID_OPT:
					memcpy(options->uuid_buf, value,
						sizeof(options->uuid_buf));
					modify->id = options->uuid_buf;
					break;
				case SNAPSHOT_MODIFY_MODE_OPT:
					modify->mode =
						convert_string2mode(value);
					check_mode(modify->mode);
					break;
				case SNAPSHOT_MODIFY_TYPE_OPT:
					modify->type =
						convert_string2type(value);
					check_type(modify->type);
					break;
				case SNAPSHOT_MODIFY_EXPIRATION_OPT:
					modify->expiration =
					    convert_string2expiration(value);
					check_expiration(modify->expiration);
					break;
				case SNAPSHOT_MODIFY_FREQUENCY_OPT:
					modify->frequency =
					    convert_string2frequency(value);
					check_frequency(modify->frequency);
					break;
				case SNAPSHOT_MODIFY_EXISTING_NUMBER_OPTS:
					modify->snapshots_threshold =
								atoi(value);
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
			break;
		case 'r':
			if (!is_operation_unknown(options)) {
				print_usage();
				exit(EXIT_FAILURE);
			} else {
				options->operation =
					SSDFS_REMOVE_SNAPSHOT;
			}

			p = optarg;
			while (*p != '\0') {
				char *value;
				struct ssdfs_snapshot_remove_options *remove;

				remove = &options->remove;

				switch (getsubopt(&p, snapshot_remove_tokens,
						  &value)) {
				case SNAPSHOT_REMOVE_NAME_OPT:
					strncpy(options->name_buf, value,
						sizeof(options->name_buf) - 1);
					remove->name = options->name_buf;
					break;
				case SNAPSHOT_REMOVE_ID_OPT:
					memcpy(options->uuid_buf, value,
						sizeof(options->uuid_buf));
					remove->id = options->uuid_buf;
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
			break;
		case 'R':
			if (!is_operation_unknown(options)) {
				print_usage();
				exit(EXIT_FAILURE);
			} else {
				options->operation =
					SSDFS_REMOVE_RANGE;
			}

			p = optarg;
			while (*p != '\0') {
				char *value;
				struct ssdfs_time_range *range;

				range = &options->remove_range.time_range;

				switch (getsubopt(&p,
						  snapshot_remove_range_tokens,
						  &value)) {
				case SNAPSHOT_REMOVE_RANGE_MINUTE_OPT:
					range->minute = atoi(value);
					check_minute(range->minute);
					break;
				case SNAPSHOT_REMOVE_RANGE_HOUR_OPT:
					range->hour = atoi(value);
					check_hour(range->hour);
					break;
				case SNAPSHOT_REMOVE_RANGE_DAY_OPT:
					range->day = atoi(value);
					check_day(range->day);
					break;
				case SNAPSHOT_REMOVE_RANGE_MONTH_OPT:
					range->month = atoi(value);
					check_month(range->month);
					break;
				case SNAPSHOT_REMOVE_RANGE_YEAR_OPT:
					range->year = atoi(value);
					check_year(range->year);
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
			break;
		case 's':
			if (!is_operation_unknown(options)) {
				print_usage();
				exit(EXIT_FAILURE);
			} else {
				options->operation =
					SSDFS_SHOW_SNAPSHOT_DETAILS;
			}

			p = optarg;
			while (*p != '\0') {
				char *value;

				switch (getsubopt(&p,
						  snapshot_show_details_tokens,
						  &value)) {
				case SNAPSHOT_SHOW_DETAILS_NAME_OPT:
					strncpy(options->name_buf, value,
						sizeof(options->name_buf) - 1);
					options->show_details.name =
							options->name_buf;
					break;
				case SNAPSHOT_SHOW_DETAILS_ID_OPT:
					memcpy(options->uuid_buf, value,
						sizeof(options->uuid_buf));
					options->show_details.id =
							options->uuid_buf;
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
			break;
		case 'V':
			print_version();
			exit(EXIT_SUCCESS);
		default:
			print_usage();
			exit(EXIT_FAILURE);
		}
	}

	if (optind != argc - 1) {
		print_usage();
		exit(EXIT_FAILURE);
	}
}
