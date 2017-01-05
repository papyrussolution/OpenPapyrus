/* GNUPLOT - plot.c */

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
#include "version.h"
#include <signal.h>
#include <setjmp.h>

#ifdef USE_MOUSE
	//#include "mouse.h" /* for GpGg.Mse.Cfg */
	#include "gpexecute.h"
#endif
#if defined(__EMX__) || (defined(WGP_CONSOLE) && defined(MSVC))
	#include <io.h>
#endif
#ifdef VMS
	#ifndef __GNUC__
		#include <unixio.h>
	#endif
	#include <smgdef.h>
	#include <ssdef.h>
	extern int vms_vkid;
	extern smg$create_virtual_keyboard();
	extern int vms_ktid;
	extern smg$create_key_table();
#endif /* VMS */
#ifdef _Windows
	#include <windows.h>
	#include "win/winmain.h"
	#include "win/wcommon.h"
#endif /* _Windows */
/* GNU readline
 * Only required by two files directly,
 * so I don't put this into a header file. -lh
 */
#if defined(HAVE_LIBREADLINE) && !defined(MISSING_RL_TILDE_EXPANSION)
	#include <readline/tilde.h>
	extern int rl_complete_with_tilde_expansion;
#endif

/* BSD editline
 */
#ifdef HAVE_LIBEDITLINE
	#include <editline/readline.h>
#endif
// enable gnuplot history with readline 
#ifdef GNUPLOT_HISTORY
	#ifndef GNUPLOT_HISTORY_FILE
		#define GNUPLOT_HISTORY_FILE "~/.gnuplot_history"
	#endif
	/*
	* expanded_history_filename points to the value from 'tilde_expand()',
	* which expands '~' to the user's home directory, or $HOME.
	* Depending on your OS you have to make sure that the "$HOME" environment
	* variable exists.  You are responsible for valid values.
	*/
	static char * expanded_history_filename;
	static void wrapper_for_write_history();
#endif // GNUPLOT_HISTORY 
static const char * user_homedir = NULL; /* user home directory */
static bool successful_initialization = false;
#ifdef X11
	extern int X11_args(int, char **); /* FIXME: defined in term/x11.trm */
#endif
// patch to get home dir, see command.c 
#ifdef DJGPP
	#include <dir.h>               /* MAXPATH */
	char HelpFile[MAXPATH];
#endif /*   - DJL */

/* a longjmp buffer to get back to the command line */
static JMP_BUF command_line_env;

//static void load_rcfile(int where);
static RETSIGTYPE inter(int anint);
//static void init_memory();

static int exit_status = EXIT_SUCCESS;

static RETSIGTYPE inter(int anint)
{
	(void)anint;            /* aovid -Wunused warning */
	signal(SIGINT, (sigfunc)inter);
	signal(SIGFPE, SIG_DFL);  /* turn off FPE trapping */
#if defined(WGP_CONSOLE)
	/* The Windows console Ctrl-C handler runs in another thread. So a
	   longjmp() would result in crash. Instead, we handle these
	   events asynchronously.
	 */
	ctrlc_flag = true;
	/* Interrupt ConsoleGetch. */
	SendMessage(graphwin->hWndGraph, WM_NULL, 0, 0);
	SendMessage(GetConsoleWindow(), WM_CHAR, 0x20, 0);
#else
	{
		term_reset();
		putc('\n', stderr);
		bail_to_command_line(); /* return to prompt */
	}
#endif
}

#ifdef LINUXVGA
/* utility functions to ensure that setuid gnuplot
 * assumes root privileges only for those parts
 * of the code which require root rights.
 *
 * By "Dr. Werner Fink" <werner@suse.de>
 */
static uid_t euid, ruid;
static gid_t egid, rgid;
static int asked_privi = 0;

void drop_privilege()
{
	if(!asked_privi) {
		euid = geteuid();
		egid = getegid();
		ruid = getuid();
		rgid = getgid();
		asked_privi = 1;
	}
	if(setegid(rgid) == -1)
		fprintf(stderr, "setegid(%d): %s\n", (int)rgid, strerror(errno));
	if(seteuid(ruid) == -1)
		fprintf(stderr, "seteuid(%d): %s\n", (int)ruid, strerror(errno));
}

void take_privilege()
{
	if(!asked_privi) {
		euid = geteuid();
		egid = getegid();
		ruid = getuid();
		rgid = getgid();
		asked_privi = 1;
	}
	if(setegid(egid) == -1)
		fprintf(stderr, "setegid(%d): %s\n", (int)egid, strerror(errno));
	if(seteuid(euid) == -1)
		fprintf(stderr, "seteuid(%d): %s\n", (int)euid, strerror(errno));
}

#endif /* LINUXVGA */
//
// a wrapper for longjmp so we can keep everything local 
//
void bail_to_command_line()
{
#ifdef _Windows
	kill_pending_Pause_dialog();
	GpGg.ctrlc_flag = false;
#endif
	LONGJMP(command_line_env, true);
}

#if defined(_Windows)
int gnu_main(int argc, char ** argv)
#else
int main(int argc, char ** argv)
#endif
{
	return GpGg.Run(argc, argv);
}

int GpGadgets::Run(int argc, char ** argv)
{
	int i;
#ifdef LINUXVGA
	LINUX_setup();          /* setup VGA before dropping privilege DBT 4/5/99 */
	drop_privilege();
#endif
/* make sure that we really have revoked root access, this might happen if
   gnuplot is compiled without vga support but is installed suid by mistake */
#ifdef __linux__
	setuid(getuid());
#endif
/* HBB: Seems this isn't needed any more for DJGPP V2? */
/* HBB: disable all floating point exceptions, just keep running... */
#if defined(DJGPP) && (DJGPP!=2)
	_control87(MCW_EM, MCW_EM);
#endif
// malloc large blocks, otherwise problems with fragmented mem 
#ifdef MALLOCDEBUG
	malloc_debug(7);
#endif
//
// get helpfile from home directory 
//
#ifdef __DJGPP__
	{
		char * s;
		strcpy(HelpFile, argv[0]);
		for(s = HelpFile; *s; s++)
			if(*s == DIRSEP1)
				*s = DIRSEP2;  /* '\\' to '/' */
		strcpy(strrchr(HelpFile, DIRSEP2), "/gnuplot.gih");
	}               /* Add also some "paranoid" tests for '\\':  AP */
#endif /* DJGPP */
#ifdef VMS
	uint status[2] = { 1, 0 };
#endif
#if defined(HAVE_LIBEDITLINE)
	rl_getc_function = getc_wrapper;
#endif
#if defined(HAVE_LIBREADLINE) || defined(HAVE_LIBEDITLINE)
	/* T.Walter 1999-06-24: 'rl_readline_name' must be this fix name.
	 * It is used to parse a 'gnuplot' specific section in '~/.inputrc'
	 * or gnuplot specific commands in '.editrc' (when using editline
	 * instead of readline) */
	rl_readline_name = "Gnuplot";
	rl_terminal_name = getenv("TERM");
#if defined(HAVE_LIBREADLINE)
	using_history();
#else
	history_init();
#endif
#endif
#if defined(HAVE_LIBREADLINE) && !defined(MISSING_RL_TILDE_EXPANSION)
	rl_complete_with_tilde_expansion = 1;
#endif
	for(i = 1; i < argc; i++) {
		if(!argv[i])
			continue;
		if(!strcmp(argv[i], "-V") || !strcmp(argv[i], "--version")) {
			printf("gnuplot %s patchlevel %s\n", gnuplot_version, gnuplot_patchlevel);
			return 0;
		}
		else if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			printf("Usage: gnuplot [OPTION] ... [FILE]\n"
#ifdef X11
			    "for X11 options see 'help X11->command-line-options'\n"
#endif
			    "  -V, --version\n"
			    "  -h, --help\n"
			    "  -p  --persist\n"
			    "  -d  --default-settings\n"
			    "  -c  scriptfile ARG1 ARG2 ... \n"
			    "  -e  \"command1; command2; ...\"\n"
			    "gnuplot %s patchlevel %s\n",
			    gnuplot_version, gnuplot_patchlevel);
#ifdef DEVELOPMENT_VERSION
			printf(
#ifdef DIST_CONTACT
			    "Report bugs to "DIST_CONTACT "\n            or %s\n",
#else
			    "Report bugs to %s\n",
#endif
			    bug_email);
#endif
			return 0;
		}
		else if(!strncmp(argv[i], "-persist", 2) || !strcmp(argv[i], "--persist")
#ifdef _Windows
		    || !_stricmp(argv[i], "-noend") || !_stricmp(argv[i], "/noend")
#endif
		    ) {
			persist_cl = true;
		}
		else if(!strncmp(argv[i], "-d", 2) || !strcmp(argv[i], "--default-settings")) {
			// Skip local customization read from ~/.gnuplot 
			skip_gnuplotrc = true;
		}
	}
#ifdef X11
	// the X11 terminal removes tokens that it recognizes from argv
	{
		int n = X11_args(argc, argv);
		argv += n;
		argc -= n;
	}
#endif
	setbuf(stderr, (char*)NULL);
#ifdef HAVE_SETVBUF
	// This was once setlinebuf(). Docs say this is identical to setvbuf(,NULL,_IOLBF,0), but MS C
	// faults this (size out of range), so we try with size of 1024 instead. [SAS/C does that, too. -lh]
	if(setvbuf(stdout, (char*)NULL, _IOLBF, (size_t)1024) != 0)
		fputs("Could not linebuffer stdout\n", stderr);
	// Switching to unbuffered mode causes all characters in the input
	// buffer to be lost. So the only safe time to do it is on program entry.
	// Do any non-X platforms suffer from this problem?
	// EAM - Jan 2013 YES.
	setvbuf(stdin, (char*)NULL, _IONBF, 0);
#endif
	gpoutfile = stdout;
	// Initialize pre-loaded user variables
	// "pi" is hard-wired as the first variable
	Ev.AddUdvByName("GNUTERM");
	Ev.AddUdvByName("NaN");
	InitConstants();
	Ev.udv_user_head = &Ev.udv_NaN->next_udv;
	GpC.InitMemory();
	IsInteractive = false;
	init_terminal(); // can set term type if it likes 
	push_terminal(0); // remember the default terminal 
	/* reset the terminal when exiting */
	/* this is done through gp_atexit so that other terminal functions
	 * can be registered to be executed before the terminal is reset. */
	gp_atexit(term_reset);
#if defined(WIN32) && !defined(WGP_CONSOLE)
	IsInteractive = true;
#else
	IsInteractive = isatty(fileno(stdin));
#endif
	/* Note: we want to know whether this is an interactive session so that we can
	 * decide whether or not to write status information to stderr.  The old test
	 * for this was to see if(argc > 1) but the addition of optional command line
	 * switches broke this.  What we really wanted to know was whether any of the
	 * command line arguments are file names or an explicit in-line "-e command".
	 */
	for(i = 1; i < argc; i++) {
# ifdef _Windows
		if(!_stricmp(argv[i], "/noend"))
			continue;
# endif
		if((argv[i][0] != '-') || (argv[i][1] == 'e') || (argv[i][1] == 'c') ) {
			IsInteractive = false;
			break;
		}
	}
	// Need this before show_version is called for the first time
	show_version(GpC, IsInteractive ? stderr : NULL); // Only load GPVAL_COMPILE_OPTIONS
#ifdef WGP_CONSOLE
#ifdef CONSOLE_SWITCH_CP
	if(cp_changed && interactive) {
		fprintf(stderr,
		    "\ngnuplot changed the codepage of this console from %i to %i to\n"	\
		    "match the graph window. Some characters might only display correctly\n" \
		    "if you change the font to a non-raster type.\n",
		    cp_input, GetConsoleCP());
	}
#else
	if((GetConsoleCP() != GetACP()) && interactive) {
		fprintf(stderr,
		    "\nWarning: The codepage of the graph window (%i) and that of the\n" \
		    "console (%i) differ. Use `set encoding` or `!chcp` if extended\n" \
		    "characters don't display correctly.\n",
		    GetACP(), GetConsoleCP());
	}
#endif
#endif
	Ev.UpdateGpValVariables(3); /* update GPVAL_ variables available to user */
#ifdef VMS
	// initialise screen management routines for command recall
	if(status[1] = smg$create_virtual_keyboard(&vms_vkid) != SS$_NORMAL)
		done(status[1]);
	if(status[1] = smg$create_key_table(&vms_ktid) != SS$_NORMAL)
		done(status[1]);
#endif /* VMS */
	if(!SETJMP(command_line_env, 1)) {
		// first time
		interrupt_setup();
		// should move this stuff to another initialisation routine,
		// something like init_set() maybe 
		get_user_env();
		init_loadpath();
		init_locale();
		memzero(&SmPalette, sizeof(SmPalette));
		GpF.Init(); // Initialization of fitting module
		InitSession(GpC);
		if(IsInteractive && term != 0) {  /* not unknown */
#ifdef GNUPLOT_HISTORY
#if defined(HAVE_LIBREADLINE) || defined(HAVE_LIBEDITLINE)
			expanded_history_filename = tilde_expand(GNUPLOT_HISTORY_FILE);
#else
			expanded_history_filename = gp_strdup(GNUPLOT_HISTORY_FILE);
			gp_expand_tilde(&expanded_history_filename);
#endif
			GpC.H.ReadHistory(expanded_history_filename);
			/*
			 * It is safe to ignore the return values of 'atexit()' and
			 * 'on_exit()'. In the worst case, there is no history of your
			 * currrent session and you have to type all again in your next session.
			 */
			gp_atexit(wrapper_for_write_history);
#endif /* GNUPLOT_HISTORY */
			fprintf(stderr, "\nTerminal type set to '%s'\n", term->name);
		}               /* if(interactive && term != 0) */
	}
	else {
		// come back here from int_error() 
		if(!successful_initialization) {
			// Only print the warning once 
			successful_initialization = true;
			fprintf(stderr, "WARNING: Error during initialization\n\n");
		}
		if(IsInteractive == false)
			exit_status = EXIT_FAILURE;
#ifdef HAVE_READLINE_RESET
		else {
			rl_reset_after_signal(); // reset properly readline after a SIGINT+longjmp
		}
#endif
		load_file_error(); // if we were in load_file(), cleanup 
		SET_CURSOR_ARROW;
#ifdef VMS
		// after catching interrupt 
		// VAX stuffs up stdout on SIGINT while writing to stdout, so reopen stdout. 
		if(gpoutfile == stdout) {
			if((stdout = freopen("SYS$OUTPUT", "w", stdout)) == NULL) {
				// couldn't reopen it so try opening it instead 
				if((stdout = fopen("SYS$OUTPUT", "w")) == NULL) {
					fputs("Error opening SYS$OUTPUT as stdout\n", stderr); // don't use int_error here - causes infinite loop!
				}
			}
			gpoutfile = stdout;
		}
#endif /* VMS */

		/* Why a goto?  Because we exited the loop below via int_error */
		/* using LONGJMP.  The compiler was not expecting this, and    */
		/* "optimized" the handling of argc and argv such that simply  */
		/* entering the loop again from the top finds them messed up.  */
		/* If we reenter the loop via a goto then there is some hope   */
		/* that code reordering does not hurt us.                      */
		/* NB: the test for interactive means that execution from a    */
		/* pipe will not continue after an error. Do we want this?     */
		if(reading_from_dash && IsInteractive)
			goto RECOVER_FROM_ERROR_IN_DASH;
		reading_from_dash = false;
		if(!IsInteractive && !noinputfiles) {
			term_reset();
			gp_exit(EXIT_FAILURE); // exit on non-interactive error 
		}
	}
	// load filenames given as arguments 
	while(--argc > 0) {
		++argv;
		GpC.CToken = 0;
		if(!strncmp(*argv, "-persist", 2) || !strcmp(*argv, "--persist")
#ifdef _Windows
		    || !_stricmp(*argv, "-noend") || !_stricmp(*argv, "/noend")
#endif
		    ) {
			FPRINTF((stderr, "'persist' command line option recognized\n"));
		}
		else if(strcmp(*argv, "-") == 0) {
#if defined(_Windows) && !defined(WGP_CONSOLE)
			TextShow(&textwin);
			IsInteractive = true;
#else
			IsInteractive = isatty(fileno(stdin));
#endif
RECOVER_FROM_ERROR_IN_DASH:
			reading_from_dash = true;
			while(!ComLine(GpC)) {
				;
			}
			reading_from_dash = false;
			IsInteractive = false;
		}
		else if(strcmp(*argv, "-e") == 0) {
			--argc; ++argv;
			if(argc <= 0) {
				fprintf(stderr, "syntax:  gnuplot -e \"commands\"\n");
				return 0;
			}
			IsInteractive = false;
			noinputfiles = false;
			GpC.DoString(*argv);
		}
		else if(!strncmp(*argv, "-d", 2) || !strcmp(*argv, "--default-settings")) {
			// Ignore this; it already had its effect 
			FPRINTF((stderr, "ignoring -d\n"));
		}
		else if(strcmp(*argv, "-c") == 0) {
			// Pass command line arguments to the gnuplot script in the next
			// argument. This consumes the remainder of the command line
			IsInteractive = false;
			noinputfiles = false;
			--argc; ++argv;
			if(argc <= 0) {
				fprintf(stderr, "syntax:  gnuplot -c scriptname args\n");
				gp_exit(EXIT_FAILURE);
			}
			for(i = 0; i<argc; i++)
				call_args[i] = gp_strdup(argv[i+1]); // Need to stash argv[i] somewhere visible to load_file() 
			call_argc = argc - 1;
			LoadFile(GpC, loadpath_fopen(*argv, "r"), gp_strdup(*argv), 5);
			gp_exit(EXIT_SUCCESS);
		}
		else if(*argv[0] == '-') {
			fprintf(stderr, "unrecognized option %s\n", *argv);
		}
		else {
			IsInteractive = false;
			noinputfiles = false;
			LoadFile(GpC, loadpath_fopen(*argv, "r"), gp_strdup(*argv), 4);
		}
	}
	// take commands from stdin 
	if(noinputfiles)
		while(!ComLine(GpC))
			ctrlc_flag = false; // reset asynchronous Ctrl-C flag 
#ifdef _Windows
	// On Windows, handle 'persist' by keeping the main input loop running (windows/wxt), 
	// but only if there are any windows open. Note that qt handles this properly. 
	if(persist_cl) {
		if(WinAnyWindowOpen()) {
#ifdef WGP_CONSOLE
			if(!interactive) {
				// no further input from pipe 
				while(WinAnyWindowOpen())
					win_sleep(100);
			}
			else
#endif
			{
				IsInteractive = true;
				while(!ComLine(GpC))
					ctrlc_flag = false; // reset asynchronous Ctrl-C flag 
				IsInteractive = false;
			}
		}
	}
#endif
#if(defined(HAVE_LIBREADLINE) || defined(HAVE_LIBEDITLINE)) && defined(GNUPLOT_HISTORY)
	#if !defined(HAVE_ATEXIT) && !defined(HAVE_ON_EXIT)
		wrapper_for_write_history(); // You should be here if you neither have 'atexit()' nor 'on_exit()' 
	#endif
#endif
	// HBB 20040223: Not all compilers like exit() to end main() 
	// exit(exit_status); 
#if !defined(_Windows)
	gp_exit_cleanup(); // Windows does the cleanup later 
#endif
	return exit_status;
}

/* Set up to catch interrupts */
void interrupt_setup()
{
	signal(SIGINT, (sigfunc)inter);
#ifdef SIGPIPE
	// ignore pipe errors, this might happen with set output "|head" 
	signal(SIGPIPE, SIG_IGN);
#endif /* SIGPIPE */
}
//
// Initialize 'constants' stored as variables (user could mangle these)
//
//void init_constants()
void GpGadgets::InitConstants()
{
	Ev.udv_pi.udv_value.SetComplex(M_PI, 0.0);
	Ev.udv_NaN = Ev.GetUdvByName("NaN");
	Ev.udv_NaN->udv_value.SetComplex(not_a_number(), 0.0);
}

/*
 * Initialize graphics context, color palette, local preferences.
 * Called both at program startup and by "reset session".
 */
//void init_session()
void GpGadgets::InitSession(GpCommand & rC)
{
	// Disable pipes and system commands during initialization 
	successful_initialization = false;
	// Undefine any previously-used variables 
	Ev.DelUdvByName("", true);
	// Restore default colors before loadin local preferences 
	SetColorSequence(rC, 1);
	// Make sure all variables start in the same state 'reset'
	// would set them to.
	ResetCommand(rC); // FIXME: this does rC.CToken++ 
	LoadRcFile(rC, 0);         // System-wide gnuplotrc if configured 
	LoadRcFile(rC, 1);         // ./.gnuplot if configured 
	// After this point we allow pipes and system commands
	successful_initialization = true;
	LoadRcFile(rC, 2);         // ~/.gnuplot 
}
//
// Read commands from an initialization file.
// where = 0: look for gnuplotrc in system shared directory
// where = 1: look for .gnuplot in current directory
// where = 2: look for .gnuplot in home directory
//
//static void load_rcfile(int where)
void GpGadgets::LoadRcFile(GpCommand & rC, int where)
{
	FILE * plotrc = NULL;
	char * rcfile = NULL;
	if(skip_gnuplotrc)
		return;
	if(where == 0) {
#ifdef GNUPLOT_SHARE_DIR
# if defined(_Windows)
		/* retrieve path relative to gnuplot executable,
		 * whose path is in szModuleName (winmain.c) */
		rcfile = malloc(strlen((char*)szPackageDir) + 1 + strlen(GNUPLOT_SHARE_DIR) + 1 + strlen("gnuplotrc") + 1);
		strcpy(rcfile, (char*)szPackageDir);
		PATH_CONCAT(rcfile, GNUPLOT_SHARE_DIR);
# else
		rcfile = (char*)malloc(strlen(GNUPLOT_SHARE_DIR) + 1 + strlen("gnuplotrc") + 1);
		strcpy(rcfile, GNUPLOT_SHARE_DIR);
# endif
		PATH_CONCAT(rcfile, "gnuplotrc");
		plotrc = fopen(rcfile, "r");
#endif
	}
	else if(where == 1) {
#ifdef USE_CWDRC
		/* Allow check for a .gnuplot init file in the current directory */
		/* This is a security risk, as someone might leave a malicious   */
		/* init file in a shared directory.                              */
		plotrc = fopen(PLOTRC, "r");
#endif /* !USE_CWDRC */
	}
	else if(where == 2 && user_homedir) {
		// length of homedir + directory separator + length of file name + \0 
		int len = (user_homedir ? strlen(user_homedir) : 0) + 1 + strlen(PLOTRC) + 1;
		rcfile = (char *)malloc(len);
		strcpy(rcfile, user_homedir);
		PATH_CONCAT(rcfile, PLOTRC);
		plotrc = fopen(rcfile, "r");
	}
	if(plotrc) {
		char * rc = gp_strdup(rcfile ? rcfile : PLOTRC);
		LoadFile(rC, plotrc, rc, 3);
		push_terminal(0); /* needed if terminal or its options were changed */
	}
	free(rcfile);
}

void get_user_env()
{
	if(user_homedir == NULL) {
		const char * env_home;
		if((env_home = getenv(HOME))
#ifdef WIN32
		    || (env_home = appdata_directory()) || (env_home = getenv("USERPROFILE"))
#endif
#ifndef VMS
		    || (env_home = getenv("HOME"))
#endif
		    )
			user_homedir = (const char*)gp_strdup(env_home);
		else if(GpGg.IsInteractive)
			int_warn(NO_CARET, "no HOME found");
	}
	// Hhm ... what about VMS? 
	if(GpGg.user_shell == NULL) {
		const char * env_shell;
		if((env_shell = getenv("SHELL")) == NULL)
#if defined(_Windows)
			if((env_shell = getenv("COMSPEC")) == NULL)
#endif
			env_shell = SHELL;
		GpGg.user_shell = (const char*)gp_strdup(env_shell);
	}
}

/* expand tilde in path
 * path cannot be a static array!
 * tilde must be the first character in *pathp;
 * we may change that later
 */
void gp_expand_tilde(char ** pathp)
{
	if(!*pathp)
		GpGg.IntError(GpC, NO_CARET, "Cannot expand empty path");
	if((*pathp)[0] == '~' && (*pathp)[1] == DIRSEP1) {
		if(user_homedir) {
			size_t n = strlen(*pathp);
			*pathp = (char *)gp_realloc(*pathp, n + strlen(user_homedir), "tilde expansion");
			// include null at the end ...
			memmove(*pathp + strlen(user_homedir) - 1, *pathp, n + 1);
			memcpy(*pathp, user_homedir, strlen(user_homedir));
		}
		else
			int_warn(NO_CARET, "HOME not set - cannot expand tilde");
	}
}

//static void init_memory()
void GpCommand::InitMemory()
{
	ExtendInputLine();
	ExtendTokenTable();
	P_ReplotLine = gp_strdup("");
}

#ifdef GNUPLOT_HISTORY

/* cancel_history() can be called by terminals that fork a helper process
 * to make sure that the helper doesn't trash the history file on exit.
 */
void cancel_history()
{
	expanded_history_filename = NULL;
}

# if defined(HAVE_LIBREADLINE) || defined(HAVE_LIBEDITLINE)

static void wrapper_for_write_history()
{
	if(expanded_history_filename) {
		if(history_is_stifled())
			unstifle_history();
		if(gnuplot_history_size >= 0)
			stifle_history(gnuplot_history_size);
		// returns 0 on success 
		if(write_history(expanded_history_filename))
			fprintf(stderr, "Warning:  Could not write history file!!!\n");
		unstifle_history();
	}
}

# else /* HAVE_LIBREADLINE || HAVE_LIBEDITLINE */

// version for gnuplot's own write_history 
static void wrapper_for_write_history()
{
	// What we really want to do is truncate(expanded_history_filename), but this is only available on BSD compatible systems
	if(expanded_history_filename) {
		remove(expanded_history_filename);
		if(GpC.H.gnuplot_history_size < 0)
			GpC.H.WriteHistory(expanded_history_filename);
		else
			GpC.H.WriteHistoryN(GpC.H.gnuplot_history_size, expanded_history_filename, "w");
	}
}

# endif /* HAVE_LIBREADLINE || HAVE_LIBEDITLINE */
#endif /* GNUPLOT_HISTORY */

void restrict_popen()
{
	if(!successful_initialization)
		GpGg.IntError(GpC, NO_CARET, "Pipes and shell commands not permitted during initialization");
}

