/*
 * cdjpeg.c
 *
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains common support routines used by the IJG application
 * programs (cjpeg, djpeg, jpegtran).
 */
#include <slib-internal.h>
#pragma hdrstop
#define JPEG_INTERNALS
#include "cdjpeg.h"
/*
 * Signal catcher to ensure that temporary files are removed before aborting.
 * NB: for Amiga Manx C this is actually a global routine named _abort();
 * we put "#define signal_catcher _abort" in jconfig.h.  Talk about bogus...
 */

#ifdef NEED_SIGNAL_CATCHER

static j_common_ptr sig_cinfo;

void signal_catcher(int signum) /* must be global for Manx C */
{
	if(sig_cinfo) {
		if(sig_cinfo->err) // turn off trace output 
			sig_cinfo->err->trace_level = 0;
		jpeg_destroy(sig_cinfo); /* clean up memory allocation & temp files */
	}
	exit(EXIT_FAILURE);
}

void  enable_signal_catcher(j_common_ptr cinfo)
{
	sig_cinfo = cinfo;
#ifdef SIGINT                   /* not all systems have SIGINT */
	signal(SIGINT, signal_catcher);
#endif
#ifdef SIGTERM                  /* not all systems have SIGTERM */
	signal(SIGTERM, signal_catcher);
#endif
}

#endif

/*
 * Optional progress monitor: display a percent-done figure on stderr.
 */

#ifdef PROGRESS_REPORT

METHODDEF(void) progress_monitor(j_common_ptr cinfo)
{
	cd_progress_ptr prog = (cd_progress_ptr)cinfo->progress;
	int total_passes = prog->pub.total_passes + prog->total_extra_passes;
	int percent_done = (int)(prog->pub.pass_counter*100L/prog->pub.pass_limit);
	if(percent_done != prog->percent_done) {
		prog->percent_done = percent_done;
		if(total_passes > 1) {
			slfprintf_stderr("\rPass %d/%d: %3d%% ", prog->pub.completed_passes + prog->completed_extra_passes + 1, total_passes, percent_done);
		}
		else {
			slfprintf_stderr("\r %3d%% ", percent_done);
		}
		fflush(stderr);
	}
}

void  start_progress_monitor(j_common_ptr cinfo, cd_progress_ptr progress)
{
	/* Enable progress display, unless trace output is on */
	if(cinfo->err->trace_level == 0) {
		progress->pub.progress_monitor = progress_monitor;
		progress->completed_extra_passes = 0;
		progress->total_extra_passes = 0;
		progress->percent_done = -1;
		cinfo->progress = &progress->pub;
	}
}

void  end_progress_monitor(j_common_ptr cinfo)
{
	/* Clear away progress display */
	if(cinfo->err->trace_level == 0) {
		slfprintf_stderr("\r                \r");
		fflush(stderr);
	}
}
#endif
/*
 * Case-insensitive matching of possibly-abbreviated keyword switches.
 * keyword is the constant keyword (must be lower case already),
 * minchars is length of minimum legal abbreviation.
 */
boolean keymatch(char * arg, const char * keyword, int minchars)
{
	int ca, ck;
	int nmatched = 0;
	while((ca = *arg++) != '\0') {
		if((ck = *keyword++) == '\0')
			return FALSE; /* arg longer than keyword, no good */
		if(isupper(ca)) /* force arg to lcase (assume ck is already) */
			ca = tolower(ca);
		if(ca != ck)
			return FALSE; /* no good */
		nmatched++; /* count matched characters */
	}
	/* reached end of argument; fail if it's too short for unique abbrev */
	if(nmatched < minchars)
		return FALSE;
	return TRUE; /* A-OK */
}
/*
 * Routines to establish binary I/O mode for stdin and stdout.
 * Non-Unix systems often require some hacking to get out of text mode.
 */
FILE * read_stdin()
{
	FILE * input_file = stdin;
#ifdef USE_SETMODE              /* need to hack file mode? */
	setmode(fileno(stdin), O_BINARY);
#endif
#ifdef USE_FDOPEN               /* need to re-open in binary mode? */
	if((input_file = fdopen(fileno(stdin), READ_BINARY)) == NULL) {
		slfprintf_stderr("Cannot reopen stdin\n");
		exit(EXIT_FAILURE);
	}
#endif
	return input_file;
}

FILE * write_stdout()
{
	FILE * output_file = stdout;
#ifdef USE_SETMODE              /* need to hack file mode? */
	setmode(fileno(stdout), O_BINARY);
#endif
#ifdef USE_FDOPEN               /* need to re-open in binary mode? */
	if((output_file = fdopen(fileno(stdout), WRITE_BINARY)) == NULL) {
		slfprintf_stderr("Cannot reopen stdout\n");
		exit(EXIT_FAILURE);
	}
#endif
	return output_file;
}
