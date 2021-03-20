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
static int    changedir(char * path);
//static char * fgets_ipc(char* dest, int len);
//static char * gp_get_string(char *, size_t, const char *);
//static void   do_system(const char *);
//static int    expand_1level_macros();

//char * gp_input_line;
//size_t gp_input_line_len;
//int    inline_num; // input line number 
//udft_entry * dummy_func;
//char * replot_line = NULL; // support for replot command 
//int    plot_token = 0; // start of 'plot' command 
//bool   replot_disabled = false; // flag to disable `replot` when some data are sent through stdin; used by mouse/hotkey capable terminals 
//FILE * print_out = NULL; // output file for the print command 
//udvt_entry * print_out_var = NULL;
//char * print_out_name = NULL;
//bool   if_open_for_else = false;
//static int clause_depth = 0;
// support for 'break' and 'continue' commands 
//static int  iteration_depth = 0;
//static bool requested_break = false;
//static bool requested_continue = false;
//static int  command_exit_requested = 0; // set when an "exit" command is encountered 
//
// support for dynamic size of input line 
//
//void extend_input_line()
void GnuPlot::ExtendInputLine()
{
	if(Pgm.InputLineLen == 0) {
		// first time 
		Pgm.P_InputLine = (char *)SAlloc::M(MAX_LINE_LEN);
		Pgm.InputLineLen = MAX_LINE_LEN;
		Pgm.P_InputLine[0] = NUL;
	}
	else {
		Pgm.P_InputLine = (char *)SAlloc::R(Pgm.P_InputLine, Pgm.InputLineLen + MAX_LINE_LEN);
		Pgm.InputLineLen += MAX_LINE_LEN;
		FPRINTF((stderr, "extending input line to %d chars\n", Pgm.InputLineLen));
	}
}

#define MAX_TOKENS 400 // constant by which token table grows 

//void extend_token_table()
void GpProgram::ExtendTokenTable()
{
	if(TokenTableSize == 0) {
		// first time 
		P_Token = (GpLexicalUnit *)SAlloc::M(MAX_TOKENS * sizeof(GpLexicalUnit));
		TokenTableSize = MAX_TOKENS;
		// HBB: for checker-runs: 
		memzero(P_Token, MAX_TOKENS * sizeof(*P_Token));
	}
	else {
		P_Token = (GpLexicalUnit *)SAlloc::R(P_Token, (TokenTableSize + MAX_TOKENS) * sizeof(GpLexicalUnit));
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
		TermCheckMultiplotOkay(_Plt.interactive); // calls IntError() if it is not happy 
		p_prompt = "multiplot> ";
	}
	if(ReadLine(p_prompt, 0))
		return 1;
	else {
		// So we can flag any new output: if false at time of error,
		// we reprint the command line before printing caret.
		// TRUE for interactive terminals, since the command line is typed.
		// FALSE for non-terminal stdin, so command line is printed anyway.
		// (DFK 11/89)
		GpU.screen_ok = _Plt.interactive;
		return BIN(DoLine());
	}
}

//int do_line()
int GnuPlot::DoLine()
{
	// Line continuation has already been handled by read_line() 
	StringExpandMacros(); // Expand any string variables in the current input line 
	// Skip leading whitespace 
	char * inlptr = Pgm.P_InputLine;
	while(isspace((uchar)*inlptr))
		inlptr++;
	// Leading '!' indicates a shell command that bypasses normal gnuplot
	// tokenization and parsing.  This doesn't work inside a bracketed clause.
	if(is_system(*inlptr)) {
		DoSystem(inlptr + 1);
		return 0;
	}
	else {
		// Strip off trailing comment 
		FPRINTF((stderr, "doline( \"%s\" )\n", Pgm.P_InputLine));
		if(strchr(inlptr, '#')) {
			Pgm.NumTokens = Scanner(&Pgm.P_InputLine, &Pgm.InputLineLen);
			if(Pgm.P_InputLine[Pgm.P_Token[Pgm.NumTokens].StartIdx] == '#')
				Pgm.P_InputLine[Pgm.P_Token[Pgm.NumTokens].StartIdx] = NUL;
		}
		if(inlptr != Pgm.P_InputLine) {
			// If there was leading whitespace, copy the actual
			// command string to the front. use memmove() because
			// source and target may overlap 
			memmove(Pgm.P_InputLine, inlptr, strlen(inlptr));
			Pgm.P_InputLine[strlen(inlptr)] = NUL; // Terminate resulting string 
		}
		FPRINTF((stderr, "  echo: \"%s\"\n", Pgm.P_InputLine));
		Pgm.NumTokens = Scanner(&Pgm.P_InputLine, &Pgm.InputLineLen);
		// 
		// Expand line if necessary to contain a complete bracketed clause {...}
		// Insert a ';' after current line and append the next input line.
		// NB: This may leave an "else" condition on the next line.
		// 
		if(Pgm.CurlyBraceCount < 0)
			IntError(NO_CARET, "Unexpected }");
		while(Pgm.CurlyBraceCount > 0) {
			if(P_LfHead && P_LfHead->depth > 0) {
				// This catches the case that we are inside a "load foo" operation
				// and therefore requesting interactive input is not an option.
				// FIXME: or is it?
				IntError(NO_CARET, "Syntax error: missing block terminator }");
			}
			else if(_Plt.interactive || _Plt.noinputfiles) {
				// If we are really in interactive mode and there are unterminated blocks,
				// then we want to display a "more>" prompt to get the rest of the block.
				// However, there are two more cases that must be dealt here:
				// One is when commands are piped to gnuplot - on the command line,
				// the other is when commands are piped to gnuplot which is opened
				// as a slave process. The test for noinputfiles is for the latter case.
				// If we didn't have that test here, unterminated blocks sent via a pipe
				// would trigger the error message in the else branch below. 
				int retval;
				strcat(Pgm.P_InputLine, ";");
				retval = ReadLine("more> ", strlen(Pgm.P_InputLine));
				if(retval)
					IntError(NO_CARET, "Syntax error: missing block terminator }");
				// Expand any string variables in the current input line 
				StringExpandMacros();
				Pgm.NumTokens = Scanner(&Pgm.P_InputLine, &Pgm.InputLineLen);
				if(Pgm.P_InputLine[Pgm.P_Token[Pgm.NumTokens].StartIdx] == '#')
					Pgm.P_InputLine[Pgm.P_Token[Pgm.NumTokens].StartIdx] = NUL;
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
			if(Pgm.command_exit_requested) {
				Pgm.command_exit_requested = 0; /* yes this is necessary */
				return 1;
			}
			if(IterationEarlyExit()) {
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
		CheckForMouseEvents(term); // This check allows event handling inside load/eval/while statements 
		return 0;
	}
}

//void do_string(const char * s)
void GnuPlot::DoString(const char * s)
{
	char * cmdline = sstrdup(s);
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
	while(Pgm.InputLineLen < strlen(cmdline) + 1)
		ExtendInputLine();
	strcpy(Pgm.P_InputLine, cmdline);
	GpU.screen_ok = FALSE;
	Pgm.command_exit_requested = DoLine();
	// 
	// "exit" is supposed to take us out of the current file from a
	// "load <file>" command.  But the LFS stack holds both files and
	// bracketed clauses, so we have to keep popping until we hit an actual file.
	// 
	if(Pgm.command_exit_requested) {
		while(P_LfHead && !P_LfHead->name) {
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
	if(Gg.VolatileData && Gg.refresh_ok != E_REFRESH_NOT_OK) {
		if(display_ipc_commands())
			fprintf(stderr, "refresh\n");
		RefreshRequest(pTerm);
	}
	else if(!Pgm.replot_disabled)
		ReplotRequest(pTerm);
	else
		IntWarn(NO_CARET, "refresh not possible and replot is disabled");
}

//void restore_prompt()
void GnuPlot::RestorePrompt()
{
	if(_Plt.interactive) {
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
		memcpy(save_dummy, _Pb.c_dummy_var, sizeof(save_dummy));
		start_token = Pgm.GetCurTokenIdx();
		do {
			// skip to the next dummy 
			Pgm.Shift();
			Pgm.Shift();
			Pgm.CopyStr(_Pb.c_dummy_var[dummy_num++], Pgm.GetCurTokenIdx(), MAX_ID_LEN);
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
		udf = Pgm.dummy_func = AddUdf(start_token);
		udf->dummy_num = dummy_num;
		if((at_tmp = PermAt()) == (struct at_type *)NULL)
			IntError(start_token, "not enough memory for function");
		if(udf->at)     /* already a dynamic a.t. there */
			free_at(udf->at); /* so free it first */
		udf->at = at_tmp; /* before re-assigning it. */
		memcpy(_Pb.c_dummy_var, save_dummy, sizeof(save_dummy));
		Pgm.MCapture(&(udf->definition), start_token, Pgm.GetPrevTokenIdx());
		Pgm.dummy_func = NULL; // dont let anyone else use our workspace 
		// Save function definition in a user-accessible variable 
		tmpnam = (char *)SAlloc::M(8+strlen(udf->udf_name));
		strcpy(tmpnam, "GPFUN_");
		strcat(tmpnam, udf->udf_name);
		Ev.FillGpValString(tmpnam, udf->definition);
		SAlloc::F(tmpnam);
	}
	else {
		// variable ! 
		const char * varname = Pgm.P_InputLine + Pgm.ÑTok().StartIdx;
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
			DelUdvByName(key, wildcard);
		Pgm.Shift();
	}
}

//static void command()
void GnuPlot::Command()
{
	for(int i = 0; i < MAX_NUM_VAR; i++)
		_Pb.c_dummy_var[i][0] = NUL; /* no dummy variables */
	if(IsDefinition(Pgm.GetCurTokenIdx()))
		Define();
	else if(IsArrayAssignment())
		;
	else {
		//(*lookup_ftable(&command_ftbl[0], GetCurTokenIdx()))();
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
				HelpCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "?"))
				HelpCommand();
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
				PauseCommand(term);
			else if(Pgm.AlmostEquals(cur_tok_idx, "p$lot"))
				PlotCommand(term);
			else if(Pgm.AlmostEquals(cur_tok_idx, "pr$int"))
				PrintCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "printerr$or"))
				PrintErrCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "pwd"))
				PwdCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "q$uit"))
				ExitCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "ref$resh"))
				RefreshCommand(term);
			else if(Pgm.AlmostEquals(cur_tok_idx, "rep$lot"))
				ReplotCommand(term);
			else if(Pgm.AlmostEquals(cur_tok_idx, "re$read"))
				RereadCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "res$et"))
				ResetCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "sa$ve"))
				SaveCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "scr$eendump"))
				ScreenDumpCommand(term);
			else if(Pgm.AlmostEquals(cur_tok_idx, "se$t"))
				SetCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "she$ll"))
				DoShell();
			else if(Pgm.AlmostEquals(cur_tok_idx, "sh$ow"))
				ShowCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "sp$lot"))
				SPlotCommand(term);
			else if(Pgm.AlmostEquals(cur_tok_idx, "st$ats"))
				StatsCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "sy$stem"))
				SystemCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "test"))
				TestCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "tog$gle"))
				ToggleCommand(term);
			else if(Pgm.AlmostEquals(cur_tok_idx, "und$efine"))
				UndefineCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "uns$et"))
				UnsetCommand();
			else if(Pgm.AlmostEquals(cur_tok_idx, "up$date"))
				UpdateCommand();
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
	array->udv_value.v.value_array = (GpValue *)SAlloc::M((nsize+1) * sizeof(GpValue));
	array->udv_value.Type = ARRAY;
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
			A[0].Type = COLORMAP_ARRAY;
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
			if(udv->udv_value.Type != ARRAY)
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
		; // @fallthrough 
	}
	else if(IsStringValue(Pgm.GetCurTokenIdx()) && (lhs = TryToGetString())) {
		FPRINTF((stderr, "Got bind quoted lhs = \"%s\"\n", lhs));
	}
	else {
		char * first = Pgm.P_InputLine + Pgm.ÑTok().StartIdx;
		int size = strcspn(first, " \";");
		lhs = (char *)SAlloc::M(size+1);
		strnzcpy(lhs, first, size+1);
		FPRINTF((stderr, "Got bind unquoted lhs = \"%s\"\n", lhs));
		while(Pgm.P_InputLine + Pgm.ÑTok().StartIdx < first+size)
			Pgm.Shift();
	}
	// 
	// get right hand side: the command to bind either (1) quoted command or (2) the rest of the line
	// 
	if(Pgm.EndOfCommand()) {
		; // @fallthrough 
	}
	else if(IsStringValue(Pgm.GetCurTokenIdx()) && (rhs = TryToGetString())) {
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

//bool iteration_early_exit()
bool GnuPlot::IterationEarlyExit()
{
	return (Pgm.requested_continue || Pgm.requested_break);
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
	GpExpandTilde(&save_file);
	// Argument list follows filename 
	LoadFile(LoadPath_fopen(save_file, "r"), save_file, 2);
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
	GpExpandTilde(&save_file);
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
	GpU.screen_ok = FALSE;
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
	Pgm.command_exit_requested = 1;
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

#if 0 // {
#define REPLACE_ELSE(tok)             \
	do {                                  \
		int idx = token[tok].StartIdx; \
		token[tok].Len = 1;            \
		gp_input_line[idx ++] = ';'; /* e */  \
		gp_input_line[idx ++] = ' '; /* l */  \
		gp_input_line[idx ++] = ' '; /* s */  \
		gp_input_line[idx ++] = ' '; /* e */  \
	} while(0)
#endif // } 0
//
// Make a copy of an input line substring delimited by { and } 
//
//static char * new_clause(int clauseStart, int clauseEnd)
char * GnuPlot::NewClause(int clauseStart, int clauseEnd)
{
	char * p_clause = (char *)SAlloc::M(clauseEnd - clauseStart);
	memcpy(p_clause, &Pgm.P_InputLine[clauseStart+1], clauseEnd - clauseStart);
	p_clause[clauseEnd - clauseStart-1] = '\0';
	return p_clause;
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
			if(Real(&condition) == 0) {
				if_state = IF_FALSE;
				Pgm.SetTokenIdx(next_token);
			}
			else {
				if_state = IF_TRUE;
				char * clause = NewClause(clause_start, clause_end);
				BeginClause();
				DoStringAndFree(clause);
				EndClause();
				if(IterationEarlyExit())
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
				clause = NewClause(clause_start, clause_end);
				BeginClause();
				DoStringAndFree(clause);
				EndClause();
				if(IterationEarlyExit())
					Pgm.SetTokenIdx(Pgm.NumTokens);
				else
					Pgm.SetTokenIdx(next_token);
			}
			Pgm.if_open_for_else = FALSE;
		}
		else {
			IntErrorCurToken("expecting bracketed else clause");
		}
	}
	else {
		// There was no "else" on this line but we might see it on another line 
		Pgm.if_open_for_else = !(if_state == IF_TRUE);
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
	IfElseCommand(Pgm.if_open_for_else ? IF_FALSE : IF_TRUE);
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
	if(Pgm.clause_depth > 0)
		IntErrorCurToken("Old-style if/else statement encountered inside brackets");
	EvaluateAt(expr, &condition);
	// find start and end of the "if" part 
	if_start = &Pgm.P_InputLine[Pgm.ÑTok().StartIdx ];
	while((Pgm.CToken < Pgm.NumTokens) && !Pgm.EqualsCur("else"))
		Pgm.Shift();
	if(Pgm.EqualsCur("else")) {
		if_end = &Pgm.P_InputLine[Pgm.ÑTok().StartIdx-1];
		*if_end = '\0';
		else_start = &Pgm.P_InputLine[Pgm.ÑTok().StartIdx + Pgm.ÑTok().Len];
	}
	if(Real(&condition) != 0.0)
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
	clause = NewClause(do_start, do_end);
	BeginClause();
	Pgm.iteration_depth++;
	// Sometimes the start point of a nested iteration is not within the
	// limits for all levels of nesting. In this case we need to advance
	// through the iteration to find the first good set of indices.
	// If we don't find one, forget the whole thing.
	if(empty_iteration(do_iterator) && !NextIteration(do_iterator)) {
		strcpy(clause, ";");
	}
	do {
		Pgm.requested_continue = false;
		DoString(clause);
		if(Pgm.command_exit_requested != 0)
			Pgm.requested_break = true;
		if(Pgm.requested_break)
			break;
	} while(NextIteration(do_iterator));
	Pgm.iteration_depth--;
	SAlloc::F(clause);
	EndClause();
	Pgm.SetTokenIdx(end_token);
	// FIXME:  If any of the above exited via GnuPlot::IntError() then this	
	// cleanup never happens and we leak memory.  But do_iterator can	
	// not be static or global because do_command() can recurse.	
	do_iterator = cleanup_iteration(do_iterator);
	Pgm.requested_break = false;
	Pgm.requested_continue = false;
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
	clause = NewClause(do_start, do_end);
	BeginClause();
	Pgm.iteration_depth++;
	while(exprval != 0) {
		Pgm.requested_continue = FALSE;
		DoString(clause);
		if(Pgm.command_exit_requested)
			Pgm.requested_break = TRUE;
		if(Pgm.requested_break)
			break;
		Pgm.SetTokenIdx(save_token);
		exprval = RealExpression();
	}
	Pgm.iteration_depth--;
	EndClause();
	SAlloc::F(clause);
	Pgm.SetTokenIdx(end_token);
	Pgm.requested_break = false;
	Pgm.requested_continue = false;
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
	strcpy(_Pb.c_dummy_var[0], "x");
	strcpy(_Pb.c_dummy_var[1], "y");
	if(Pgm.EqualsCur("z") || Pgm.EqualsCur("cb"))
		strcpy(_Pb.c_dummy_var[0], "z");
	if(Pgm.EqualsCur("r"))
		strcpy(_Pb.c_dummy_var[0], "r");
	/*
	 * "set nonlinear" currently supports axes x x2 y y2 z r cb
	 */
	if(Pgm.Equals(command_token, "nonlinear")) {
		AXIS_INDEX axis;
		if((axis = (AXIS_INDEX)Pgm.LookupTableForCurrentToken(axisname_tbl)) >= 0)
			secondary_axis = &AxS[axis];
		else
			IntErrorCurToken("not a valid nonlinear axis");
		primary_axis = GetShadowAxis(secondary_axis);
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
		primary_axis->link_udf = (udft_entry *)SAlloc::M(sizeof(udft_entry));
		memzero(primary_axis->link_udf, sizeof(udft_entry));
	}
	if(!secondary_axis->link_udf) {
		secondary_axis->link_udf = (udft_entry *)SAlloc::M(sizeof(udft_entry));
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
		char * datablock_name = sstrdup(Pgm.ParseDatablockName());
		LoadFile(NULL, datablock_name, 6);
	}
	else {
		// These need to be local so that recursion works. 
		// They will eventually be freed by lf_pop(). 
		FILE * fp;
		char * save_file = TryToGetString();
		if(!save_file)
			IntErrorCurToken("expecting filename");
		GpExpandTilde(&save_file);
		fp = strcmp(save_file, "-") ? LoadPath_fopen(save_file, "r") : stdout;
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
	*pClauseStart = P_Token[CToken].StartIdx;
	for(i = ++CToken, depth = 1; i < NumTokens; i++) {
		if(Equals(i, "{"))
			depth++;
		else if(Equals(i, "}"))
			depth--;
		if(depth == 0)
			break;
	}
	*pClauseEnd = P_Token[i].StartIdx;
	return (i+1);
}

//void begin_clause()
void GnuPlot::BeginClause()
{
	Pgm.clause_depth++;
	Pgm.Shift();
}

//void end_clause()
void GnuPlot::EndClause()
{
	if(Pgm.clause_depth == 0)
		IntErrorCurToken("unexpected }");
	else
		Pgm.clause_depth--;
	Pgm.Shift();
}

//void clause_reset_after_error()
void GnuPlot::ClauseResetAfterError()
{
	if(Pgm.clause_depth)
		FPRINTF((stderr, "CLAUSE RESET after error at depth %d\n", Pgm.clause_depth));
	Pgm.clause_depth = 0;
	Pgm.iteration_depth = 0;
}
//
// helper routine to multiplex mouse event handling with a timed pause command 
//
//void timed_pause(double sleep_time)
void GnuPlot::TimedPause(GpTermEntry * pTerm, double sleepTime)
{
	if(sleepTime > 0.0) {
#if defined(HAVE_USLEEP) && defined(USE_MOUSE) && !defined(_WIN32)
		if(pTerm->waitforinput)          /* If the terminal supports it */
			while(sleepTime > 0.05) { /* we poll 20 times a second */
				usleep(50000); // Sleep for 50 msec 
				CheckForMouseEvents(pTerm);
				sleepTime -= 0.05;
			}
		usleep((useconds_t)(sleepTime * 1e6));
		CheckForMouseEvents(pTerm);
#else
		GP_SLEEP(pTerm, static_cast<uint>(sleepTime));
#endif
	}
}

/* process the 'pause' command */
#define EAT_INPUT_WITH(slurp) do {int junk = 0; do {junk = slurp;} while(junk != EOF && junk != '\n');} while(0)

//void pause_command()
void GnuPlot::PauseCommand(GpTermEntry * pTerm)
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
			paused_for_mouse = NZOR(end_condition, PAUSE_CLICK);
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
		buf = sstrdup("paused"); /* default message, used in Windows GUI pause dialog */
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
		_Plt.ctrlc_flag = FALSE;
#if defined(WGP_CONSOLE) && defined(USE_MOUSE)
		if(!paused_for_mouse || !MousableWindowOpened(pTerm)) {
			int junk = 0;
			if(buf) {
				// Use of fprintf() triggers a bug in MinGW + SJIS encoding 
				fputs(buf, stderr); fputs("\n", stderr);
			}
			// cannot use EAT_INPUT_WITH here 
			do {
				junk = getch();
				if(ctrlc_flag)
					bail_to_command_line();
			} while(junk != EOF && junk != '\n' && junk != '\r');
		}
		else /* paused_for_mouse */
#endif /* !WGP_CONSOLE */
		{
			if(!Pause(pTerm, buf)) // returns false if Ctrl-C or Cancel was pressed 
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
		TimedPause(term, sleep_time);
	if(text != 0 && sleep_time >= 0)
		fputc('\n', stderr);
	GpU.screen_ok = FALSE;
}
//
// process the 'plot' command 
//
//void plot_command()
void GnuPlot::PlotCommand(GpTermEntry * pTerm)
{
	Pgm.plot_token = Pgm.Shift();
	_Df.plotted_data_from_stdin = false;
	Gg.refresh_nplots = 0;
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

//void print_set_output(char * pName, bool datablock, bool append_p)
void GnuPlot::PrintSetOutput(char * pName, bool datablock, bool append_p)
{
	if(Pgm.print_out && Pgm.print_out != stderr && Pgm.print_out != stdout) {
#ifdef PIPES
		if(print_out_name[0] == '|') {
			if(0 > pclose(print_out))
				perror(print_out_name);
		}
		else
#endif
		if(0 > fclose(Pgm.print_out))
			perror(Pgm.print_out_name);
		Pgm.print_out = stderr;
	}
	ZFREE(Pgm.print_out_name);
	Pgm.print_out_var = NULL;
	if(!pName) {
		Pgm.print_out = stderr;
		return;
	}
	if(strcmp(pName, "-") == 0) {
		Pgm.print_out = stdout;
		return;
	}
#ifdef PIPES
	if(pName[0] == '|') {
		RestrictPOpen();
		Pgm.print_out = popen(pName + 1, "w");
		if(!Pgm.print_out)
			perror(pName);
		else
			Pgm.print_out_name = pName;
		return;
	}
#endif
	if(!datablock) {
		Pgm.print_out = fopen(pName, append_p ? "a" : "w");
		if(!Pgm.print_out) {
			perror(pName);
			return;
		}
	}
	else {
		Pgm.print_out_var = Ev.AddUdvByName(pName);
		if(!append_p)
			gpfree_datablock(&Pgm.print_out_var->udv_value);
		// If this is not an existing datablock to be appended 
		// then make it a new empty datablock 
		if(Pgm.print_out_var->udv_value.Type != DATABLOCK) {
			Pgm.print_out_var->udv_value.Destroy();
			Pgm.print_out_var->udv_value.Type = DATABLOCK;
			Pgm.print_out_var->udv_value.v.data_array = NULL;
		}
	}
	Pgm.print_out_name = pName;
}

//char * print_show_output()
char * GnuPlot::PrintShowOutput()
{
	if(Pgm.print_out_name)
		return Pgm.print_out_name;
	else if(Pgm.print_out == stdout)
		return "<stdout>";
	else if(!Pgm.print_out || Pgm.print_out == stderr || !Pgm.print_out_name)
		return "<stderr>";
	else
		return Pgm.print_out_name;
}
//
// 'printerr' is the same as 'print' except that output is always to stderr 
//
//void printerr_command()
void GnuPlot::PrintErrCommand()
{
	FILE * save_print_out = Pgm.print_out;
	Pgm.print_out = stderr;
	PrintCommand();
	Pgm.print_out = save_print_out;
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
	SETIFZ(Pgm.print_out, stderr);
	if(Pgm.print_out_var) { // print to datablock 
		dataline = (char *)SAlloc::M(size);
		*dataline = NUL;
	}
	GpU.screen_ok = FALSE;
	do {
		Pgm.Shift();
		if(Pgm.EqualsCur("$") && Pgm.IsLetter(Pgm.GetCurTokenIdx()+1) && !Pgm.Equals(Pgm.GetCurTokenIdx()+2, "[")) {
			char * datablock_name = Pgm.ParseDatablockName();
			char ** line = GetDatablock(datablock_name);
			// Printing a datablock into itself would cause infinite recursion 
			if(Pgm.print_out_var && sstreq(datablock_name, Pgm.print_out_name))
				continue;
			while(line && *line) {
				if(Pgm.print_out_var)
					append_to_datablock(&Pgm.print_out_var->udv_value, sstrdup(*line));
				else
					fprintf(Pgm.print_out, "%s\n", *line);
				line++;
			}
			continue;
		}
		if(TypeUdv(Pgm.GetCurTokenIdx()) == ARRAY && !Pgm.EqualsNext("[")) {
			udvt_entry * array = AddUdv(Pgm.CToken++);
			SaveArrayContent(Pgm.print_out, array->udv_value.v.value_array);
			continue;
		}
		ConstExpress(&a);
		if(a.Type == STRING) {
			if(dataline)
				len = strappend(&dataline, &size, len, a.v.string_val);
			else
				fputs(a.v.string_val, Pgm.print_out);
			need_space = FALSE;
		}
		else {
			if(need_space) {
				if(dataline)
					len = strappend(&dataline, &size, len, " ");
				else
					putc(' ', Pgm.print_out);
			}
			if(dataline)
				len = strappend(&dataline, &size, len, ValueToStr(&a, FALSE));
			else
				DispValue(Pgm.print_out, &a, FALSE);
			need_space = TRUE;
		}
		a.Destroy();
	} while(!Pgm.EndOfCommand() && Pgm.EqualsCur(","));
	if(dataline) {
		append_multiline_to_datablock(&Pgm.print_out_var->udv_value, dataline);
	}
	else {
		putc('\n', Pgm.print_out);
		fflush(Pgm.print_out);
	}
}
//
// process the 'pwd' command 
//
//void pwd_command()
void GnuPlot::PwdCommand()
{
	char * save_file = (char *)SAlloc::M(PATH_MAX);
	if(GP_GETCWD(save_file, PATH_MAX) == NULL)
		fprintf(stderr, "<invalid>\n");
	else
		fprintf(stderr, "%s\n", save_file);
	SAlloc::F(save_file);
	Pgm.Shift();
}
// 
// EAM April 2007
// The "refresh" command replots the previous graph without going back to read
// the original data. This allows zooming or other operations on data that was
// only transiently available in the input stream.
// 
//void refresh_command()
void GnuPlot::RefreshCommand(GpTermEntry * pTerm)
{
	Pgm.Shift();
	RefreshRequest(pTerm);
}

//void refresh_request()
void GnuPlot::RefreshRequest(GpTermEntry * pTerm)
{
	/*AXIS_INDEX*/int axis;
	if((!_Plt.P_FirstPlot && (Gg.refresh_ok == E_REFRESH_OK_2D)) || (!_Plt.first_3dplot && (Gg.refresh_ok == E_REFRESH_OK_3D)) || (!*Pgm.replot_line && (Gg.refresh_ok == E_REFRESH_NOT_OK)))
		IntError(NO_CARET, "no active plot; cannot refresh");
	if(Gg.refresh_ok == E_REFRESH_NOT_OK) {
		IntWarn(NO_CARET, "cannot refresh from this state. trying full replot");
		ReplotRequest(pTerm);
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
		if(Gg.refresh_ok == E_REFRESH_OK_2D) {
			RefreshBounds(_Plt.P_FirstPlot, Gg.refresh_nplots);
			DoPlot(pTerm, _Plt.P_FirstPlot, Gg.refresh_nplots);
			UpdateGpvalVariables(1);
		}
		else if(Gg.refresh_ok == E_REFRESH_OK_3D) {
			Refresh3DBounds(pTerm, _Plt.first_3dplot, Gg.refresh_nplots);
			Do3DPlot(pTerm, _Plt.first_3dplot, Gg.refresh_nplots, /*0*/NORMAL_REPLOT);
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
void GnuPlot::ReplotCommand(GpTermEntry * pTerm)
{
	if(!*Pgm.replot_line)
		IntErrorCurToken("no previous plot");
	if(Gg.VolatileData && Gg.refresh_ok != E_REFRESH_NOT_OK && !Pgm.replot_disabled) {
		FPRINTF((stderr, "volatile_data %d refresh_ok %d plotted_data_from_stdin %d\n", Gg.VolatileData, Gg.refresh_ok, _Df.plotted_data_from_stdin));
		RefreshCommand(pTerm);
	}
	else {
		// Disable replot for some reason; currently used by the mouse/hotkey
		// capable terminals to avoid replotting when some data come from stdin,
		// i.e. when  plotted_data_from_stdin==1  after plot "-".
		if(Pgm.replot_disabled) {
			Pgm.replot_disabled = false;
			bail_to_command_line(); /* be silent --- don't mess the screen */
		}
		if(!pTerm) // unknown terminal 
			IntErrorCurToken("use 'set term' to set terminal type first");
		Pgm.Shift();
		SET_CURSOR_WAIT;
		if(pTerm->flags & TERM_INIT_ON_REPLOT)
			pTerm->init(pTerm);
		ReplotRequest(pTerm);
		SET_CURSOR_ARROW;
	}
}
//
// process the 'reread' command 
//
//void reread_command()
void GnuPlot::RereadCommand()
{
	FILE * fp = LfTop();
	if(fp)
		rewind(fp);
	Pgm.Shift();
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
	Pgm.Shift();
	int what = Pgm.LookupTableForCurrentToken(&save_tbl[0]);
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
		RestrictPOpen();
		fp = popen(save_file+1, "w");
	}
	else
#endif
	{
		GpExpandTilde(&save_file);
#ifdef _WIN32
		fp = sstreq(save_file, "-") ? stdout : LoadPath_fopen(save_file, append ? "a" : "w");
#else
		fp = sstreq(save_file, "-") ? stdout : fopen(save_file, append ? "a" : "w");
#endif
	}
	if(!fp)
		OsError(Pgm.GetCurTokenIdx(), "Cannot open save file");
	switch(what) {
		case SAVE_FUNCS: SaveFunctions(fp); break;
		case SAVE_SET: SaveSet(fp); break;
		case SAVE_TERMINAL: SaveTerm(term, fp); break;
		case SAVE_VARS: SaveVariables(fp); break;
		case SAVE_FIT: SaveFit(fp); break;
		case SAVE_DATABLOCKS: SaveDatablocks(fp); break;
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
//void screendump_command()
void GnuPlot::ScreenDumpCommand(GpTermEntry * pTerm)
{
	Pgm.Shift();
#ifdef _WIN32
	ScreenDump(pTerm);
#else
	fputs("screendump not implemented\n", stderr);
#endif
}

/* set_command() is in set.c */
/* 'shell' command is processed by DoShell(), see below */

/* show_command() is in show.c */
//
// process the 'splot' command 
//
//void splot_command()
void GnuPlot::SPlotCommand(GpTermEntry * pTerm)
{
	Pgm.plot_token = Pgm.Shift();
	_Df.plotted_data_from_stdin = false;
	Gg.refresh_nplots = 0;
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
//void stats_command()
void GnuPlot::StatsCommand()
{
#ifdef USE_STATS
	StatsRequest();
#else
	IntError(NO_CARET, "This copy of gnuplot was not configured with support for the stats command");
#endif
}
//
// process the 'system' command 
//
//void system_command()
void GnuPlot::SystemCommand()
{
	Pgm.Shift();
	char * cmd = TryToGetString();
	DoSystem(cmd);
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
	if(datablock->udv_value.Type != NOTDEFINED)
		gpfree_datablock(&datablock->udv_value);
	datablock->udv_value.Type = DATABLOCK;
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
	save_replot_line = sstrdup(Pgm.replot_line);
	save_is_3d_plot = Gg.Is3DPlot;
	fputs(pre1, f);
	fputs(pre2, f);
	fputs(pre3, f);
	// save current gnuplot 'set' status because of the tricky sets
	// for our temporary testing plot.
	SaveSet(f);
	SavePixmaps(f);
	// execute all commands from the temporary file 
	rewind(f);
	LoadFile(f, NULL, 1); /* note: it does fclose(f) */
	// enable reset_palette() and restore replot line 
	enable_reset_palette = 1;
	SAlloc::F(Pgm.replot_line);
	Pgm.replot_line = save_replot_line;
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
	if(!Pgm.replot_line || !(*Pgm.replot_line)) {
		Pgm.MCapture(&Pgm.replot_line, save_token, Pgm.GetCurTokenIdx());
	}
}
// 
// toggle a single plot on/off from the command line (only possible for qt, wxt, x11, win)
// 
//void toggle_command()
void GnuPlot::ToggleCommand(GpTermEntry * pTerm)
{
	int plotno = -1;
	char * plottitle = NULL;
	bool foundit = false;
	Pgm.Shift();
	if(Pgm.EqualsCur("all")) {
		Pgm.Shift();
	}
	else if((plottitle = TryToGetString()) != NULL) {
		curve_points * plot;
		int last = strlen(plottitle) - 1;
		if(Gg.refresh_ok == E_REFRESH_OK_2D)
			plot = _Plt.P_FirstPlot;
		else if(Gg.refresh_ok == E_REFRESH_OK_3D)
			plot = (curve_points *)_Plt.first_3dplot;
		else
			plot = NULL;
		if(last >= 0) {
			for(plotno = 0; plot; plot = plot->next, plotno++) {
				if(plot->title)
					if(sstreq(plot->title, plottitle) || (plottitle[last] == '*' && !strncmp(plot->title, plottitle, last))) {
						foundit = TRUE;
						break;
					}
			}
		}
		SAlloc::F(plottitle);
		if(!foundit) {
			IntWarn(NO_CARET, "Did not find a plot with that title");
			return;
		}
	}
	else {
		plotno = IntExpression() - 1;
	}
	if(pTerm->modify_plots)
		pTerm->modify_plots(MODPLOTS_INVERT_VISIBILITIES, plotno);
}

//void update_command()
void GnuPlot::UpdateCommand()
{
	IntError(NO_CARET, "DEPRECATED command 'update', please use 'save fit' instead");
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
	memcpy(save_dummy, _Pb.c_dummy_var, sizeof(save_dummy));
	do {
		Pgm.Shift(); // skip to the next dummy 
		Pgm.Shift();
		Pgm.CopyStr(_Pb.c_dummy_var[dummy_num++], Pgm.GetCurTokenIdx(), MAX_ID_LEN);
	} while(Pgm.Equals(Pgm.GetCurTokenIdx()+1, ",") && (dummy_num < MAX_NUM_VAR));
	Pgm.Shift();
	if(Pgm.EqualsCur(","))
		IntError(Pgm.GetCurTokenIdx()+1, "function contains too many parameters");
	if(!Pgm.EqualsCurShift(")"))
		IntErrorCurToken("missing ')'");
	if(!Pgm.EqualsCur("from"))
		IntErrorCurToken("Expecting 'from <sharedobj>'");
	Pgm.Shift();
	udf = Pgm.dummy_func = AddUdf(start_token+1);
	udf->dummy_num = dummy_num;
	free_at(udf->at); // In case there was a previous function by this name 
	udf->at = ExternalAt(udf->udf_name);
	memcpy(_Pb.c_dummy_var, save_dummy, sizeof(save_dummy));
	Pgm.dummy_func = NULL; /* dont let anyone else use our workspace */
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
		char * rest_args = &Pgm.P_InputLine[Pgm.ÑTok().StartIdx];
		size_t replot_len = strlen(Pgm.replot_line);
		size_t rest_len = strlen(rest_args);
		/* preserve commands following 'replot ;' */
		/* move rest of input line to the start
		 * necessary because of realloc() in ExtendInputLine() */
		memmove(Pgm.P_InputLine, rest_args, rest_len+1);
		// reallocs if necessary 
		while(Pgm.InputLineLen < replot_len+rest_len+1)
			ExtendInputLine();
		// move old rest args off begin of input line to make space for replot_line 
		memmove(Pgm.P_InputLine+replot_len, Pgm.P_InputLine, rest_len+1);
		// copy previous plot command to start of input line 
		memcpy(Pgm.P_InputLine, Pgm.replot_line, replot_len);
	}
	else {
		char * replot_args = NULL; /* else m_capture will free it */
		int last_token = Pgm.NumTokens - 1;
		// length = length of old part + length of new part + ", " + \0 
		size_t newlen = strlen(Pgm.replot_line) + Pgm.P_Token[last_token].StartIdx + Pgm.P_Token[last_token].Len - Pgm.ÑTok().StartIdx + 3;
		Pgm.MCapture(&replot_args, Pgm.GetCurTokenIdx(), last_token); /* might be empty */
		while(Pgm.InputLineLen < newlen)
			ExtendInputLine();
		strcpy(Pgm.P_InputLine, Pgm.replot_line);
		strcat(Pgm.P_InputLine, ", ");
		strcat(Pgm.P_InputLine, replot_args);
		SAlloc::F(replot_args);
	}
	Pgm.plot_token = 0;         /* whole line to be saved as replot line */
	SET_REFRESH_OK(E_REFRESH_NOT_OK, 0);            /* start of replot will destroy existing data */
	GpU.screen_ok = FALSE;
	Pgm.NumTokens = Scanner(&Pgm.P_InputLine, &Pgm.InputLineLen);
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
void GnuPlot::HelpCommand()
{
	HWND parent;
	Pgm.Shift();
	parent = GetDesktopWindow();
	// open help file if necessary 
	help_window = HtmlHelp(parent, winhelpname, HH_GET_WIN_HANDLE, (DWORD_PTR)NULL);
	if(help_window == NULL) {
		help_window = HtmlHelp(parent, winhelpname, HH_DISPLAY_TOPIC, (DWORD_PTR)NULL);
		if(help_window == NULL) {
			fprintf(stderr, "Error: Could not open help file \"" TCHARFMT "\"\n", winhelpname);
			return;
		}
	}
	if(Pgm.EndOfCommand()) {
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
		int start = Pgm.GetCurTokenIdx();
		while(!Pgm.EndOfCommand())
			Pgm.Shift();
		Pgm.Capture(buf, start, Pgm.GetPrevTokenIdx(), sizeof(buf));
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
void GnuPlot::HelpCommand()
{
	while(!Pgm.EndOfCommand())
		Pgm.Shift();
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
void GnuPlot::HelpCommand()
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
#if defined(SHELFIND)
	static char help_fname[256] = ""; /* keep helpfilename across calls */
#endif
	char * help_ptr = getenv("GNUHELP"); // name of help file 
	if(!help_ptr)
#ifndef SHELFIND
		help_ptr = HELPFILE; // if can't find environment variable then just use HELPFILE 
#else
		// try whether we can find the helpfile via shell_find. If not, just use the default. (tnx Andreas) 
		if(!strchr(HELPFILE, ':') && !strchr(HELPFILE, '/') && !strchr(HELPFILE, '\\')) {
			if(strlen(help_fname) == 0) {
				strcpy(help_fname, HELPFILE);
				if(shel_find(help_fname) == 0)
					strcpy(help_fname, HELPFILE);
			}
			help_ptr = help_fname;
		}
		else
			help_ptr = HELPFILE;
#endif
	// Since MSDOS DGROUP segment is being overflowed we can not allow such  
	// huge static variables (1k each). Instead we dynamically allocate them 
	// on the first call to this function...                                 
	if(helpbuf == NULL) {
		helpbuf = (char *)SAlloc::M(MAX_LINE_LEN);
		prompt = (char *)SAlloc::M(MAX_LINE_LEN);
		helpbuf[0] = prompt[0] = 0;
	}
	if(toplevel)
		helpbuf[0] = prompt[0] = 0; /* in case user hit ^c last time */
	// if called recursively, toplevel == 0; toplevel must == 1 if called
	// from command() to get the same behaviour as before when toplevel
	// supplied as function argument
	toplevel = 1;
	len = base = strlen(helpbuf);
	Pgm.Shift();
	start = Pgm.GetCurTokenIdx();
	// find the end of the help command 
	while(!Pgm.EndOfCommand())
		Pgm.Shift();
	// copy new help input into helpbuf 
	if(len > 0)
		helpbuf[len++] = ' '; /* add a space */
	Pgm.Capture(helpbuf + len, start, Pgm.GetPrevTokenIdx(), MAX_LINE_LEN - len);
	squash_spaces(helpbuf + base, 1); /* only bother with new stuff */
	len = strlen(helpbuf);
	// now, a lone ? will print subtopics only 
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
	switch(Help(helpbuf, help_ptr, &subtopics)) {
		case H_FOUND: {
		    // already printed the help info 
		    // subtopics now is true if there were any subtopics 
		    GpU.screen_ok = false;
		    do {
			    if(subtopics && !only) {
				    // prompt for subtopic with current help string 
				    if(len > 0) {
					    strcpy(prompt, "Subtopic of ");
					    strncat(prompt, helpbuf, MAX_LINE_LEN - 16);
					    strcat(prompt, ": ");
				    }
				    else
					    strcpy(prompt, "Help topic: ");
				    ReadLine(prompt, 0);
				    Pgm.NumTokens = Scanner(&Pgm.P_InputLine, &Pgm.InputLineLen);
				    Pgm.SetTokenIdx(0);
				    more_help = !Pgm.EndOfCommand();
				    if(more_help) {
					    Pgm.Rollback();
					    toplevel = 0;
					    HelpCommand(); // base for next level is all of current helpbuf 
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
		    IntError(NO_CARET, "Impossible case in switch");
		    break;
	}
	helpbuf[base] = NUL;    /* cut it off where we started */
}

#endif /* !NO_GIH */

#ifndef VMS

//static void do_system(const char * cmd)
void GnuPlot::DoSystem(const char * cmd)
{
	// (am, 19980929)
 	// OS/2 related note: cmd.exe returns 255 if called w/o argument.
	// i.e. calling a shell by "!" will always end with an error message.
	// A workaround has to include checking for EMX,OS/2, two environment variables,...
	if(cmd) {
		int ierr;
		RestrictPOpen();
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
		ReportError(ierr);
	}
}
// 
// is_history_command:
// Test if line starts with an (abbreviated) history command.
// Modified copy of almost_equals() (util.c).
// 
static bool is_history_command(const char * pLine)
{
	int i;
	int start = 0;
	int length = 0;
	int after = 0;
	const char str[] = "hi$story";
	// skip leading whitespace 
	while(isblank((uchar)pLine[start]))
		start++;
	// find end of "token" 
	while(pLine[start+length] != NUL && !isblank((uchar)pLine[start + length]))
		length++;
	for(i = 0; i < length + after; i++) {
		if(str[i] != pLine[start + i]) {
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
//static char * rlgets(char * s, size_t n, const char * pPrompt)
char * GnuPlot::RlGets(char * s, size_t n, const char * pPrompt)
{
	static char * p_line_ = (char *)NULL;
	static int leftover = -1; /* index of 1st char leftover from last call */
	if(leftover == -1) {
		ZFREE(p_line_); // If we already have a line, first free it 
		// so that ^C or int_error during readline() does not result in line being free-ed twice 
		p_line_ = ReadLine(_Plt.interactive ? pPrompt : "");
		leftover = 0;
		// If it's not an EOF 
		if(!isempty(p_line_)) {
#if defined(READLINE) || defined(HAVE_LIBREADLINE)
			int found;
			using_history(); // Initialize readline history functions 
			// search in the history for entries containing line.
			// They may have other tokens before and after line, hence
			// the check on strcmp below. 
			if(!is_history_command(p_line_)) {
				if(!history_full) {
					found = history_search(p_line_, -1);
					if(found != -1 && sstreq(current_history()->line, p_line_)) {
						// this line is already in the history, remove the earlier entry 
						HIST_ENTRY * removed = remove_history(where_history());
						// according to history docs we are supposed to free the stuff 
						if(removed) {
							SAlloc::F(removed->line);
							SAlloc::F(removed->data);
							SAlloc::F(removed);
						}
					}
				}
				add_history(p_line_);
			}
#elif defined(HAVE_LIBEDITLINE)
			if(!is_history_command(p_line_)) {
				// deleting history entries does not work, so suppress adjacent duplicates only 
				int found = 0;
				using_history();
				if(!history_full)
					found = history_search(p_line_, -1);
				if(found <= 0)
					add_history(p_line_);
			}
#endif
		}
	}
	if(p_line_) {
		// s will be NUL-terminated here 
		strnzcpy(s, p_line_ + leftover, n);
		leftover += strlen(s);
		if(p_line_[leftover] == NUL)
			leftover = -1;
		return s;
	}
	return NULL;
}
#endif                         /* USE_READLINE */

#if defined(MSDOS) || defined(_WIN32)

//void do_shell()
void GnuPlot::DoShell()
{
	GpU.screen_ok = false;
	Pgm.Shift();
	if(_Plt.user_shell) {
#if defined(_WIN32)
		if(WinExec(_Plt.user_shell, SW_SHOWNORMAL) <= 32)
#elif defined(__DJGPP__)
		if(system(user_shell) == -1)
#else
		if(spawnl(P_WAIT, user_shell, NULL) == -1)
#endif
			OsError(NO_CARET, "unable to spawn shell");
	}
}
#else
/* plain old Unix */

#define EXEC "exec "
//void do_shell()
void GnuPlot::DoShell()
{
	static char exec[100] = EXEC;
	screen_ok = FALSE;
	Pgm.Shift();
	if(user_shell) {
		if(system(strnzcpy(&exec[sizeof(EXEC)-1], user_shell, sizeof(exec) - sizeof(EXEC) - 1)))
			OsError(NO_CARET, "system() failed");
	}
	putc('\n', stderr);
}
#endif                         /* !MSDOS */

// read from stdin, everything except VMS 
#ifndef USE_READLINE
	#define PUT_STRING(s) fputs(s, stderr)
	#define GET_STRING(s, l) fgets(s, l, stdin)
#endif                         /* !USE_READLINE */
// 
// this function is called for non-interactive operation. Its usage is
// like fgets(), but additionally it checks for ipc events from the
// terminals waitforinput() (if USE_MOUSE, and terminal is
// mouseable). This function will be used when reading from a pipe.
// fgets() reads in at most one less than size characters from stream and
// stores them into the buffer pointed to by s.
// Reading stops after an EOF or a newline.  If a newline is read, it is
// stored into the buffer.  A '\0' is stored  after the last character in
// the buffer. 
// 
//static char * fgets_ipc(char * dest/* string to fill */, int len/* size of it */)
char * GnuPlot::FGetsIpc(GpTermEntry * pTerm, char * dest/* string to fill */, int len/* size of it */)
{
#ifdef USE_MOUSE
	if(pTerm && pTerm->waitforinput) {
		// This a mouseable terminal --- must expect input from it 
		int    c; // char gotten from waitforinput() 
		int    i = 0; // position inside dest 
		dest[0] = '\0';
		for(i = 0; i < len-1; i++) {
			c = pTerm->waitforinput(0);
			if('\n' == c) {
				dest[i] = '\n';
				i++;
				break;
			}
			else if(EOF == c) {
				dest[i] = '\0';
				return (char *)0;
			}
			else
				dest[i] = c;
		}
		dest[i] = '\0';
		return dest;
	}
	else
#endif
	return fgets(dest, len, stdin);
}
//
// get a line from stdin, and display a prompt if interactive 
//
//static char * gp_get_string(char * buffer, size_t len, const char * pPrompt)
char * GnuPlot::GpGetString(char * buffer, size_t len, const char * pPrompt)
{
#ifdef USE_READLINE
	if(_Plt.interactive)
		return RlGets(buffer, len, pPrompt);
	else
		return FGetsIpc(term, buffer, len);
#else
	if(interactive)
		PUT_STRING(pPrompt);
	return GET_STRING(buffer, len);
#endif
}
//
// Non-VMS version 
//
//static int read_line(const char * prompt, int start)
int GnuPlot::ReadLine(const char * pPrompt, int start)
{
	bool more = FALSE;
	GpU.current_prompt = pPrompt;
	// Once we start to read a new line, the tokens pointing into the old
	// line are no longer valid.  We used to _not_ clear things here, but
	// that lead to errors when a mouse-triggered replot request came in
	// while a new line was being read.   Bug 3602388 Feb 2013.
	if(start == 0) {
		Pgm.SetTokenIdx(0);
		Pgm.NumTokens = 0;
		Pgm.P_InputLine[0] = '\0';
	}
	do {
		// grab some input 
		if(GpGetString(Pgm.P_InputLine + start, Pgm.InputLineLen - start, ((more) ? ">" : pPrompt)) == (char *)NULL) {
			// end-of-file 
			if(_Plt.interactive)
				putc('\n', stderr);
			Pgm.P_InputLine[start] = NUL;
			Pgm.inline_num++;
			if(start > 0 && Pgm.CurlyBraceCount == 0) // don't quit yet - process what we have 
				more = FALSE;
			else
				return 1; // exit gnuplot 
		}
		else {
			// normal line input 
			// P_InputLine must be NUL-terminated for strlen not to pass the
			// the bounds of this array 
			int last = strlen(Pgm.P_InputLine) - 1;
			if(last >= 0) {
				if(Pgm.P_InputLine[last] == '\n') { /* remove any newline */
					Pgm.P_InputLine[last] = NUL;
					if(last > 0 && Pgm.P_InputLine[last-1] == '\r')
						Pgm.P_InputLine[--last] = NUL;
					// Watch out that we don't backup beyond 0 (1-1-1) 
					if(last > 0)
						--last;
				}
				else if((last + 2) >= static_cast<int>(Pgm.InputLineLen)) {
					ExtendInputLine();
					// read rest of line, don't print "> " 
					start = last + 1;
					more = TRUE;
					continue;
					// else fall through to continuation handling 
				} // if(grow buffer?) 
				if(Pgm.P_InputLine[last] == '\\') {
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
// 
// Walk through the input line looking for string variables preceded by @.
// Replace the characters @<varname> with the contents of the string.
// Anything inside quotes is not expanded.
// Allow up to 3 levels of nested macros.
// 
//void string_expand_macros()
void GnuPlot::StringExpandMacros()
{
	if(Expand1LevelMacros() && Expand1LevelMacros() && Expand1LevelMacros() && Expand1LevelMacros())
		IntError(NO_CARET, "Macros nested too deeply");
}

//int expand_1level_macros()
int GnuPlot::Expand1LevelMacros()
{
#define COPY_CHAR do { Pgm.P_InputLine[o++] = *c; after_backslash = FALSE; } while(0)
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
	udvt_entry * udv;
	// Most lines have no macros 
	if(!strchr(Pgm.P_InputLine, '@'))
		return 0;
	temp_string = (char *)SAlloc::M(Pgm.InputLineLen);
	len = strlen(Pgm.P_InputLine);
	if(len >= static_cast<int>(Pgm.InputLineLen)) 
		len = Pgm.InputLineLen-1;
	strncpy(temp_string, Pgm.P_InputLine, len);
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
				    udv = Ev.GetUdvByName(m);
				    if(udv && udv->udv_value.Type == STRING) {
					    nfound++;
					    m = udv->udv_value.v.string_val;
					    FPRINTF((stderr, "Replacing @%s with \"%s\"\n", udv->udv_name, m));
					    while(strlen(m) + o + len > Pgm.InputLineLen)
						    ExtendInputLine();
					    while(*m)
						    Pgm.P_InputLine[o++] = (*m++);
				    }
				    else {
					    Pgm.P_InputLine[o] = '\0';
					    IntWarn(NO_CARET, "%s is not a string variable", m);
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
			    Pgm.P_InputLine[o++] = *c; break;
			case '#':
			    if(!in_squote && !in_dquote)
				    in_comment = TRUE;
			default:
			    COPY_CHAR; break;
		}
	}
	Pgm.P_InputLine[o] = '\0';
	SAlloc::F(temp_string);
	if(nfound)
		FPRINTF((stderr, "After string substitution command line is:\n\t%s\n", Pgm.P_InputLine));
	return(nfound);
#undef COPY_CHAR
}

#define MAX_TOTAL_LINE_LEN (1024 * MAX_LINE_LEN) // much more than what can be useful 

//int do_system_func(const char * cmd, char ** output)
int GnuPlot::DoSystemFunc(const char * cmd, char ** output)
{
#if defined(VMS) || defined(PIPES)
	int c;
	FILE * f;
	int result_allocated, result_pos;
	char* result;
	int ierr = 0;
	// open stream 
	RestrictPOpen();
	if((f = popen(cmd, "r")) == NULL)
		OsError(NO_CARET, "popen failed");
	// get output 
	result_pos = 0;
	result_allocated = MAX_LINE_LEN;
	result = (char *)SAlloc::M(MAX_LINE_LEN);
	result[0] = NUL;
	while(1) {
		if((c = getc(f)) == EOF)
			break;
		// result <- c 
		result[result_pos++] = c;
		if(result_pos == result_allocated) {
			if(result_pos >= MAX_TOTAL_LINE_LEN) {
				result_pos--;
				IntWarn(NO_CARET, "*very* long system call output has been truncated");
				break;
			}
			else {
				result = (char *)SAlloc::R(result, result_allocated + MAX_LINE_LEN);
				result_allocated += MAX_LINE_LEN;
			}
		}
	}
	result[result_pos] = NUL;
	// close stream 
	ierr = pclose(f);
	ierr = ReportError(ierr);
	result = (char *)SAlloc::R(result, strlen(result)+1);
	*output = result;
	return ierr;
#else // VMS || PIPES 
	IntWarn(NO_CARET, "system() requires support for pipes");
	*output = sstrdup("");
	return 0;
#endif // VMS || PIPES 
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
