/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Derick Rethans
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "timelib.h"
#include "timelib_private.h"
#include <ctype.h>
#include <math.h>

#define TIMELIB_LLABS(y) (y < 0 ? (y * -1) : y)

const char *timelib_error_messages[8] = {
	"No error",
	"Can not allocate buffer for parsing",
	"Corrupt tzfile: The transitions in the file don't always increase",
	"Corrupt tzfile: The expected 64-bit preamble is missing",
	"Corrupt tzfile: No abbreviation could be found for a transition",
	"The version used in this timezone identifier is unsupported",
	"No timezone with this name could be found",
};

const char *timelib_get_error_message(int error_code)
{
	int entries = sizeof(timelib_error_messages) / sizeof(char*);

	if (error_code >= 0 && error_code < entries) {
		return timelib_error_messages[error_code];
	}
	return "Unknown error code";
}

timelib_time* timelib_time_ctor(void)
{
	timelib_time *t;
	t = timelib_calloc(1, sizeof(timelib_time));

	return t;
}

void timelib_time_dtor(timelib_time* t)
{
	TIMELIB_TIME_FREE(t->tz_abbr);
	TIMELIB_TIME_FREE(t);
}

int timelib_time_compare(timelib_time *t1, timelib_time *t2)
{
	if (t1->sse == t2->sse) {
		if (t1->us == t2->us) {
			return 0;
		}

		return (t1->us < t2->us) ? -1 : 1;
	}

	return (t1->sse < t2->sse) ? -1 : 1;
}

timelib_time* timelib_time_clone(timelib_time *orig)
{
	timelib_time *tmp = timelib_time_ctor();
	memcpy(tmp, orig, sizeof(timelib_time));
	if (orig->tz_abbr) {
		tmp->tz_abbr = timelib_strdup(orig->tz_abbr);
	}
	if (orig->tz_info) {
		tmp->tz_info = orig->tz_info;
	}
	return tmp;
}

timelib_rel_time* timelib_rel_time_ctor(void)
{
	timelib_rel_time *t;
	t = timelib_calloc(1, sizeof(timelib_rel_time));

	return t;
}

void timelib_rel_time_dtor(timelib_rel_time* t)
{
	TIMELIB_TIME_FREE(t);
}

timelib_rel_time* timelib_rel_time_clone(timelib_rel_time *rel)
{
	timelib_rel_time *tmp = timelib_rel_time_ctor();
	memcpy(tmp, rel, sizeof(timelib_rel_time));
	return tmp;
}

void timelib_time_tz_abbr_update(timelib_time* tm, char* tz_abbr)
{
	unsigned int i;
	size_t tz_abbr_len = strlen(tz_abbr);

	TIMELIB_TIME_FREE(tm->tz_abbr);
	tm->tz_abbr = timelib_strdup(tz_abbr);
	for (i = 0; i < tz_abbr_len; i++) {
		tm->tz_abbr[i] = toupper(tz_abbr[i]);
	}
}

timelib_time_offset* timelib_time_offset_ctor(void)
{
	timelib_time_offset *t;
	t = timelib_calloc(1, sizeof(timelib_time_offset));

	return t;
}

void timelib_time_offset_dtor(timelib_time_offset* t)
{
	TIMELIB_TIME_FREE(t->abbr);
	TIMELIB_TIME_FREE(t);
}

char *timelib_get_tz_abbr_ptr(timelib_time *t)
{
	if (!t->sse_uptodate) {
		timelib_update_ts(t, NULL);
	};
	return t->tz_abbr;
}

void timelib_error_container_dtor(timelib_error_container *errors)
{
	int i;

	for (i = 0; i < errors->warning_count; i++) {
		timelib_free(errors->warning_messages[i].message);
	}
	timelib_free(errors->warning_messages);
	for (i = 0; i < errors->error_count; i++) {
		timelib_free(errors->error_messages[i].message);
	}
	timelib_free(errors->error_messages);
	timelib_free(errors);
}

timelib_long timelib_date_to_int(timelib_time *d, int *error)
{
	timelib_sll ts;

	ts = d->sse;

	if (ts < TIMELIB_LONG_MIN || ts > TIMELIB_LONG_MAX) {
		if (error) {
			*error = 1;
		}
		return 0;
	}
	if (error) {
		*error = 0;
	}
	return (timelib_long) d->sse;
}

void timelib_decimal_hour_to_hms(double h, int *hour, int *min, int *sec)
{
	if (h > 0) {
		*hour = floor(h);
		*min = floor((h - *hour) * 60);
		*sec = (h - *hour - ((float) *min / 60)) * 3600;
	} else {
		*hour = ceil(h);
		*min = 0 - ceil((h - *hour) * 60);
		*sec = 0 - (h - *hour - ((float) *min / -60)) * 3600;
	}
}

void timelib_hms_to_decimal_hour(int hour, int min, int sec, double *h)
{
	if (hour > 0) {
		*h = ((double)hour + (double)min / 60 + (double)sec / 3600);
	} else {
		*h = ((double)hour - (double)min / 60 - (double)sec / 3600);
	}
}

void timelib_dump_date(timelib_time *d, int options)
{
	if ((options & 2) == 2) {
		printf("TYPE: %d ", d->zone_type);
	}
	printf("TS: %lld | %s%04lld-%02lld-%02lld %02lld:%02lld:%02lld",
		d->sse, d->y < 0 ? "-" : "", TIMELIB_LLABS(d->y), d->m, d->d, d->h, d->i, d->s);
	if (d->us > 0) {
		printf(" 0.%06lld", d->us);
	}

	if (d->is_localtime) {
		switch (d->zone_type) {
			case TIMELIB_ZONETYPE_OFFSET: /* Only offset */
				printf(" GMT %05d%s", d->z, d->dst == 1 ? " (DST)" : "");
				break;
			case TIMELIB_ZONETYPE_ID: /* Timezone struct */
				/* Show abbreviation if wanted */
				if (d->tz_abbr) {
					printf(" %s", d->tz_abbr);
				}
				/* Do we have a TimeZone struct? */
				if (d->tz_info) {
					printf(" %s", d->tz_info->name);
				}
				break;
			case TIMELIB_ZONETYPE_ABBR:
				printf(" %s", d->tz_abbr);
				printf(" %05d%s", d->z, d->dst == 1 ? " (DST)" : "");
				break;
		}
	}

	if ((options & 1) == 1) {
		if (d->have_relative) {
			printf("%3lldY %3lldM %3lldD / %3lldH %3lldM %3lldS",
				d->relative.y, d->relative.m, d->relative.d, d->relative.h, d->relative.i, d->relative.s);
			if (d->relative.us) {
				printf(" 0.%06lld", d->relative.us);
			}
			if (d->relative.first_last_day_of != 0) {
				switch (d->relative.first_last_day_of) {
					case 1:
						printf(" / first day of");
						break;
					case 2:
						printf(" / last day of");
						break;
				}
			}
			if (d->relative.have_weekday_relative) {
				printf(" / %d.%d", d->relative.weekday, d->relative.weekday_behavior);
			}
			if (d->relative.have_special_relative) {
				switch (d->relative.special.type) {
					case TIMELIB_SPECIAL_WEEKDAY:
						printf(" / %lld weekday", d->relative.special.amount);
						break;
					case TIMELIB_SPECIAL_DAY_OF_WEEK_IN_MONTH:
						printf(" / x y of z month");
						break;
					case TIMELIB_SPECIAL_LAST_DAY_OF_WEEK_IN_MONTH:
						printf(" / last y of z month");
						break;
				}
			}
		}
	}
	printf("\n");
}

void timelib_dump_rel_time(timelib_rel_time *d)
{
	printf("%3lldY %3lldM %3lldD / %3lldH %3lldM %3lldS (days: %lld)%s",
		d->y, d->m, d->d, d->h, d->i, d->s, d->days, d->invert ? " inverted" : "");
	if (d->first_last_day_of != 0) {
		switch (d->first_last_day_of) {
			case 1:
				printf(" / first day of");
				break;
			case 2:
				printf(" / last day of");
				break;
		}
	}
	printf("\n");
}
