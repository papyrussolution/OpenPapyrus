/*
 * $Id: command.h,v 1.71 2016/02/07 22:15:36 sfeam Exp $
 */

/* GNUPLOT - command.h */

/*[
 * Copyright 1999, 2004   Thomas Williams, Colin Kelley
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

#ifndef GNUPLOT_COMMAND_H
#define GNUPLOT_COMMAND_H

//#include "gp_types.h"
#include "stdfn.h"

//#define END_OF_COMMAND (GpC.CToken >= GpC.NumTokens || GpC.Eq(";"))
#define END_OF_COMMAND GpC.EndOfCommand()

struct LexicalUnit {   // produced by scanner 
    bool is_token;  // true if token, false if a value 
    t_value l_val;
    int    start_index; // index of first char in token 
    int    length;      // length of token in chars 
};

class GpCommand {
public:
	GpCommand()
	{
		P_InputLine = 0;
		InputLineLen = 0;
		InlineNum = 0;
		IfDepth = 0;
		IfOpenForElse = false;
		IfCondition = false;
		IsReplotDisabled = false;
		P_Token = 0;
		TokenTableSize = 0;
		PlotToken = 0;
		P_ReplotLine = 0;

		F_PrintOut = 0;
		P_PrintOutVar = 0;
		P_PrintOutName = 0;
		P_DummyFunc = 0;
		NumTokens = 0;
		CToken = 0;
	}
	int    DoLine(); // do_line()
	void   Command();
	bool   IsArrayAssignment();
	int    IsString(int t_num) const;
	char * TryToGetString();
	void   Capture(char * str, int start, int end, int max);
	void   MCapture(char ** str, int start, int end);
	void   MQuoteCapture(char ** str, int start, int end);
	int    EndOfCommand() const
	{
		return (CToken >= NumTokens || Eq(";"));
	}
	// support for dynamic size of input line 
	//void   extend_input_line()
	void   ExtendInputLine();
	//
	// AlmostEq() compares string value of token number CToken with str[], and returns true if 
	// they are identical up to the first $ in str[].
	//
	int AlmostEq(const char * str) const
	{
		return AlmostEq(CToken, str);
		/*
		if(CToken < 0 || CToken >= NumTokens) // safer to test here than to trust all callers
			return false;
		else if(!str)
			return false;
		else if(!P_Token[CToken].is_token)
			return false; // must be a value--can't be equal
		else {
			const  int length = P_Token[CToken].length;
			int    start = P_Token[CToken].start_index;
			int    after = 0;
			int    i;
			for(i = 0; i < length + after; i++) {
				if(str[i] != P_InputLine[start + i]) {
					if(str[i] != '$')
						return (false);
					else {
						after = 1;
						start--; // back up token ptr
					}
				}
			}
			// i now beyond end of token string
			return (after || str[i] == '$' || str[i] == NUL);
		}
		*/
	}
	int AlmostEq(int t_num, const char * str) const
	{
		if(!str || t_num < 0 || t_num >= NumTokens || !P_Token[t_num].is_token)
			return false;
		else {
			const  int length = P_Token[t_num].length;
			int    start = P_Token[t_num].start_index;
			int    after = 0;
			int    i;
			for(i = 0; i < length + after; i++) {
				if(str[i] != P_InputLine[start + i]) {
					if(str[i] != '$')
						return (false);
					else {
						after = 1;
						start--; // back up token ptr
					}
				}
			}
			// i now beyond end of token string
			return (after || str[i] == '$' || str[i] == NUL);
		}
	}
	//
	// GpC.Eq() compares string value of token number t_num with str[], and returns true if they are identical.
	//
	int    Eq(const char * str) const;
	int    Eq(int t_num, const char * str) const;
	void   ParseColorSpec(t_colorspec * tc, int options);

	char * P_InputLine;
	size_t InputLineLen;
	int    InlineNum;
	int    IfDepth;         // old if/else syntax only
	bool   IfOpenForElse;   // new if/else syntax only 
	bool   IfCondition;     // used by both old and new syntax
	bool   IsReplotDisabled; // flag to disable `replot` when some data are sent through stdin; used by mouse/hotkey capable terminals 
	LexicalUnit * P_Token;
	int    TokenTableSize;
	int    PlotToken;
	char * P_ReplotLine;
	FILE * F_PrintOut; // output file for the print command 
	UdvtEntry * P_PrintOutVar;
	char * P_PrintOutName;
	UdftEntry * P_DummyFunc;
	// input data, parsing variables 
	int    NumTokens;
	int    CToken;
};

extern GpCommand GpC;

#ifdef USE_MOUSE
	extern int paused_for_mouse;	/* Flag the end condition we are paused until */
	#define PAUSE_BUTTON1   001		/* Mouse button 1 */
	#define PAUSE_BUTTON2   002		/* Mouse button 2 */
	#define PAUSE_BUTTON3   004		/* Mouse button 3 */
	#define PAUSE_CLICK	007		/* Any button click */
	#define PAUSE_KEYSTROKE 010		/* Any keystroke */
	#define PAUSE_WINCLOSE	020		/* Window close event */
	#define PAUSE_ANY       077		/* Terminate on any of the above */
#endif
#ifndef STDOUT
	#define STDOUT 1
#endif
#if defined(MSDOS) && defined(DJGPP)
	extern char HelpFile[];         /* patch for do_help  - AP */
#endif /* MSDOS */
#ifdef _Windows
	#define SET_CURSOR_WAIT SetCursor(LoadCursor((HINSTANCE) NULL, IDC_WAIT))
	#define SET_CURSOR_ARROW SetCursor(LoadCursor((HINSTANCE) NULL, IDC_ARROW))
#else
	#define SET_CURSOR_WAIT        /* nought, zilch */
	#define SET_CURSOR_ARROW       /* nought, zilch */
#endif
// Include code to support deprecated "call" syntax. 
#ifdef BACKWARDS_COMPATIBLE
	#define OLD_STYLE_CALL_ARGS
#endif

void raise_lower_command(int);
void raise_command();
void lower_command();
#ifdef OS2
	extern void pm_raise_terminal_window();
	extern void pm_lower_terminal_window();
#endif
#ifdef X11
	extern void x11_raise_terminal_window(int);
	extern void x11_raise_terminal_group();
	extern void x11_lower_terminal_window(int);
	extern void x11_lower_terminal_group();
#endif
#ifdef _Windows
	extern void win_raise_terminal_window(int);
	extern void win_raise_terminal_group();
	extern void win_lower_terminal_window(int);
	extern void win_lower_terminal_group();
#endif
#ifdef WXWIDGETS
	extern void wxt_raise_terminal_window(int);
	extern void wxt_raise_terminal_group();
	extern void wxt_lower_terminal_window(int);
	extern void wxt_lower_terminal_group();
#endif
extern void string_expand_macros();
#ifdef USE_MOUSE
	void bind_command();
	void restore_prompt();
#else
	#define bind_command()
#endif
void array_command();
void break_command();
void call_command();
void changedir_command();
void clear_command();
void continue_command();
void eval_command();
void exit_command();
void help_command();
void history_command();
void do_command();
void if_command();
void else_command();
void import_command();
void invalid_command();
void link_command();
void load_command();
void begin_clause();
void clause_reset_after_error();
void end_clause();
void null_command();
void pause_command();
void plot_command();
void print_command();
void printerr_command();
void pwd_command();
void refresh_request();
void refresh_command();
void replot_command();
void reread_command();
void save_command();
void screendump_command();
void splot_command();
void stats_command();
void system_command();
void test_command();
void toggle_command();
void update_command();
void do_shell();
void undefine_command();
void while_command();
//
// Prototypes for functions exported by command.c 
//
//void extend_input_line();
void extend_token_table();
int com_line();
//int do_line();
void do_string(const char* s);
void do_string_and_free(char* s);
bool iteration_early_exit();
#ifdef USE_MOUSE
	void toggle_display_of_ipc_commands();
	int display_ipc_commands();
	void do_string_replot(const char* s);
#endif
#ifdef VMS                     /* HBB 990829: used only on VMS */
	void done(int status);
#endif

void define();
void replotrequest(); /* used in command.c & mouse.c */
void print_set_output(char *, bool, bool); /* set print output file */
char *print_show_output(); /* show print output file */
/* Activate/deactive effects of 'set view map' before 'splot'/'plot',
 * respectively. Required for proper mousing during 'set view map';
 * actually it won't be necessary if gnuplot keeps a copy of all variables for
 * the current plot and don't share them with 'set' commands.
 *   These routines need to be executed before each plotrequest() and
 * plot3drequest().
 */
void splot_map_activate();
void splot_map_deactivate();
int do_system_func(const char *cmd, char **output);

#endif /* GNUPLOT_COMMAND_H */
