/* GNUPLOT - misc.c */

/*[
 * Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the complete modified source code.  Modifications are to
 * be distributed as patches to the released version.  Permission to
 * distribute binaries produced by compiling modified sources is granted,
 * provided you
 *   1. distribute the corresponding source modifications from the
 *    released version in the form of a patch file along with the binaries,
 *   2. add special version identification to distinguish your version
 *    in addition to the base release version number,
 *   3. provide your name and address as the primary contact for the
 *    support of your modified version, and
 *   4. retain our contact information in regard to use of the base
 *    software.
 * Permission to distribute the released version of the source code along
 * with corresponding source modifications in the form of a patch file is
 * granted with same provisions 2 through 4 for binary distributions.
 *
 * This software is provided "as is" without express or implied warranty
 * to the extent permitted by applicable law.
   ]*/
#include <gnuplot.h>
#pragma hdrstop
#include <sys\stat.h>
#include "stdfn.h"
#ifdef _Windows
	#include <fcntl.h>
	#if defined(__WATCOMC__) || defined(__MSC__)
		#include <io.h>        /* for setmode() */
	#endif
#endif
#if defined(HAVE_DIRENT_H)
	#include <sys/types.h>
	#include <dirent.h>
#elif defined(_WIN32)
	/* Windows version of opendir() and friends in stdfn.c */
	#ifdef __WATCOM
		#include <direct.h>
	#endif
#endif

static char * recursivefullname(const char * path, const char * filename, bool recursive);
static void prepare_call(GpCommand & rC, int calltype);

/* State information for load_file(), to recover from errors
 * and properly handle recursive load_file calls
 */
LFS * lf_head = NULL;            /* NULL if not in load_file */

/* these are global so that plot.c can load them for the -c option */
int call_argc;
char * call_args[10] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
static char * argname[] = {"ARG0", "ARG1", "ARG2", "ARG3", "ARG4", "ARG5", "ARG6", "ARG7", "ARG8", "ARG9"};

/*
 * iso_alloc() allocates a iso_curve structure that can hold 'num'
 * points.
 */
iso_curve * iso_alloc(int num)                   
{
	iso_curve * ip;
	ip = (iso_curve*)malloc(sizeof(iso_curve));
	ip->p_max = (num >= 0 ? num : 0);
	ip->p_count = 0;
	if(num > 0) {
		ip->points = (GpCoordinate GPHUGE*)malloc(num * sizeof(GpCoordinate));
		memzero(ip->points, num * sizeof(GpCoordinate));
	}
	else
		ip->points = (GpCoordinate GPHUGE*)NULL;
	ip->next = NULL;
	return (ip);
}
/*
 * iso_extend() reallocates a iso_curve structure to hold "num"
 * points. This will either expand or shrink the storage.
 */
void iso_extend(iso_curve * ip, int num)
{
	if(num != ip->p_max) {
		if(num > 0) {
			if(ip->points == NULL) {
				ip->points = (GpCoordinate *)malloc(num * sizeof(GpCoordinate));
			}
			else {
				ip->points = (GpCoordinate *)gp_realloc(ip->points, num * sizeof(GpCoordinate), "expanding curve points");
			}
			if(num > ip->p_max)
				memzero(&(ip->points[ip->p_max]), (num - ip->p_max) * sizeof(GpCoordinate));
			ip->p_max = num;
		}
		else {
			ZFREE(ip->points);
			ip->p_max = 0;
		}
	}
}
//
// iso_free() releases any memory which was previously malloc()'d to hold iso curve points.
//
void iso_free(iso_curve * ip)
{
	if(ip) {
		free(ip->points);
		free(ip);
	}
}

static void prepare_call(GpCommand & rC, int calltype)
{
	UdvtEntry * udv;
	int argindex;
	if(calltype == 2) {
		call_argc = 0;
		while(!rC.EndOfCommand() && call_argc <= 9) {
			call_args[call_argc] = rC.TryToGetString();
			if(!call_args[call_argc]) {
				int save_token = rC.CToken;
				// This catches call "file" STRINGVAR (expression) 
				if(rC.TypeDdv(rC.CToken) == STRING) {
					call_args[call_argc] = gp_strdup(GpGg.Ev.AddUdv(rC, rC.CToken)->udv_value.v.string_val);
					rC.CToken++;

					/* Evaluates a parenthesized expression and store the result in a string */
				}
				else if(rC.Eq("(")) {
					char val_as_string[32];
					t_value a;
					rC.ConstExpress(&a);
					switch(a.type) {
						case CMPLX: /* FIXME: More precision? Some way to provide a format? */
						    sprintf(val_as_string, "%g", a.v.cmplx_val.real);
						    call_args[call_argc] = gp_strdup(val_as_string);
						    break;
						default:
						    GpGg.IntError(save_token, "Unrecognized argument type");
						    break;
						case INTGR:
						    sprintf(val_as_string, "%d", a.v.int_val);
						    call_args[call_argc] = gp_strdup(val_as_string);
						    break;
					}

					/* old (pre version 5) style wrapping of bare tokens as strings */
					/* is still useful for passing unquoted numbers */
				}
				else {
					rC.MCapture(&call_args[call_argc], rC.CToken, rC.CToken);
					rC.CToken++;
				}
			}
			call_argc++;
		}
		lf_head->CToken = rC.CToken;
		if(!rC.EndOfCommand())
			GpGg.IntError(++rC.CToken, "too many arguments for 'call <file>'");
	}
	else if(calltype == 5) {
		// lf_push() moved our call arguments from call_args[] to lf->call_args[] 
		// call_argc was determined at program entry 
		for(argindex = 0; argindex < 10; argindex++) {
			call_args[argindex] = lf_head->call_args[argindex];
			lf_head->call_args[argindex] = NULL; /* just to be safe */
		}
	}
	else {
		call_argc = 0; // "load" command has no arguments 
	}
	// Old-style "call" arguments were referenced as $0 ... $9 and $# 
	// New-style has ARG0 = script-name, ARG1 ... ARG9 and ARGC 
	// FIXME:  If we defined these on entry, we could use get_udv* here 
	udv = GpGg.Ev.AddUdvByName("ARGC");
	udv->udv_value.SetInt(call_argc);
	udv = GpGg.Ev.AddUdvByName("ARG0");
	gpfree_string(&(udv->udv_value));
	Gstring(&(udv->udv_value), gp_strdup(lf_head->name));
	for(argindex = 1; argindex <= 9; argindex++) {
		char * arg = gp_strdup(call_args[argindex-1]);
		udv = GpGg.Ev.AddUdvByName(argname[argindex]);
		gpfree_string(&(udv->udv_value));
		Gstring(&(udv->udv_value), arg ? arg : gp_strdup(""));
	}
}

#ifdef OLD_STYLE_CALL_ARGS

const char * expand_call_arg(int c)
{
	static char numstr[3];
	if(c == '$') {
		return "$";
	}
	else if(c == '#') {
		assert(call_argc >= 0 && call_argc <= 9);
		sprintf(numstr, "%i", call_argc);
		return numstr;
	}
	else if(c >= '0' && c <= '9') {
		int ind = c - '0';
		if(ind >= call_argc)
			return "";
		else
			return call_args[ind];
	}
	else {
		/* pass through unrecognized syntax elements that begin with $, e.g. datablock names */
		sprintf(numstr, "$%c", c);
		return numstr;
	}
	return NULL; /* Avoid compiler warning */
}

//static void expand_call_args()
void GpCommand::ExpandCallArgs()
{
	int il = 0;
	int len;
	char * raw_line = gp_strdup(P_InputLine);
	char * rl = raw_line;
	*P_InputLine = '\0';
	while(*rl) {
		if(*rl == '$') {
			const char * sub = expand_call_arg(*(++rl));
			len = strlen(sub);
			while(InputLineLen - il < len + 1)
				extend_input_line();
			strcpy(P_InputLine + il, sub);
			il += len;
		}
		else {
			if(il + 1 > InputLineLen)
				extend_input_line();
			P_InputLine[il++] = *rl;
		}
		rl++;
	}
	if(il + 1 > InputLineLen)
		extend_input_line();
	P_InputLine[il] = '\0';
	free(raw_line);
}

#endif /* OLD_STYLE_CALL_ARGS */

/*
 * load_file() is called from
 * (1) the "load" command, no arguments substitution is done
 * (2) the "call" command, arguments are substituted for $0, $1, etc.
 * (3) on program entry to load initialization files (acts like "load")
 * (4) to execute script files given on the command line (acts like "load")
 * (5) to execute a single script file given with -c (acts like "call")
 */
void GpGadgets::LoadFile(FILE * pFp, char * pFileName, int calltype)
{
	//int    len;
	int    start, left;
	int    more;
	int    stop = false;
	//
	// Provide a user-visible copy of the current line number in the input file
	//
	UdvtEntry * gpval_lineno = Ev.AddUdvByName("GPVAL_LINENO");
	gpval_lineno->udv_value.SetInt(0);
	lf_push(pFp, pFileName, NULL); /* save state for errors and recursion */
	if(pFp == (FILE*)NULL) {
		IntErrorNoCaret("Cannot open script file '%s'", pFileName);
		// won't actually reach here
	}
	else if(pFp == stdin) {
		/* DBT 10-6-98  go interactive if "-" named as load file */
		IsInteractive = true;
		while(!ComLine(Gp__C)) 
			;
		lf_pop(Gp__C);
	}
	else {
		// We actually will read from a file 
		prepare_call(Gp__C, calltype);
		// things to do after lf_push 
		Gp__C.InlineNum = 0;
		// go into non-interactive mode during load 
		// will be undone below, or in load_file_error 
		IsInteractive = false;
		while(!stop) { // read all lines in file 
			left = Gp__C.InputLineLen;
			start = 0;
			more = true;
			// read one logical line
			while(more) {
				if(fgets(&(Gp__C.P_InputLine[start]), left, pFp) == (char*)NULL) {
					stop = true; /* EOF in file */
					Gp__C.P_InputLine[start] = '\0';
					more = false;
				}
				else {
					Gp__C.InlineNum++;
					gpval_lineno->udv_value.v.int_val = Gp__C.InlineNum; // User visible copy
					int    len = strlen(Gp__C.P_InputLine) - 1;
					if(Gp__C.P_InputLine[len] == '\n') { /* remove any newline */
						Gp__C.P_InputLine[len] = '\0';
						/* Look, len was 1-1 = 0 before, take care here! */
						if(len > 0)
							--len;
						if(Gp__C.P_InputLine[len] == '\r') { /* remove any carriage return */
							Gp__C.P_InputLine[len] = NUL;
							if(len > 0)
								--len;
						}
					}
					else if(len + 2 >= left) {
						Gp__C.ExtendInputLine();
						left = Gp__C.InputLineLen - len - 1;
						start = len + 1;
						continue; /* don't check for '\' */
					}
					if(Gp__C.P_InputLine[len] == '\\') {
						/* line continuation */
						start = len;
						left = Gp__C.InputLineLen - start;
					}
					else {
						/* EAM May 2011 - handle multi-line bracketed clauses {...}.
						* Introduces a requirement for scanner.c and scanner.h
						* This code is redundant with part of do_line(),
						* but do_line() assumes continuation lines come from stdin.
						*/

						/* macros in a clause are problematic, as they are */
						/* only expanded once even if the clause is replayed */
						Gp__C.StringExpandMacros();
						// Strip off trailing comment and count curly braces 
						Gp__C.NumTokens = Gp__C.Scanner(&Gp__C.P_InputLine, &Gp__C.InputLineLen);
						if(Gp__C.P_InputLine[Gp__C.P_Token[Gp__C.NumTokens].start_index] == '#') {
							Gp__C.P_InputLine[Gp__C.P_Token[Gp__C.NumTokens].start_index] = NUL;
							start = Gp__C.P_Token[Gp__C.NumTokens].start_index;
							left = Gp__C.InputLineLen - start;
						}
						/* Read additional lines if necessary to complete a
						* bracketed clause {...}
						*/
						if(curly_brace_count < 0)
							IntErrorNoCaret("Unexpected }");
						if(curly_brace_count > 0) {
							if((len + 4) > (int)Gp__C.InputLineLen)
								Gp__C.ExtendInputLine();
							strcat(Gp__C.P_InputLine, ";\n");
							start = strlen(Gp__C.P_InputLine);
							left = Gp__C.InputLineLen - start;
							continue;
						}
						more = false;
					}
				}
			}
			/* If we hit a 'break' or 'continue' statement in the lines just processed */
			if(Gp__C.IterationEarlyExit())
				continue;
			/* process line */
			if(strlen(Gp__C.P_InputLine) > 0) {
	#ifdef OLD_STYLE_CALL_ARGS
				if(calltype == 2 || calltype == 5)
					Gp__C.ExpandCallArgs();
	#endif
				screen_ok = false; /* make sure command line is echoed on error */
				if(Gp__C.DoLine())
					stop = true;
			}
		}
		/* pop state */
		lf_pop(Gp__C); // also closes file pFp
	}
}
//
// pop from load_file state stack
// false if stack was empty
// called by load_file and load_file_error 
//
bool lf_pop(GpCommand & rC)
{
	int argindex;
	UdvtEntry * udv;
	if(lf_head == NULL)
		return (false);
	else {
		LFS * lf = lf_head;
		if(lf->fp == NULL || lf->fp == stdin)
			/* Do not close stdin in the case that "-" is named as a load file */
			;
#if defined(PIPES)
		else if(lf->name != NULL && lf->name[0] == '<')
			pclose(lf->fp);
#endif
		else
			fclose(lf->fp);
		// call arguments are not relevant when invoked from do_string_and_free
		if(lf->cmdline == NULL) {
			for(argindex = 0; argindex < 10; argindex++) {
				free(call_args[argindex]);
				call_args[argindex] = lf->call_args[argindex];
			}
			call_argc = lf->call_argc;
			// Restore ARGC and ARG0 ... ARG9 
			if((udv = GpGg.Ev.GetUdvByName("ARGC"))) {
				udv->udv_value.SetInt(call_argc);
			}
			if((udv = GpGg.Ev.GetUdvByName("ARG0"))) {
				gpfree_string(&(udv->udv_value));
				Gstring(&(udv->udv_value), (lf->prev && lf->prev->name) ? gp_strdup(lf->prev->name) : gp_strdup(""));
			}
			for(argindex = 1; argindex <= 9; argindex++) {
				if((udv = GpGg.Ev.GetUdvByName(argname[argindex]))) {
					gpfree_string(&(udv->udv_value));
					if(!call_args[argindex-1])
						udv->udv_value.type = NOTDEFINED;
					else
						Gstring(&(udv->udv_value), gp_strdup(call_args[argindex-1]));
				}
			}
		}
		GpGg.IsInteractive = lf->interactive;
		rC.InlineNum = lf->inline_num;
		GpGg.Ev.AddUdvByName("GPVAL_LINENO")->udv_value.v.int_val = rC.InlineNum;
		rC.IfDepth = lf->if_depth;
		rC.IfCondition = lf->if_condition;
		rC.IfOpenForElse = lf->if_open_for_else;
		// Restore saved input state and free the copy
		if(lf->P_Ttokens) {
			rC.NumTokens = lf->NumTokens;
			rC.CToken = lf->CToken;
			assert(rC.TokenTableSize >= lf->NumTokens+1);
			memcpy(rC.P_Token, lf->P_Ttokens, (lf->NumTokens+1) * sizeof(LexicalUnit));
			free(lf->P_Ttokens);
		}
		if(lf->input_line) {
			strcpy(rC.P_InputLine, lf->input_line);
			free(lf->input_line);
		}
		free(lf->name);
		free(lf->cmdline);
		lf_head = lf->prev;
		free(lf);
		return (true);
	}
}

/* lf_push is called from two different contexts:
 *    load_file passes fp and file name (3rd param NULL)
 *    do_string_and_free passes cmdline (1st and 2nd params NULL)
 * In either case the routines lf_push/lf_pop save and restore state
 * information that may be changed by executing commands from a file
 * or from the passed command line.
 */
void lf_push(FILE * fp, char * name, char * cmdline)
{
	int argindex;
	LFS * lf = (LFS*)malloc(sizeof(LFS));
	if(lf == (LFS*)NULL) {
		SFile::ZClose(&fp);  // it won't be otherwise 
		GpGg.IntErrorCurToken("not enough memory to load file");
	}
	lf->fp = fp;            /* save this file pointer */
	lf->name = name;
	lf->cmdline = cmdline;
	lf->interactive = GpGg.IsInteractive; // save current state 
	lf->inline_num = GpGg.Gp__C.InlineNum;    /* save current line number */
	lf->call_argc = call_argc;
	// Call arguments are irrelevant if invoked from do_string_and_free 
	if(cmdline == NULL) {
		for(argindex = 0; argindex < 10; argindex++) {
			lf->call_args[argindex] = call_args[argindex];
			call_args[argindex] = NULL; /* initially no args */
		}
	}
	lf->depth = lf_head ? lf_head->depth+1 : 0; /* recursion depth */
	if(lf->depth > STACK_DEPTH)
		GpGg.IntErrorNoCaret("load/eval nested too deeply");
	lf->if_depth = GpGg.Gp__C.IfDepth;
	lf->if_open_for_else = GpGg.Gp__C.IfOpenForElse;
	lf->if_condition = GpGg.Gp__C.IfCondition;
	lf->CToken = GpGg.Gp__C.CToken;
	lf->NumTokens = GpGg.Gp__C.NumTokens;
	lf->P_Ttokens = (LexicalUnit *)malloc((GpGg.Gp__C.NumTokens+1) * sizeof(LexicalUnit));
	memcpy(lf->P_Ttokens, GpGg.Gp__C.P_Token, (GpGg.Gp__C.NumTokens+1) * sizeof(LexicalUnit));
	lf->input_line = gp_strdup(GpGg.Gp__C.P_InputLine);
	lf->prev = lf_head; // link to stack 
	lf_head = lf;
}

/* used for reread  vsnyder@math.jpl.nasa.gov */
FILE * lf_top()
{
	return lf_head ? (lf_head->fp) : 0;
}

/* called from main */
void load_file_error()
{
	/* clean up from error in load_file */
	/* pop off everything on stack */
	while(lf_pop(GpGg.Gp__C))
		;
}

FILE * loadpath_fopen(const char * filename, const char * mode)
{
	FILE * fp;
#if defined(PIPES)
	if(*filename == '<') {
		restrict_popen();
		if((fp = popen(filename + 1, "r")) == (FILE*)NULL)
			return (FILE*)0;
	}
	else
#endif /* PIPES */
	if((fp = fopen(filename, mode)) == (FILE*)NULL) {
		/* try 'loadpath' variable */
		char * fullname = NULL, * path;
		while((path = get_loadpath()) != NULL) {
			/* length of path, dir separator, filename, \0 */
			fullname = (char *)gp_realloc(fullname, strlen(path) + 1 + strlen(filename) + 1, "loadpath_fopen");
			strcpy(fullname, path);
			PATH_CONCAT(fullname, filename);
			if((fp = fopen(fullname, mode)) != NULL) {
				ZFREE(fullname);
				/* reset loadpath internals!
				 * maybe this can be replaced by calling get_loadpath with
				 * a NULL argument and some loadpath_handler internal logic */
				while(get_loadpath()) ;
				break;
			}
		}
		free(fullname);
	}

#ifdef _WIN32
	if(fp != NULL)
		_setmode(_fileno(fp), _O_BINARY);
#endif
	return fp;
}

/* Harald Harders <h.harders@tu-bs.de> */
static char * recursivefullname(const char * path, const char * filename, bool recursive)
{
	FILE * fp = 0;
	/* length of path, dir separator, filename, \0 */
	char * fullname = (char *)malloc(strlen(path) + 1 + strlen(filename) + 1);
	strcpy(fullname, path);
	PATH_CONCAT(fullname, filename);
	if((fp = fopen(fullname, "r")) != NULL) {
		fclose(fp);
		return fullname;
	}
	else {
		ZFREE(fullname);
	}
	if(recursive) {
#if defined HAVE_DIRENT_H || defined(_WIN32)
		struct dirent * direntry;
		struct stat buf;
		DIR * dir = opendir(path);
		if(dir) {
			while((direntry = readdir(dir)) != NULL) {
				char * fulldir = (char*)malloc(strlen(path) + 1 + strlen(direntry->d_name) + 1);
				strcpy(fulldir, path);
#if defined(VMS)
				if(fulldir[strlen(fulldir) - 1] == ']')
					fulldir[strlen(fulldir) - 1] = '\0';
				strcpy(&(fulldir[strlen(fulldir)]), ".");
				strcpy(&(fulldir[strlen(fulldir)]), direntry->d_name);
				strcpy(&(fulldir[strlen(fulldir)]), "]");
#else
				PATH_CONCAT(fulldir, direntry->d_name);
#endif
				stat(fulldir, &buf);
				if((S_ISDIR(buf.st_mode)) && (strcmp(direntry->d_name, ".") != 0) && (strcmp(direntry->d_name, "..") != 0)) {
					fullname = recursivefullname(fulldir, filename, true);
					if(fullname != NULL)
						break;
				}
				free(fulldir);
			}
			closedir(dir);
		}
#else
		IntWarn(NO_CARET, "Recursive directory search not supported\n\t('%s!')", path);
#endif
	}
	return fullname;
}
//
// may return NULL
//
char * fontpath_fullname(const char * filename)
{
	FILE * fp;
	char * fullname = NULL;
#if defined(PIPES)
	if(*filename == '<') {
		os_error(NO_CARET, "fontpath_fullname: No Pipe allowed");
	}
	else
#endif /* PIPES */
	if((fp = fopen(filename, "r")) == (FILE*)NULL) {
		/* try 'fontpath' variable */
		char * tmppath, * path = NULL;
		while((tmppath = get_fontpath()) != NULL) {
			bool subdirs = false;
			path = gp_strdup(tmppath);
			if(path[strlen(path) - 1] == '!') {
				path[strlen(path) - 1] = '\0';
				subdirs = true;
			}       /* if */
			fullname = recursivefullname(path, filename, subdirs);
			if(fullname != NULL) {
				while(get_fontpath()) 
					;
				free(path);
				break;
			}
			free(path);
		}
	}
	else
		fullname = gp_strdup(filename);
	return fullname;
}

/* Push current terminal.
 * Called 1. in main(), just after init_terminal(),
 *        2. from load_rcfile(),
 *        3. anytime by user command "set term push".
 */
static char * push_term_name = NULL;
static char * push_term_opts = NULL;

void push_terminal(int is_interactive)
{
	if(term) {
		free(push_term_name);
		free(push_term_opts);
		push_term_name = gp_strdup(term->name);
		push_term_opts = gp_strdup(term_options);
		if(is_interactive)
			fprintf(stderr, "   pushed terminal %s %s\n", push_term_name, push_term_opts);
	}
	else {
		if(is_interactive)
			fputs("\tcurrent terminal type is unknown\n", stderr);
	}
}
//
// Pop the terminal. Called anytime by user command "set term pop".
//
void pop_terminal()
{
	if(push_term_name) {
		char * s;
		int i = strlen(push_term_name) + 11;
		if(push_term_opts) {
			// do_string() does not like backslashes -- thus remove them 
			for(s = push_term_opts; *s; s++)
				if(*s=='\\' || *s=='\n') 
					*s = ' ';
			i += strlen(push_term_opts);
		}
		s = (char *)malloc(i);
		i = GpGg.IsInteractive;
		GpGg.IsInteractive = 0;
		sprintf(s, "set term %s %s", push_term_name, (push_term_opts ? push_term_opts : ""));
		GpGg.Gp__C.DoStringAndFree(s);
		GpGg.IsInteractive = i ? true : false;
		if(GpGg.IsInteractive)
			fprintf(stderr, "   restored terminal is %s %s\n", term->name, ((*term_options) ? term_options : ""));
	}
	else
		fprintf(stderr, "No terminal has been pushed yet\n");
}
//
// Parse a plot style. Used by 'set style {data|function}' and by (s)plot
//
enum PLOT_STYLE get_style(GpCommand & rC)
{
	enum PLOT_STYLE ps; // defined in plot.h
	rC.CToken++;
	ps = (enum PLOT_STYLE)rC.LookupTable(&plotstyle_tbl[0], rC.CToken);
	rC.CToken++;
	if(ps == PLOT_STYLE_NONE)
		GpGg.IntErrorCurToken("unrecognized plot type");
	return ps;
}
//
// Parse options for style filledcurves and fill fco accordingly.
// If no option given, then set fco->opt_given to 0.
//
void get_filledcurves_style_options(GpCommand & rC, filledcurves_opts * fco)
{
	int    p = rC.LookupTable(&filledcurves_opts_tbl[0], rC.CToken);
	if(p == FILLEDCURVES_ABOVE) {
		fco->oneside = 1;
		p = rC.LookupTable(&filledcurves_opts_tbl[0], ++rC.CToken);
	}
	else if(p == FILLEDCURVES_BELOW) {
		fco->oneside = -1;
		p = rC.LookupTable(&filledcurves_opts_tbl[0], ++rC.CToken);
	}
	else
		fco->oneside = 0;
	if(p == -1) {
		fco->opt_given = 0; // no option given
	}
	else {
		fco->opt_given = 1;
		rC.CToken++;
		fco->closeto = p;
		fco->at = 0;
		if(rC.Eq("=")) {
			// parameter required for filledcurves x1=... and friends
			if(p < FILLEDCURVES_ATXY)
				fco->closeto += 4;
			rC.CToken++;
			fco->at = rC.RealExpression();
			if(p == FILLEDCURVES_ATXY) {
				// two values required for FILLEDCURVES_ATXY
				if(!rC.Eq(","))
					GpGg.IntErrorCurToken("syntax is xy=<x>,<y>");
				rC.CToken++;
				fco->aty = rC.RealExpression();
			}
		}
	}
}
//
// Print filledcurves style options to a file (used by 'show' and 'save' commands).
//
void filledcurves_options_tofile(filledcurves_opts * fco, FILE * fp)
{
	if(fco->opt_given) {
		if(fco->oneside)
			fputs(fco->oneside > 0 ? "above " : "below ", fp);
		if(fco->closeto == FILLEDCURVES_CLOSED) {
			fputs("closed", fp);
		}
		else if(fco->closeto <= FILLEDCURVES_Y2) {
			fputs(filledcurves_opts_tbl[fco->closeto].key, fp);
		}
		else if(fco->closeto <= FILLEDCURVES_ATY2) {
			fprintf(fp, "%s=%g", filledcurves_opts_tbl[fco->closeto - 4].key, fco->at);
		}
		else if(fco->closeto == FILLEDCURVES_ATXY) {
			fprintf(fp, "xy=%g,%g", fco->at, fco->aty);
		}
	}
}

//bool need_fill_border(fill_style_type * pFillStyle)
bool GpGadgets::NeedFillBorder(GpTermEntry * pT, fill_style_type * pFillStyle)
{
	lp_style_type p;
	p.pm3d_color = pFillStyle->border_color;
	if(p.pm3d_color.type == TC_LT) {
		// Doesn't want a border at all 
		if(p.pm3d_color.lt == LT_NODRAW)
			return false;
		load_linetype(&p, p.pm3d_color.lt+1);
	}
	// Wants a border in a new color
	if(p.pm3d_color.type != TC_DEFAULT)
		ApplyPm3DColor(pT, &p.pm3d_color);
	return true;
}

//int parse_dashtype(t_dashtype * dt)
int GpCommand::ParseDashType(t_dashtype * pDashType)
{
	int res = DASHTYPE_SOLID;
	int j = 0;
	int k = 0;
	char * dash_str = NULL;
	// Erase any previous contents 
	memzero(pDashType, sizeof(t_dashtype));
	// Fill in structure based on keyword ... 
	if(Eq("solid")) {
		res = DASHTYPE_SOLID;
		CToken++;
		// Or numerical pattern consisting of pairs solid,empty,solid,empty...
	}
	else if(Eq("(")) {
		CToken++;
		while(!EndOfCommand()) {
			if(j >= DASHPATTERN_LENGTH) {
				GpGg.IntErrorCurToken("too many pattern elements");
			}
			pDashType->pattern[j++] = (float)RealExpression(); /* The solid portion */
			if(!Eq(CToken++, ","))
				GpGg.IntErrorCurToken("expecting comma");
			pDashType->pattern[j++] = (float)RealExpression(); /* The empty portion */
			if(Eq(")"))
				break;
			if(!Eq(CToken++, ","))
				GpGg.IntErrorCurToken("expecting comma");
		}
		if(!Eq(")"))
			GpGg.IntErrorCurToken("expecting , or )");
		CToken++;
		res = DASHTYPE_CUSTOM;

		/* Or string representing pattern elements ... */
	}
	else if((dash_str = TryToGetString())) {
#define DSCALE 10.
		while(dash_str[j] && (k < DASHPATTERN_LENGTH || dash_str[j] == ' ')) {
			/* .      Dot with short space
			 * -      Dash with regular space
			 * _      Long dash with regular space
			 * space  Don't add new dash, just increase last space */
			switch(dash_str[j]) {
				case '.':
				    pDashType->pattern[k++] = 0.2 * DSCALE;
				    pDashType->pattern[k++] = 0.5 * DSCALE;
				    break;
				case '-':
				    pDashType->pattern[k++] = 1.0 * DSCALE;
				    pDashType->pattern[k++] = 1.0 * DSCALE;
				    break;
				case '_':
				    pDashType->pattern[k++] = 2.0 * DSCALE;
				    pDashType->pattern[k++] = 1.0 * DSCALE;
				    break;
				case ' ':
				    if(k > 0)
					    pDashType->pattern[k-1] += 1.0 * DSCALE;
				    break;
				default:
				    GpGg.IntError(CToken - 1, "expecting one of . - _ or space");
			}
			j++;
#undef  DSCALE
		}
		/* truncate dash_str if we ran out of space in the array representation */
		dash_str[j] = '\0';
		strncpy(pDashType->dstring, dash_str, sizeof(pDashType->dstring)-1);
		free(dash_str);
		res = DASHTYPE_CUSTOM;
		/* Or index of previously defined dashtype */
		/* FIXME: Is the index enough or should we copy its contents into this one? */
		/* FIXME: What happens if there is a recursive definition? */
	}
	else {
		res = IntExpression();
		if(res < 0)
			GpGg.IntError(CToken - 1, "dashtype must be non-negative");
		else
			res = (res == 0) ? DASHTYPE_AXIS : (res - 1);
	}
	return res;
}
//
// destination_class tells us whether we are filling in a line style ('set style line'),
// a persistant linetype ('set linetype') or an ad hoc set of properties for a single
// use ('plot ... lc foo lw baz').
// allow_point controls whether we accept a point attribute in this lp_style.
//
//int lp_parse(lp_style_type * lp, lp_class destination_class, bool allow_point)
int GpGadgets::LpParse(GpCommand & rC, lp_style_type & rLp, lp_class destinationClass, bool allowPoint)
{
	// keep track of which options were set during this call 
	int    set_lt = 0;
	int    set_pal = 0;
	int    set_lw = 0;
	int    set_pt = 0;
	int    set_ps  = 0;
	int    set_pi = 0;
	int    set_dt = 0;
	int    new_lt = 0;
	//
	// EAM Mar 2010 - We don't want properties from a user-defined default
	// linetype to override properties explicitly set here.  So fill in a
	// local lp_style_type as we go and then copy over the specifically
	// requested properties on top of the default ones.
	//
	lp_style_type newlp = rLp;
	if((destinationClass == LP_ADHOC) && (rC.AlmostEq("lines$tyle") || rC.Eq("ls"))) {
		rC.CToken++;
		lp_use_properties(&rLp, rC.IntExpression());
	}
	while(!rC.EndOfCommand()) {
		// This special case is to flag an attemp to "set object N lt <lt>",
		// which would otherwise be accepted but ignored, leading to confusion
		// FIXME:  Couldn't this be handled at a higher level?
		if((destinationClass == LP_NOFILL) && (rC.Eq("lt") || rC.AlmostEq("linet$ype"))) {
			IntErrorCurToken("object linecolor must be set using fillstyle border");
		}
		if(rC.AlmostEq("linet$ype") || rC.Eq("lt")) {
			if(set_lt++)
				break;
			else {
				if(destinationClass == LP_TYPE)
					IntErrorCurToken("linetype definition cannot use linetype");
				rC.CToken++;
				if(rC.AlmostEq("rgb$color")) {
					if(set_pal++)
						break;
					else {
						rC.CToken--;
						rC.ParseColorSpec(&(newlp.pm3d_color), TC_RGB);
					}
				}
				else if(rC.AlmostEq("pal$ette")) { // both syntaxes allowed: 'with lt pal' as well as 'with pal'
					if(set_pal++)
						break;
					else {
						rC.CToken--;
						rC.ParseColorSpec(&(newlp.pm3d_color), TC_Z);
					}
				}
				else if(rC.Eq("bgnd")) {
					rLp = BackgroundLp;
					rC.CToken++;
				}
				else if(rC.Eq("black")) {
					rLp = DefaultBorderLp;
					rC.CToken++;
				}
				else if(rC.Eq("nodraw")) {
					rLp.l_type = LT_NODRAW;
					rC.CToken++;
				}
				else {
					// These replace the base style 
					new_lt = rC.IntExpression();
					rLp.l_type = new_lt - 1;
					// user may prefer explicit line styles 
					if(prefer_line_styles && destinationClass != LP_STYLE)
						lp_use_properties(&rLp, new_lt);
					else
						load_linetype(&rLp, new_lt);
				}
			}
		}
		// both syntaxes allowed: 'with lt pal' as well as 'with pal'
		if(rC.AlmostEq("pal$ette")) {
			if(set_pal++)
				break;
			else {
				rC.CToken--;
				rC.ParseColorSpec(&(newlp.pm3d_color), TC_Z);
				continue;
			}
		}
		// This is so that "set obj ... lw N fc <colorspec>" doesn't eat 
		// up the colorspec as a line property.  We need to parse it later as a _fill_ property
		if((destinationClass == LP_NOFILL) && (rC.Eq("fc") || rC.AlmostEq("fillc$olor")))
			break;
		if(rC.Eq("lc") || rC.AlmostEq("linec$olor") || rC.Eq("fc") || rC.AlmostEq("fillc$olor")) {
			if(set_pal++)
				break;
			else {
				rC.CToken++;
				if(rC.AlmostEq("rgb$color") || rC.IsString(rC.CToken)) {
					rC.CToken--;
					rC.ParseColorSpec(&(newlp.pm3d_color), TC_RGB);
				}
				else if(rC.AlmostEq("pal$ette")) {
					rC.CToken--;
					rC.ParseColorSpec(&(newlp.pm3d_color), TC_Z);
				}
				else if(rC.Eq("bgnd")) {
					newlp.pm3d_color.type = TC_LT;
					newlp.pm3d_color.lt = LT_BACKGROUND;
					rC.CToken++;
				}
				else if(rC.Eq("black")) {
					newlp.pm3d_color.type = TC_LT;
					newlp.pm3d_color.lt = LT_BLACK;
					rC.CToken++;
				}
				else if(rC.AlmostEq("var$iable")) {
					rC.CToken++;
					newlp.l_type = LT_COLORFROMCOLUMN;
					newlp.pm3d_color.type = TC_LINESTYLE;
				}
				else {
					// Pull the line colour from a default linetype, but 
					// only if we are not in the middle of defining one! 
					if(destinationClass != LP_STYLE) {
						lp_style_type temp;
						load_linetype(&temp, rC.IntExpression());
						newlp.pm3d_color = temp.pm3d_color;
					}
					else {
						newlp.pm3d_color.type = TC_LT;
						newlp.pm3d_color.lt = rC.IntExpression() - 1;
					}
				}
				continue;
			}
		}
		if(rC.AlmostEq("linew$idth") || rC.Eq("lw")) {
			if(set_lw++)
				break;
			else {
				rC.CToken++;
				newlp.l_width = rC.RealExpression();
				if(newlp.l_width < 0)
					newlp.l_width = 0;
				continue;
			}
		}
		if(rC.Eq("bgnd")) {
			if(set_lt++)
				break;
			else {
				rC.CToken++;
				rLp = BackgroundLp;
				continue;
			}
		}
		if(rC.Eq("black")) {
			if(set_lt++)
				break;
			else {
				rC.CToken++;
				rLp = DefaultBorderLp;
				continue;
			}
		}
		if(rC.AlmostEq("pointt$ype") || rC.Eq("pt")) {
			if(allowPoint) {
				if(set_pt++)
					break;
				else {
					rC.CToken++;
					char * symbol = rC.TryToGetString();
					if(symbol) {
						newlp.p_type = PT_CHARACTER;
						// An alternative mechanism would be to use utf8toulong(&newlp.p_char, symbol);
						strncpy(newlp.p_char, symbol, sizeof(newlp.p_char)-1);
						// Truncate ascii text to single character 
						if((newlp.p_char[0] & 0x80) == 0)
							newlp.p_char[1] = '\0';
						// strncpy does not guarantee null-termination 
						newlp.p_char[sizeof(newlp.p_char)-1] = '\0';
						free(symbol);
					}
					else if(rC.AlmostEq("var$iable") && (destinationClass == LP_ADHOC)) {
						newlp.p_type = PT_VARIABLE;
						rC.CToken++;
					}
					else {
						newlp.p_type = rC.IntExpression() - 1;
					}
				}
			}
			else {
				IntWarn(rC.CToken, "No pointtype specifier allowed, here");
				rC.CToken += 2;
			}
			continue;
		}
		if(rC.AlmostEq("points$ize") || rC.Eq("ps")) {
			if(allowPoint) {
				if(set_ps++)
					break;
				else {
					rC.CToken++;
					if(rC.AlmostEq("var$iable")) {
						newlp.p_size = PTSZ_VARIABLE;
						rC.CToken++;
					}
					else if(rC.AlmostEq("def$ault")) {
						newlp.p_size = PTSZ_DEFAULT;
						rC.CToken++;
					}
					else {
						newlp.p_size = rC.RealExpression();
						if(newlp.p_size < 0)
							newlp.p_size = 0;
					}
				}
			}
			else {
				IntWarn(rC.CToken, "No pointsize specifier allowed, here");
				rC.CToken += 2;
			}
			continue;
		}
		if(rC.AlmostEq("pointi$nterval") || rC.Eq("pi")) {
			rC.CToken++;
			if(allowPoint) {
				newlp.p_interval = rC.IntExpression();
				set_pi = 1;
			}
			else {
				IntWarn(rC.CToken, "No pointinterval specifier allowed, here");
				rC.IntExpression();
			}
			continue;
		}
		if(rC.AlmostEq("dasht$ype") || rC.Eq("dt")) {
			if(set_dt++)
				break;
			rC.CToken++;
			int tmp = rC.ParseDashType(&newlp.custom_dash_pattern);
			// Pull the dashtype from the list of already defined dashtypes,
			// but only if it we didn't get an explicit one back from parse_dashtype
			if(tmp == DASHTYPE_AXIS)
				rLp.l_type = LT_AXIS;
			if(tmp >= 0)
				tmp = load_dashtype(&newlp.custom_dash_pattern, tmp + 1);
			newlp.d_type = tmp;
			continue;
		}
		
		break; // caught unknown option -> quit the while(1) loop
	}
	if(set_lt > 1 || set_pal > 1 || set_lw > 1 || set_pt > 1 || set_ps > 1 || set_dt > 1)
		IntErrorCurToken("duplicated arguments in style specification");
	if(set_pal) {
		rLp.pm3d_color = newlp.pm3d_color;
		// hidden3d uses this to decide that a single color surface is wanted
		rLp.flags |= LP_EXPLICIT_COLOR;
	}
	else {
		rLp.flags &= ~LP_EXPLICIT_COLOR;
	}
	if(set_lw)
		rLp.l_width = newlp.l_width;
	if(set_pt) {
		rLp.p_type = newlp.p_type;
		memcpy(rLp.p_char, newlp.p_char, sizeof(newlp.p_char));
	}
	if(set_ps)
		rLp.p_size = newlp.p_size;
	if(set_pi)
		rLp.p_interval = newlp.p_interval;
	if(newlp.l_type == LT_COLORFROMCOLUMN)
		rLp.l_type = LT_COLORFROMCOLUMN;
	if(set_dt) {
		rLp.d_type = newlp.d_type;
		rLp.custom_dash_pattern = newlp.custom_dash_pattern;
	}
	return new_lt;
}

/* <fillstyle> = {empty | solid {<density>} | pattern {<n>}} {noborder | border {<lt>}} */
const GenTable fs_opt_tbl[] = {
	{"e$mpty", FS_EMPTY},
	{"s$olid", FS_SOLID},
	{"p$attern", FS_PATTERN},
	{NULL, -1}
};

//void parse_fillstyle(fill_style_type * fs, int def_style, int def_density, int def_pattern, t_colorspec def_bordertype)
void GpCommand::ParseFillStyle(fill_style_type * fs, int def_style, int def_density, int def_pattern, t_colorspec def_bordertype)
{
	// Set defaults
	fs->fillstyle = def_style;
	fs->filldensity = def_density;
	fs->fillpattern = def_pattern;
	fs->border_color = def_bordertype;
	if(!EndOfCommand() && (Eq("fs") || AlmostEq("fill$style"))) {
		bool set_fill = false;
		bool set_border = false;
		bool transparent = false;
		CToken++;
		while(!EndOfCommand()) {
			if(AlmostEq("trans$parent")) {
				transparent = true;
				CToken++;
			}
			else {
				int i = LookupTable(fs_opt_tbl, CToken);
				switch(i) {
					default:
						break;
					case FS_EMPTY:
					case FS_SOLID:
					case FS_PATTERN:
						if(set_fill && fs->fillstyle != i)
							GpGg.IntErrorCurToken("conflicting option");
						fs->fillstyle = i;
						set_fill = true;
						CToken++;
						if(IsANumber(CToken) || TypeDdv(CToken) == INTGR || TypeDdv(CToken) == CMPLX) {
							if(fs->fillstyle == FS_SOLID) {
								/* user sets 0...1, but is stored as an integer 0..100 */
								fs->filldensity = (int)(100.0 * RealExpression() + 0.5);
								SETMAX(fs->filldensity, 0);
								SETMIN(fs->filldensity, 100);
							}
							else if(fs->fillstyle == FS_PATTERN) {
								fs->fillpattern = IntExpression();
								SETMAX(fs->fillpattern, 0);
							}
							else
								GpGg.IntErrorCurToken("this fill style does not have a parameter");
						}
						continue;
				}
				if(AlmostEq("bo$rder")) {
					if(set_border && fs->border_color.lt == LT_NODRAW)
						GpGg.IntErrorCurToken("conflicting option");
					fs->border_color.type = TC_DEFAULT;
					set_border = true;
					CToken++;
					if(!EndOfCommand()) {
						if(Eq("-") || IsANumber(CToken)) {
							fs->border_color.type = TC_LT;
							fs->border_color.lt = IntExpression() - 1;
						}
						else if(Eq("lc") || AlmostEq("linec$olor")) {
							ParseColorSpec(&fs->border_color, TC_Z);
						}
						else if(Eq("rgb") || Eq("lt") || AlmostEq("linet$ype")) {
							CToken--;
							ParseColorSpec(&fs->border_color, TC_Z);
						}
					}
					continue;
				}
				else if(AlmostEq("nobo$rder")) {
					if(set_border && fs->border_color.lt != LT_NODRAW)
						GpGg.IntErrorCurToken("conflicting option");
					fs->border_color.type = TC_LT;
					fs->border_color.lt = LT_NODRAW;
					set_border = true;
					CToken++;
					continue;
				}
				// Keyword must belong to someone else
				break;
			}
		}
		if(transparent) {
			if(fs->fillstyle == FS_SOLID)
				fs->fillstyle = FS_TRANSPARENT_SOLID;
			else if(fs->fillstyle == FS_PATTERN)
				fs->fillstyle = FS_TRANSPARENT_PATTERN;
		}
	}
}

/*
 * Parse the sub-options of text color specification
 *   { def$ault | lt <linetype> | pal$ette { cb <val> | frac$tion <val> | z }
 * The ordering of alternatives shown in the line above is kept in the symbol definitions
 * TC_DEFAULT TC_LT TC_LINESTYLE TC_RGB TC_CB TC_FRAC TC_Z TC_VARIABLE (0 1 2 3 4 5 6 7)
 * and the "options" parameter to parse_colorspec limits legal input to the
 * corresponding point in the series. So TC_LT allows only default or linetype
 * coloring, while TC_Z allows all coloring options up to and including pal z
 */
//void parse_colorspec(t_colorspec * tc, int options)
void GpCommand::ParseColorSpec(t_colorspec * tc, int options)
{
	CToken++;
	if(EndOfCommand())
		GpGg.IntErrorCurToken("expected colorspec");
	if(AlmostEq("def$ault")) {
		CToken++;
		tc->type = TC_DEFAULT;
	}
	else if(Eq("bgnd")) {
		CToken++;
		tc->type = TC_LT;
		tc->lt = LT_BACKGROUND;
	}
	else if(Eq("black")) {
		CToken++;
		tc->type = TC_LT;
		tc->lt = LT_BLACK;
	}
	else if(Eq("lt")) {
		CToken++;
		lp_style_type lptemp;
		if(EndOfCommand())
			GpGg.IntErrorCurToken("expected linetype");
		tc->type = TC_LT;
		tc->lt = IntExpression()-1;
		if(tc->lt < LT_BACKGROUND) {
			tc->type = TC_DEFAULT;
			GpGg.IntWarn(CToken, "illegal linetype");
		}
		/*
		 * July 2014 - translate linetype into user-defined linetype color.
		 * This is a CHANGE!
		 * FIXME: calling load_linetype here may obviate the need to call it
		 * many places in the higher level code.  They could be removed.
		 */
		load_linetype(&lptemp, tc->lt + 1);
		*tc = lptemp.pm3d_color;
	}
	else if(options <= TC_LT) {
		tc->type = TC_DEFAULT;
		GpGg.IntErrorCurToken("only tc lt <n> possible here");
	}
	else if(Eq("ls") || AlmostEq("lines$tyle")) {
		CToken++;
		tc->type = TC_LINESTYLE;
		tc->lt = (int)RealExpression();
	}
	else if(AlmostEq("rgb$color")) {
		CToken++;
		tc->type = TC_RGB;
		if(AlmostEq("var$iable")) {
			tc->value = -1.0;
			CToken++;
		}
		else {
			tc->value = 0.0;
			tc->lt = parse_color_name();
		}
	}
	else if(AlmostEq("pal$ette")) {
		CToken++;
		if(Eq("z")) {
			// The actual z value is not yet known, fill it in later 
			if(options >= TC_Z) {
				tc->type = TC_Z;
			}
			else {
				tc->type = TC_DEFAULT;
				GpGg.IntErrorCurToken("palette z not possible here");
			}
			CToken++;
		}
		else if(Eq("cb")) {
			tc->type = TC_CB;
			CToken++;
			if(EndOfCommand())
				GpGg.IntErrorCurToken("expected cb value");
			tc->value = RealExpression();
		}
		else if(AlmostEq("frac$tion")) {
			tc->type = TC_FRAC;
			CToken++;
			if(EndOfCommand())
				GpGg.IntErrorCurToken("expected palette fraction");
			tc->value = RealExpression();
			if(tc->value < 0. || tc->value > 1.0)
				GpGg.IntErrorCurToken("palette fraction out of range");
		}
		else {
			// EndOfCommand() or palette <blank> 
			if(options >= TC_Z)
				tc->type = TC_Z;
		}
	}
	else if(options >= TC_VARIABLE && AlmostEq("var$iable")) {
		CToken++;
		tc->type = TC_VARIABLE;
		// New: allow to skip the rgb keyword, as in  'plot $foo lc "blue"' 
	}
	else if(IsString(CToken)) {
		tc->type = TC_RGB;
		tc->lt = parse_color_name();
	}
	else {
		GpGg.IntErrorCurToken("colorspec option not recognized");
	}
}

long parse_color_name()
{
	char * string;
	long color = -2;
	/* Terminal drivers call this after seeing a "background" option */
	if(GpGg.Gp__C.AlmostEq("rgb$color") && GpGg.Gp__C.AlmostEq(GpGg.Gp__C.CToken-1, "back$ground"))
		GpGg.Gp__C.CToken++;
	if((string = GpGg.Gp__C.TryToGetString())) {
		int iret = lookup_table_nth(pm3d_color_names_tbl, string);
		if(iret >= 0)
			color = pm3d_color_names_tbl[iret].value;
		else if(string[0] == '#')
			iret = sscanf(string, "#%lx", &color);
		else if(string[0] == '0' && (string[1] == 'x' || string[1] == 'X'))
			iret = sscanf(string, "%lx", &color);
		free(string);
		if(color == -2)
			GpGg.IntErrorCurToken("unrecognized color name and not a string \"#AARRGGBB\" or \"0xAARRGGBB\"");
	}
	else {
		color = GpGg.Gp__C.IntExpression();
	}
	return (uint)(color);
}

/* arrow parsing...
 *
 * allow_as controls whether we are allowed to accept arrowstyle in
 * the current context [ie not when doing a  set style arrow command]
 */

void arrow_use_properties(arrow_style_type * arrow, int tag)
{
	/*  This function looks for an arrowstyle defined by 'tag' and
	 *  copies its data into the structure 'ap'. */
	arrowstyle_def * p_this;
	p_this = GpGg.first_arrowstyle;
	while(p_this != NULL) {
		if(p_this->tag == tag) {
			*arrow = p_this->arrow_properties;
			return;
		}
		else {
			p_this = p_this->next;
		}
	}
	/* tag not found: */
	default_arrow_style(arrow);
	GpGg.IntWarn(NO_CARET, "arrowstyle %d not found", tag);
}

void arrow_parse(GpCommand & rC, arrow_style_type * arrow, bool allow_as)
{
	int set_layer = 0, set_line = 0, set_head = 0;
	int set_headsize = 0, set_headfilled = 0;
	// Use predefined arrow style 
	if(allow_as && (rC.AlmostEq("arrows$tyle") || rC.Eq("as"))) {
		rC.CToken++;
		if(rC.AlmostEq("var$iable")) {
			arrow->tag = AS_VARIABLE;
			rC.CToken++;
		}
		else
			arrow_use_properties(arrow, rC.IntExpression());
	}
	else {
		// No predefined arrow style; read properties from command line 
		// avoid duplicating options 
		while(!rC.EndOfCommand()) {
			if(rC.Eq("nohead")) {
				if(set_head++)
					break;
				rC.CToken++;
				arrow->head = NOHEAD;
			}
			else if(rC.Eq("head")) {
				if(set_head++)
					break;
				rC.CToken++;
				arrow->head = END_HEAD;
			}
			else if(rC.Eq("backhead")) {
				if(set_head++)
					break;
				rC.CToken++;
				arrow->head = BACKHEAD;
			}
			else if(rC.Eq("heads")) {
				if(set_head++)
					break;
				rC.CToken++;
				arrow->head = (t_arrow_head)(BACKHEAD | END_HEAD);
			}
			else if(rC.AlmostEq("nobo$rder")) {
				if(set_headfilled++)
					break;
				rC.CToken++;
				arrow->headfill = AS_NOBORDER;
			}
			else if(rC.AlmostEq("fill$ed")) {
				if(set_headfilled++)
					break;
				rC.CToken++;
				arrow->headfill = AS_FILLED;
			}
			else if(rC.AlmostEq("empty")) {
				if(set_headfilled++)
					break;
				rC.CToken++;
				arrow->headfill = AS_EMPTY;
			}
			else if(rC.AlmostEq("nofill$ed")) {
				if(set_headfilled++)
					break;
				rC.CToken++;
				arrow->headfill = AS_NOFILL;
			}
			else if(rC.Eq("size")) {
				GpPosition hsize;
				if(set_headsize++)
					break;
				hsize.scalex = hsize.scaley = hsize.scalez = first_axes;
				// only scalex used; scaley is angle of the head in [deg] 
				rC.CToken++;
				if(rC.EndOfCommand())
					GpGg.IntErrorCurToken("head size expected");
				GpGg.GetPosition(rC, &hsize);
				arrow->head_length = hsize.x;
				arrow->head_lengthunit = hsize.scalex;
				arrow->head_angle = hsize.y;
				arrow->head_backangle = hsize.z;
				// invalid backangle --> default of 90.0 degrees 
				if(arrow->head_backangle <= arrow->head_angle)
					arrow->head_backangle = 90.0;
				// Assume adjustable size but check for 'fixed' instead 
				arrow->head_fixedsize = false;
			}
			else if(rC.AlmostEq("fix$ed")) {
				arrow->head_fixedsize = true;
				rC.CToken++;
			}
			else if(rC.Eq("back")) {
				if(set_layer++)
					break;
				rC.CToken++;
				arrow->layer = LAYER_BACK;
			}
			else if(rC.Eq("front")) {
				if(set_layer++)
					break;
				rC.CToken++;
				arrow->layer = LAYER_FRONT;
			}
			else {
				// pick up a line spec - allow ls, but no point
				{
					int stored_token = rC.CToken;
					GpGg.LpParse(rC, arrow->lp_properties, LP_ADHOC, false);
					if(stored_token == rC.CToken || set_line++)
						break;
					continue;
				}
				// unknown option caught -> quit the while(1) loop 
				break;
			}
		}
		if(set_layer>1 || set_line>1 || set_head>1 || set_headsize>1 || set_headfilled>1)
			GpGg.IntErrorCurToken("duplicated arguments in style specification");
	}
}

void get_image_options(t_image * image)
{
	if(GpGg.Gp__C.AlmostEq("pix$els") || GpGg.Gp__C.Eq("failsafe")) {
		GpGg.Gp__C.CToken++;
		image->fallback = true;
	}
}

