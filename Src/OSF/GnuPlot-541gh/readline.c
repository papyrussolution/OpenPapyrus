// GNUPLOT - readline.c 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
/*
 * AUTHORS
 *   Original Software: Tom Tkacik
 *   Msdos port and some enhancements: Gershon Elber and many others.
 *   Adapted to work with UTF-8 encoding. Ethan A Merritt  April 2011
 */
#include <gnuplot.h>
#pragma hdrstop
#ifdef WGP_CONSOLE
	#include "win/winmain.h"
#endif
// 
// adaptor routine for gnu libreadline
// to allow multiplexing terminal and mouse input
// 
#if defined(HAVE_LIBREADLINE) || defined(HAVE_LIBEDITLINE)
int getc_wrapper(FILE * fp)
{
	int c;
	while(1) {
		errno = 0;
#ifdef USE_MOUSE
		/* EAM June 2020:
		 * For 20 years this was conditional on interactive.
		 *    if(GPT.P_Term && GPT.P_Term->waitforinput && interactive)
		 * Now I am suspicious that this was the cause of dropped
		 * characters when mixing piped input with mousing,
		 * e.g. Bugs 2134, 2279
		 */
		if(GPT.P_Term && GPT.P_Term->waitforinput) {
			c = GPT.P_Term->waitforinput(GPT.P_Term, 0);
		}
		else
#endif
#if defined(WGP_CONSOLE)
		c = ConsoleGetch();
#else
		if(fp)
			c = getc(fp);
		else
			c = getchar(); /* HAVE_LIBEDITLINE */
		if(c == EOF && errno == EINTR)
			continue;
#endif
		return c;
	}
}

#endif /* HAVE_LIBREADLINE || HAVE_LIBEDITLINE */
#if defined(HAVE_LIBREADLINE) && defined(HAVE_READLINE_SIGNAL_HANDLER)
	/*
	 * The signal handler of libreadline sets a flag when SIGTSTP is received
	 * but does not suspend until this flag is checked by other library
	 * routines.  Since gnuplot's GPT.P_Term->waitforinput() + getc_wrapper()
	 * replace these other routines, we must do the test and suspend ourselves.
	 */
	void wrap_readline_signal_handler()
	{
		int sig;
		// FIXME:	At the moment, there is no portable way to invoke the signal handler. 
		extern void _rl_signal_handler(int);
	#ifdef HAVE_READLINE_PENDING_SIGNAL
		sig = rl_pending_signal();
	#else
		// XXX: We assume all versions of readline have this... 
		extern int volatile _rl_caught_signal;
		sig = _rl_caught_signal;
	#endif
		if(sig) 
			_rl_signal_handler(sig);
	}
#endif /* defined(HAVE_LIBREADLINE) && defined(HAVE_READLINE_SIGNAL_HANDLER) */

#ifdef READLINE

/* This is a small portable version of GNU's readline that does not require
 * any terminal capabilities except backspace and space overwrites a character.
 * It is not the BASH or GNU EMACS version of READLINE due to Copyleft
 * restrictions.
 * Configuration option:   ./configure --with-readline=builtin
 */

/* NANO-EMACS line editing facility
 * printable characters print as themselves (insert not overwrite)
 * ^A moves to the beginning of the line
 * ^B moves back a single character
 * ^E moves to the end of the line
 * ^F moves forward a single character
 * ^K kills from current position to the end of line
 * ^P moves back through history
 * ^N moves forward through history
 * ^H deletes the previous character
 * ^D deletes the current character, or EOF if line is empty
 * ^L redraw line in case it gets trashed
 * ^U kills the entire line
 * ^W deletes previous full or partial word
 * ^V disables interpretation of the following key
 * LF and CR return the entire line regardless of the cursor position
 * DEL deletes previous or current character (configuration dependent)
 * TAB will perform filename completion
 * ^R start a backward-search of the history
 * EOF with an empty line returns (char *)NULL
 *
 * all other characters are ignored
 */

#ifdef HAVE_SYS_IOCTL_H
/* For ioctl() prototype under Linux (and BeOS?) */
#include <sys/ioctl.h>
#endif

/* replaces the previous kludge in configure */
#if defined(HAVE_TERMIOS_H) && defined(HAVE_TCGETATTR)
	#define TERMIOS
#else /* not HAVE_TERMIOS_H && HAVE_TCGETATTR */
	#ifdef HAVE_SGTTY_H
		#define SGTTY
	#endif
#endif /* not HAVE_TERMIOS_H && HAVE_TCGETATTR */
#if !defined(MSDOS) && !defined(_WIN32)
/*
 * Set up structures using the proper include file
 */
#if defined(_IBMR2) || defined(alliant)
	#define SGTTY
#endif
/* submitted by Francois.Dagorn@cicb.fr */
#ifdef SGTTY
	#include <sgtty.h>
	static struct sgttyb orig_termio;
	static struct sgttyb rl_termio;
	static struct tchars s_tchars; // define terminal control characters 
	#ifndef VERASE
		#define VERASE    0
	#endif                  /* not VERASE */
	#ifndef VEOF
		#define VEOF      1
	#endif                  /* not VEOF */
	#ifndef VKILL
		#define VKILL     2
	#endif                  /* not VKILL */
	#ifdef TIOCGLTC         /* available only with the 'new' line discipline */
		static struct ltchars s_ltchars;
		#ifndef VWERASE
			#define VWERASE   3
		#endif                  /* not VWERASE */
		#ifndef VREPRINT
			#define VREPRINT  4
		#endif                  /* not VREPRINT */
		#ifndef VSUSP
			#define VSUSP     5
		#endif                  /* not VSUP */
	#endif                  /* TIOCGLTC */
	#ifndef NCCS
		#define NCCS      6
	#endif                  /* not NCCS */
#else                           /* not SGTTY */
	/* SIGTSTP defines job control
	 * if there is job control then we need termios.h instead of termio.h
	 * (Are there any systems with job control that use termio.h?  I hope not.)
	 */
	#if defined(SIGTSTP) || defined(TERMIOS)
		#ifndef TERMIOS
			#define TERMIOS
		#endif                  /* not TERMIOS */
		#include <termios.h>
		/* Added by Robert Eckardt, RobertE@beta.TP2.Ruhr-Uni-Bochum.de */
		#ifdef ISC22
			#ifndef ONOCR               /* taken from sys/termio.h */
				#define ONOCR 0000020      /* true at least for ISC 2.2 */
			#endif                      /* not ONOCR */
			#ifndef IUCLC
				#define IUCLC 0001000
			#endif                      /* not IUCLC */
		#endif                  /* ISC22 */
		#if !defined(IUCLC)
			#define IUCLC 0 // translate upper to lower case not supported 
		#endif                  /* not IUCLC */
		static struct termios orig_termio, rl_termio;
	#else                           /* not SIGSTP || TERMIOS */
		#include <termio.h>
		static struct termio orig_termio, rl_termio;
		/* termio defines NCC instead of NCCS */
		#define NCCS    NCC
	#endif                  /* not SIGTSTP || TERMIOS */
#endif /* SGTTY */
/* ULTRIX defines VRPRNT instead of VREPRINT */
#if defined(VRPRNT) && !defined(VREPRINT)
	#define VREPRINT VRPRNT
#endif /* VRPRNT */
static char term_chars[NCCS]; /* define characters to use with our input character handler */
static int term_set = 0; /* =1 if rl_termio set */
#define special_getc(t) ansi_getc(t)
static int ansi_getc(GpTermEntry * pTerm);
#define DEL_ERASES_CURRENT_CHAR
#else /* MSDOS or _WIN32 */
#ifdef _WIN32
	#include "win/winmain.h"
	#include "win/wcommon.h"
	#define TEXTUSER 0xf1
	#define TEXTGNUPLOT 0xf0
	#ifdef WGP_CONSOLE
		#define special_getc(t) WinGetch(t)
		//static int win_getch();
	#else
		// The wgnuplot text window will suppress intermediate
		// screen updates in 'suspend' mode and only redraw the
	    // input line after 'resume'. 
		#define SUSPENDOUTPUT TextSuspend(&_WinM.TxtWin)
		#define RESUMEOUTPUT  TextResume(&_WinM.TxtWin)
		#define special_getc(t) MsDosGetch(t)
		//static int msdos_getch();
	#endif /* WGP_CONSOLE */
	#define DEL_ERASES_CURRENT_CHAR
#endif /* _WIN32 */
#endif /* MSDOS or _WIN32 */

// initial size and increment of input line length 
#define MAXBUF  1024
#define BACKSPACE '\b'   /* ^H */
#define SPACE   ' '
#define NEWLINE '\n'
#define MAX_COMPLETIONS 50
#ifndef SUSPENDOUTPUT
	#define SUSPENDOUTPUT
	#define RESUMEOUTPUT
#endif

static void set_termio();
static void reset_termio();
static int  user_putc(int ch);
static int  user_puts(const char * str);
#ifndef _WIN32
	static int mbwidth(const char * c);
#endif
static int strwidth(const char * str);
// 
// user_putc and user_puts should be used in the place of
// fputc(ch,stderr) and fputs(str,stderr) for all output
// of user typed characters.  This allows MS-Windows to
// display user input in a different color.
// 
static int user_putc(int ch)
{
	int rv;
#if defined(_WIN32) && !defined(WGP_CONSOLE)
	TextAttr(&_WinM.TxtWin, TEXTUSER);
#endif
	rv = fputc(ch, stderr);
#if defined(_WIN32) && !defined(WGP_CONSOLE)
	TextAttr(&_WinM.TxtWin, TEXTGNUPLOT);
#endif
	return rv;
}

static int user_puts(const char * str)
{
	int rv;
#if defined(_WIN32) && !defined(WGP_CONSOLE)
	TextAttr(&_WinM.TxtWin, TEXTUSER);
#endif
	rv = fputs(str, stderr);
#if defined(_WIN32) && !defined(WGP_CONSOLE)
	TextAttr(&_WinM.TxtWin, TEXTGNUPLOT);
#endif
	return rv;
}

#if !defined(_WIN32)
	// 
	// EAM FIXME
	// This test is intended to determine if the current character, of which
	// we have only seen the first byte so far, will require twice the width
	// of an ascii character.  The test catches glyphs above unicode 0x3000,
	// which is roughly the set of CJK characters.
	// It should be replaced with a more accurate test.
	// 
	static int mbwidth(const char * c)
	{
		switch(encoding) {
			case S_ENC_UTF8:
				return ((uchar)(*c) >= 0xe3 ? 2 : 1);
			case S_ENC_SJIS: // Assume all double-byte characters have double-width. 
				return is_sjis_lead_byte(*c) ? 2 : 1;
			default:
				return 1;
		}
	}
#endif

static int strwidth(const char * str)
{
#if !defined(_WIN32)
	int width = 0;
	int i = 0;
	switch(encoding) {
		case S_ENC_UTF8:
		    while(str[i]) {
			    const char * ch = &str[i++];
			    if((*ch & 0xE0) == 0xC0) {
				    i += 1;
			    }
			    else if((*ch & 0xF0) == 0xE0) {
				    i += 2;
			    }
			    else if((*ch & 0xF8) == 0xF0) {
				    i += 3;
			    }
			    width += mbwidth(ch);
		    }
		    break;
		case S_ENC_SJIS:
		    /* Assume all double-byte characters have double-width. */
		    width = gp_strlen(str);
		    break;
		default:
		    width = strlen(str);
	}
	return width;
#else
	// double width characters are handled in the backend 
	return gp_strlen(str);
#endif
}

int GnuPlot::IsDoubleWidth(size_t pos)
{
#if defined(_WIN32)
	return FALSE; // double width characters are handled in the backend 
#else
	return mbwidth(RlB_.P_CurLine + pos) > 1;
#endif
}
// 
// Determine length of multi-byte sequence starting at current position
//
int GnuPlot::CharSeqLen()
{
	switch(GPT._Encoding) {
		case S_ENC_UTF8: {
		    size_t i = RlB_.CurPos;
		    do {
			    i++;
		    } while(((RlB_.P_CurLine[i] & 0xc0) != 0xc0) && ((RlB_.P_CurLine[i] & 0x80) != 0) && (i < RlB_.MaxPos));
		    return (i - RlB_.CurPos);
	    }
		case S_ENC_SJIS:
		    return is_sjis_lead_byte(RlB_.P_CurLine[RlB_.CurPos]) ? 2 : 1;
		default:
		    return 1;
	}
}
//
// Back up over one multi-byte character sequence immediately preceding
// the current position.  Non-destructive.  Affects both cur_pos and screen cursor.
//
int GnuPlot::BackSpace()
{
	switch(GPT._Encoding) {
		case S_ENC_UTF8: {
		    int seqlen = 0;
		    do {
			    RlB_.CurPos--;
			    seqlen++;
		    } while(((RlB_.P_CurLine[RlB_.CurPos] & 0xc0) != 0xc0) && ((RlB_.P_CurLine[RlB_.CurPos] & 0x80) != 0) && (RlB_.CurPos > 0));
		    if(((RlB_.P_CurLine[RlB_.CurPos] & 0xc0) == 0xc0) || isprint((uchar)RlB_.P_CurLine[RlB_.CurPos]))
			    user_putc(BACKSPACE);
		    if(IsDoubleWidth(RlB_.CurPos))
			    user_putc(BACKSPACE);
		    return seqlen;
	    }
		case S_ENC_SJIS: {
		    /* With S-JIS you cannot always determine if a byte is a single byte or part
		       of a double-byte sequence by looking of an arbitrary byte in a string.
		       Always test from the start of the string instead.
		     */
		    int seqlen = 1;
		    for(size_t i = 0; i < RlB_.CurPos; i += seqlen) {
			    seqlen = is_sjis_lead_byte(RlB_.P_CurLine[i]) ? 2 : 1;
		    }
		    RlB_.CurPos -= seqlen;
		    user_putc(BACKSPACE);
		    if(IsDoubleWidth(RlB_.CurPos))
			    user_putc(BACKSPACE);
		    return seqlen;
	    }
		default:
		    RlB_.CurPos--;
		    user_putc(BACKSPACE);
		    return 1;
	}
}
// 
// Step forward over one multi-byte character sequence.
// We don't assume a non-destructive forward space, so we have
// to redraw the character as we go.
// 
void GnuPlot::StepForward()
{
	int i, seqlen;
	switch(GPT._Encoding) {
		case S_ENC_UTF8:
		case S_ENC_SJIS:
		    seqlen = CharSeqLen();
		    for(i = 0; i < seqlen; i++)
			    user_putc(RlB_.P_CurLine[RlB_.CurPos++]);
		    break;
		default:
		    user_putc(RlB_.P_CurLine[RlB_.CurPos++]);
		    break;
	}
}
// 
// Delete the character we are on and collapse all subsequent characters back one
// 
void GnuPlot::DeleteForward()
{
	if(RlB_.CurPos < RlB_.MaxPos) {
		size_t i;
		int seqlen = CharSeqLen();
		RlB_.MaxPos -= seqlen;
		for(i = RlB_.CurPos; i < RlB_.MaxPos; i++)
			RlB_.P_CurLine[i] = RlB_.P_CurLine[i + seqlen];
		RlB_.P_CurLine[RlB_.MaxPos] = '\0';
		FixLine();
	}
}
// 
// Delete the previous character and collapse all subsequent characters back one
// 
void GnuPlot::DeleteBackward()
{
	if(RlB_.CurPos > 0) {
		int seqlen = BackSpace();
		RlB_.MaxPos -= seqlen;
		for(size_t i = RlB_.CurPos; i < RlB_.MaxPos; i++)
			RlB_.P_CurLine[i] = RlB_.P_CurLine[i + seqlen];
		RlB_.P_CurLine[RlB_.MaxPos] = '\0';
		FixLine();
	}
}

void GnuPlot::ExtendCurLine()
{
	// extend input line length 
	char * new_line = (char *)SAlloc::R(RlB_.P_CurLine, RlB_.LineLen + MAXBUF);
	if(!new_line) {
		reset_termio();
		IntError(NO_CARET, "Can't extend readline length");
	}
	RlB_.P_CurLine = new_line;
	RlB_.LineLen += MAXBUF;
	FPRINTF((stderr, "\nextending readline length to %d chars\n", RlB_.LineLen));
}

#if defined(HAVE_DIRENT)
char * GnuPlot::FnCompletion(size_t anchor_pos, int direction)
{
	static char * completions[MAX_COMPLETIONS];
	static int n_completions = 0;
	static int completion_idx = 0;
	if(direction == 0) {
		// new completion 
		DIR * dir;
		char * start, * path;
		char * t, * search;
		char * name = NULL;
		size_t nlen;
		if(n_completions) {
			// new completion, cleanup first 
			for(int i = 0; i < n_completions; i++)
				SAlloc::F(completions[i]);
			memzero(completions, sizeof(completions));
			n_completions = 0;
			completion_idx = 0;
		}
		// extract path to complete 
		start = RlB_.P_CurLine + anchor_pos;
		if(anchor_pos > 0) {
			/* first, look for a quote to start the string */
			for(; start >= RlB_.P_CurLine; start--) {
				if((*start == '"') || (*start == '\'')) {
					start++;
					// handle pipe commands 
					if((*start == '<') || (*start == '|'))
						start++;
					break;
				}
			}
			// if not found, search for a space or a system command '!' instead 
			if(start <= RlB_.P_CurLine) {
				for(start = RlB_.P_CurLine + anchor_pos; start >= RlB_.P_CurLine; start--) {
					if((*start == ' ') || (*start == '!')) {
						start++;
						break;
					}
				}
			}
			SETMAX(start, RlB_.P_CurLine);
			path = strndup(start, RlB_.P_CurLine - start + anchor_pos);
			GpExpandTilde(&path);
		}
		else {
			path = sstrdup("");
		}
		// separate directory and (partial) file directory name 
		t = sstrrchr(path, DIRSEP1);
#if DIRSEP2 != NUL
		SETIFZQ(t, sstrrchr(path, DIRSEP2));
#endif
		if(t == NULL) {
			/* name... */
			search = sstrdup(".");
			name = sstrdup(path);
		}
		else if(t == path) {
			/* root dir: /name... */
			search = strndup(path, 1);
			nlen = RlB_.CurPos - (t - path) - 1;
			name = strndup(t + 1, nlen);
		}
		else {
			/* normal case: dir/dir/name... */
			search = strndup(path, t - path);
			nlen = RlB_.CurPos - (t - path) - 1;
			name = strndup(t + 1, nlen);
		}
		nlen = strlen(name);
		SAlloc::F(path);
		n_completions = 0;
		if((dir = opendir(search))) {
			struct dirent * entry;
			while((entry = readdir(dir)) != NULL) {
				// ignore files and directories starting with a dot 
				if(entry->d_name[0] == '.')
					continue;
				// skip entries which don't match 
				if(nlen > 0)
					if(strncmp(entry->d_name, name, nlen) != 0) 
						continue;
				completions[n_completions] = sstrdup(entry->d_name + nlen);
				n_completions++;
				// limit number of completions 
				if(n_completions == MAX_COMPLETIONS) 
					break;
			}
			closedir(dir);
			SAlloc::F(search);
			SAlloc::F(name);
			return (n_completions > 0) ? completions[0] : NULL;
		}
		SAlloc::F(search);
		SAlloc::F(name);
	}
	else {
		// cycle trough previous results 
		if(n_completions > 0) {
			if(direction > 0)
				completion_idx = (completion_idx + 1) % n_completions;
			else
				completion_idx = (completion_idx + n_completions - 1) % n_completions;
			return completions[completion_idx];
		}
		else
			return NULL;
	}
	return NULL;
}

void GnuPlot::TabCompletion(bool forward)
{
	size_t i;
	static size_t last_tab_pos = -1;
	static size_t last_completion_len = 0;
	int direction;
	// detect tab cycling 
	if((last_tab_pos + last_completion_len) != RlB_.CurPos) {
		last_completion_len = 0;
		last_tab_pos = RlB_.CurPos;
		direction = 0; // new completion 
	}
	else {
		direction = (forward ? 1 : -1);
	}
	// find completion 
	const char * p_completion = FnCompletion(last_tab_pos, direction);
	if(p_completion) {
		// make room for new completion 
		const size_t completion_len = strlen(p_completion);
		if(completion_len > last_completion_len)
			while(RlB_.MaxPos + completion_len - last_completion_len + 1 > RlB_.LineLen)
				ExtendCurLine();
		SUSPENDOUTPUT;
		// erase from last_tab_pos to eol 
		while(RlB_.CurPos > last_tab_pos)
			BackSpace();
		while(RlB_.CurPos < RlB_.MaxPos) {
			user_putc(SPACE);
			if(IsDoubleWidth(RlB_.CurPos))
				user_putc(SPACE);
			RlB_.CurPos += CharSeqLen();
		}
		// rewind to last_tab_pos 
		while(RlB_.CurPos > last_tab_pos)
			BackSpace();
		// insert completion string 
		if(RlB_.MaxPos > (last_tab_pos - last_completion_len))
			memmove(RlB_.P_CurLine + last_tab_pos + completion_len, RlB_.P_CurLine + last_tab_pos + last_completion_len, RlB_.MaxPos-last_tab_pos-last_completion_len);
		memcpy(RlB_.P_CurLine + last_tab_pos, p_completion, completion_len);
		RlB_.MaxPos += completion_len - last_completion_len;
		RlB_.P_CurLine[RlB_.MaxPos] = '\0';
		// draw new completion 
		for(i = 0; i < completion_len; i++)
			user_putc(RlB_.P_CurLine[last_tab_pos+i]);
		RlB_.CurPos += completion_len;
		FixLine();
		RESUMEOUTPUT;
		// remember this completion 
		last_tab_pos  = RlB_.CurPos - completion_len;
		last_completion_len = completion_len;
	}
}

#endif /* HAVE_DIRENT */

//char * readline(const char * pPrompt)
char * GnuPlot::ReadLine(const char * pPrompt)
{
	int cur_char;
	char * new_line;
	bool next_verbatim = FALSE;
	char * prev_line;
	// start with a string of MAXBUF chars 
	if(RlB_.LineLen) {
		SAlloc::F(RlB_.P_CurLine);
		RlB_.LineLen = 0;
	}
	RlB_.P_CurLine = (char *)SAlloc::M(MAXBUF);
	RlB_.LineLen = MAXBUF;
	// set the termio so we can do our own input processing 
	set_termio();
	// print the prompt 
	fputs(pPrompt, stderr);
	RlB_.P_CurLine[0] = '\0';
	RlB_.CurPos = 0;
	RlB_.MaxPos = 0;
	// move to end of history 
	while(NextHistory())
		;
	// init global variables 
	RlB_.SearchMode = false;
	// get characters 
	for(;;) {
		cur_char = special_getc(GPT.P_Term);
		// Accumulate ascii (7bit) printable characters and all leading 8bit characters.
		if(((isprint(cur_char) || (((cur_char & 0x80) != 0) && (cur_char != EOF))) && cur_char != '\t') /*TAB is a printable character in some locales*/ || next_verbatim) {
			size_t i;
			if((RlB_.MaxPos + 1) >= RlB_.LineLen) {
				ExtendCurLine();
			}
			for(i = RlB_.MaxPos; i > RlB_.CurPos; i--) {
				RlB_.P_CurLine[i] = RlB_.P_CurLine[i-1];
			}
			user_putc(cur_char);
			RlB_.P_CurLine[RlB_.CurPos] = cur_char;
			RlB_.CurPos += 1;
			RlB_.MaxPos += 1;
			RlB_.P_CurLine[RlB_.MaxPos] = '\0';
			if(RlB_.CurPos < RlB_.MaxPos) {
				switch(GPT._Encoding) {
					case S_ENC_UTF8:
					    if((cur_char & 0xc0) == 0) {
						    next_verbatim = FALSE;
						    FixLine(); /* Normal ascii character */
					    }
					    else if((cur_char & 0xc0) == 0xc0) {
						    ; /* start of a multibyte sequence. */
					    }
					    else if(((cur_char & 0xc0) == 0x80) && ((uchar)(RlB_.P_CurLine[RlB_.CurPos-2]) >= 0xe0)) {
						    ; /* second byte of a >2 byte sequence */
					    }
					    else {
						    /* Last char of multi-byte sequence */
						    next_verbatim = FALSE;
						    FixLine();
					    }
					    break;

					case S_ENC_SJIS: {
					    /* S-JIS requires a state variable */
					    static int mbwait = 0;
					    if(mbwait == 0) {
						    if(!is_sjis_lead_byte(cur_char)) {
							    /* single-byte character */
							    next_verbatim = FALSE;
							    FixLine();
						    }
						    else {
							    /* first byte of a double-byte sequence */
							    ;
						    }
					    }
					    else {
						    /* second byte of a double-byte sequence */
						    mbwait = 0;
						    next_verbatim = FALSE;
						    FixLine();
					    }
				    }
					default:
					    next_verbatim = FALSE;
					    FixLine();
					    break;
				}
			}
			else {
				static int mbwait = 0;
				next_verbatim = FALSE;
				if(RlB_.SearchMode) {
					// Only update the search at the end of a multi-byte sequence. 
					if(mbwait == 0) {
						if(GPT._Encoding == S_ENC_SJIS)
							mbwait = is_sjis_lead_byte(cur_char) ? 1 : 0;
						if(GPT._Encoding == S_ENC_UTF8) {
							char ch = cur_char;
							if(ch & 0x80)
								while((ch = (ch << 1)) & 0x80)
									mbwait++;
						}
					}
					else {
						mbwait--;
					}
					if(!mbwait)
						DoSearch(-1);
				}
			}
			// ignore special characters in search_mode 
		}
		else if(!RlB_.SearchMode) {
			if(0) {
				;
				// else interpret unix terminal driver characters 
#ifdef VERASE
			}
			else if(cur_char == term_chars[VERASE]) { /* ^H */
				DeleteBackward();
#endif /* VERASE */
#ifdef VEOF
			}
			else if(cur_char == term_chars[VEOF]) { /* ^D? */
				if(RlB_.MaxPos == 0) {
					reset_termio();
					return ((char *)NULL);
				}
				DeleteForward();
#endif /* VEOF */
#ifdef VKILL
			}
			else if(cur_char == term_chars[VKILL]) { /* ^U? */
				ClearLine(pPrompt);
#endif /* VKILL */
#ifdef VWERASE
			}
			else if(cur_char == term_chars[VWERASE]) { /* ^W? */
				DeletePreviousWord();
#endif /* VWERASE */
#ifdef VREPRINT
#if 0 /* conflict with reverse-search */
			}
			else if(cur_char == term_chars[VREPRINT]) { /* ^R? */
				putc(NEWLINE, stderr); /* go to a fresh line */
				RedrawLine(pPrompt);
#endif
#endif /* VREPRINT */
#ifdef VSUSP
			}
			else if(cur_char == term_chars[VSUSP]) {
				reset_termio();
				kill(0, SIGTSTP);

				/* process stops here */

				set_termio();
				/* print the prompt */
				RedrawLine(pPrompt);
#endif /* VSUSP */
			}
			else {
				/* do normal editing commands */
				/* some of these are also done above */
				switch(cur_char) {
					case EOF:
					    reset_termio();
					    return ((char *)NULL);
					case 001: /* ^A */
					    while(RlB_.CurPos > 0)
						    BackSpace();
					    break;
					case 002: /* ^B */
					    if(RlB_.CurPos > 0)
						    BackSpace();
					    break;
					case 005: /* ^E */
					    while(RlB_.CurPos < RlB_.MaxPos) {
						    user_putc(RlB_.P_CurLine[RlB_.CurPos]);
						    RlB_.CurPos += 1;
					    }
					    break;
					case 006: /* ^F */
					    if(RlB_.CurPos < RlB_.MaxPos) {
						    StepForward();
					    }
					    break;
#if defined(HAVE_DIRENT)
					case 011: /* ^I / TAB */
					    TabCompletion(TRUE); /* next tab completion */
					    break;
					case 034: /* remapped by wtext.c or ansi_getc from Shift-Tab */
					    TabCompletion(FALSE); /* previous tab completion */
					    break;
#endif
					case 013: /* ^K */
					    ClearEoline(pPrompt);
					    RlB_.MaxPos = RlB_.CurPos;
					    break;
					case 020: /* ^P */
					    if(PreviousHistory()) {
						    ClearLine(pPrompt);
						    CopyLine(CurrentHistory()->line);
					    }
					    break;
					case 016: /* ^N */
					    ClearLine(pPrompt);
					    if(NextHistory()) {
						    CopyLine(CurrentHistory()->line);
					    }
					    else {
						    RlB_.CurPos = RlB_.MaxPos = 0;
					    }
					    break;
					case 022: /* ^R */
					    prev_line = sstrdup(RlB_.P_CurLine);
					    SwitchPrompt(pPrompt, RlB_.P_SearchPrompt);
					    while(NextHistory())
							; // seek to end of history 
					    RlB_.P_SearchResult = NULL;
					    RlB_.SearchResultWidth = 0;
					    RlB_.SearchMode = TRUE;
					    PrintSearchResult(NULL);
					    break;
					case 014: /* ^L */
					    putc(NEWLINE, stderr); /* go to a fresh line */
					    RedrawLine(pPrompt);
					    break;
#ifndef DEL_ERASES_CURRENT_CHAR
					case 0177: /* DEL */
					case 023: /* Re-mapped from CSI~3 in ansi_getc() */
#endif
					case 010: /* ^H */
					    DeleteBackward();
					    break;
					case 004: /* ^D */
					    // Also catch asynchronous termination signal on Windows 
					    if(RlB_.MaxPos == 0 || _Plt.terminate_flag) {
						    reset_termio();
						    return NULL;
					    }
					    /* intentionally omitting break */
#ifdef DEL_ERASES_CURRENT_CHAR
					case 0177: /* DEL */
					case 023: /* Re-mapped from CSI~3 in ansi_getc() */
#endif
					    DeleteForward();
					    break;
					case 025: /* ^U */
					    ClearLine(pPrompt);
					    break;
					case 026: /* ^V */
					    next_verbatim = TRUE;
					    break;
					case 027: /* ^W */
					    DeletePreviousWord();
					    break;
					case '\n': /* ^J */
					case '\r': /* ^M */
					    RlB_.P_CurLine[RlB_.MaxPos + 1] = '\0';
					    putc(NEWLINE, stderr);
						// 
						// Shrink the block down to fit the string ?
						// if the alloc fails, we still own block at cur_line,
						// but this shouldn't really fail.
						// 
					    new_line = (char *)SAlloc::R(RlB_.P_CurLine, strlen(RlB_.P_CurLine) + 1);
					    if(new_line)
						    RlB_.P_CurLine = new_line;
					    // else we just hang on to what we had - it's not a problem 
					    RlB_.LineLen = 0;
					    FPRINTF((stderr, "Resizing input line to %d chars\n", strlen(RlB_.P_CurLine)));
					    reset_termio();
					    return (RlB_.P_CurLine);
					default:
					    break;
				}
			}
		}
		else { /* search-mode */
#ifdef VERASE
			if(cur_char == term_chars[VERASE]) { /* ^H */
				DeleteBackward();
				DoSearch(-1);
			}
			else
#endif /* VERASE */
			{
				switch(cur_char) {
					case 022: /* ^R */
					    // search next 
					    PreviousHistory();
					    if(DoSearch(-1) == -1)
						    NextHistory();
					    break;
					case 023: /* ^S */
					    // search previous 
					    NextHistory();
					    if(DoSearch(1) == -1)
						    PreviousHistory();
					    break;
					    break;
					case '\n': /* ^J */
					case '\r': /* ^M */
					    // accept 
					    SwitchPrompt(RlB_.P_SearchPrompt, pPrompt);
					    if(RlB_.P_SearchResult)
						    CopyLine(RlB_.P_SearchResult->line);
					    SAlloc::F(prev_line);
					    RlB_.SearchResultWidth = 0;
					    RlB_.SearchMode = false;
					    break;
#ifndef DEL_ERASES_CURRENT_CHAR
					case 0177: /* DEL */
					    /* FIXME: conflict! */
					    //case 023:		/* Re-mapped from CSI~3 in ansi_getc() */
#endif
					case 010: /* ^H */
					    DeleteBackward();
					    DoSearch(1);
					    break;
					default:
					    // abort, restore previous input line 
					    SwitchPrompt(RlB_.P_SearchPrompt, pPrompt);
					    CopyLine(prev_line);
					    SAlloc::F(prev_line);
					    RlB_.SearchResultWidth = 0;
					    RlB_.SearchMode = false;
					    break;
				}
			}
		}
	}
}

int GnuPlot::DoSearch(int dir)
{
	int ret = HistorySearch(RlB_.P_CurLine, dir);
	if(ret != -1)
		RlB_.P_SearchResult = CurrentHistory();
	PrintSearchResult(RlB_.P_SearchResult);
	return ret;
}

//void print_search_result(const HIST_ENTRY * result)
void GnuPlot::PrintSearchResult(const HIST_ENTRY * result)
{
	int    width = 0;
	int    i;
	SUSPENDOUTPUT;
	fputs(RlB_.P_SearchPrompt2, stderr);
	if(result && result->line) {
		fputs(result->line, stderr);
		width = strwidth(result->line);
	}
	// overwrite previous search result, and the line might just have gotten 1 double-width character shorter 
	for(i = 0; i < RlB_.SearchResultWidth - width + 2; i++)
		putc(SPACE, stderr);
	for(i = 0; i < RlB_.SearchResultWidth - width + 2; i++)
		putc(BACKSPACE, stderr);
	RlB_.SearchResultWidth = width;
	// restore cursor position 
	for(i = 0; i < width; i++)
		putc(BACKSPACE, stderr);
	{
		const int splen = sstrleni(RlB_.P_SearchPrompt2);
		for(i = 0; i < splen; i++)
			putc(BACKSPACE, stderr);
	}
	RESUMEOUTPUT;
}

void GnuPlot::SwitchPrompt(const char * pOldPrompt, const char * pNewPrompt)
{
	int i, len;
	SUSPENDOUTPUT;
	// clear search results (if any) 
	if(RlB_.SearchMode) {
		const int spl = sstrleni(RlB_.P_SearchPrompt2);
		for(i = 0; i < RlB_.SearchResultWidth + spl; i++)
			user_putc(SPACE);
		for(i = 0; i < RlB_.SearchResultWidth + spl; i++)
			user_putc(BACKSPACE);
	}
	// clear current line 
	ClearLine(pOldPrompt);
	putc('\r', stderr);
	fputs(pNewPrompt, stderr);
	RlB_.CurPos = 0;
	// erase remainder of previous prompt 
	len = MAX((int)strlen(pOldPrompt) - (int)strlen(pNewPrompt), 0);
	for(i = 0; i < len; i++)
		user_putc(SPACE);
	for(i = 0; i < len; i++)
		user_putc(BACKSPACE);
	RESUMEOUTPUT;
}
//
// Fix up the line from cur_pos to max_pos.
// Does not need any terminal capabilities except backspace,
// and space overwrites a character
//
void GnuPlot::FixLine()
{
	size_t i;
	SUSPENDOUTPUT;
	// write tail of string 
	for(i = RlB_.CurPos; i < RlB_.MaxPos; i++)
		user_putc(RlB_.P_CurLine[i]);
	// We may have just shortened the line by deleting a character.
	// Write a space at the end to over-print the former last character.
	// It needs 2 spaces in case the former character was double width.
	user_putc(SPACE);
	user_putc(SPACE);
	if(RlB_.SearchMode) {
		int j;
		for(j = 0; j < RlB_.SearchResultWidth; j++)
			user_putc(SPACE);
		for(j = 0; j < RlB_.SearchResultWidth; j++)
			user_putc(BACKSPACE);
	}
	user_putc(BACKSPACE);
	user_putc(BACKSPACE);
	// Back up to original position 
	i = RlB_.CurPos;
	for(RlB_.CurPos = RlB_.MaxPos; RlB_.CurPos > i;)
		BackSpace();
	RESUMEOUTPUT;
}
//
// redraw the entire line, putting the cursor where it belongs 
//
void GnuPlot::RedrawLine(const char * pPrompt)
{
	size_t i;
	SUSPENDOUTPUT;
	fputs(pPrompt, stderr);
	user_puts(RlB_.P_CurLine);
	// put the cursor where it belongs 
	i = RlB_.CurPos;
	for(RlB_.CurPos = RlB_.MaxPos; RlB_.CurPos > i;)
		BackSpace();
	RESUMEOUTPUT;
}
//
// clear cur_line and the screen line 
//
void GnuPlot::ClearLine(const char * pPrompt)
{
	SUSPENDOUTPUT;
	putc('\r', stderr);
	fputs(pPrompt, stderr);
	RlB_.CurPos = 0;
	while(RlB_.CurPos < RlB_.MaxPos) {
		user_putc(SPACE);
		if(IsDoubleWidth(RlB_.CurPos))
			user_putc(SPACE);
		RlB_.CurPos += CharSeqLen();
	}
	while(RlB_.MaxPos > 0)
		RlB_.P_CurLine[--RlB_.MaxPos] = '\0';
	putc('\r', stderr);
	fputs(pPrompt, stderr);
	RlB_.CurPos = 0;
	RESUMEOUTPUT;
}
//
// clear to end of line and the screen end of line 
//
void GnuPlot::ClearEoline(const char * pPrompt)
{
	const size_t save_pos = RlB_.CurPos;
	SUSPENDOUTPUT;
	while(RlB_.CurPos < RlB_.MaxPos) {
		user_putc(SPACE);
		if(IsDoubleWidth(RlB_.P_CurLine[RlB_.CurPos]))
			user_putc(SPACE);
		RlB_.CurPos += CharSeqLen();
	}
	RlB_.CurPos = save_pos;
	while(RlB_.MaxPos > RlB_.CurPos)
		RlB_.P_CurLine[--RlB_.MaxPos] = '\0';
	putc('\r', stderr);
	fputs(pPrompt, stderr);
	user_puts(RlB_.P_CurLine);
	RESUMEOUTPUT;
}
//
// delete the full or partial word immediately before cursor position 
//
void GnuPlot::DeletePreviousWord()
{
	const size_t save_pos = RlB_.CurPos;
	SUSPENDOUTPUT;
	// skip whitespace 
	while((RlB_.CurPos > 0) && (RlB_.P_CurLine[RlB_.CurPos-1] == SPACE)) {
		BackSpace();
	}
	// find start of previous word 
	while((RlB_.CurPos > 0) && (RlB_.P_CurLine[RlB_.CurPos-1] != SPACE)) {
		BackSpace();
	}
	if(RlB_.CurPos != save_pos) {
		size_t new_cur_pos = RlB_.CurPos;
		size_t m = RlB_.MaxPos - save_pos;
		// erase to eol 
		while(RlB_.CurPos < RlB_.MaxPos) {
			user_putc(SPACE);
			if(IsDoubleWidth(RlB_.CurPos))
				user_putc(SPACE);
			RlB_.CurPos += CharSeqLen();
		}
		while(RlB_.CurPos > new_cur_pos)
			BackSpace();
		// overwrite previous word with trailing characters 
		memmove(RlB_.P_CurLine + RlB_.CurPos, RlB_.P_CurLine + save_pos, m);
		// overwrite characters at end of string with NULs 
		memzero(RlB_.P_CurLine + RlB_.CurPos + m, save_pos - RlB_.CurPos);
		// update display and line length 
		RlB_.MaxPos = RlB_.CurPos + m;
		FixLine();
	}
	RESUMEOUTPUT;
}
//
// copy line to cur_line, draw it and set cur_pos and max_pos 
//
void GnuPlot::CopyLine(const char * pLine)
{
	while((strlen(pLine) + 1) > RlB_.LineLen) {
		ExtendCurLine();
	}
	strcpy(RlB_.P_CurLine, pLine);
	user_puts(RlB_.P_CurLine);
	RlB_.CurPos = RlB_.MaxPos = strlen(RlB_.P_CurLine);
}

#if !defined(MSDOS) && !defined(_WIN32)
// Convert ANSI arrow keys to control characters 
static int ansi_getc(GpTermEntry * pTerm)
{
	int c;
#ifdef USE_MOUSE
	// EAM June 2020 why only interactive?
	// if (pTerm && pTerm->waitforinput && interactive)
	if(pTerm && pTerm->waitforinput)
		c = pTerm->waitforinput(pTerm, 0);
	else
#endif
	c = getc(stdin);
	if(c == 033) {
		c = getc(stdin); /* check for CSI */
		if(c == '[') {
			c = getc(stdin); /* get command character */
			switch(c) {
				case 'D': /* left arrow key */
				    c = 002;
				    break;
				case 'C': /* right arrow key */
				    c = 006;
				    break;
				case 'A': /* up arrow key */
				    c = 020;
				    break;
				case 'B': /* down arrow key */
				    c = 016;
				    break;
				case 'F': /* end key */
				    c = 005;
				    break;
				case 'H': /* home key */
				    c = 001;
				    break;
				case 'Z': /* shift-tab key */
				    c = 034; /* FS: non-standard! */
				    break;
				case '3': /* DEL can be <esc>[3~ */
				    getc(stdin); /* eat the ~ */
				    c = 023; /* DC3 ^S NB: non-standard!! */
			}
		}
	}
	return c;
}

#endif

#if defined(_WIN32)
#ifdef WGP_CONSOLE
	int GnuPlot::WinGetch(GpTermEntry * pTerm)
	{
		return (pTerm && pTerm->waitforinput) ? pTerm->waitforinput(pTerm, 0) : ConsoleGetch();
	}
#else
//
// Convert Arrow keystrokes to Control characters: 
//
int GnuPlot::MsDosGetch(GpTermEntry * pTerm)
{
	int c;
#ifdef DJGPP
	// no need to handle mouse input here: it's done in pTerm->text() 
	int ch = getkey();
	c = (ch & 0xff00) ? 0 : ch & 0xff;
#elif defined (OS2)
	c = getc(stdin);
#else
#if defined (USE_MOUSE)
	if(pTerm && pTerm->waitforinput && _Plt.interactive)
		c = pTerm->waitforinput(pTerm, 0);
	else
#endif
	c = getch();
#endif
	if(c == 0) {
#ifdef DJGPP
		c = ch & 0xff;
#elif defined(OS2)
		c = getc(stdin);
#else
#if defined (USE_MOUSE)
		if(pTerm && pTerm->waitforinput && _Plt.interactive)
			c = pTerm->waitforinput(pTerm, 0);
		else
#endif
		c = getch(); // Get the extended code. 
#endif
		switch(c) {
			case 75: // Left Arrow. 
			    c = 002;
			    break;
			case 77: /* Right Arrow. */
			    c = 006;
			    break;
			case 72: /* Up Arrow. */
			    c = 020;
			    break;
			case 80: /* Down Arrow. */
			    c = 016;
			    break;
			case 115: /* Ctl Left Arrow. */
			case 71: /* Home */
			    c = 001;
			    break;
			case 116: /* Ctl Right Arrow. */
			case 79: /* End */
			    c = 005;
			    break;
			case 83: /* Delete */
			    c = 0177;
			    break;
			case 15: /* BackTab / Shift-Tab */
			    c = 034; /* FS: remap to non-standard code for tab-completion */
			    break;
			default:
			    c = 0;
			    break;
		}
	}
	else if(c == 033) { /* ESC */
		c = 025;
	}
	return c;
}

#endif /* WGP_CONSOLE */
#endif /* MSDOS || _WIN32 || OS2 */
//
// set termio so we can do our own input processing 
//
static void set_termio()
{
#if !defined(MSDOS) && !defined(_WIN32)
	// set termio so we can do our own input processing 
	// and save the old terminal modes so we can reset them later 
	if(term_set == 0) {
		/*
		 * Get terminal modes.
		 */
#ifdef SGTTY
		ioctl(0, TIOCGETP, &orig_termio);
#else                           /* not SGTTY */
#ifdef TERMIOS
#ifdef TCGETS
		ioctl(0, TCGETS, &orig_termio);
#    else                       /* not TCGETS */
		tcgetattr(0, &orig_termio);
#    endif                      /* not TCGETS */
#   else                        /* not TERMIOS */
		ioctl(0, TCGETA, &orig_termio);
#endif                  /* TERMIOS */
#endif                  /* not SGTTY */

		/*
		 * Save terminal modes
		 */
		rl_termio = orig_termio;

		/*
		 * Set the modes to the way we want them
		 *  and save our input special characters
		 */
#ifdef SGTTY
		rl_termio.sg_flags |= CBREAK;
		rl_termio.sg_flags &= ~(ECHO | XTABS);
		ioctl(0, TIOCSETN, &rl_termio);

		ioctl(0, TIOCGETC, &s_tchars);
		term_chars[VERASE] = orig_termio.sg_erase;
		term_chars[VEOF] = s_tchars.t_eofc;
		term_chars[VKILL] = orig_termio.sg_kill;
#ifdef TIOCGLTC
		ioctl(0, TIOCGLTC, &s_ltchars);
		term_chars[VWERASE] = s_ltchars.t_werasc;
		term_chars[VREPRINT] = s_ltchars.t_rprntc;
		term_chars[VSUSP] = s_ltchars.t_suspc;

		/* disable suspending process on ^Z */
		s_ltchars.t_suspc = 0;
		ioctl(0, TIOCSLTC, &s_ltchars);
#endif                  /* TIOCGLTC */
#else                           /* not SGTTY */
		rl_termio.c_iflag &= ~(BRKINT | PARMRK | INPCK | IUCLC | IXON | IXOFF);
		rl_termio.c_iflag |= (IGNBRK | IGNPAR);
		/* rl_termio.c_oflag &= ~(ONOCR); Costas Sphocleous Irvine,CA */
		rl_termio.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | NOFLSH);
		rl_termio.c_lflag |= (ISIG);
		rl_termio.c_cc[VMIN] = 1;
		rl_termio.c_cc[VTIME] = 0;

#ifndef VWERASE
#define VWERASE 3
#endif                  /* VWERASE */
		term_chars[VERASE] = orig_termio.c_cc[VERASE];
		term_chars[VEOF] = orig_termio.c_cc[VEOF];
		term_chars[VKILL] = orig_termio.c_cc[VKILL];
#ifdef TERMIOS
		term_chars[VWERASE] = orig_termio.c_cc[VWERASE];
#ifdef VREPRINT
		term_chars[VREPRINT] = orig_termio.c_cc[VREPRINT];
#    else                       /* not VREPRINT */
#ifdef VRPRNT
		term_chars[VRPRNT] = orig_termio.c_cc[VRPRNT];
#endif                     /* VRPRNT */
#    endif                      /* not VREPRINT */
		term_chars[VSUSP] = orig_termio.c_cc[VSUSP];

		/* disable suspending process on ^Z */
		rl_termio.c_cc[VSUSP] = 0;
#endif                  /* TERMIOS */
#endif                  /* not SGTTY */

		/*
		 * Set the new terminal modes.
		 */
#ifdef SGTTY
		ioctl(0, TIOCSLTC, &s_ltchars);
#else                           /* not SGTTY */
#ifdef TERMIOS
#ifdef TCSETSW
		ioctl(0, TCSETSW, &rl_termio);
#    else                       /* not TCSETSW */
		tcsetattr(0, TCSADRAIN, &rl_termio);
#    endif                      /* not TCSETSW */
#   else                        /* not TERMIOS */
		ioctl(0, TCSETAW, &rl_termio);
#endif                  /* not TERMIOS */
#endif                  /* not SGTTY */
		term_set = 1;
	}
#endif /* not MSDOS && not _WIN32 */
}

static void reset_termio()
{
#if !defined(MSDOS) && !defined(_WIN32)
/* reset saved terminal modes */
	if(term_set == 1) {
#ifdef SGTTY
		ioctl(0, TIOCSETN, &orig_termio);
#ifdef TIOCGLTC
		// enable suspending process on ^Z 
		s_ltchars.t_suspc = term_chars[VSUSP];
		ioctl(0, TIOCSLTC, &s_ltchars);
#endif                  /* TIOCGLTC */
#else                           /* not SGTTY */
#ifdef TERMIOS
#ifdef TCSETSW
		ioctl(0, TCSETSW, &orig_termio);
#    else                       /* not TCSETSW */
		tcsetattr(0, TCSADRAIN, &orig_termio);
#    endif                      /* not TCSETSW */
#   else                        /* not TERMIOS */
		ioctl(0, TCSETAW, &orig_termio);
#endif                  /* TERMIOS */
#endif                  /* not SGTTY */
		term_set = 0;
	}
#endif /* not MSDOS && not _WIN32 */
}

#endif /* READLINE */
