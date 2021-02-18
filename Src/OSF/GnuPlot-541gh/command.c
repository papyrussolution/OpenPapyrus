// GNUPLOT - command.c 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
/*
 * Changes:
 *
 * Feb 5, 1992  Jack Veenstra   (veenstra@cs.rochester.edu) Added support to
 * filter data values read from a file through a user-defined function before
 * plotting. The keyword "thru" was added to the "plot" command. Example
 * syntax: f(x) = x / 100 plot "test.data" thru f(x) This example divides all
 * the y values by 100 before plotting. The filter function processes the
 * data before any log-scaling occurs. This capability should be generalized
 * to filter x values as well and a similar feature should be added to the
 * "splot" command.
 *
 * 19 September 1992  Lawrence Crowl  (crowl@cs.orst.edu)
 * Added user-specified bases for log scaling.
 *
 * April 1999 Franz Bakan (bakan@ukezyk.desy.de)
 * Added code to support mouse-input from OS/2 PM window
 * Changes marked by USE_MOUSE
 *
 * May 1999, update by Petr Mikulik
 * Use gnuplot's pid in shared mem name
 *
 * August 1999 Franz Bakan and Petr Mikulik
 * Encapsulating read_line into a thread, acting on input when thread or
 * gnupmdrv posts an event semaphore. Thus mousing works even when gnuplot
 * is used as a plotting device (commands passed via pipe).
 *
 * May 2011 Ethan A Merritt
 * Introduce block structure defined by { ... }, which may span multiple lines.
 * In order to have the entire block available at one time we now count
 * +/- curly brackets during input and keep extending the current input line
 * until the net count is zero.  This is done in do_line() for interactive
 * input, and load_file() for non-interactive input.
 */
#include <gnuplot.h>
#pragma hdrstop
#ifdef USE_MOUSE
	int paused_for_mouse = 0;
#endif
#define PROMPT "gnuplot> "
#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	//#include <windows.h>
	#ifdef _MSC_VER
		//#include <malloc.h>
		#include <direct.h>          /* getcwd() */
	#endif
	#include <htmlhelp.h>
	#include "win/winmain.h"
#endif /* _WIN32 */
#ifdef __DJGPP__
	#include <pc.h>                /* getkey() */
	#define useconds_t unsigned int
#endif
#ifdef __WATCOMC__
	#include <conio.h>             /* for getch() */
#endif
/* static prototypes */
//static void command();
//static bool is_array_assignment();
static int changedir(char * path);
static char* fgets_ipc(char* dest, int len);
static char* gp_get_string(char *, size_t, const char *);
//static int read_line(const char * prompt, int start);
static void do_system(const char *);
//static void test_palette_subcommand();
//static int find_clause(int *, int *);
//static void if_else_command(ifstate if_state);
//static void old_if_command(at_type * expr);
//static int report_error(int ierr);
static int expand_1level_macros();
char * gp_input_line;
size_t gp_input_line_len;
int    inline_num; // input line number 
udft_entry * dummy_func;
char * replot_line = NULL; // support for replot command 
int    plot_token = 0; // start of 'plot' command 
bool   replot_disabled = FALSE; // flag to disable `replot` when some data are sent through stdin; used by mouse/hotkey capable terminals 
FILE * print_out = NULL; // output file for the print command 
udvt_entry * print_out_var = NULL;
char * print_out_name = NULL;
// input data, parsing variables 

//lexical_unit * token;
//int    token_table_size;
//int    num_tokens;
//int    c_token;

bool   if_open_for_else = FALSE;
static int clause_depth = 0;
// support for 'break' and 'continue' commands 
static int  iteration_depth = 0;
static bool requested_break = FALSE;
static bool requested_continue = FALSE;
static int  command_exit_requested = 0; // set when an "exit" command is encountered 
//
// support for dynamic size of input line 
//
void extend_input_line()
{
	if(gp_input_line_len == 0) {
		// first time 
		gp_input_line = (char *)gp_alloc(MAX_LINE_LEN, "gp_input_line");
		gp_input_line_len = MAX_LINE_LEN;
		gp_input_line[0] = NUL;
	}
	else {
		gp_input_line = (char *)gp_realloc(gp_input_line, gp_input_line_len + MAX_LINE_LEN, "extend input line");
		gp_input_line_len += MAX_LINE_LEN;
		FPRINTF((stderr, "extending input line to %d chars\n", gp_input_line_len));
	}
}

#define MAX_TOKENS 400 // constant by which token table grows 

//void extend_token_table()
void GpProgram::ExtendTokenTable()
{
	if(TokenTableSize == 0) {
		// first time 
		P_Token = (lexical_unit *)gp_alloc(MAX_TOKENS * sizeof(lexical_unit), "token table");
		TokenTableSize = MAX_TOKENS;
		// HBB: for checker-runs: 
		memzero(P_Token, MAX_TOKENS * sizeof(*P_Token));
	}
	else {
		P_Token = (lexical_unit *)gp_realloc(P_Token, (TokenTableSize + MAX_TOKENS) * sizeof(lexical_unit), "extend token table");
		memzero(P_Token+TokenTableSize, MAX_TOKENS * sizeof(*P_Token));
		TokenTableSize += MAX_TOKENS;
		FPRINTF((stderr, "extending token table to %d elements\n", TokenTableSize));
	}
}

//int com_line()
int GnuPlot::ComLine()
{
	const char * p_prompt = PROMPT;
	if(multiplot) {
		TermCheckMultiplotOkay(interactive); // calls IntError() if it is not happy 
		p_prompt = "multiplot> ";
	}
	if(Pgm.ReadLine(p_prompt, 0))
		return 1;
	else {
		// So we can flag any new output: if false at time of error,
		// we reprint the command line before printing caret.
		// TRUE for interactive terminals, since the command line is typed.
		// FALSE for non-terminal stdin, so command line is printed anyway.
		// (DFK 11/89)
		screen_ok = interactive;
		return BIN(DoLine());
	}
}

//int do_line()
int GnuPlot::DoLine()
{
	// Line continuation has already been handled by read_line() 
	string_expand_macros(); // Expand any string variables in the current input line 
	// Skip leading whitespace 
	char * inlptr = gp_input_line;
	while(isspace((uchar)*inlptr))
		inlptr++;
	// Leading '!' indicates a shell command that bypasses normal gnuplot
	// tokenization and parsing.  This doesn't work inside a bracketed clause.
	if(is_system(*inlptr)) {
		do_system(inlptr + 1);
		return (0);
	}
	else {
		// Strip off trailing comment 
		FPRINTF((stderr, "doline( \"%s\" )\n", gp_input_line));
		if(strchr(inlptr, '#')) {
			Pgm.NumTokens = Pgm.Scanner(&gp_input_line, &gp_input_line_len);
			if(gp_input_line[Pgm.P_Token[Pgm.NumTokens].start_index] == '#')
				gp_input_line[Pgm.P_Token[Pgm.NumTokens].start_index] = NUL;
		}
		if(inlptr != gp_input_line) {
			// If there was leading whitespace, copy the actual
			// command string to the front. use memmove() because
			// source and target may overlap 
			memmove(gp_input_line, inlptr, strlen(inlptr));
			gp_input_line[strlen(inlptr)] = NUL; // Terminate resulting string 
		}
		FPRINTF((stderr, "  echo: \"%s\"\n", gp_input_line));
		Pgm.NumTokens = Pgm.Scanner(&gp_input_line, &gp_input_line_len);
		// 
		// Expand line if necessary to contain a complete bracketed clause {...}
		// Insert a ';' after current line and append the next input line.
		// NB: This may leave an "else" condition on the next line.
		// 
		if(Pgm.CurlyBraceCount < 0)
			IntError(NO_CARET, "Unexpected }");
		while(Pgm.CurlyBraceCount > 0) {
			if(lf_head && lf_head->depth > 0) {
				// This catches the case that we are inside a "load foo" operation
				// and therefore requesting interactive input is not an option.
				// FIXME: or is it?
				IntError(NO_CARET, "Syntax error: missing block terminator }");
			}
			else if(interactive || noinputfiles) {
				// If we are really in interactive mode and there are unterminated blocks,
				// then we want to display a "more>" prompt to get the rest of the block.
				// However, there are two more cases that must be dealt here:
				// One is when commands are piped to gnuplot - on the command line,
				// the other is when commands are piped to gnuplot which is opened
				// as a slave process. The test for noinputfiles is for the latter case.
				// If we didn't have that test here, unterminated blocks sent via a pipe
				// would trigger the error message in the else branch below. 
				int retval;
				strcat(gp_input_line, ";");
				retval = Pgm.ReadLine("more> ", strlen(gp_input_line));
				if(retval)
					IntError(NO_CARET, "Syntax error: missing block terminator }");
				// Expand any string variables in the current input line 
				string_expand_macros();
				Pgm.NumTokens = Pgm.Scanner(&gp_input_line, &gp_input_line_len);
				if(gp_input_line[Pgm.P_Token[Pgm.NumTokens].start_index] == '#')
					gp_input_line[Pgm.P_Token[Pgm.NumTokens].start_index] = NUL;
			}
			else {
				// Non-interactive mode here means that we got a string from -e.
				// Having Pgm.CurlyBraceCount > 0 means that there are at least one
				// unterminated blocks in the string.
				// Likely user error, so we die with an error message. 
				IntError(NO_CARET, "Syntax error: missing block terminator }");
			}
		}
		Pgm.CToken = 0;
		while(Pgm.CToken < Pgm.NumTokens) {
			Command();
			if(command_exit_requested) {
				command_exit_requested = 0; /* yes this is necessary */
				return 1;
			}
			if(iteration_early_exit()) {
				Pgm.CToken = Pgm.NumTokens;
				break;
			}
			if(Pgm.CToken < Pgm.NumTokens) { // something after command 
				if(Pgm.EqualsCur(";"))
					Pgm.Shift();
				else if(Pgm.EqualsCur("{"))
					BeginClause();
				else if(Pgm.EqualsCur("}"))
					EndClause();
				else
					IntErrorCurToken("unexpected or unrecognized token: %s", Pgm.TokenToString(Pgm.GetCurTokenIdx()));
			}
		}
		check_for_mouse_events(); // This check allows event handling inside load/eval/while statements 
		return (0);
	}
}

//void do_string(const char * s)
void GnuPlot::DoString(const char * s)
{
	char * cmdline = gp_strdup(s);
	DoStringAndFree(cmdline);
}

//void do_string_and_free(char * cmdline)
void GnuPlot::DoStringAndFree(char * cmdline)
{
#ifdef USE_MOUSE
	if(display_ipc_commands())
		fprintf(stderr, "%s\n", cmdline);
#endif
	LfPush(NULL, NULL, cmdline); /* save state for errors and recursion */
	while(gp_input_line_len < strlen(cmdline) + 1)
		extend_input_line();
	strcpy(gp_input_line, cmdline);
	screen_ok = FALSE;
	command_exit_requested = DoLine();
	// 
	// "exit" is supposed to take us out of the current file from a
	// "load <file>" command.  But the LFS stack holds both files and
	// bracketed clauses, so we have to keep popping until we hit an actual file.
	// 
	if(command_exit_requested) {
		while(lf_head && !lf_head->name) {
			FPRINTF((stderr, "pop one level of non-file LFS\n"));
			LfPop();
		}
	}
	else
		LfPop();
}

#ifdef USE_MOUSE
void toggle_display_of_ipc_commands() { mouse_setting.verbose = mouse_setting.verbose ? 0 : 1; }
int  display_ipc_commands() { return mouse_setting.verbose; }

//void do_string_replot(const char * pS)
void GnuPlot::DoStringReplot(GpTermEntry * pTerm, const char * pS)
{
	DoString(pS);
	if(Gg.VolatileData && refresh_ok != E_REFRESH_NOT_OK) {
		if(display_ipc_commands())
			fprintf(stderr, "refresh\n");
		RefreshRequest();
	}
	else if(!replot_disabled)
		ReplotRequest(pTerm);
	else
		IntWarn(NO_CARET, "refresh not possible and replot is disabled");
}

void restore_prompt()
{
	if(interactive) {
#if defined(HAVE_LIBREADLINE) || defined(HAVE_LIBEDITLINE)
#if !defined(MISSING_RL_FORCED_UPDATE_DISPLAY)
		rl_forced_update_display();
#else
		rl_redisplay();
#endif
#else
		fputs(PROMPT, stderr);
		fflush(stderr);
#endif
	}
}

#endif /* USE_MOUSE */

//void define()
void GnuPlot::Define()
{
	int start_token; /* the 1st token in the function definition */
	udvt_entry * udv;
	udft_entry * udf;
	GpValue result;
	if(Pgm.EqualsNext("(")) {
		// function ! 
		int dummy_num = 0;
		at_type * at_tmp;
		char * tmpnam;
		char save_dummy[MAX_NUM_VAR][MAX_ID_LEN+1];
		memcpy(save_dummy, c_dummy_var, sizeof(save_dummy));
		start_token = Pgm.GetCurTokenIdx();
		do {
			// skip to the next dummy 
			Pgm.Shift();
			Pgm.Shift();
			Pgm.CopyStr(c_dummy_var[dummy_num++], Pgm.GetCurTokenIdx(), MAX_ID_LEN);
		} while(Pgm.EqualsNext(",") && (dummy_num < MAX_NUM_VAR));
		if(Pgm.EqualsNext(","))
			IntError(Pgm.GetCurTokenIdx()+2, "function contains too many parameters");
		// skip (, dummy, ) and = 
		Pgm.Shift();
		Pgm.Shift();
		Pgm.Shift();
		//
		if(Pgm.EndOfCommand())
			IntErrorCurToken("function definition expected");
		udf = dummy_func = AddUdf(start_token);
		udf->dummy_num = dummy_num;
		if((at_tmp = PermAt()) == (struct at_type *)NULL)
			IntError(start_token, "not enough memory for function");
		if(udf->at)     /* already a dynamic a.t. there */
			free_at(udf->at); /* so free it first */
		udf->at = at_tmp; /* before re-assigning it. */
		memcpy(c_dummy_var, save_dummy, sizeof(save_dummy));
		Pgm.MCapture(&(udf->definition), start_token, Pgm.GetPrevTokenIdx());
		dummy_func = NULL; // dont let anyone else use our workspace 
		// Save function definition in a user-accessible variable 
		tmpnam = (char *)gp_alloc(8+strlen(udf->udf_name), "varname");
		strcpy(tmpnam, "GPFUN_");
		strcat(tmpnam, udf->udf_name);
		Ev.FillGpValString(tmpnam, udf->definition);
		SAlloc::F(tmpnam);
	}
	else {
		// variable ! 
		char * varname = gp_input_line + Pgm.P_Token[Pgm.CToken].start_index;
		if(!strncmp(varname, "GPVAL_", 6) || !strncmp(varname, "GPFUN_", 6) || !strncmp(varname, "MOUSE_", 6))
			IntErrorCurToken("Cannot set internal variables GPVAL_ GPFUN_ MOUSE_");
		start_token = Pgm.GetCurTokenIdx();
		Pgm.Shift();
		Pgm.Shift();
		udv = AddUdv(start_token);
		ConstExpress(&result);
		// Prevents memory leak if the variable name is re-used 
		udv->udv_value.Destroy();
		udv->udv_value = result;
	}
}

//void undefine_command()
void GnuPlot::UndefineCommand()
{
	char   key[MAX_ID_LEN+1];
	bool   wildcard;
	Pgm.Shift(); // consume the command name 
	while(!Pgm.EndOfCommand()) {
		// copy next var name into key 
		Pgm.CopyStr(key, Pgm.GetCurTokenIdx(), MAX_ID_LEN);
		// Peek ahead - must do this, because a '*' is returned as a separate token, not as part of the 'key' 
		wildcard = Pgm.EqualsNext("*");
		if(wildcard)
			Pgm.Shift();
		// The '$' starting a data block name is a separate token 
		else if(*key == '$')
			Pgm.CopyStr(&key[1], ++Pgm.CToken, MAX_ID_LEN-1);
		// Other strange stuff on command line 
		else if(!Pgm.IsLetter(Pgm.GetCurTokenIdx()))
			IntErrorCurToken("Not a variable name");
		// This command cannot deal with array elements or functions 
		if(Pgm.EqualsNext("[") || Pgm.EqualsNext("("))
			IntErrorCurToken("Cannot undefine function or array element");
		// ignore internal variables 
		if(strncmp(key, "GPVAL_", 6) && strncmp(key, "MOUSE_", 6))
			Ev.DelUdvByName(key, wildcard);
		Pgm.Shift();
	}
}

//static void command()
void GnuPlot::Command()
{
	for(int i = 0; i < MAX_NUM_VAR; i++)
		c_dummy_var[i][0] = NUL; /* no dummy variables */
	if(IsDefinition(Pgm.GetCurTokenIdx()))
		Define();
	else if(IsArrayAssignment())
		;
	else {
		//(*lookup_ftable(&command_ftbl[0], GetCurTokenIdx()))();
		/*
			parsefuncp_t lookup_ftable(const struct gen_ftable * ftbl, int find_token)
			{
				while(ftbl->key) {
					if(Pgm.AlmostEquals(find_token, ftbl->key))
						return ftbl->value;
					ftbl++;
				}
				return ftbl->value;
			}
		*/
		{
			int cur_tok_idx = Pgm.GetCurTokenIdx();
			if(Pgm.AlmostEquals(cur_tok_idx, "ra$ise"))
				RaiseCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "low$er"))
				LowerCommand();
		#ifdef USE_MOUSE
			else if(Pgm.AlmostEquals(cur_tok_idx, "bi$nd"))
				BindCommand();
		#endif
			else if(Pgm.AlmostEquals(cur_tok_idx, "array"))
				ArrayCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "break"))
				Pgm.BreakCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "ca$ll"))
				CallCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "cd"))
				ChangeDirCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "cl$ear"))
				ClearCommand(term);
			else if(Pgm.AlmostEquals(cur_tok_idx, "continue"))
				Pgm.ContinueCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "do"))
				DoCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "eval$uate"))
				EvalCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "ex$it"))
				ExitCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "f$it"))
				FitCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "h$elp"))
				Pgm.HelpCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "?"))
				Pgm.HelpCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "hi$story"))
				HistoryCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "if"))
				IfCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "import"))
				ImportCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "else"))
				ElseCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "l$oad"))
				LoadCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "pa$use"))
				PauseCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "p$lot"))
				PlotCommand(term);
			else if(Pgm.AlmostEquals(cur_tok_idx, "pr$int"))
				PrintCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "printerr$or"))
				printerr_command();
			else if(Pgm.AlmostEquals(cur_tok_idx, "pwd"))
				pwd_command();
			else if(Pgm.AlmostEquals(cur_tok_idx, "q$uit"))
				ExitCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "ref$resh"))
				RefreshCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "rep$lot"))
				ReplotCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "re$read"))
				reread_command();
			else if(Pgm.AlmostEquals(cur_tok_idx, "res$et"))
				ResetCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "sa$ve"))
				SaveCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "scr$eendump"))
				screendump_command();
			else if(Pgm.AlmostEquals(cur_tok_idx, "se$t"))
				SetCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "she$ll"))
				do_shell();
			else if(Pgm.AlmostEquals(cur_tok_idx, "sh$ow"))
				ShowCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "sp$lot"))
				SPlotCommand(term);
			else if(Pgm.AlmostEquals(cur_tok_idx, "st$ats"))
				stats_command();
			else if(Pgm.AlmostEquals(cur_tok_idx, "sy$stem"))
				system_command();
			else if(Pgm.AlmostEquals(cur_tok_idx, "test"))
				TestCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "tog$gle"))
				toggle_command();
			else if(Pgm.AlmostEquals(cur_tok_idx, "und$efine"))
				UndefineCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "uns$et"))
				UnsetCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "up$date"))
				update_command();
			else if(Pgm.AlmostEquals(cur_tok_idx, "vclear"))
				VClearCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "vfill"))
				VFillCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "vgfill"))
				VFillCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "voxel"))
				VoxelCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "while"))
				WhileCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "{"))
				BeginClause();
			else if(Pgm.AlmostEquals(cur_tok_idx, "}"))
				EndClause();
			else if(Pgm.AlmostEquals(cur_tok_idx, ";"))
				null_command();
			else if(Pgm.AlmostEquals(cur_tok_idx, "$"))
				DatablockCommand();
			else
				InvalidCommand();
		}
	}
}
//
// process the 'raise' or 'lower' command 
//
//void raise_lower_command(int lower)
void GnuPlot::RaiseLowerCommand(int lower)
{
	Pgm.Shift();
	if(Pgm.EndOfCommand()) {
		if(lower) {
#ifdef X11
			x11_lower_terminal_group();
#endif
#ifdef _WIN32
			win_lower_terminal_group();
#endif
#ifdef WXWIDGETS
			wxt_lower_terminal_group();
#endif
		}
		else {
#ifdef X11
			x11_raise_terminal_group();
#endif
#ifdef _WIN32
			win_raise_terminal_group();
#endif
#ifdef WXWIDGETS
			wxt_raise_terminal_group();
#endif
		}
		return;
	}
	else {
		int number;
		int negative = Pgm.EqualsCur("-");
		if(negative || Pgm.EqualsCur("+")) 
			Pgm.Shift();
		if(!Pgm.EndOfCommand() && Pgm.IsANumber(Pgm.GetCurTokenIdx())) {
			number = static_cast<int>(RealExpression());
			if(negative)
				number = -number;
			if(lower) {
#ifdef X11
				x11_lower_terminal_window(number);
#endif
#ifdef _WIN32
				win_lower_terminal_window(number);
#endif
#ifdef WXWIDGETS
				wxt_lower_terminal_window(number);
#endif
			}
			else {
#ifdef X11
				x11_raise_terminal_window(number);
#endif
#ifdef _WIN32
				win_raise_terminal_window(number);
#endif
#ifdef WXWIDGETS
				wxt_raise_terminal_window(number);
#endif
			}
			Pgm.Shift();
			return;
		}
	}
	if(lower)
		IntErrorCurToken("usage: lower {plot_id}");
	else
		IntErrorCurToken("usage: raise {plot_id}");
}

//void raise_command()
void GnuPlot::RaiseCommand() { RaiseLowerCommand(0); }
//void lower_command()
void GnuPlot::LowerCommand() { RaiseLowerCommand(1); }
/*
 * Arrays are declared using the syntax
 *    array A[size] { = [ element, element, ... ] }
 * where size is an integer and space is reserved for elements A[1] through A[size]
 * The size itself is stored in A[0].v.int_val.A
 * The list of initial values is optional.
 * Any element that is not initialized is set to NOTDEFINED.
 *
 * Elements in an existing array can be accessed like any other gnuplot variable.
 * Each element can be one of INTGR, CMPLX, STRING.
 */
//void array_command()
void GnuPlot::ArrayCommand()
{
	int    nsize = 0;  /* Size of array when we leave */
	int    est_size = 0; /* Estimated size */
	udvt_entry * array;
	GpValue * A;
	int    i;
	// Create or recycle a udv containing an array with the requested name 
	if(!Pgm.IsLetter(++Pgm.CToken))
		IntErrorCurToken("illegal variable name");
	array = AddUdv(Pgm.GetCurTokenIdx());
	array->udv_value.Destroy();
	Pgm.Shift();
	if(Pgm.EqualsCur("[")) {
		Pgm.Shift();
		nsize = IntExpression();
		if(!Pgm.Equals(Pgm.CToken++, "]"))
			IntError(Pgm.GetPrevTokenIdx(), "expecting array[size>0]");
	}
	else if(Pgm.EqualsCur("=") && Pgm.EqualsNext("[")) {
		// Estimate size of array by counting commas in the initializer 
		for(i = Pgm.CToken+2; i < Pgm.NumTokens; i++) {
			if(Pgm.Equals(i, ",") || Pgm.Equals(i, "]"))
				est_size++;
			if(Pgm.Equals(i, "]"))
				break;
		}
		nsize = est_size;
	}
	if(nsize <= 0)
		IntError(Pgm.GetPrevTokenIdx(), "expecting array[size>0]");
	array->udv_value.v.value_array = (GpValue *)gp_alloc((nsize+1) * sizeof(GpValue), "array_command");
	array->udv_value.type = ARRAY;
	// Element zero of the new array is not visible but contains the size 
	A = array->udv_value.v.value_array;
	A[0].v.int_val = nsize;
	for(i = 0; i <= nsize; i++) {
		A[i].SetNotDefined();
	}
	// Element zero can also hold an indicator that this is a colormap 
	// FIXME: more sanity checks?  e.g. all entries INTGR 
	if(Pgm.EqualsCur("colormap")) {
		Pgm.Shift();
		if(nsize >= 2) /* Need at least 2 entries to calculate range */
			A[0].type = COLORMAP_ARRAY;
	}
	// Initializer syntax:   array A[10] = [x,y,z,,"foo",] 
	if(Pgm.EqualsCur("=")) {
		int initializers = 0;
		Pgm.Shift();
		if(!Pgm.EqualsCur("["))
			IntErrorCurToken("expecting Array[size] = [x,y,...]");
		Pgm.Shift();
		for(i = 1; i <= nsize; i++) {
			if(Pgm.EqualsCur("]"))
				break;
			if(Pgm.EqualsCur(",")) {
				initializers++;
				Pgm.Shift();
				continue;
			}
			ConstExpress(&A[i]);
			initializers++;
			if(Pgm.EqualsCur("]"))
				break;
			if(Pgm.EqualsCur(","))
				Pgm.Shift();
			else
				IntErrorCurToken("expecting Array[size] = [x,y,...]");
		}
		Pgm.Shift();
		// If the size is determined by the number of initializers 
		if(A[0].v.int_val == 0)
			A[0].v.int_val = initializers;
	}
}
// 
// Check for command line beginning with Array[<expr>] = <expr>
// This routine is modeled on command.c:define()
// 
//bool is_array_assignment()
bool GnuPlot::IsArrayAssignment()
{
	udvt_entry * udv;
	GpValue newvalue;
	int    index;
	bool   looks_OK = FALSE;
	int    brackets;
	if(!Pgm.IsLetter(Pgm.GetCurTokenIdx()) || !Pgm.EqualsNext("["))
		return FALSE;
	else {
		// There are other legal commands where the 2nd token is [
		// e.g.  "plot [min:max] foo"
		// so we check that the closing ] is immediately followed by =.
		for(index = Pgm.CToken+2, brackets = 1; index < Pgm.NumTokens; index++) {
			if(Pgm.Equals(index, ";"))
				return FALSE;
			if(Pgm.Equals(index, "["))
				brackets++;
			if(Pgm.Equals(index, "]"))
				brackets--;
			if(brackets == 0) {
				if(!Pgm.Equals(index+1, "="))
					return FALSE;
				looks_OK = TRUE;
				break;
			}
		}
		if(!looks_OK)
			return FALSE;
		else {
			udv = AddUdv(Pgm.GetCurTokenIdx());
			if(udv->udv_value.type != ARRAY)
				IntErrorCurToken("Not a known array");
			// Evaluate index 
			Pgm.Shift();
			Pgm.Shift();
			index = IntExpression();
			if(index <= 0 || index > udv->udv_value.v.value_array[0].v.int_val)
				IntErrorCurToken("array index out of range");
			if(!Pgm.EqualsCur("]") || !Pgm.EqualsNext("="))
				IntErrorCurToken("Expecting Arrayname[<expr>] = <expr>");
			// Evaluate right side of assignment 
			Pgm.Shift();
			Pgm.Shift();
			ConstExpress(&newvalue);
			udv->udv_value.v.value_array[index] = newvalue;
			return TRUE;
		}
	}
}

#ifdef USE_MOUSE
//
// process the 'bind' command 
// EAM - rewritten 2015 
//
//void bind_command()
void GnuPlot::BindCommand()
{
	char * lhs = NULL;
	char * rhs = NULL;
	bool allwindows = FALSE;
	Pgm.Shift();
	if(Pgm.AlmostEqualsCur("all$windows")) {
		allwindows = TRUE;
		Pgm.Shift();
	}
	// get left hand side: the key or key sequence
	// either (1) entire sequence is in quotes
	// or (2) sequence goes until the first whitespace
	if(Pgm.EndOfCommand()) {
		; // Fall through 
	}
	else if(Pgm.IsStringValue(Pgm.GetCurTokenIdx()) && (lhs = TryToGetString())) {
		FPRINTF((stderr, "Got bind quoted lhs = \"%s\"\n", lhs));
	}
	else {
		char * first = gp_input_line + Pgm.P_Token[Pgm.CToken].start_index;
		int size = strcspn(first, " \";");
		lhs = (char *)gp_alloc(size + 1, "bind_command->lhs");
		strncpy(lhs, first, size);
		lhs[size] = '\0';
		FPRINTF((stderr, "Got bind unquoted lhs = \"%s\"\n", lhs));
		while(gp_input_line + Pgm.P_Token[Pgm.CToken].start_index < first+size)
			Pgm.Shift();
	}
	// 
	// get right hand side: the command to bind either (1) quoted command or (2) the rest of the line
	// 
	if(Pgm.EndOfCommand()) {
		; // Fall through 
	}
	else if(Pgm.IsStringValue(Pgm.GetCurTokenIdx()) && (rhs = TryToGetString())) {
		FPRINTF((stderr, "Got bind quoted rhs = \"%s\"\n", rhs));
	}
	else {
		int save_token = Pgm.GetCurTokenIdx();
		while(!Pgm.EndOfCommand())
			Pgm.Shift();
		Pgm.MCapture(&rhs, save_token, Pgm.GetPrevTokenIdx());
		FPRINTF((stderr, "Got bind unquoted rhs = \"%s\"\n", rhs));
	}
	// bind_process() will eventually free lhs / rhs ! 
	BindProcess(lhs, rhs, allwindows);
}

#endif /* USE_MOUSE */
// 
// 'break' and 'continue' commands act as in the C language.
// Skip to the end of current loop iteration and (for break)
// do not iterate further
// 
//void break_command()
void GpProgram::BreakCommand()
{
	Shift();
	if(iteration_depth) {
		// Skip to end of current iteration 
		CToken = NumTokens;
		// request that subsequent iterations should be skipped also 
		requested_break = TRUE;
	}
}

//void continue_command()
void GpProgram::ContinueCommand()
{
	Shift();
	if(iteration_depth) {
		// Skip to end of current clause 
		CToken = NumTokens;
		// request that remainder of this iteration be skipped also 
		requested_continue = TRUE;
	}
}

bool iteration_early_exit()
{
	return (requested_continue || requested_break);
}
/*
 * Command parser functions
 */
//
// process the 'call' command 
//
//void call_command()
void GnuPlot::CallCommand()
{
	char * save_file = NULL;
	Pgm.Shift();
	save_file = TryToGetString();
	if(!save_file)
		IntErrorCurToken("expecting filename");
	gp_expand_tilde(&save_file);
	// Argument list follows filename 
	LoadFile(loadpath_fopen(save_file, "r"), save_file, 2);
}
//
// process the 'cd' command 
//
//void changedir_command()
void GnuPlot::ChangeDirCommand()
{
	Pgm.Shift();
	char * save_file = TryToGetString();
	if(!save_file)
		IntErrorCurToken("expecting directory name");
	gp_expand_tilde(&save_file);
	if(changedir(save_file))
		IntErrorCurToken("Can't change to this directory");
	else
		UpdateGpvalVariables(5);
	SAlloc::F(save_file);
}
//
// process the 'clear' command 
//
//void clear_command()
void GnuPlot::ClearCommand(GpTermEntry * pTerm)
{
	TermStartPlot(pTerm);
	if(multiplot && pTerm->fillbox) {
		int xx1 = static_cast<int>(V.Offset.X * pTerm->MaxX);
		int yy1 = static_cast<int>(V.Offset.Y * pTerm->MaxY);
		uint width  = static_cast<uint>(V.Size.x * pTerm->MaxX);
		uint height = static_cast<uint>(V.Size.y * pTerm->MaxY);
		(pTerm->fillbox)(pTerm, 0, xx1, yy1, width, height);
	}
	TermEndPlot(pTerm);
	screen_ok = FALSE;
	Pgm.Shift();
}
// 
// process the 'evaluate' command 
// 
//void eval_command()
void GnuPlot::EvalCommand()
{
	Pgm.Shift();
	char * command = TryToGetString();
	if(!command)
		IntErrorCurToken("Expected command string");
	DoStringAndFree(command);
}
//
// process the 'exit' and 'quit' commands 
//
//void exit_command()
void GnuPlot::ExitCommand()
{
	// If the command is "exit gnuplot" then do so */
	if(Pgm.EqualsNext("gnuplot"))
		gp_exit(EXIT_SUCCESS);
	if(Pgm.EqualsNext("status")) {
		Pgm.Shift();
		Pgm.Shift();
		int status = IntExpression();
		gp_exit(status);
	}
	// exit error 'error message'  returns to the top command line 
	if(Pgm.EqualsNext("error")) {
		Pgm.Shift();
		Pgm.Shift();
		IntError(NO_CARET, TryToGetString());
	}
	// else graphics will be tidied up in main 
	command_exit_requested = 1;
}

/* fit_command() is in fit.c */

/* help_command() is below */
//
// process the 'history' command 
//
//void history_command()
void GnuPlot::HistoryCommand()
{
#ifdef USE_READLINE
	Pgm.Shift();
	if(!Pgm.EndOfCommand() && Pgm.EqualsCur("?")) {
		static char * search_str = NULL; // string from command line to search for 
		// find and show the entries 
		Pgm.Shift();
		Pgm.MCapture(&search_str, Pgm.GetCurTokenIdx(), Pgm.GetCurTokenIdx()); // reallocates memory 
		printf("history ?%s\n", search_str);
		if(!history_find_all(search_str))
			IntErrorCurToken("not in history");
		Pgm.Shift();
	}
	else if(!Pgm.EndOfCommand() && Pgm.EqualsCur("!")) {
		const char * line_to_do = NULL; /* command returned by search	*/
		Pgm.Shift();
		if(Pgm.IsANumber(Pgm.GetCurTokenIdx())) {
			int i = IntExpression();
			line_to_do = history_find_by_number(i);
		}
		else {
			char * search_str = NULL; /* string from command line to search for */
			Pgm.MCapture(&search_str, Pgm.GetCurTokenIdx(), Pgm.GetCurTokenIdx());
			line_to_do = history_find(search_str);
			SAlloc::F(search_str);
		}
		if(line_to_do == NULL)
			IntErrorCurToken("not in history");
		// Add the command to the history.
		// Note that history commands themselves are no longer added to the history. 
		add_history((char *)line_to_do);
		printf("  Executing:\n\t%s\n", line_to_do);
		DoString(line_to_do);
		Pgm.Shift();
	}
	else {
		int n = 0;         /* print only <last> entries */
		char * tmp;
		bool append = FALSE; /* rewrite output file or append it */
		static char * name = NULL; /* name of the output file; NULL for stdout */
		bool quiet = history_quiet;
		if(!Pgm.EndOfCommand() && Pgm.AlmostEqualsCur("q$uiet")) {
			/* option quiet to suppress history entry numbers */
			quiet = TRUE;
			Pgm.Shift();
		}
		/* show history entries */
		if(!Pgm.EndOfCommand() && Pgm.IsANumber(Pgm.GetCurTokenIdx())) {
			n = IntExpression();
		}
		if((tmp = TryToGetString())) {
			SAlloc::F(name);
			name = tmp;
			if(!Pgm.EndOfCommand() && Pgm.AlmostEqualsCur("ap$pend")) {
				append = TRUE;
				Pgm.Shift();
			}
		}
		write_history_n(n, (quiet ? "" : name), (append ? "a" : "w"));
	}
#else
	Pgm.Shift();
	IntWarn(NO_CARET, "This copy of gnuplot was built without support for command history.");
#endif /* defined(USE_READLINE) */
}

#define REPLACE_ELSE(tok)             \
	do {                                  \
		int idx = token[tok].start_index; \
		token[tok].length = 1;            \
		gp_input_line[idx ++] = ';'; /* e */  \
		gp_input_line[idx ++] = ' '; /* l */  \
		gp_input_line[idx ++] = ' '; /* s */  \
		gp_input_line[idx ++] = ' '; /* e */  \
	} while(0)
//
// Make a copy of an input line substring delimited by { and } 
//
static char * new_clause(int clause_start, int clause_end)
{
	char * clause = (char *)gp_alloc(clause_end - clause_start, "clause");
	memcpy(clause, &gp_input_line[clause_start+1], clause_end - clause_start);
	clause[clause_end - clause_start - 1] = '\0';
	return clause;
}
// 
// if / else command
// 
// Aug 2020:
// Restructured to handle if / else if / else sequences in a single routine.
// Because the routine must handle recursion, the state flag (if_status)
// is made a parameter rather than a global.
// 
//static void if_else_command(ifstate if_state)
void GnuPlot::IfElseCommand(ifstate if_state)
{
	int    clause_start;
	int    clause_end;
	int    next_token;
	// initial or recursive ("else if") if clause 
	if(Pgm.EqualsCur("if")) {
		at_type * expr;
		if(!Pgm.Equals(++Pgm.CToken, "("))
			IntErrorCurToken("expecting (expression)");
		// advance past if condition whether or not we evaluate it 
		expr = TempAt();
		if(Pgm.EqualsCur("{")) {
			next_token = Pgm.FindClause(&clause_start, &clause_end);
		}
		else {
			// pre-v5 syntax for "if" with no curly brackets 
			OldIfCommand(expr);
			return;
		}
		if(if_state == IF_TRUE) {
			// This means we are here recursively in an "else if"
			// following an "if" clause that was already executed.
			// Skip both the expression and the bracketed clause.
			Pgm.SetTokenIdx(next_token);
		}
		else if(TRUE || if_state == IF_INITIAL) {
			GpValue condition;
			EvaluateAt(expr, &condition);
			if(real(&condition) == 0) {
				if_state = IF_FALSE;
				Pgm.SetTokenIdx(next_token);
			}
			else {
				if_state = IF_TRUE;
				char * clause = new_clause(clause_start, clause_end);
				BeginClause();
				DoStringAndFree(clause);
				EndClause();
				if(iteration_early_exit())
					Pgm.SetTokenIdx(Pgm.NumTokens);
				else
					Pgm.SetTokenIdx(next_token);
			}
		}
		else
			IntErrorCurToken("unexpected if_state");
	}
	// Done with "if" portion.  Check for "else" 
	if(Pgm.EqualsCur("else")) {
		Pgm.Shift();
		if(Pgm.EqualsCur("if")) {
			IfElseCommand(if_state); // @recursion
		}
		else if(Pgm.EqualsCur("{")) {
			next_token = Pgm.FindClause(&clause_start, &clause_end);
			if(if_state == IF_TRUE) {
				Pgm.SetTokenIdx(next_token);
			}
			else {
				char * clause;
				if_state = IF_TRUE;
				clause = new_clause(clause_start, clause_end);
				BeginClause();
				DoStringAndFree(clause);
				EndClause();
				if(iteration_early_exit())
					Pgm.SetTokenIdx(Pgm.NumTokens);
				else
					Pgm.SetTokenIdx(next_token);
			}
			if_open_for_else = FALSE;
		}
		else {
			IntErrorCurToken("expecting bracketed else clause");
		}
	}
	else {
		// There was no "else" on this line but we might see it on another line 
		if_open_for_else = !(if_state == IF_TRUE);
	}
}
// 
// The original if_command and else_command become wrappers
// 
void GnuPlot::IfCommand()
{
	IfElseCommand(IF_INITIAL);
}

void GnuPlot::ElseCommand()
{
	IfElseCommand(if_open_for_else ? IF_FALSE : IF_TRUE);
}
// 
// Old if/else syntax (no curly braces) is confined to a single input line.
// The rest of the line is *always* consumed.
// 
//static void old_if_command(at_type * expr)
void GnuPlot::OldIfCommand(at_type * expr)
{
	GpValue condition;
	char * if_start, * if_end;
	char * else_start = NULL;
	if(clause_depth > 0)
		IntErrorCurToken("Old-style if/else statement encountered inside brackets");
	EvaluateAt(expr, &condition);
	// find start and end of the "if" part 
	if_start = &gp_input_line[Pgm.P_Token[Pgm.CToken].start_index ];
	while((Pgm.CToken < Pgm.NumTokens) && !Pgm.EqualsCur("else"))
		Pgm.Shift();
	if(Pgm.EqualsCur("else")) {
		if_end = &gp_input_line[Pgm.P_Token[Pgm.CToken].start_index-1];
		*if_end = '\0';
		else_start = &gp_input_line[Pgm.P_Token[Pgm.CToken].start_index + Pgm.P_Token[Pgm.CToken].length ];
	}
	if(real(&condition) != 0.0)
		DoString(if_start);
	else if(else_start)
		DoString(else_start);
	Pgm.SetTokenIdx(0);
	Pgm.NumTokens = 0; // discard rest of line 
}
//
// process commands of the form 'do for [i=1:N] ...' 
//
//void do_command()
void GnuPlot::DoCommand()
{
	int do_start, do_end;
	int end_token;
	char * clause;
	Pgm.Shift();
	GpIterator * do_iterator = CheckForIteration();
	if(forever_iteration(do_iterator)) {
		cleanup_iteration(do_iterator);
		IntError(Pgm.CToken-2, "unbounded iteration not accepted here");
	}
	if(!Pgm.EqualsCur("{")) {
		cleanup_iteration(do_iterator);
		IntErrorCurToken("expecting {do-clause}");
	}
	end_token = Pgm.FindClause(&do_start, &do_end);
	clause = new_clause(do_start, do_end);
	BeginClause();
	iteration_depth++;
	// Sometimes the start point of a nested iteration is not within the
	// limits for all levels of nesting. In this case we need to advance
	// through the iteration to find the first good set of indices.
	// If we don't find one, forget the whole thing.
	if(empty_iteration(do_iterator) && !NextIteration(do_iterator)) {
		strcpy(clause, ";");
	}
	do {
		requested_continue = FALSE;
		DoString(clause);
		if(command_exit_requested != 0)
			requested_break = TRUE;
		if(requested_break)
			break;
	} while(NextIteration(do_iterator));
	iteration_depth--;
	SAlloc::F(clause);
	EndClause();
	Pgm.SetTokenIdx(end_token);
	// FIXME:  If any of the above exited via GPO.IntError() then this	
	// cleanup never happens and we leak memory.  But do_iterator can	
	// not be static or global because do_command() can recurse.	
	do_iterator = cleanup_iteration(do_iterator);
	requested_break = FALSE;
	requested_continue = FALSE;
}
// 
// process commands of the form 'while (foo) {...}' */
// FIXME:  For consistency there should be an iterator associated
// with this statement.
// 
//void while_command()
void GnuPlot::WhileCommand()
{
	int    do_start, do_end;
	char * clause;
	int    end_token;
	Pgm.Shift();
	int    save_token = Pgm.GetCurTokenIdx();
	double exprval = RealExpression();
	if(!Pgm.EqualsCur("{"))
		IntErrorCurToken("expecting {while-clause}");
	end_token = Pgm.FindClause(&do_start, &do_end);
	clause = new_clause(do_start, do_end);
	BeginClause();
	iteration_depth++;
	while(exprval != 0) {
		requested_continue = FALSE;
		DoString(clause);
		if(command_exit_requested)
			requested_break = TRUE;
		if(requested_break)
			break;
		Pgm.SetTokenIdx(save_token);
		exprval = GPO.RealExpression();
	}
	iteration_depth--;
	EndClause();
	SAlloc::F(clause);
	Pgm.SetTokenIdx(end_token);
	requested_break = FALSE;
	requested_continue = FALSE;
}
/*
 * set link [x2|y2] {via <expression1> {inverse <expression2>}}
 * set nonlinear <axis> via <expression1> inverse <expression2>
 * unset link [x2|y2]
 * unset nonlinear <axis>
 */
//void link_command()
void GnuPlot::LinkCommand()
{
	GpAxis * primary_axis = NULL;
	GpAxis * secondary_axis = NULL;
	bool linked = FALSE;
	int command_token = Pgm.GetCurTokenIdx();    /* points to "link" or "nonlinear" */
	Pgm.Shift();
	// Set variable name accepatable for the via/inverse functions 
	strcpy(c_dummy_var[0], "x");
	strcpy(c_dummy_var[1], "y");
	if(Pgm.EqualsCur("z") || Pgm.EqualsCur("cb"))
		strcpy(c_dummy_var[0], "z");
	if(Pgm.EqualsCur("r"))
		strcpy(c_dummy_var[0], "r");
	/*
	 * "set nonlinear" currently supports axes x x2 y y2 z r cb
	 */
	if(Pgm.Equals(command_token, "nonlinear")) {
		AXIS_INDEX axis;
		if((axis = (AXIS_INDEX)Pgm.LookupTableForCurrentToken(axisname_tbl)) >= 0)
			secondary_axis = &AxS[axis];
		else
			IntErrorCurToken("not a valid nonlinear axis");
		primary_axis = AxS.GetShadowAxis(secondary_axis);
		/* Trap attempt to set an already-linked axis to nonlinear */
		/* This catches the sequence "set link y; set nonlinear y2" */
		if(secondary_axis->linked_to_primary && secondary_axis->linked_to_primary->index > 0)
			IntError(NO_CARET, "must unlink axis before setting it to nonlinear");
		if(secondary_axis->linked_to_secondary && secondary_axis->linked_to_secondary->index > 0)
			IntError(NO_CARET, "must unlink axis before setting it to nonlinear");
		/* Clear previous log status */
		secondary_axis->log = FALSE;
		secondary_axis->ticdef.logscaling = FALSE;
		/*
		 * "set link" applies to either x|x2 or y|y2
		 * Flag the axes as being linked, and copy the range settings
		 * from the primary axis into the linked secondary axis
		 */
	}
	else {
		if(Pgm.AlmostEqualsCur("x$2")) {
			primary_axis = &AxS[FIRST_X_AXIS];
			secondary_axis = &AxS[SECOND_X_AXIS];
		}
		else if(Pgm.AlmostEqualsCur("y$2")) {
			primary_axis = &AxS[FIRST_Y_AXIS];
			secondary_axis = &AxS[SECOND_Y_AXIS];
		}
		else {
			IntErrorCurToken("expecting x2 or y2");
		}
		// This catches the sequence "set nonlinear x; set link x2" 
		if(primary_axis->linked_to_primary)
			IntError(NO_CARET, "You must clear nonlinear x or y before linking it");
		// This catches the sequence "set nonlinear x2; set link x2" 
		if(secondary_axis->linked_to_primary && secondary_axis->linked_to_primary->index <= 0)
			IntError(NO_CARET, "You must clear nonlinear x2 or y2 before linking it");
	}
	Pgm.Shift();
	// "unset link {x|y}" command 
	if(Pgm.Equals(command_token-1, "unset")) {
		primary_axis->linked_to_secondary = NULL;
		if(secondary_axis->linked_to_primary == NULL)
			/* It wasn't linked anyhow */
			return;
		else
			secondary_axis->linked_to_primary = NULL;
		// FIXME: could return here except for the need to free link_udf->at 
		linked = FALSE;
	}
	else
		linked = TRUE;
	// Initialize the action tables for the mapping function[s] 
	if(!primary_axis->link_udf) {
		primary_axis->link_udf = (udft_entry *)gp_alloc(sizeof(udft_entry), "link_at");
		memzero(primary_axis->link_udf, sizeof(udft_entry));
	}
	if(!secondary_axis->link_udf) {
		secondary_axis->link_udf = (udft_entry *)gp_alloc(sizeof(udft_entry), "link_at");
		memzero(secondary_axis->link_udf, sizeof(udft_entry));
	}
	if(Pgm.EqualsCur("via")) {
		ParseLinkVia(secondary_axis->link_udf);
		if(Pgm.AlmostEqualsCur("inv$erse")) {
			ParseLinkVia(primary_axis->link_udf);
		}
		else {
			IntWarnCurToken("inverse mapping function required");
			linked = FALSE;
		}
	}
	else if(Pgm.Equals(command_token, "nonlinear") && linked) {
		IntWarnCurToken("via mapping function required");
		linked = FALSE;
	}
	if(Pgm.Equals(command_token, "nonlinear") && linked) {
		// Save current user-visible axis range (note reversed order!) 
		udft_entry * temp = primary_axis->link_udf;
		primary_axis->link_udf = secondary_axis->link_udf;
		secondary_axis->link_udf = temp;
		secondary_axis->linked_to_primary = primary_axis;
		primary_axis->linked_to_secondary = secondary_axis;
		CloneLinkedAxes(secondary_axis, primary_axis);
	}
	else if(linked) {
		// Clone the range information 
		secondary_axis->linked_to_primary = primary_axis;
		primary_axis->linked_to_secondary = secondary_axis;
		CloneLinkedAxes(primary_axis, secondary_axis);
	}
	else {
		free_at(secondary_axis->link_udf->at);
		secondary_axis->link_udf->at = NULL;
		free_at(primary_axis->link_udf->at);
		primary_axis->link_udf->at = NULL;
		// Shouldn't be necessary, but it doesn't hurt 
		primary_axis->linked_to_secondary = NULL;
		secondary_axis->linked_to_primary = NULL;
	}
	if(secondary_axis->index == POLAR_AXIS)
		RRangeToXY();
}
//
// process the 'load' command 
//
//void load_command()
void GnuPlot::LoadCommand()
{
	Pgm.Shift();
	if(Pgm.EqualsCur("$") && Pgm.IsLetter(Pgm.GetCurTokenIdx()+1) && !Pgm.Equals(Pgm.GetCurTokenIdx()+2, "[")) {
		// "load" a datablock rather than a file 
		// datablock_name will eventually be freed by lf_pop() 
		char * datablock_name = gp_strdup(Pgm.ParseDatablockName());
		LoadFile(NULL, datablock_name, 6);
	}
	else {
		// These need to be local so that recursion works. 
		// They will eventually be freed by lf_pop(). 
		FILE * fp;
		char * save_file = TryToGetString();
		if(!save_file)
			IntErrorCurToken("expecting filename");
		gp_expand_tilde(&save_file);
		fp = strcmp(save_file, "-") ? loadpath_fopen(save_file, "r") : stdout;
		LoadFile(fp, save_file, 1);
	}
}

/* null command */
void null_command()
{
	return;
}
// 
// Clauses enclosed by curly brackets:
// do for [i = 1:N] { a; b; c; }
// if(<test>) {
//   line1;
//   line2;
// } 
// else {
//   ...
// }
// 
// 
// Find the start and end character positions within gp_input_line
// bounding a clause delimited by {...}.
// Assumes that c_token indexes the opening left curly brace.
// Returns the index of the first token after the closing curly brace.
// 
//int find_clause(int * clause_start, int * clause_end)
int GpProgram::FindClause(int * pClauseStart, int * pClauseEnd)
{
	int i, depth;
	*pClauseStart = P_Token[CToken].start_index;
	for(i = ++CToken, depth = 1; i < NumTokens; i++) {
		if(Equals(i, "{"))
			depth++;
		else if(Equals(i, "}"))
			depth--;
		if(depth == 0)
			break;
	}
	*pClauseEnd = P_Token[i].start_index;
	return (i+1);
}

//void begin_clause()
void GnuPlot::BeginClause()
{
	clause_depth++;
	Pgm.Shift();
}

//void end_clause()
void GnuPlot::EndClause()
{
	if(clause_depth == 0)
		IntErrorCurToken("unexpected }");
	else
		clause_depth--;
	Pgm.Shift();
}

void clause_reset_after_error()
{
	if(clause_depth)
		FPRINTF((stderr, "CLAUSE RESET after error at depth %d\n", clause_depth));
	clause_depth = 0;
	iteration_depth = 0;
}
//
// helper routine to multiplex mouse event handling with a timed pause command 
//
void timed_pause(double sleep_time)
{
	if(sleep_time > 0.0) {
#if defined(HAVE_USLEEP) && defined(USE_MOUSE) && !defined(_WIN32)
		if(term->waitforinput)          /* If the terminal supports it */
			while(sleep_time > 0.05) { /* we poll 20 times a second */
				usleep(50000);  /* Sleep for 50 msec */
				check_for_mouse_events();
				sleep_time -= 0.05;
			}
		usleep((useconds_t)(sleep_time * 1e6));
		check_for_mouse_events();
#else
		GP_SLEEP(static_cast<uint>(sleep_time));
#endif
	}
}

/* process the 'pause' command */
#define EAT_INPUT_WITH(slurp) do {int junk = 0; do {junk = slurp;} while(junk != EOF && junk != '\n');} while(0)

//void pause_command()
void GnuPlot::PauseCommand()
{
	int text = 0;
	double sleep_time;
	static char * buf = NULL;
	Pgm.Shift();
#ifdef USE_MOUSE
	paused_for_mouse = 0;
	if(Pgm.EqualsCur("mouse")) {
		sleep_time = -1;
		Pgm.Shift();
		// EAM FIXME - This is not the correct test; what we really want 
		// to know is whether or not the terminal supports mouse feedback 
		// if (term_initialised) { 
		if(mouse_setting.on && term) {
			udvt_entry * current;
			int end_condition = 0;
			while(!(Pgm.EndOfCommand())) {
				if(Pgm.AlmostEqualsCur("key$press")) {
					end_condition |= PAUSE_KEYSTROKE;
					Pgm.Shift();
				}
				else if(Pgm.EqualsCur(",")) {
					Pgm.Shift();
				}
				else if(Pgm.EqualsCur("any")) {
					end_condition |= PAUSE_ANY;
					Pgm.Shift();
				}
				else if(Pgm.EqualsCur("button1")) {
					end_condition |= PAUSE_BUTTON1;
					Pgm.Shift();
				}
				else if(Pgm.EqualsCur("button2")) {
					end_condition |= PAUSE_BUTTON2;
					Pgm.Shift();
				}
				else if(Pgm.EqualsCur("button3")) {
					end_condition |= PAUSE_BUTTON3;
					Pgm.Shift();
				}
				else if(Pgm.EqualsCur("close")) {
					end_condition |= PAUSE_WINCLOSE;
					Pgm.Shift();
				}
				else
					break;
			}
			if(end_condition)
				paused_for_mouse = end_condition;
			else
				paused_for_mouse = PAUSE_CLICK;
			// Set the pause mouse return codes to -1 
			current = Ev.AddUdvByName("MOUSE_KEY");
			Ginteger(&current->udv_value, -1);
			current = Ev.AddUdvByName("MOUSE_BUTTON");
			Ginteger(&current->udv_value, -1);
		}
		else
			IntWarn(NO_CARET, "Mousing not active");
	}
	else
#endif
	sleep_time = RealExpression();
	if(Pgm.EndOfCommand()) {
		SAlloc::F(buf); /* remove the previous message */
		buf = gp_strdup("paused"); /* default message, used in Windows GUI pause dialog */
	}
	else {
		char * tmp = TryToGetString();
		if(!tmp)
			IntErrorCurToken("expecting string");
		else {
#ifdef _WIN32
			SAlloc::F(buf);
			buf = tmp;
			if(sleep_time >= 0) {
				fputs(buf, stderr);
			}
#else /* Not _WIN32 or OS2 */
			SAlloc::F(buf);
			buf = tmp;
			fputs(buf, stderr);
#endif
			text = 1;
		}
	}
	if(sleep_time < 0) {
#if defined(_WIN32)
		ctrlc_flag = FALSE;
#if defined(WGP_CONSOLE) && defined(USE_MOUSE)
		if(!paused_for_mouse || !MousableWindowOpened()) {
			int junk = 0;
			if(buf) {
				/* Use of fprintf() triggers a bug in MinGW + SJIS encoding */
				fputs(buf, stderr); fputs("\n", stderr);
			}
			/* cannot use EAT_INPUT_WITH here */
			do {
				junk = getch();
				if(ctrlc_flag)
					bail_to_command_line();
			} while(junk != EOF && junk != '\n' && junk != '\r');
		}
		else /* paused_for_mouse */
#endif /* !WGP_CONSOLE */
		{
			if(!Pause(buf)) /* returns false if Ctrl-C or Cancel was pressed */
				bail_to_command_line();
		}
#else /* !(_WIN32 || OS2) */
#ifdef USE_MOUSE
		if(term && term->waitforinput) {
			/* It does _not_ work to do EAT_INPUT_WITH(term->waitforinput()) */
			term->waitforinput(0);
		}
		else
#endif
		EAT_INPUT_WITH(fgetc(stdin));
#endif /* !(_WIN32 || OS2) */
	}
	if(sleep_time > 0)
		timed_pause(sleep_time);
	if(text != 0 && sleep_time >= 0)
		fputc('\n', stderr);
	screen_ok = FALSE;
}
//
// process the 'plot' command 
//
//void plot_command()
void GnuPlot::PlotCommand(GpTermEntry * pTerm)
{
	plot_token = Pgm.Shift();
	plotted_data_from_stdin = FALSE;
	refresh_nplots = 0;
	SET_CURSOR_WAIT;
#ifdef USE_MOUSE
	PlotMode(pTerm, MODE_PLOT);
	Ev.AddUdvByName("MOUSE_X")->udv_value.SetNotDefined();
	Ev.AddUdvByName("MOUSE_Y")->udv_value.SetNotDefined();
	Ev.AddUdvByName("MOUSE_X2")->udv_value.SetNotDefined();
	Ev.AddUdvByName("MOUSE_Y2")->udv_value.SetNotDefined();
	Ev.AddUdvByName("MOUSE_BUTTON")->udv_value.SetNotDefined();
	Ev.AddUdvByName("MOUSE_SHIFT")->udv_value.SetNotDefined();
	Ev.AddUdvByName("MOUSE_ALT")->udv_value.SetNotDefined();
	Ev.AddUdvByName("MOUSE_CTRL")->udv_value.SetNotDefined();
#endif
	PlotRequest(pTerm);
	// Clear "hidden" flag for any plots that may have been toggled off 
	if(pTerm->modify_plots)
		pTerm->modify_plots(MODPLOTS_SET_VISIBLE, -1);
	SET_CURSOR_ARROW;
}

void print_set_output(char * name, bool datablock, bool append_p)
{
	if(print_out && print_out != stderr && print_out != stdout) {
#ifdef PIPES
		if(print_out_name[0] == '|') {
			if(0 > pclose(print_out))
				perror(print_out_name);
		}
		else
#endif
		if(0 > fclose(print_out))
			perror(print_out_name);
		print_out = stderr;
	}
	ZFREE(print_out_name);
	print_out_var = NULL;
	if(!name) {
		print_out = stderr;
		return;
	}
	if(strcmp(name, "-") == 0) {
		print_out = stdout;
		return;
	}
#ifdef PIPES
	if(name[0] == '|') {
		restrict_popen();
		print_out = popen(name + 1, "w");
		if(!print_out)
			perror(name);
		else
			print_out_name = name;
		return;
	}
#endif
	if(!datablock) {
		print_out = fopen(name, append_p ? "a" : "w");
		if(!print_out) {
			perror(name);
			return;
		}
	}
	else {
		print_out_var = GPO.Ev.AddUdvByName(name);
		if(!append_p)
			gpfree_datablock(&print_out_var->udv_value);
		/* If this is not an existing datablock to be appended */
		/* then make it a new empty datablock */
		if(print_out_var->udv_value.type != DATABLOCK) {
			print_out_var->udv_value.Destroy();
			print_out_var->udv_value.type = DATABLOCK;
			print_out_var->udv_value.v.data_array = NULL;
		}
	}
	print_out_name = name;
}

char * print_show_output()
{
	if(print_out_name)
		return print_out_name;
	else if(print_out == stdout)
		return "<stdout>";
	else if(!print_out || print_out == stderr || !print_out_name)
		return "<stderr>";
	else
		return print_out_name;
}
//
// 'printerr' is the same as 'print' except that output is always to stderr 
//
void printerr_command()
{
	FILE * save_print_out = print_out;
	print_out = stderr;
	GPO.PrintCommand();
	print_out = save_print_out;
}
//
// process the 'print' command 
//
//void print_command()
void GnuPlot::PrintCommand()
{
	GpValue a;
	// space printed between two expressions only 
	bool need_space = FALSE;
	char * dataline = NULL;
	size_t size = 256;
	size_t len = 0;
	SETIFZ(print_out, stderr);
	if(print_out_var) { // print to datablock 
		dataline = (char *)gp_alloc(size, "dataline");
		*dataline = NUL;
	}
	screen_ok = FALSE;
	do {
		Pgm.Shift();
		if(Pgm.EqualsCur("$") && Pgm.IsLetter(Pgm.GetCurTokenIdx()+1) && !Pgm.Equals(Pgm.GetCurTokenIdx()+2, "[")) {
			char * datablock_name = Pgm.ParseDatablockName();
			char ** line = GetDatablock(datablock_name);
			// Printing a datablock into itself would cause infinite recursion 
			if(print_out_var && !strcmp(datablock_name, print_out_name))
				continue;
			while(line && *line) {
				if(print_out_var)
					append_to_datablock(&print_out_var->udv_value, sstrdup(*line));
				else
					fprintf(print_out, "%s\n", *line);
				line++;
			}
			continue;
		}
		if(Pgm.TypeUdv(Pgm.GetCurTokenIdx()) == ARRAY && !Pgm.EqualsNext("[")) {
			udvt_entry * array = AddUdv(Pgm.CToken++);
			save_array_content(print_out, array->udv_value.v.value_array);
			continue;
		}
		ConstExpress(&a);
		if(a.type == STRING) {
			if(dataline)
				len = strappend(&dataline, &size, len, a.v.string_val);
			else
				fputs(a.v.string_val, print_out);
			need_space = FALSE;
		}
		else {
			if(need_space) {
				if(dataline)
					len = strappend(&dataline, &size, len, " ");
				else
					putc(' ', print_out);
			}
			if(dataline != NULL)
				len = strappend(&dataline, &size, len, ValueToStr(&a, FALSE));
			else
				DispValue(print_out, &a, FALSE);
			need_space = TRUE;
		}
		a.Destroy();
	} while(!Pgm.EndOfCommand() && Pgm.EqualsCur(","));
	if(dataline) {
		append_multiline_to_datablock(&print_out_var->udv_value, dataline);
	}
	else {
		putc('\n', print_out);
		fflush(print_out);
	}
}
//
// process the 'pwd' command 
//
void pwd_command()
{
	char * save_file = (char *)gp_alloc(PATH_MAX, "print current dir");
	if(GP_GETCWD(save_file, PATH_MAX) == NULL)
		fprintf(stderr, "<invalid>\n");
	else
		fprintf(stderr, "%s\n", save_file);
	SAlloc::F(save_file);
	GPO.Pgm.Shift();
}
// 
// EAM April 2007
// The "refresh" command replots the previous graph without going back to read
// the original data. This allows zooming or other operations on data that was
// only transiently available in the input stream.
// 
//void refresh_command()
void GnuPlot::RefreshCommand()
{
	Pgm.Shift();
	RefreshRequest();
}

//void refresh_request()
void GnuPlot::RefreshRequest()
{
	/*AXIS_INDEX*/int axis;
	if((!P_FirstPlot && (refresh_ok == E_REFRESH_OK_2D)) || (!first_3dplot && (refresh_ok == E_REFRESH_OK_3D)) || (!*replot_line && (refresh_ok == E_REFRESH_NOT_OK)))
		IntError(NO_CARET, "no active plot; cannot refresh");
	if(refresh_ok == E_REFRESH_NOT_OK) {
		IntWarn(NO_CARET, "cannot refresh from this state. trying full replot");
		ReplotRequest(term);
	}
	else {
		// The margins from "set offset" were already applied; don't reapply them here
		Gr.RetainOffsets = true;
		// Restore the axis range/scaling state from original plot.
		// Dima Kogan April 2018
		for(axis = (AXIS_INDEX)0; axis < NUMBER_OF_MAIN_VISIBLE_AXES; axis++) {
			GpAxis * this_axis = &AxS[axis];
			if((this_axis->set_autoscale & AUTOSCALE_MIN) && (this_axis->writeback_min < VERYLARGE))
				this_axis->set_min = this_axis->writeback_min;
			else
				this_axis->min = this_axis->set_min;
			if((this_axis->set_autoscale & AUTOSCALE_MAX) && (this_axis->writeback_max > -VERYLARGE))
				this_axis->set_max = this_axis->writeback_max;
			else
				this_axis->max = this_axis->set_max;
			if(this_axis->linked_to_secondary)
				CloneLinkedAxes(this_axis, this_axis->linked_to_secondary);
			else if(this_axis->linked_to_primary) {
				if(this_axis->linked_to_primary->autoscale != AUTOSCALE_BOTH)
					CloneLinkedAxes(this_axis, this_axis->linked_to_primary);
			}
		}
		if(refresh_ok == E_REFRESH_OK_2D) {
			RefreshBounds(P_FirstPlot, refresh_nplots);
			DoPlot(term, P_FirstPlot, refresh_nplots);
			UpdateGpvalVariables(1);
		}
		else if(refresh_ok == E_REFRESH_OK_3D) {
			Refresh3DBounds(term, first_3dplot, refresh_nplots);
			Do3DPlot(term, first_3dplot, refresh_nplots, /*0*/NORMAL_REPLOT);
			UpdateGpvalVariables(1);
		}
		else
			IntError(NO_CARET, "Internal error - refresh of unknown plot type");
	}
}
//
// process the 'replot' command 
//
//void replot_command()
void GnuPlot::ReplotCommand()
{
	if(!*replot_line)
		IntErrorCurToken("no previous plot");
	if(Gg.VolatileData && (refresh_ok != E_REFRESH_NOT_OK) && !replot_disabled) {
		FPRINTF((stderr, "volatile_data %d refresh_ok %d plotted_data_from_stdin %d\n", Gg.VolatileData, refresh_ok, plotted_data_from_stdin));
		RefreshCommand();
	}
	else {
		// Disable replot for some reason; currently used by the mouse/hotkey
		// capable terminals to avoid replotting when some data come from stdin,
		// i.e. when  plotted_data_from_stdin==1  after plot "-".
		if(replot_disabled) {
			replot_disabled = FALSE;
			bail_to_command_line(); /* be silent --- don't mess the screen */
		}
		if(!term) // unknown terminal 
			IntErrorCurToken("use 'set term' to set terminal type first");
		Pgm.Shift();
		SET_CURSOR_WAIT;
		if(term->flags & TERM_INIT_ON_REPLOT)
			term->init(term);
		ReplotRequest(term);
		SET_CURSOR_ARROW;
	}
}
//
// process the 'reread' command 
//
void reread_command()
{
	FILE * fp = lf_top();
	if(fp)
		rewind(fp);
	GPO.Pgm.Shift();
}
//
// process the 'save' command 
//
//void save_command()
void GnuPlot::SaveCommand()
{
	FILE * fp;
	char * save_file = NULL;
	bool append = FALSE;
	int what;
	Pgm.Shift();
	what = Pgm.LookupTableForCurrentToken(&save_tbl[0]);
	switch(what) {
		case SAVE_FUNCS:
		case SAVE_SET:
		case SAVE_TERMINAL:
		case SAVE_VARS:
		case SAVE_FIT:
		case SAVE_DATABLOCKS:
		    Pgm.Shift();
		    break;
		default:
		    break;
	}
	save_file = TryToGetString();
	if(!save_file)
		IntErrorCurToken("expecting filename");
	if(Pgm.EqualsCur("append")) {
		append = TRUE;
		Pgm.Shift();
	}
#ifdef PIPES
	if(save_file[0]=='|') {
		restrict_popen();
		fp = popen(save_file+1, "w");
	}
	else
#endif
	{
		gp_expand_tilde(&save_file);
#ifdef _WIN32
		fp = !strcmp(save_file, "-") ? stdout : loadpath_fopen(save_file, append ? "a" : "w");
#else
		fp = !strcmp(save_file, "-") ? stdout : fopen(save_file, append ? "a" : "w");
#endif
	}
	if(!fp)
		os_error(Pgm.GetCurTokenIdx(), "Cannot open save file");
	switch(what) {
		case SAVE_FUNCS: save_functions(fp); break;
		case SAVE_SET: save_set(fp); break;
		case SAVE_TERMINAL: save_term(fp); break;
		case SAVE_VARS: save_variables(fp); break;
		case SAVE_FIT: SaveFit(fp); break;
		case SAVE_DATABLOCKS: save_datablocks(fp); break;
		default: SaveAll(fp);
	}
	if(stdout != fp) {
#ifdef PIPES
		if(save_file[0] == '|')
			pclose(fp);
		else
#endif
		fclose(fp);
	}
	SAlloc::F(save_file);
}
//
// process the 'screendump' command 
//
void screendump_command()
{
	GPO.Pgm.Shift();
#ifdef _WIN32
	screen_dump();
#else
	fputs("screendump not implemented\n", stderr);
#endif
}

/* set_command() is in set.c */

/* 'shell' command is processed by do_shell(), see below */

/* show_command() is in show.c */
//
// process the 'splot' command 
//
//void splot_command()
void GnuPlot::SPlotCommand(GpTermEntry * pTerm)
{
	plot_token = Pgm.Shift();
	plotted_data_from_stdin = FALSE;
	refresh_nplots = 0;
	SET_CURSOR_WAIT;
#ifdef USE_MOUSE
	PlotMode(pTerm, MODE_SPLOT);
	Ev.AddUdvByName("MOUSE_X")->udv_value.SetNotDefined();
	Ev.AddUdvByName("MOUSE_Y")->udv_value.SetNotDefined();
	Ev.AddUdvByName("MOUSE_X2")->udv_value.SetNotDefined();
	Ev.AddUdvByName("MOUSE_Y2")->udv_value.SetNotDefined();
	Ev.AddUdvByName("MOUSE_BUTTON")->udv_value.SetNotDefined();
#endif
	Plot3DRequest(pTerm);
	// Clear "hidden" flag for any plots that may have been toggled off 
	if(pTerm->modify_plots)
		pTerm->modify_plots(MODPLOTS_SET_VISIBLE, -1);
	SET_CURSOR_ARROW;
}
//
// process the 'stats' command 
//
void stats_command()
{
#ifdef USE_STATS
	GPO.StatsRequest();
#else
	GPO.IntError(NO_CARET, "This copy of gnuplot was not configured with support for the stats command");
#endif
}
//
// process the 'system' command 
//
void system_command()
{
	GPO.Pgm.Shift();
	char * cmd = GPO.TryToGetString();
	do_system(cmd);
	SAlloc::F(cmd);
}
// 
// process the 'test palette' command
// 1) Write a sequence of plot commands + set commands into a temp file
// 2) Create a datablock with palette values
// 3) Load the temp file to plot from the datablock
//   The set commands then act to restore the initial state
// 
//static void test_palette_subcommand()
void GnuPlot::TestPaletteSubcommand()
{
	enum {
		test_palette_colors = 256
	};
	udvt_entry * datablock;
	char * save_replot_line;
	bool save_is_3d_plot;
	int i;
	static const char pre1[] =
	    "\
reset;\
uns border; se tics scale 0;\
se cbtic 0,0.1,1 mirr format '' scale 1;\
se xr[0:1];se yr[0:1];se zr[0:1];se cbr[0:1];\
set colorbox hor user orig 0.05,0.02 size 0.925,0.12;";

	static const char pre2[] =
	    "\
se lmarg scre 0.05;se rmarg scre 0.975; se bmarg scre 0.22; se tmarg scre 0.86;\
se grid; se xtics 0,0.1;se ytics 0,0.1;\
se key top right at scre 0.975,0.975 horizontal \
title 'R,G,B profiles of the current color palette';";

	static const char pre3[] =
	    "\
p NaN lc palette notit,\
$PALETTE u 1:2 t 'red' w l lt 1 lc rgb 'red',\
'' u 1:3 t 'green' w l lt 1 lc rgb 'green',\
'' u 1:4 t 'blue' w l lt 1 lc rgb 'blue',\
'' u 1:5 t 'NTSC' w l lt 1 lc rgb 'black'\
\n";

	FILE * f = tmpfile();
#if defined(_MSC_VER) || defined(__MINGW32__)
	// On Vista/Windows 7 tmpfile() fails. 
	if(!f) {
		char buf[PATH_MAX];
		// We really want the "ANSI" version 
		GetTempPathA(sizeof(buf), buf);
		strcat(buf, "gnuplot-pal.tmp");
		f = fopen(buf, "w+");
	}
#endif
	while(!Pgm.EndOfCommand())
		Pgm.Shift();
	if(!f)
		IntError(NO_CARET, "cannot write temporary file");
	// Store R/G/B/Int curves in a datablock 
	datablock = Ev.AddUdvByName("$PALETTE");
	if(datablock->udv_value.type != NOTDEFINED)
		gpfree_datablock(&datablock->udv_value);
	datablock->udv_value.type = DATABLOCK;
	datablock->udv_value.v.data_array = NULL;
	// Part of the purpose for writing these values into a datablock 
	// is so that the user can read them back if desired.  But data  
	// will be read back using the current numeric locale, so for    
	// consistency we must also use the locale when creating it.     
	set_numeric_locale();
	for(i = 0; i < test_palette_colors; i++) {
		char dataline[64];
		rgb_color rgb;
		double ntsc;
		double z = (double)i / (test_palette_colors - 1);
		double gray = (SmPltt.Positive == SMPAL_NEGATIVE) ? 1. - z : z;
		Rgb1FromGray(gray, &rgb);
		ntsc = 0.299 * rgb.r + 0.587 * rgb.g + 0.114 * rgb.b;
		sprintf(dataline, "%0.4f %0.4f %0.4f %0.4f %0.4f %c", z, rgb.r, rgb.g, rgb.b, ntsc, '\0');
		append_to_datablock(&datablock->udv_value, sstrdup(dataline));
	}
	reset_numeric_locale();
	// commands to setup the test palette plot 
	enable_reset_palette = 0;
	save_replot_line = gp_strdup(replot_line);
	save_is_3d_plot = Gg.Is3DPlot;
	fputs(pre1, f);
	fputs(pre2, f);
	fputs(pre3, f);
	// save current gnuplot 'set' status because of the tricky sets
	// for our temporary testing plot.
	save_set(f);
	SavePixmaps(f);
	// execute all commands from the temporary file 
	rewind(f);
	LoadFile(f, NULL, 1); /* note: it does fclose(f) */
	// enable reset_palette() and restore replot line 
	enable_reset_palette = 1;
	SAlloc::F(replot_line);
	replot_line = save_replot_line;
	Gg.Is3DPlot = save_is_3d_plot;
}
//
// process the 'test' command 
//
//void test_command()
void GnuPlot::TestCommand()
{
	int what;
	const int save_token = Pgm.Shift();
	if(!term) // unknown terminal 
		IntErrorCurToken("use 'set term' to set terminal type first");
	what = Pgm.LookupTableForCurrentToken(&test_tbl[0]);
	switch(what) {
		default:
		    if(!Pgm.EndOfCommand())
			    IntErrorCurToken("unrecognized test option");
		// otherwise fall through to test_term 
		case TEST_TERMINAL: TestTerminal(term); break;
		case TEST_PALETTE: TestPaletteSubcommand(); break;
	}
	// prevent annoying error messages if there was no previous plot 
	// and the "test" window is resized. 
	if(!replot_line || !(*replot_line)) {
		Pgm.MCapture(&replot_line, save_token, Pgm.GetCurTokenIdx());
	}
}
// 
// toggle a single plot on/off from the command line (only possible for qt, wxt, x11, win)
// 
void toggle_command()
{
	int plotno = -1;
	char * plottitle = NULL;
	bool foundit = FALSE;
	GPO.Pgm.Shift();
	if(GPO.Pgm.EqualsCur("all")) {
		GPO.Pgm.Shift();
	}
	else if((plottitle = GPO.TryToGetString()) != NULL) {
		curve_points * plot;
		int last = strlen(plottitle) - 1;
		if(refresh_ok == E_REFRESH_OK_2D)
			plot = P_FirstPlot;
		else if(refresh_ok == E_REFRESH_OK_3D)
			plot = (curve_points *)first_3dplot;
		else
			plot = NULL;
		if(last >= 0) {
			for(plotno = 0; plot != NULL; plot = plot->next, plotno++) {
				if(plot->title)
					if(!strcmp(plot->title, plottitle) || (plottitle[last] == '*' && !strncmp(plot->title, plottitle, last))) {
						foundit = TRUE;
						break;
					}
			}
		}
		SAlloc::F(plottitle);
		if(!foundit) {
			GPO.IntWarn(NO_CARET, "Did not find a plot with that title");
			return;
		}
	}
	else {
		plotno = GPO.IntExpression() - 1;
	}
	if(term->modify_plots)
		term->modify_plots(MODPLOTS_INVERT_VISIBILITIES, plotno);
}

void update_command()
{
	GPO.IntError(NO_CARET, "DEPRECATED command 'update', please use 'save fit' instead");
}
//
// the "import" command is only implemented if support is configured for */
// using functions from external shared objects as plugins. 
//
//void import_command()
void GnuPlot::ImportCommand()
{
	int start_token = Pgm.GetCurTokenIdx();
#ifdef HAVE_EXTERNAL_FUNCTIONS
	udft_entry * udf;
	int dummy_num = 0;
	char save_dummy[MAX_NUM_VAR][MAX_ID_LEN+1];
	Pgm.Shift();
	if(!Pgm.Equals(Pgm.GetCurTokenIdx()+1, "("))
		IntErrorCurToken("Expecting function template");
	memcpy(save_dummy, c_dummy_var, sizeof(save_dummy));
	do {
		Pgm.Shift(); // skip to the next dummy 
		Pgm.Shift();
		Pgm.CopyStr(c_dummy_var[dummy_num++], Pgm.GetCurTokenIdx(), MAX_ID_LEN);
	} while(Pgm.Equals(Pgm.GetCurTokenIdx()+1, ",") && (dummy_num < MAX_NUM_VAR));
	Pgm.Shift();
	if(Pgm.EqualsCur(","))
		IntError(Pgm.GetCurTokenIdx()+1, "function contains too many parameters");
	if(!Pgm.EqualsCurShift(")"))
		IntErrorCurToken("missing ')'");
	if(!Pgm.EqualsCur("from"))
		IntErrorCurToken("Expecting 'from <sharedobj>'");
	Pgm.Shift();
	udf = dummy_func = AddUdf(start_token+1);
	udf->dummy_num = dummy_num;
	free_at(udf->at); // In case there was a previous function by this name 
	udf->at = ExternalAt(udf->udf_name);
	memcpy(c_dummy_var, save_dummy, sizeof(save_dummy));
	dummy_func = NULL; /* dont let anyone else use our workspace */
	if(!udf->at)
		IntError(NO_CARET, "failed to load external function");
	// Don't copy the definition until we know it worked 
	Pgm.MCapture(&(udf->definition), start_token, Pgm.GetCurTokenIdx()-1);
#else
	while(!Pgm.EndOfCommand())
		Pgm.Shift();
	IntError(start_token, "This copy of gnuplot does not support plugins");
#endif
}
//
// process invalid commands and, on OS/2, REXX commands 
//
//void invalid_command()
void GnuPlot::InvalidCommand()
{
	const int save_token = Pgm.GetCurTokenIdx();
	// Skip the rest of the command; otherwise we're left pointing to 
	// the middle of a command we already know is not valid.          
	while(!Pgm.EndOfCommand())
		Pgm.Shift();
	IntError(save_token, "invalid command");
}
/*
 * Auxiliary routines
 */
/* used by changedir_command() */
static int changedir(char * path)
{
#if defined(_WIN32)
	LPWSTR pathw = UnicodeText(path, encoding);
	int ret = !SetCurrentDirectoryW(pathw);
	SAlloc::F(pathw);
	return ret;
#elif defined(__EMX__) && defined(OS2)
	return _chdir2(path);
#else
	return chdir(path);
#endif
}
//
// used by ReplotCommand() 
//
//void replotrequest()
void GnuPlot::ReplotRequest(GpTermEntry * pTerm)
{
	// do not store directly into the replot_line string until the
	// new plot line has been successfully plotted. This way,
	// if user makes a typo in a replot line, they do not have
	// to start from scratch. The replot_line will be committed
	// after do_plot has returned, whence we know all is well
	if(Pgm.EndOfCommand()) {
		char * rest_args = &gp_input_line[Pgm.P_Token[Pgm.CToken].start_index];
		size_t replot_len = strlen(replot_line);
		size_t rest_len = strlen(rest_args);
		/* preserve commands following 'replot ;' */
		/* move rest of input line to the start
		 * necessary because of realloc() in extend_input_line() */
		memmove(gp_input_line, rest_args, rest_len+1);
		// reallocs if necessary 
		while(gp_input_line_len < replot_len+rest_len+1)
			extend_input_line();
		// move old rest args off begin of input line to make space for replot_line 
		memmove(gp_input_line+replot_len, gp_input_line, rest_len+1);
		// copy previous plot command to start of input line 
		memcpy(gp_input_line, replot_line, replot_len);
	}
	else {
		char * replot_args = NULL; /* else m_capture will free it */
		int last_token = Pgm.NumTokens - 1;
		// length = length of old part + length of new part + ", " + \0 
		size_t newlen = strlen(replot_line) + Pgm.P_Token[last_token].start_index + Pgm.P_Token[last_token].length - Pgm.P_Token[Pgm.CToken].start_index + 3;
		Pgm.MCapture(&replot_args, Pgm.GetCurTokenIdx(), last_token); /* might be empty */
		while(gp_input_line_len < newlen)
			extend_input_line();
		strcpy(gp_input_line, replot_line);
		strcat(gp_input_line, ", ");
		strcat(gp_input_line, replot_args);
		SAlloc::F(replot_args);
	}
	plot_token = 0;         /* whole line to be saved as replot line */
	SET_REFRESH_OK(E_REFRESH_NOT_OK, 0);            /* start of replot will destroy existing data */
	screen_ok = FALSE;
	Pgm.NumTokens = Pgm.Scanner(&gp_input_line, &gp_input_line_len);
	Pgm.SetTokenIdx(1); // Skip the "plot" token 
	if(Pgm.AlmostEquals(0, "test")) {
		Pgm.SetTokenIdx(0);
		TestCommand();
	}
	else if(Pgm.AlmostEquals(0, "s$plot"))
		Plot3DRequest(pTerm);
	else
		PlotRequest(pTerm);
}
//
// Support for input, shell, and help for various systems 
//
#ifdef NO_GIH
#ifdef _WIN32
//void help_command()
void GpProgram::HelpCommand()
{
	HWND parent;
	Shift();
	parent = GetDesktopWindow();
	/* open help file if necessary */
	help_window = HtmlHelp(parent, winhelpname, HH_GET_WIN_HANDLE, (DWORD_PTR)NULL);
	if(help_window == NULL) {
		help_window = HtmlHelp(parent, winhelpname, HH_DISPLAY_TOPIC, (DWORD_PTR)NULL);
		if(help_window == NULL) {
			fprintf(stderr, "Error: Could not open help file \"" TCHARFMT "\"\n", winhelpname);
			return;
		}
	}
	if(EndOfCommand()) {
		// show table of contents 
		HtmlHelp(parent, winhelpname, HH_DISPLAY_TOC, (DWORD_PTR)NULL);
	}
	else {
		// lookup topic in index 
		HH_AKLINK link;
		char buf[128];
#ifdef UNICODE
		WCHAR wbuf[128];
#endif
		int start = GetCurTokenIdx();
		while(!(EndOfCommand()))
			Shift();
		GPO.Pgm.Capture(buf, start, GetPrevTokenIdx(), sizeof(buf));
		link.cbStruct =     sizeof(HH_AKLINK);
		link.fReserved =    FALSE;
#ifdef UNICODE
		MultiByteToWideChar(WinGetCodepage(encoding), 0, buf, sizeof(buf), wbuf, sizeof(wbuf) / sizeof(WCHAR));
		link.pszKeywords =  wbuf;
#else
		link.pszKeywords =  buf;
#endif
		link.pszUrl =       NULL;
		link.pszMsgText =   NULL;
		link.pszMsgTitle =  NULL;
		link.pszWindow =    NULL;
		link.fIndexOnFail = TRUE;
		HtmlHelp(parent, winhelpname, HH_KEYWORD_LOOKUP, (DWORD_PTR)&link);
	}
}
#else  /* !_WIN32 */
#ifndef VMS
//void help_command()
void GpProgram::HelpCommand()
{
	while(!(EndOfCommand()))
		Shift();
	fputs("This gnuplot was not built with inline help\n", stderr);
}
#endif /* VMS */
#endif /* _WIN32 */
#endif /* NO_GIH */
/*
 * help_command: (not VMS, although it would work) Give help to the user. It
 * parses the command line into helpbuf and supplies help for that string.
 * Then, if there are subtopics available for that key, it prompts the user
 * with this string. If more input is given, help_command is called
 * recursively, with argument 0.  Thus a more specific help can be supplied.
 * This can be done repeatedly.  If null input is given, the function returns,
 * effecting a backward climb up the tree.
 * David Kotz (David.Kotz@Dartmouth.edu) 10/89
 * drd - The help buffer is first cleared when called with toplevel=1.
 * This is to fix a bug where help is broken if ^C is pressed whilst in the
 * help.
 * Lars - The "int toplevel" argument is gone. I have converted it to a
 * static variable.
 *
 * FIXME - helpbuf is never SAlloc::F()'d
 */
#ifndef NO_GIH
//void help_command()
void GpProgram::HelpCommand()
{
	static char * helpbuf = NULL;
	static char * prompt = NULL;
	static int toplevel = 1;
	int base;               /* index of first char AFTER help string */
	int len;                /* length of current help string */
	bool more_help;
	bool only;          /* TRUE if only printing subtopics */
	bool subtopics;     /* 0 if no subtopics for this topic */
	int start;              /* starting token of help string */
	char * help_ptr;        /* name of help file */
#if defined(SHELFIND)
	static char help_fname[256] = ""; /* keep helpfilename across calls */
#endif
	if((help_ptr = getenv("GNUHELP")) == (char *)NULL)
#ifndef SHELFIND
		// if can't find environment variable then just use HELPFILE 
		help_ptr = HELPFILE;
#else
		// try whether we can find the helpfile via shell_find. If not, just use the default. (tnx Andreas) 
		if(!strchr(HELPFILE, ':') && !strchr(HELPFILE, '/') && !strchr(HELPFILE, '\\')) {
			if(strlen(help_fname) == 0) {
				strcpy(help_fname, HELPFILE);
				if(shel_find(help_fname) == 0) {
					strcpy(help_fname, HELPFILE);
				}
			}
			help_ptr = help_fname;
		}
		else {
			help_ptr = HELPFILE;
		}
#endif
	// Since MSDOS DGROUP segment is being overflowed we can not allow such  
	// huge static variables (1k each). Instead we dynamically allocate them 
	// on the first call to this function...                                 
	if(helpbuf == NULL) {
		helpbuf = (char *)gp_alloc(MAX_LINE_LEN, "help buffer");
		prompt = (char *)gp_alloc(MAX_LINE_LEN, "help prompt");
		helpbuf[0] = prompt[0] = 0;
	}
	if(toplevel)
		helpbuf[0] = prompt[0] = 0; /* in case user hit ^c last time */
	/* if called recursively, toplevel == 0; toplevel must == 1 if called
	 * from command() to get the same behaviour as before when toplevel
	 * supplied as function argument
	 */
	toplevel = 1;
	len = base = strlen(helpbuf);
	Shift();
	start = GetCurTokenIdx();
	// find the end of the help command 
	while(!(EndOfCommand()))
		Shift();
	// copy new help input into helpbuf 
	if(len > 0)
		helpbuf[len++] = ' '; /* add a space */
	Capture(helpbuf + len, start, GetPrevTokenIdx(), MAX_LINE_LEN - len);
	squash_spaces(helpbuf + base, 1); /* only bother with new stuff */
	len = strlen(helpbuf);
	/* now, a lone ? will print subtopics only */
	if(strcmp(helpbuf + (base ? base + 1 : 0), "?") == 0) {
		// subtopics only 
		subtopics = 1;
		only = true;
		helpbuf[base] = NUL; /* cut off question mark */
	}
	else {
		// normal help request 
		subtopics = 0;
		only = false;
	}
	switch(help(helpbuf, help_ptr, &subtopics)) {
		case H_FOUND: {
		    // already printed the help info 
		    // subtopics now is true if there were any subtopics 
		    screen_ok = false;
		    do {
			    if(subtopics && !only) {
				    /* prompt for subtopic with current help string */
				    if(len > 0) {
					    strcpy(prompt, "Subtopic of ");
					    strncat(prompt, helpbuf, MAX_LINE_LEN - 16);
					    strcat(prompt, ": ");
				    }
				    else
					    strcpy(prompt, "Help topic: ");
				    ReadLine(prompt, 0);
				    NumTokens = Scanner(&gp_input_line, &gp_input_line_len);
				    SetTokenIdx(0);
				    more_help = !EndOfCommand();
				    if(more_help) {
					    Rollback();
					    toplevel = 0;
					    // base for next level is all of current helpbuf 
					    HelpCommand();
				    }
			    }
			    else
				    more_help = FALSE;
		    } while(more_help);
		    break;
	    }
		case H_NOTFOUND:
		    printf("Sorry, no help for '%s'\n", helpbuf);
		    break;
		case H_ERROR:
		    perror(help_ptr);
		    break;
		default:
		    GPO.IntError(NO_CARET, "Impossible case in switch");
		    break;
	}
	helpbuf[base] = NUL;    /* cut it off where we started */
}

#endif /* !NO_GIH */

#ifndef VMS

static void do_system(const char * cmd)
{
	// (am, 19980929)
 	// OS/2 related note: cmd.exe returns 255 if called w/o argument.
	// i.e. calling a shell by "!" will always end with an error message.
	// A workaround has to include checking for EMX,OS/2, two environment variables,...
	if(cmd) {
		int ierr;
		restrict_popen();
#if defined(_WIN32) && !defined(WGP_CONSOLE)
		WinOpenConsole(); // Open a console so we can see the command's output 
#endif
#if defined(_WIN32) && !defined(HAVE_BROKEN_WSYSTEM)
		{
			LPWSTR wcmd = UnicodeText(cmd, encoding);
			ierr = _wsystem(wcmd);
			SAlloc::F(wcmd);
		}
#else
		ierr = system(cmd);
#endif
		GPO.ReportError(ierr);
	}
}
// 
// is_history_command:
// Test if line starts with an (abbreviated) history command.
// Modified copy of almost_equals() (util.c).
// 
static bool is_history_command(const char * line)
{
	int i;
	int start = 0;
	int length = 0;
	int after = 0;
	const char str[] = "hi$story";
	// skip leading whitespace 
	while(isblank((uchar)line[start]))
		start++;
	// find end of "token" 
	while((line[start+length] != NUL) && !isblank((uchar)line[start + length]))
		length++;
	for(i = 0; i < length + after; i++) {
		if(str[i] != line[start + i]) {
			if(str[i] != '$')
				return FALSE;
			else {
				after = 1;
				start--; /* back up token ptr */
			}
		}
	}
	// i now beyond end of token string 
	return (after || str[i] == '$' || str[i] == NUL);
}

#ifdef USE_READLINE
static char * rlgets(char * s, size_t n, const char * prompt)
{
	static char * line = (char *)NULL;
	static int leftover = -1; /* index of 1st char leftover from last call */
	if(leftover == -1) {
		ZFREE(line); // If we already have a line, first free it 
		// so that ^C or int_error during readline() does not result in line being free-ed twice 
		line = readline((interactive) ? prompt : "");
		leftover = 0;
		/* If it's not an EOF */
		if(line && *line) {
#if defined(READLINE) || defined(HAVE_LIBREADLINE)
			int found;
			/* Initialize readline history functions */
			using_history();
			/* search in the history for entries containing line.
			 * They may have other tokens before and after line, hence
			 * the check on strcmp below. */
			if(!is_history_command(line)) {
				if(!history_full) {
					found = history_search(line, -1);
					if(found != -1 && !strcmp(current_history()->line, line)) {
						/* this line is already in the history, remove the earlier entry */
						HIST_ENTRY * removed = remove_history(where_history());
						/* according to history docs we are supposed to free the stuff */
						if(removed) {
							SAlloc::F(removed->line);
							SAlloc::F(removed->data);
							SAlloc::F(removed);
						}
					}
				}
				add_history(line);
			}
#elif defined(HAVE_LIBEDITLINE)
			if(!is_history_command(line)) {
				/* deleting history entries does not work, so suppress adjacent duplicates only */
				int found = 0;
				using_history();

				if(!history_full)
					found = history_search(line, -1);
				if(found <= 0)
					add_history(line);
			}
#endif
		}
	}
	if(line) {
		/* s will be NUL-terminated here */
		safe_strncpy(s, line + leftover, n);
		leftover += strlen(s);
		if(line[leftover] == NUL)
			leftover = -1;
		return s;
	}
	return NULL;
}

#endif                         /* USE_READLINE */

#if defined(MSDOS) || defined(_WIN32)

void do_shell()
{
	screen_ok = false;
	GPO.Pgm.Shift();
	if(user_shell) {
#if defined(_WIN32)
		if(WinExec(user_shell, SW_SHOWNORMAL) <= 32)
#elif defined(__DJGPP__)
		if(system(user_shell) == -1)
#else
		if(spawnl(P_WAIT, user_shell, NULL) == -1)
#endif
			os_error(NO_CARET, "unable to spawn shell");
	}
}
#else
/* plain old Unix */

#define EXEC "exec "
void do_shell()
{
	static char exec[100] = EXEC;
	screen_ok = FALSE;
	GPO.Pgm.Shift();
	if(user_shell) {
		if(system(safe_strncpy(&exec[sizeof(EXEC) - 1], user_shell,
		    sizeof(exec) - sizeof(EXEC) - 1)))
			os_error(NO_CARET, "system() failed");
	}
	putc('\n', stderr);
}
#endif                         /* !MSDOS */

/* read from stdin, everything except VMS */

#ifndef USE_READLINE
	#define PUT_STRING(s) fputs(s, stderr)
	#define GET_STRING(s, l) fgets(s, l, stdin)
#endif                         /* !USE_READLINE */

/* this function is called for non-interactive operation. Its usage is
 * like fgets(), but additionally it checks for ipc events from the
 * terminals waitforinput() (if USE_MOUSE, and terminal is
 * mouseable). This function will be used when reading from a pipe.
 * fgets() reads in at most one less than size characters from stream and
 * stores them into the buffer pointed to by s.
 * Reading stops after an EOF or a newline.  If a newline is read, it is
 * stored into the buffer.  A '\0' is stored  after the last character in
 * the buffer. */
static char* fgets_ipc(char * dest/* string to fill */, int len/* size of it */)
{
#ifdef USE_MOUSE
	if(term && term->waitforinput) {
		// This a mouseable terminal --- must expect input from it 
		int    c; // char gotten from waitforinput() 
		int    i = 0; // position inside dest 
		dest[0] = '\0';
		for(i = 0; i < len-1; i++) {
			c = term->waitforinput(0);
			if('\n' == c) {
				dest[i] = '\n';
				i++;
				break;
			}
			else if(EOF == c) {
				dest[i] = '\0';
				return (char *)0;
			}
			else {
				dest[i] = c;
			}
		}
		dest[i] = '\0';
		return dest;
	}
	else
#endif
	return fgets(dest, len, stdin);
}

/* get a line from stdin, and display a prompt if interactive */
static char* gp_get_string(char * buffer, size_t len, const char * prompt)
{
#ifdef USE_READLINE
	if(interactive)
		return rlgets(buffer, len, prompt);
	else
		return fgets_ipc(buffer, len);
#else
	if(interactive)
		PUT_STRING(prompt);

	return GET_STRING(buffer, len);
#endif
}
//
// Non-VMS version 
//
//static int read_line(const char * prompt, int start)
int GpProgram::ReadLine(const char * pPrompt, int start)
{
	bool more = FALSE;
	int last = 0;
	current_prompt = pPrompt;
	// Once we start to read a new line, the tokens pointing into the old
	// line are no longer valid.  We used to _not_ clear things here, but
	// that lead to errors when a mouse-triggered replot request came in
	// while a new line was being read.   Bug 3602388 Feb 2013.
	if(start == 0) {
		SetTokenIdx(0);
		NumTokens = 0;
		gp_input_line[0] = '\0';
	}
	do {
		// grab some input 
		if(gp_get_string(gp_input_line + start, gp_input_line_len - start, ((more) ? ">" : pPrompt)) == (char *)NULL) {
			// end-of-file 
			if(interactive)
				putc('\n', stderr);
			gp_input_line[start] = NUL;
			inline_num++;
			if(start > 0 && CurlyBraceCount == 0) // don't quit yet - process what we have 
				more = FALSE;
			else
				return 1; // exit gnuplot 
		}
		else {
			// normal line input 
			// gp_input_line must be NUL-terminated for strlen not to pass the
			// the bounds of this array 
			last = strlen(gp_input_line) - 1;
			if(last >= 0) {
				if(gp_input_line[last] == '\n') { /* remove any newline */
					gp_input_line[last] = NUL;
					if(last > 0 && gp_input_line[last-1] == '\r')
						gp_input_line[--last] = NUL;
					// Watch out that we don't backup beyond 0 (1-1-1) 
					if(last > 0)
						--last;
				}
				else if((last + 2) >= static_cast<int>(gp_input_line_len)) {
					extend_input_line();
					// read rest of line, don't print "> " 
					start = last + 1;
					more = TRUE;
					continue;
					// else fall through to continuation handling 
				} // if(grow buffer?) 
				if(gp_input_line[last] == '\\') {
					// line continuation 
					start = last;
					more = TRUE;
				}
				else
					more = FALSE;
			}
			else
				more = FALSE;
		}
	} while(more);
	return 0;
}

#endif /* !VMS */

/*
 * Walk through the input line looking for string variables preceded by @.
 * Replace the characters @<varname> with the contents of the string.
 * Anything inside quotes is not expanded.
 * Allow up to 3 levels of nested macros.
 */
void string_expand_macros()
{
	if(expand_1level_macros() && expand_1level_macros() && expand_1level_macros() && expand_1level_macros())
		GPO.IntError(NO_CARET, "Macros nested too deeply");
}

#define COPY_CHAR do {gp_input_line[o++] = *c; \
		      after_backslash = FALSE; } while(0)

int expand_1level_macros()
{
	bool in_squote = FALSE;
	bool in_dquote = FALSE;
	bool after_backslash = FALSE;
	bool in_comment = FALSE;
	int len;
	int o = 0;
	int nfound = 0;
	char * c;
	char * temp_string;
	char temp_char;
	char * m;
	struct udvt_entry * udv;
	/* Most lines have no macros */
	if(!strchr(gp_input_line, '@'))
		return 0;
	temp_string = (char *)gp_alloc(gp_input_line_len, "string variable");
	len = strlen(gp_input_line);
	if(len >= static_cast<int>(gp_input_line_len)) 
		len = gp_input_line_len-1;
	strncpy(temp_string, gp_input_line, len);
	temp_string[len] = '\0';
	for(c = temp_string; len && c && *c; c++, len--) {
		switch(*c) {
			case '@': // The only tricky bit 
			    if(!in_squote && !in_dquote && !in_comment && isalpha((uchar)c[1])) {
				    // Isolate the udv key as a null-terminated substring 
				    m = ++c;
				    while(isalnum((uchar)*c) || (*c=='_')) c++;
				    temp_char = *c; *c = '\0';
				    // Look up the key and restore the original following char 
				    udv = GPO.Ev.GetUdvByName(m);
				    if(udv && udv->udv_value.type == STRING) {
					    nfound++;
					    m = udv->udv_value.v.string_val;
					    FPRINTF((stderr, "Replacing @%s with \"%s\"\n", udv->udv_name, m));
					    while(strlen(m) + o + len > gp_input_line_len)
						    extend_input_line();
					    while(*m)
						    gp_input_line[o++] = (*m++);
				    }
				    else {
					    gp_input_line[o] = '\0';
					    GPO.IntWarn(NO_CARET, "%s is not a string variable", m);
				    }
				    *c-- = temp_char;
			    }
			    else
				    COPY_CHAR;
			    break;

			case '"':
			    if(!after_backslash)
				    in_dquote = !in_dquote;
			    COPY_CHAR; break;
			case '\'':
			    in_squote = !in_squote;
			    COPY_CHAR; break;
			case '\\':
			    if(in_dquote)
				    after_backslash = !after_backslash;
			    gp_input_line[o++] = *c; break;
			case '#':
			    if(!in_squote && !in_dquote)
				    in_comment = TRUE;
			default:
			    COPY_CHAR; break;
		}
	}
	gp_input_line[o] = '\0';
	SAlloc::F(temp_string);
	if(nfound)
		FPRINTF((stderr, "After string substitution command line is:\n\t%s\n", gp_input_line));
	return(nfound);
}

#define MAX_TOTAL_LINE_LEN (1024 * MAX_LINE_LEN) // much more than what can be useful 

int do_system_func(const char * cmd, char ** output)
{
#if defined(VMS) || defined(PIPES)
	int c;
	FILE * f;
	int result_allocated, result_pos;
	char* result;
	int ierr = 0;
	// open stream 
	restrict_popen();
	if((f = popen(cmd, "r")) == NULL)
		os_error(NO_CARET, "popen failed");
	// get output 
	result_pos = 0;
	result_allocated = MAX_LINE_LEN;
	result = (char *)gp_alloc(MAX_LINE_LEN, "do_system_func");
	result[0] = NUL;
	while(1) {
		if((c = getc(f)) == EOF)
			break;
		/* result <- c */
		result[result_pos++] = c;
		if(result_pos == result_allocated) {
			if(result_pos >= MAX_TOTAL_LINE_LEN) {
				result_pos--;
				GPO.IntWarn(NO_CARET, "*very* long system call output has been truncated");
				break;
			}
			else {
				result = (char *)gp_realloc(result, result_allocated + MAX_LINE_LEN, "extend in do_system_func");
				result_allocated += MAX_LINE_LEN;
			}
		}
	}
	result[result_pos] = NUL;
	// close stream 
	ierr = pclose(f);
	ierr = GPO.ReportError(ierr);
	result = (char *)gp_realloc(result, strlen(result)+1, "do_system_func");
	*output = result;
	return ierr;
#else /* VMS || PIPES */
	GPO.IntWarn(NO_CARET, "system() requires support for pipes");
	*output = gp_strdup("");
	return 0;
#endif /* VMS || PIPES */
}

//static int report_error(int ierr)
int GnuPlot::ReportError(int ierr)
{
	int reported_error;
	// FIXME:  This does not seem to report all reasonable errors correctly 
	if(ierr == -1 && errno != 0)
		reported_error = errno;
	else
		reported_error = WEXITSTATUS(ierr);
	Ev.FillGpValInteger("GPVAL_SYSTEM_ERRNO", reported_error);
	if(reported_error == 127)
		Ev.FillGpValString("GPVAL_SYSTEM_ERRMSG", "command not found or shell failed");
	else
		Ev.FillGpValString("GPVAL_SYSTEM_ERRMSG", strerror(reported_error));
	return reported_error;
}
