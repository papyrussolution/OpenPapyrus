// GNUPLOT - time.c 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
/* This module provides a set of routines to read or write times using
 * human-friendly units (hours, days, years, etc).  Unlike similar
 * ansi routines, these ones support sub-second precision and relative
 * times.  E.g.   -1.23 seconds is a valid time.
 */
#include <gnuplot.h>
#pragma hdrstop

static char * read_int(char * s, int nr, int * d)
{
	int result = 0;
	while(--nr >= 0 && *s >= '0' && *s <= '9')
		result = result * 10 + (*s++ - '0');
	*d = result;
	return (s);
}

//static int mndday[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
//static size_t xstrftime(char * buf, int bsz, const char * fmt, struct tm * tm, double usec, double fulltime);
//
// days in year 
//
static int gdysize(int yr)
{
	return IsLeapYear_Gregorian(yr) ? 366 : 365;
	/*if(!(yr % 4)) {
		if((!(yr % 100)) && yr % 400)
			return (365);
		return (366);
	}
	return (365);*/
}

/* gstrptime() interprets a time_spec format string
 * and fills in a time structure that can be passed to gmtime() to
 * recover number of seconds from the EPOCH date.
 * Return value:
 * DT_TIMEDATE	indicates "normal" format elements corresponding to
 *		a date that is returned in tm, with fractional seconds
 *		returned in usec
 * DT_DMS	indicates relative time format elements were encountered
 *		(tD tH tM tS).  The relative time in seconds is returned
 *		in reltime.
 * DT_BAD	time format could not be interpreted
 *
 * parameters and return values revised for gnuplot version 5.3
 */
td_type GnuPlot::GStrPTime(char * s, char * fmt, struct tm * tm, double * usec, double * reltime)
{
	int    yday = 0;
	bool   sanity_check_date = FALSE;
	bool   reltime_formats = FALSE;
	bool   explicit_pm = FALSE;
	bool   explicit_am = FALSE;
	bool   leading_minus_sign = FALSE;
	tm->tm_mday = 1;
	tm->tm_mon = tm->tm_hour = tm->tm_min = tm->tm_sec = 0;
	// make relative times work (user-defined tic step) 
	tm->tm_year = ZERO_YEAR;
	init_timezone(tm);
	// Fractional seconds will be returned separately, since
	// there is no slot for the fraction in struct tm.
	*usec = 0.0;
	// we do not yet calculate wday or yday, so make them illegal [but yday will be read by %j]
	tm->tm_yday = tm->tm_wday = -1;
	// If the format requests explicit day, month, or year, then we will
	// do sanity checking to make sure the input makes sense.
	// For backward compatibility with gnuplot versions through 4.6.6
	// hour, minute, seconds default to zero with no error return
	// if the corresponding field cannot be found or interpreted.
	if(strstr(fmt, "%d")) {
		tm->tm_mday = -1;
		sanity_check_date = TRUE;
	}
	if(strstr(fmt, "%y") || strstr(fmt, "%Y")) {
		tm->tm_year = -1;
		sanity_check_date = TRUE;
	}
	if(strstr(fmt, "%m") || strstr(fmt, "%B") || strstr(fmt, "%b")) {
		tm->tm_mon = -1;
		sanity_check_date = TRUE;
	}
	// Relative time formats tD tH tM tS cannot be mixed with date formats 
	if(strstr(fmt, "%t")) {
		reltime_formats = TRUE;
		*reltime = 0.0;
		sanity_check_date = FALSE;
	}
	while(*fmt) {
		if(*fmt != '%') {
			if(*fmt == ' ') {
				/* space in format means zero or more spaces in input */
				while(*s == ' ')
					++s;
				++fmt;
				continue;
			}
			else if(*fmt == *s) {
				++s;
				++fmt;
				continue;
			}
			else
				break; // literal match has failed 
		}
		// we are processing a percent escape 
		switch(*++fmt) {
			case 'b': // abbreviated month name 
		    {
			    int m;
			    for(m = 0; m < 12; ++m) {
				    if(strncasecmp(s, abbrev_month_names[m], strlen(abbrev_month_names[m])) == 0) {
					    s += strlen(abbrev_month_names[m]);
					    goto found_abbrev_mon;
				    }
				}
			    // get here => not found 
			    IntWarn(DATAFILE, "Bad abbreviated month name");
			    m = 0;
found_abbrev_mon:
			    tm->tm_mon = m;
			    break;
		    }
			case 'B': // full month name 
		    {
			    int m;
			    for(m = 0; m < 12; ++m)
				    if(strncasecmp(s, full_month_names[m], strlen(full_month_names[m])) == 0) {
					    s += strlen(full_month_names[m]);
					    goto found_full_mon;
				    }
			    // get here => not found 
			    IntWarn(DATAFILE, "Bad full month name");
			    m = 0;
found_full_mon:
			    tm->tm_mon = m;
			    break;
		    }
			case 'd': // read a day of month 
			    s = read_int(s, 2, &tm->tm_mday);
			    break;
			case 'm': // month number 
			    s = read_int(s, 2, &tm->tm_mon);
			    --tm->tm_mon;
			    break;
			case 'y': // year number 
			    s = read_int(s, 2, &tm->tm_year);
			    // In line with the current UNIX98 specification by
			    // The Open Group and major Unix vendors,
			    // two-digit years 69-99 refer to the 20th century, and
			    // values in the range 00-68 refer to the 21st century.
			    if(tm->tm_year <= 68)
				    tm->tm_year += 100;
			    tm->tm_year += 1900;
			    break;
			case 'Y':
			    s = read_int(s, 4, &tm->tm_year);
			    break;
			case 'j':
			    s = read_int(s, 3, &tm->tm_yday);
			    tm->tm_yday--;
			    sanity_check_date = TRUE;
			    yday++;
			    break;
			case 'H':
			    s = read_int(s, 2, &tm->tm_hour);
			    break;
			case 'M':
			    s = read_int(s, 2, &tm->tm_min);
			    break;
			case 'S':
			    s = read_int(s, 2, &tm->tm_sec);
			    if(*s == '.' || (GpU.decimalsign && *s == *GpU.decimalsign))
				    *usec = satof(s);
			    break;
			case 's':
			    // read EPOCH data
			    // EPOCH is the std. unix timeformat seconds since 01.01.1970 UTC
				{
					char  * fraction = strchr(s, GpU.decimalsign ? *GpU.decimalsign : '.');
					double ufraction = 0;
					double when = strtod(s, &s) - SEC_OFFS_SYS;
					GGmTime(tm, when);
					if(fraction && fraction < s)
						ufraction = satof(fraction);
					if(ufraction < 1.) /* Filter out e.g. 123.456e7 */
						*usec = ufraction;
					*reltime = when; /* not used unless we return DT_DMS ... */
					if(when < 0) /* ... which we force for negative times */
						reltime_formats = TRUE;
				}
				break;
			case 't':
			    // Relative time formats tD tH tM tS 
				{
					double cont = 0;
					// Special case of negative time with first field 0, e.g.  -00:12:34
					if(*reltime == 0) {
						while(isspace(*s)) 
							s++;
						if(*s == '-')
							leading_minus_sign = TRUE;
					}
					fmt++;
					if(*fmt == 'D')
						cont = 86400.0 * strtod(s, &s);
					else if(*fmt == 'H')
						cont = 3600.0 * strtod(s, &s);
					else if(*fmt == 'M')
						cont = 60.0 * strtod(s, &s);
					else if(*fmt == 'S')
						cont = strtod(s, &s);
					else
						return DT_BAD;
					if(*reltime < 0)
						*reltime -= fabs(cont);
					else if(*reltime > 0)
						*reltime += fabs(cont);
					else if(leading_minus_sign)
						*reltime -= fabs(cont);
					else
						*reltime = cont;
					// FIXME:  leading precision field should be accepted but ignored 
				}
				break;
			case 'a': /* weekday name (ignored) */
			case 'A': /* weekday name (ignored) */
			    while(isalpha(*s))
				    s++;
			    break;
			case 'w': /* one or two digit weekday number (ignored) */
			case 'W': /* one or two digit week number (ignored) */
			    if(isdigit(*s))
				    s++;
			    if(isdigit(*s))
				    s++;
			    break;
			case 'p': /* am or pm */
			    if(!strncmp(s, "pm", 2) || !strncmp(s, "PM", 2))
				    explicit_pm = TRUE;
			    if(!strncmp(s, "am", 2) || !strncmp(s, "AM", 2))
				    explicit_am = TRUE;
			    s += 2;
			    break;
#ifdef HAVE_STRUCT_TM_TM_GMTOFF
			case 'z': /* timezone offset  */
				{
					int neg = (*s == '-') ? -1 : 1;
					int off_h, off_m;
					if(*s == '-' || *s == '+')
						s++;
					s = read_int(s, 2, &off_h);
					if(*s == ':')
						s++;
					s = read_int(s, 2, &off_m);
					tm->tm_gmtoff = 3600*off_h + 60*off_m;
					tm->tm_gmtoff *= neg;
				}
				break;
			case 'Z': /* timezone name (ignored) */
			    while(*s && !isspace(*s))
				    s++;
			    break;
#endif
			default:
			    IntWarn(DATAFILE, "Bad time format %%%c", *fmt);
		}
		fmt++;
	}
	// Relative times are easy.  Just return the value in reltime 
	if(reltime_formats) {
		return DT_DMS;
	}
	FPRINTF((stderr, "read date-time : %02d/%02d/%d:%02d:%02d:%02d\n", tm->tm_mday, tm->tm_mon + 1, tm->tm_year, tm->tm_hour,
	    tm->tm_min, tm->tm_sec));
	// apply AM/PM correction 
	if((tm->tm_hour < 12) && explicit_pm)
		tm->tm_hour += 12;
	if((tm->tm_hour == 12) && explicit_am)
		tm->tm_hour = 0;
	// now sanity check the date/time entered, normalising if necessary
	// read_int cannot read a -ve number, but can read %m=0 then decrement it to -1
#define S (tm->tm_sec)
#define M (tm->tm_min)
#define H (tm->tm_hour)
	if(S >= 60) {
		M += S / 60;
		S %= 60;
	}
	if(M >= 60) {
		H += M / 60;
		M %= 60;
	}
	if(H >= 24) {
		if(yday)
			tm->tm_yday += H / 24;
		tm->tm_mday += H / 24;
		H %= 24;
	}
#undef S
#undef M
#undef H
	FPRINTF((stderr, "normalised time : %02d/%02d/%d:%02d:%02d:%02d\n", tm->tm_mday, tm->tm_mon + 1, tm->tm_year, tm->tm_hour,
	    tm->tm_min, tm->tm_sec));
	if(sanity_check_date) {
		if(yday) {
			if(tm->tm_yday < 0) {
				return DT_BAD;
			}
			// we just set month to jan, day to yday, and let the
			// normalising code do the work.
			tm->tm_mon = 0;
			// yday is 0->365, day is 1->31 
			tm->tm_mday = tm->tm_yday + 1;
		}
		if(tm->tm_mon < 0 || tm->tm_mday < 1)
			return DT_BAD;
		else {
			if(tm->tm_mon > 11) {
				tm->tm_year += tm->tm_mon / 12;
				tm->tm_mon %= 12;
			}
			{
				int days_in_month;
				while(tm->tm_mday > (days_in_month = (/*mndday*/daysPerMonth[tm->tm_mon] + (tm->tm_mon == 1 && (gdysize(tm->tm_year) > 365))))) {
					if(++tm->tm_mon == 12) {
						++tm->tm_year;
						tm->tm_mon = 0;
					}
					tm->tm_mday -= days_in_month;
				}
			}
		}
	}
	return DT_TIMEDATE;
}

size_t GnuPlot::GStrFTime(char * pS, size_t bsz, const char * pFmt, double l_clock)
{
	struct tm tm;
	GGmTime(&tm, l_clock);
	double usec = l_clock - (double)floor(l_clock);
	return XStrFTime(pS, bsz, pFmt, &tm, usec, l_clock);
}

size_t GnuPlot::XStrFTime(char * pStr/* output buffer */, int bsz/* remaining space available in buffer */, const char * pFmt, struct tm * pTm, double usec, double fulltime)
{
	size_t l = 0; // chars written so far 
	int incr = 0; // chars just written 
	char * s = pStr;
	bool sign_printed = FALSE;
	if(bsz <= 0)
		return 0;
	memzero(pStr, bsz);
	while(*pFmt != '\0') {
		if(*pFmt != '%') {
			if(static_cast<int>(l) >= (bsz-1))
				return 0;
			*s++ = *pFmt++;
			l++;
		}
		else {
			// set up format modifiers 
			int w = 0;
			int z = 0;
			int p = 0;
			if(*++pFmt == '0') {
				z = 1;
				++pFmt;
			}
			while(*pFmt >= '0' && *pFmt <= '9') {
				w = w * 10 + (*pFmt - '0');
				++pFmt;
			}
			if(*pFmt == '.') {
				++pFmt;
				while(*pFmt >= '0' && *pFmt <= '9') {
					p = p * 10 + (*pFmt - '0');
					++pFmt;
				}
				SETMIN(p, 6);
			}
			switch(*pFmt++) {
				// some shorthands : check that there is space in the output string. 
#define CHECK_SPACE(n) do { if(static_cast<int>(l+(n)) > bsz) return 0; } while(0)
// copy a fixed string, checking that there's room 
#define COPY_STRING(z) do { CHECK_SPACE(strlen(z)); strcpy(s, z); } while(0)

				/* format a string, using default spec if none given w
				 * and z are width and zero-flag dw and dz are the
				 * defaults for these In fact, CHECK_SPACE(w) is not a
				 * sufficient test, since sprintf("%2d", 365) outputs
				 * three characters
				 */
#define FORMAT_STRING(dz, dw, x) do {                           \
		if(w == 0) {                                 \
			w = (dw);                                 \
			SETIFZ(z, (dz));                             \
		}                                           \
		incr = snprintf(s, bsz-l-1, z ? "%0*d" : "%*d", w, (x));    \
		CHECK_SPACE(incr);                          \
} while(0)

				case '%':
				    CHECK_SPACE(1);
				    *s = '%';
				    break;
				case 'a':
				    COPY_STRING(abbrev_day_names[pTm->tm_wday]);
				    break;
				case 'A':
				    COPY_STRING(full_day_names[pTm->tm_wday]);
				    break;
				case 'b':
				case 'h':
				    COPY_STRING(abbrev_month_names[pTm->tm_mon]);
				    break;
				case 'B':
				    COPY_STRING(full_month_names[pTm->tm_mon]);
				    break;
				case 'd':
				    FORMAT_STRING(1, 2, pTm->tm_mday); /* %02d */
				    break;
				case 'D':
				    if(!XStrFTime(s, bsz - l, "%m/%d/%y", pTm, 0.0, fulltime))
					    return 0;
				    break;
				case 'F':
				    if(!XStrFTime(s, bsz - l, "%Y-%m-%d", pTm, 0.0, fulltime))
					    return 0;
				    break;
				case 'H':
				    FORMAT_STRING(1, 2, pTm->tm_hour); /* %02d */
				    break;
				case 'I':
				    FORMAT_STRING(1, 2, (pTm->tm_hour + 11) % 12 + 1); /* %02d */
				    break;
				case 'j':
				    FORMAT_STRING(1, 3, pTm->tm_yday + 1); /* %03d */
				    break;
				// not in linux strftime man page. Not really needed now 
				case 'k':
				    FORMAT_STRING(0, 2, pTm->tm_hour); /* %2d */
				    break;
				case 'l':
				    FORMAT_STRING(0, 2, (pTm->tm_hour + 11) % 12 + 1); /* %2d */
				    break;
				case 'm':
				    FORMAT_STRING(1, 2, pTm->tm_mon + 1); /* %02d */
				    break;
				case 'M':
				    FORMAT_STRING(1, 2, pTm->tm_min); /* %02d */
				    break;
				case 'p':
				    CHECK_SPACE(2);
				    strcpy(s, (pTm->tm_hour < 12) ? "am" : "pm");
				    break;
				case 'r':
				    if(!XStrFTime(s, bsz - l, "%I:%M:%S %p", pTm, 0.0, fulltime))
					    return 0;
				    break;
				case 'R':
				    if(!XStrFTime(s, bsz - l, "%H:%M", pTm, 0.0, fulltime))
					    return 0;
				    break;
				case 's':
				    CHECK_SPACE(12); // large enough for year 9999 
				    sprintf(s, "%.0f", gtimegm(pTm));
				    break;
				case 'S':
				    FORMAT_STRING(1, 2, pTm->tm_sec); /* %02d */
				    // EAM FIXME - need to implement an actual format specifier 
				    if(p > 0) {
					    double base = pow(10., (double)p);
					    int msec = ffloori(0.5 + base * usec);
					    char * f = &s[strlen(s)];
					    CHECK_SPACE(p+1);
					    sprintf(f, ".%0*d", p, msec<(int)base ? msec : (int)base-1);
				    }
				    break;
				case 'T':
				    if(!XStrFTime(s, bsz - l, "%H:%M:%S", pTm, 0.0, fulltime))
					    return 0;
				    break;
				case 't': // Time (as opposed to Date) formats 
			    {
				    int tminute, tsecond;
				    switch(*pFmt++) {
					    case 'D':
						// +/- fractional days 
						if(p > 0) {
							incr = snprintf(s, bsz-l-1, "%*.*f", w, p, fulltime/86400.);
							CHECK_SPACE(incr);
							break;
						}
						// Set flag in case hours come next 
						if(fulltime < 0) {
							CHECK_SPACE(1); // the minus sign 
							*s++ = '-';
							l++;
						}
						sign_printed = TRUE;
						// +/- integral day truncated toward zero 
						sprintf(s, "%0*d", w, (int)floor(fabs(fulltime/86400.0)));
						/* Subtract the day component from the total */
						fulltime -= sgn(fulltime) * 86400. * floor(fabs(fulltime/86400.0));
						break;
					    case 'H':
						/* +/- fractional hours (not wrapped at 24h) */
						if(p > 0) {
							incr = snprintf(s, bsz-l-1, "%*.*f", w, p, sign_printed ? fabs(fulltime)/3600. : fulltime/3600.);
							CHECK_SPACE(incr);
							break;
						}
						/* Set flag in case minutes come next */
						if(fulltime < 0 && !sign_printed) {
							CHECK_SPACE(1); /* the minus sign */
							*s++ = '-';
							l++;
						}
						sign_printed = TRUE;
						/* +/- integral hour truncated toward zero */
						sprintf(s, "%0*d", w, (int)floor(fabs(fulltime/3600.)));

						/* Subtract the hour component from the total */
						fulltime -= sgn(fulltime) * 3600. * floor(fabs(fulltime/3600.));
						break;
					    case 'M':
						/* +/- fractional minutes (not wrapped at 60m) */
						if(p > 0) {
							incr = snprintf(s, bsz-l-1, "%*.*f", w, p, sign_printed ? fabs(fulltime)/60. : fulltime/60.);
							CHECK_SPACE(incr);
							break;
						}
						/* +/- integral minute truncated toward zero */
						tminute = ffloori((fabs(fulltime/60.)));
						if(fulltime < 0 && !sign_printed) {
							*s++ = '-';
							l++;
						}
						sign_printed = TRUE;
						FORMAT_STRING(1, 2, tminute); /* %02d */

						/* Subtract the minute component from the total */
						fulltime -= sgn(fulltime) * 60. * floor(fabs(fulltime/60.));
						break;
					    case 'S':
						/* +/- fractional seconds */
						tsecond = ffloori(fabs(fulltime));
						if(fulltime < 0) {
							if(usec > 0)
								usec = 1.0 - usec;
							if(!sign_printed) {
								*s++ = '-';
								l++;
							}
						}
						FORMAT_STRING(1, 2, tsecond); /* %02d */
						if(p > 0) {
							double base = pow(10., (double)p);
							int msec = ffloori(0.5 + base * usec);
							char * f = &s[strlen(s)];
							CHECK_SPACE(p+1);
							sprintf(f, ".%0*d", p, msec<(int)base ? msec : (int)base-1);
						}
						break;
					    default:
						break;
				    }
				    break;
			    }
				case 'W': /* Mon..Sun is day 0..6 */
				    // CHANGE Jan 2021
				    // Follow ISO 8601 standard week date convention.
					{
						int week = TmWeek(fulltime);
						FORMAT_STRING(1, 2, week); /* %02d */
					}
					break;
				case 'U': /* sun 1 day of week */
			    {
				    int week, bw;
				    if(pTm->tm_yday <= pTm->tm_wday) {
					    week = 1;
					    if((pTm->tm_mday - pTm->tm_yday) > 4) {
						    week = 52;
					    }
				    }
				    else {
					    // sat prev week 
					    bw = pTm->tm_yday - pTm->tm_wday - 1;
					    if(pTm->tm_wday >= 0)
						    bw += 7; /* sat end of week */
					    week = (int)bw / 7;
					    if((bw % 7) > 1) { /* jan 1 is before friday */
						    week++;
					    }
				    }
				    FORMAT_STRING(1, 2, week); /* %02d */
				    break;
			    }
				case 'w': /* day of week, sun=0 */
				    FORMAT_STRING(1, 2, pTm->tm_wday); /* %02d */
				    break;
				case 'y':
				    FORMAT_STRING(1, 2, pTm->tm_year % 100); /* %02d */
				    break;
				case 'Y':
				    FORMAT_STRING(1, 4, pTm->tm_year); /* %04d */
				    break;
			}       /* switch */
			while(*s != '\0') {
				s++;
				l++;
			}
#undef CHECK_SPACE
#undef COPY_STRING
#undef FORMAT_STRING
		} /* switch(fmt letter) */
	} /* if(fmt letter not '%') */
	return (l);
}

/* time_t  */
double gtimegm(struct tm * tm)
{
	int i;
	/* returns sec from year ZERO_YEAR in UTC, defined in gp_time.h */
	double dsec = 0.;
	if(tm->tm_year < ZERO_YEAR) {
		for(i = tm->tm_year; i < ZERO_YEAR; i++) {
			dsec -= (double)gdysize(i);
		}
	}
	else {
		for(i = ZERO_YEAR; i < tm->tm_year; i++) {
			dsec += (double)gdysize(i);
		}
	}
	if(tm->tm_mday > 0) {
		for(i = 0; i < tm->tm_mon; i++) {
			dsec += (double)/*mndday*/daysPerMonth[i] + (i == 1 && (gdysize(tm->tm_year) > 365));
		}
		dsec += (double)tm->tm_mday - 1;
	}
	else {
		dsec += (double)tm->tm_yday;
	}
	dsec *= (double)24;
	dsec += tm->tm_hour;
	dsec *= 60.0;
	dsec += tm->tm_min;
	dsec *= 60.0;
	dsec += tm->tm_sec;

#ifdef HAVE_STRUCT_TM_TM_GMTOFF
	dsec -= tm->tm_gmtoff;
	FPRINTF((stderr, "broken-down time : %02d/%02d/%d:%02d:%02d:%02d:(%02d:%02d) = %g seconds\n",
	    tm->tm_mday, tm->tm_mon + 1, tm->tm_year, tm->tm_hour,
	    tm->tm_min, tm->tm_sec, tm->tm_gmtoff / 3600, (tm->tm_gmtoff % 3600) / 60, dsec));
#endif
	return (dsec);
}

//int ggmtime(struct tm * pTm, double l_clock)
int GnuPlot::GGmTime(struct tm * pTm, double l_clock)
{
	// l_clock is relative to ZERO_YEAR, jan 1, 00:00:00,defined in plot.h 
	int i, days;
	// dodgy way of doing wday - i hope it works ! 
	int wday = JAN_FIRST_WDAY; /* eg 6 for 2000 */
	FPRINTF((stderr, "%g seconds = ", l_clock));
	if(fabs(l_clock) > 1.e12) { /* Some time in the year 33688 */
		IntWarn(NO_CARET, "time value out of range");
		return -1;
	}
	else {
		pTm->tm_year = ZERO_YEAR;
		pTm->tm_mday = pTm->tm_yday = pTm->tm_mon = pTm->tm_hour = pTm->tm_min = pTm->tm_sec = 0;
		init_timezone(pTm);
		if(l_clock >= 0) {
			for(;;) {
				int days_in_year = gdysize(pTm->tm_year);
				if(l_clock < days_in_year * SlConst::SecsPerDayR)
					break;
				l_clock -= days_in_year * SlConst::SecsPerDayR;
				pTm->tm_year++;
				// only interested in result modulo 7, but %7 is expensive 
				wday += (days_in_year - 364);
			}
		}
		else {
			while(l_clock < 0) {
				int days_in_year = gdysize(--pTm->tm_year);
				l_clock += days_in_year * SlConst::SecsPerDayR; // 24*3600 
				// adding 371 is noop in modulo 7 arithmetic, but keeps wday +ve 
				wday += 371 - days_in_year;
			}
		}
		pTm->tm_yday = (int)(l_clock / SlConst::SecsPerDayR);
		l_clock -= pTm->tm_yday * SlConst::SecsPerDayR;
		pTm->tm_hour = (int)l_clock / 3600;
		l_clock -= pTm->tm_hour * 3600;
		pTm->tm_min = (int)l_clock / 60;
		l_clock -= pTm->tm_min * 60;
		pTm->tm_sec = (int)l_clock;
		days = pTm->tm_yday;
		pTm->tm_wday = (wday + days) % 7; /* wday%7 should be day of week of first day of year */
		while(days >= (i = /*mndday*/daysPerMonth[pTm->tm_mon] + (pTm->tm_mon == 1 && (gdysize(pTm->tm_year) > 365)))) {
			days -= i;
			pTm->tm_mon++;
			// This catches round-off error that initially assigned a date to year N-1 
			// but counting out seconds puts it in the first second of Jan year N.     
			if(pTm->tm_mon > 11) {
				pTm->tm_mon = 0;
				pTm->tm_year++;
			}
		}
		pTm->tm_mday = days + 1;
		FPRINTF((stderr, "broken-down time : %02d/%02d/%d:%02d:%02d:%02d\n", pTm->tm_mday, pTm->tm_mon + 1, pTm->tm_year, pTm->tm_hour, pTm->tm_min, pTm->tm_sec));
		return 0;
	}
}
// 
// ISO 8601 week date standard
// return week of year given time in seconds
// Notes:
// Each week runs from Monday to Sunday.
// The first week of the year starts on the Monday closest to 1 January.
// Corollaries: Up to three days in Week 1 may be in previous year.
// The last week of a year may extend into the next calendar year.
// The highest week number in a year is either 52 or 53.
// 
int GnuPlot::TmWeek(double time)
{
	struct tm tm;
	int wday, week;
	// Fill time structure from time since epoch in seconds 
	GGmTime(&tm, time);
	// ISO C tm->tm_wday using 0 = Sunday but we want 0 = Monday 
	wday = (6 + tm.tm_wday) % 7;
	week = (int)(10 + tm.tm_yday - wday ) / 7;
	// Up to three days in December may belong in week 1 of the next year. 
	if(tm.tm_mon == 11) {
		if((tm.tm_mday == 31 && wday < 3) || (tm.tm_mday == 30 && wday < 2) || (tm.tm_mday == 29 && wday < 1))
			week = 1;
	}
	// Up to three days in January may be in the last week of the previous year.
	// That might be either week 52 or week 53 depending on the leap year cycle.
	if(week == 0) {
		struct tm temp = tm;
		int Jan01, Dec31;
		// Was either 1 Jan or 31 Dec of the previous year a Thursday? 
		temp.tm_year -= 1; temp.tm_mon = 0; temp.tm_mday = 1;
		GGmTime(&temp, gtimegm(&temp) );
		Jan01 = temp.tm_wday;
		temp.tm_mon = 11; temp.tm_mday = 31;
		GGmTime(&temp, gtimegm(&temp) );
		Dec31 = temp.tm_wday;
		if(Jan01 == 4 || Dec31 == 4)
			week = 53;
		else
			week = 52;
	}
	return week;
}
