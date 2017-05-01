/* GNUPLOT - command.c */

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
#include <locale.h>
#ifdef USE_MOUSE
	//#include "mouse.h"
	int paused_for_mouse = 0;
#endif
#define PROMPT "gnuplot> "
#ifdef _Windows
	#include <windows.h>
	#ifdef __MSC__
		#include <malloc.h>
		#include <direct.h>          /* getcwd() */
	#else
		//#include <alloc.h>
	#endif                         /* !MSC */
	#include <htmlhelp.h>
	#include "win/winmain.h"
#endif /* _Windows */
#ifdef VMS
	int vms_vkid;                   /* Virtual keyboard id */
	int vms_ktid;                   /* key table id, for translating keystrokes */
#endif /* VMS */

#define MAX_TOKENS 400 // constant by which token table grows

//
// static prototypes
//
//static void   command();
//static bool   is_array_assignment();
static int    changedir(char* path);
static char * fgets_ipc(GpTermEntry * pT, char* dest, int len);
static char * gp_get_string(char *, size_t, const char *);
//static int    read_line(const char * prompt, int start);
static void   do_system(const char*);
//static void   test_palette_subcommand();
//static int    find_clause(int *, int *);
//static int    expand_1level_macros();

//void   extend_input_line()
void GpCommand::ExtendInputLine()
{
	if(InputLineLen == 0) {
		// first time
		P_InputLine = (char *)malloc(MAX_LINE_LEN);
		InputLineLen = MAX_LINE_LEN;
		P_InputLine[0] = NUL;
	}
	else {
		P_InputLine = (char *)gp_realloc(P_InputLine, InputLineLen + MAX_LINE_LEN, "extend input line");
		InputLineLen += MAX_LINE_LEN;
		FPRINTF((stderr, "extending input line to %d chars\n", InputLineLen));
	}
}

//void extend_token_table()
void GpCommand::ExtendTokenTable()
{
	if(TokenTableSize == 0) {
		// first time
		P_Token = (LexicalUnit *)malloc(MAX_TOKENS * sizeof(LexicalUnit));
		TokenTableSize = MAX_TOKENS;
		// HBB: for checker-runs:
		memzero(P_Token, MAX_TOKENS * sizeof(*P_Token));
	}
	else {
		P_Token = (LexicalUnit *)gp_realloc(P_Token, (TokenTableSize + MAX_TOKENS) * sizeof(LexicalUnit), "extend token table");
		memzero(P_Token + TokenTableSize, MAX_TOKENS * sizeof(*P_Token));
		TokenTableSize += MAX_TOKENS;
		FPRINTF((stderr, "extending token table to %d elements\n", TokenTableSize));
	}
}

int GpGadgets::ComLine(GpCommand & rC)
{
	if(IsMultiPlot) {
		// calls int_error() if it is not happy
		//term_check_multiplot_okay(IsInteractive);
		//void term_check_multiplot_okay(bool f_interactive)
		{
			FPRINTF((stderr, "term_multiplot_okay(%d)\n", IsInteractive));
			if(term_initialised) {
				//
				// make sure that it is safe to issue an interactive prompt
				// it is safe if
				//    it is not an interactive read, or
				//    the terminal supports interactive multiplot, or
				//    we are not writing to stdout and terminal doesn't
				//    refuse multiplot outright
				//
				if(!IsInteractive || (term->flags & TERM_CAN_MULTIPLOT) || ((gpoutfile != stdout) && !(term->flags & TERM_CANNOT_MULTIPLOT))) {
					//term_suspend(); // it's okay to use multiplot here, but suspend first
					//static void term_suspend()
					{
						FPRINTF((stderr, "term_suspend()\n"));
						if(term_initialised && !term_suspended && term->suspend) {
							FPRINTF((stderr, "- calling term->suspend()\n"));
							(*term->suspend)();
							term_suspended = true;
						}
					}
				}
				else {
					// uh oh: they're not allowed to be in multiplot here 
					TermEndMultiplot(term);
					// at this point we know that it is interactive and that the
					// terminal can either only do multiplot when writing to
					// to a file, or it does not do multiplot at all
					if(term->flags & TERM_CANNOT_MULTIPLOT)
						IntErrorNoCaret("This terminal does not support multiplot");
					else
						IntErrorNoCaret("Must set output to a file or put all multiplot commands on one input line");
				}
			}
		}
		if(rC.ReadLine("multiplot> ", 0))
			return (1);
	}
	else {
		if(rC.ReadLine(PROMPT, 0))
			return (1);
	}
	/* So we can flag any new output: if false at time of error,
	 * we reprint the command line before printing caret.
	 * true for interactive terminals, since the command line is typed.
	 * false for non-terminal stdin, so command line is printed anyway.
	 * (DFK 11/89)
	 */
	screen_ok = IsInteractive;
	return BIN(rC.DoLine());
}

int GpCommand::DoLine(/*GpGadgets & rGg*/)
{
	// Expand any string variables in the current input line
	StringExpandMacros();
	char * inlptr = P_InputLine; // Line continuation has already been handled by read_line()
	while(isspace((uchar)*inlptr))
		inlptr++;
	// Leading '!' indicates a shell command that bypasses normal gnuplot
	// tokenization and parsing.  This doesn't work inside a bracketed clause.
	if(is_system(*inlptr)) {
		do_system(inlptr + 1);
	}
	else {
		// Strip off trailing comment
		FPRINTF((stderr, "doline( \"%s\" )\n", P_InputLine));
		if(strchr(inlptr, '#')) {
			NumTokens = Scanner(&P_InputLine, &InputLineLen);
			if(P_InputLine[P_Token[NumTokens].start_index] == '#')
				P_InputLine[P_Token[NumTokens].start_index] = NUL;
		}
		if(inlptr != P_InputLine) {
			// If there was leading whitespace, copy the actual
			// command string to the front. use memmove() because
			// source and target may overlap
			memmove(P_InputLine, inlptr, strlen(inlptr));
			// Terminate resulting string
			P_InputLine[strlen(inlptr)] = NUL;
		}
		FPRINTF((stderr, "  echo: \"%s\"\n", P_InputLine));
		IfDepth = 0;
		NumTokens = Scanner(&P_InputLine, &InputLineLen);
		/*
		* Expand line if necessary to contain a complete bracketed clause {...}
		* Insert a ';' after current line and append the next input line.
		* NB: This may leave an "else" condition on the next line.
		*/
		if(curly_brace_count < 0)
			R_Gg.IntErrorNoCaret("Unexpected }");
		while(curly_brace_count > 0) {
			if(lf_head && lf_head->depth > 0) {
				// This catches the case that we are inside a "load foo" operation
				// and therefore requesting interactive input is not an option. FIXME: or is it?
				R_Gg.IntErrorNoCaret("Syntax error: missing block terminator }");
			}
			else if(R_Gg.IsInteractive || R_Gg.noinputfiles) {
				/* If we are really in interactive mode and there are unterminated blocks,
				* then we want to display a "more>" prompt to get the rest of the block.
				* However, there are two more cases that must be dealt here:
				* One is when commands are piped to gnuplot - on the command line,
				* the other is when commands are piped to gnuplot which is opened
				* as a slave process. The test for noinputfiles is for the latter case.
				* If we didn't have that test here, unterminated blocks sent via a pipe
				* would trigger the error message in the else branch below. */
				strcat(P_InputLine, ";");
				int retval = ReadLine("more> ", strlen(P_InputLine));
				if(retval)
					R_Gg.IntErrorNoCaret("Syntax error: missing block terminator }");
				// Expand any string variables in the current input line 
				StringExpandMacros();
				NumTokens = Scanner(&P_InputLine, &InputLineLen);
				if(P_InputLine[P_Token[NumTokens].start_index] == '#')
					P_InputLine[P_Token[NumTokens].start_index] = NUL;
			}
			else {
				/* Non-interactive mode here means that we got a string from -e.
				* Having curly_brace_count > 0 means that there are at least one
				* unterminated blocks in the string.
				* Likely user error, so we die with an error message. */
				R_Gg.IntErrorNoCaret("Syntax error: missing block terminator }");
			}
		}
		CToken = 0;
		while(CToken < NumTokens) {
			Command();
			if(IterationEarlyExit()) {
				CToken = NumTokens;
				break;
			}
			if(command_exit_status) {
				command_exit_status = 0;
				return 1;
			}
			if(CToken < NumTokens) { // something after command
				if(Eq(";")) {
					CToken++;
				}
				else if(Eq("{")) {
					BeginClause();
				}
				else if(Eq("}")) {
					EndClause();
				}
				else
					R_Gg.IntErrorCurToken("unexpected or unrecognized token");
			}
		}
		// This check allows event handling inside load/eval/while statements
		check_for_mouse_events();
	}
	return (0);
}

//void do_string(const char * s)
void GpCommand::DoString(const char * s)
{
	char * cmdline = gp_strdup(s);
	DoStringAndFree(cmdline);
}

//void do_string_and_free(char * cmdline)
void GpCommand::DoStringAndFree(char * cmdline)
{
#ifdef USE_MOUSE
	if(display_ipc_commands())
		fprintf(stderr, "%s\n", cmdline);
#endif
	lf_push(NULL, NULL, cmdline); // save state for errors and recursion
	while(InputLineLen < strlen(cmdline) + 1)
		ExtendInputLine();
	strcpy(P_InputLine, cmdline);
	R_Gg.screen_ok = false;
	command_exit_status = DoLine();
	// We don't know if screen_ok is appropriate so leave it false.
	lf_pop(*this);
}

#ifdef USE_MOUSE
void toggle_display_of_ipc_commands()
{
	GpGg.Mse.Cfg.verbose = GpGg.Mse.Cfg.verbose ? 0 : 1;
}

int display_ipc_commands()
{
	return GpGg.Mse.Cfg.verbose;
}

//void do_string_replot(const char * s)
void GpGadgets::DoStringReplot(GpCommand & rC, const char * pStr)
{
	rC.DoString(pStr);
	if(IsVolatileData && (E_REFRESH_NOT_OK != RefreshOk)) {
		if(display_ipc_commands())
			fprintf(stderr, "refresh\n");
		RefreshRequest();
	}
	else if(!rC.IsReplotDisabled)
		ReplotRequest(rC);
	else
		IntWarn(NO_CARET, "refresh not possible and replot is disabled");
}

void restore_prompt()
{
	if(GpGg.IsInteractive) {
#if defined(HAVE_LIBREADLINE) || defined(HAVE_LIBEDITLINE)
#  if !defined(MISSING_RL_FORCED_UPDATE_DISPLAY)
		rl_forced_update_display();
#  else
		rl_redisplay();
#  endif
#else
		fputs(PROMPT, stderr);
		fflush(stderr);
#endif
	}
}

#endif /* USE_MOUSE */

//void define()
void GpCommand::Define()
{
	int start_token; // the 1st token in the function definition 
	UdvtEntry * udv;
	UdftEntry * udf;
	t_value result;
	if(Eq(CToken + 1, "(")) {
		// function ! 
		int dummy_num = 0;
		AtType * at_tmp;
		char * tmpnam;
		char save_dummy[MAX_NUM_VAR][MAX_ID_LEN+1];
		memcpy(save_dummy, P.CDummyVar, sizeof(save_dummy));
		start_token = CToken;
		do {
			CToken += 2; // skip to the next dummy 
			CopyStr(P.CDummyVar[dummy_num++], CToken, MAX_ID_LEN);
		} while(Eq(CToken + 1, ",") && (dummy_num < MAX_NUM_VAR));
		if(Eq(CToken + 1, ","))
			R_Gg.IntError(CToken + 2, "function contains too many parameters");
		CToken += 3; // skip (, dummy, ) and = 
		if(EndOfCommand())
			R_Gg.IntErrorCurToken("function definition expected");
		udf = P_DummyFunc = R_Gg.Ev.AddUdf(*this, start_token);
		udf->dummy_num = dummy_num;
		if((at_tmp = P.PermAt()) == (AtType*)NULL)
			R_Gg.IntError(start_token, "not enough memory for function");
		AtType::Destroy(udf->at);
		udf->at = at_tmp; // before re-assigning it.
		memcpy(P.CDummyVar, save_dummy, sizeof(save_dummy));
		MCapture(&(udf->definition), start_token, CToken - 1);
		P_DummyFunc = NULL; // dont let anyone else use our workspace
		// Save function definition in a user-accessible variable
		tmpnam = (char *)malloc(8+strlen(udf->udf_name));
		strcpy(tmpnam, "GPFUN_");
		strcat(tmpnam, udf->udf_name);
		R_Gg.Ev.FillGpValString(tmpnam, udf->definition);
		free(tmpnam);
	}
	else {
		// variable ! 
		char * p_varname = P_InputLine + P_Token[CToken].start_index;
		if(!strncmp(p_varname, "GPVAL_", 6) || !strncmp(p_varname, "MOUSE_", 6))
			R_Gg.IntErrorCurToken("Cannot set internal variables GPVAL_ and MOUSE_");
		start_token = CToken;
		CToken += 2;
		udv = R_Gg.Ev.AddUdv(*this, start_token);
		P.ConstExpress(*this, &result);
		// Prevents memory leak if the variable name is re-used 
		gpfree_array(&udv->udv_value);
		gpfree_string(&udv->udv_value);
		udv->udv_value = result;
	}
}

void undefine_command(GpCommand & rC)
{
	char   key[MAX_ID_LEN+1];
	rC.CToken++;           /* consume the command name */
	while(!rC.EndOfCommand()) {
		// copy next var name into key
		rC.CopyStr(key, rC.CToken, MAX_ID_LEN);
		// Peek ahead - must do this, because a '*' is returned as a separate token, not as part of the 'key'
		const bool wildcard = rC.Eq(rC.CToken+1, "*") ? true : false;
		if(wildcard)
			rC.CToken++;
		else if(*key == '$') // The '$' starting a data block name is a separate token
			rC.CopyStr(&key[1], ++rC.CToken, MAX_ID_LEN-1);
		// ignore internal variables
		if(strncmp(key, "GPVAL_", 6) && strncmp(key, "MOUSE_", 6))
			GpGg.Ev.DelUdvByName(key, wildcard);
		rC.CToken++;
	}
}

void GpCommand::Command()
{
#if 0 // {
	static const GpGenFTable command_ftbl[] =
	{
		{ "ra$ise", raise_command },
		{ "low$er", lower_command },
	#ifdef USE_MOUSE
		{ "bi$nd", bind_command },
	#endif
		{ "array", array_command },
		{ "break", GpCommand::BreakCommand },
		{ "ca$ll", call_command },
		{ "cd", changedir_command },
		{ "cl$ear", clear_command },
		{ "continue", GpCommand::ContinueCommand },
		{ "do", GpCommand::DoCommand },
		{ "eval$uate", eval_command },
		{ "ex$it", GpCommand::ExitCommand },
		{ "f$it", GpFit::FitCommand },
		{ "h$elp", help_command },
		{ "?", help_command },
		{ "hi$story", history_command },
		{ "if", GpCommand::IfCommand },
		{ "import", import_command },
		{ "else", GpCommand::ElseCommand },
		{ "l$oad", load_command },
		{ "pa$use", pause_command },
		{ "p$lot", plot_command },
		{ "pr$int", print_command },
		{ "printerr$or", printerr_command },
		{ "pwd", pwd_command },
		{ "q$uit", GpCommand::ExitCommand },
		{ "ref$resh", refresh_command },
		{ "rep$lot", replot_command },
		{ "re$read", reread_command },
		{ "res$et", reset_command },
		{ "sa$ve", save_command },
		{ "scr$eendump", screendump_command },
		{ "se$t", set_command },
		{ "she$ll", do_shell },
		{ "sh$ow", show_command },
		{ "sp$lot", splot_command },
		{ "st$ats", stats_command },
		{ "sy$stem", system_command },
		{ "test", test_command },
		{ "tog$gle", toggle_command },
		{ "und$efine", undefine_command },
		{ "uns$et", unset_command },
		{ "up$date", update_command },
		{ "while", GpCommand::WhileCommand },
		{ "{", GpCommand::BeginClause },
		{ "}", GpCommand::EndClause },
		{ ";", null_command },
		{ "$", datablock_command },
		/* last key must be NULL */
		{ NULL, invalid_command }
	};
#endif // } 0
	for(int i = 0; i < MAX_NUM_VAR; i++)
		P.CDummyVar[i][0] = NUL;  // no dummy variables
	if(IsDefinition())
		Define();
	else if(IsArrayAssignment())
		;
	else {
		//(*lookup_ftable(&command_ftbl[0], CToken))();
#if 1 // @construction {
		if(AlmostEq(CToken, "ra$ise")) { 
			RaiseLowerCommand(0); 
		}
		else if(AlmostEq(CToken, "low$er")) { 
			RaiseLowerCommand(1); 
		}
#ifdef USE_MOUSE
		else if(AlmostEq(CToken, "bi$nd")) { 
			R_Gg.BindCommand(*this); 
		}
#endif
		else if(AlmostEq(CToken, "array")) { 
			ArrayCommand(); 
		}
		else if(AlmostEq(CToken, "break")) { 
			BreakCommand(); 
		}
		else if(AlmostEq(CToken, "ca$ll")) { 
			CallCommand(); 
		}
		else if(AlmostEq(CToken, "cd")) { 
			R_Gg.ChangeDirCommand(*this); 
		}
		else if(AlmostEq(CToken, "cl$ear")) { 
			R_Gg.ClearCommand(*this); 
		}
		else if(AlmostEq(CToken, "continue")) { 
			ContinueCommand(); 
		}
		else if(AlmostEq(CToken, "do")) { 
			DoCommand(); 
		}
		else if(AlmostEq(CToken, "eval$uate")) { 
			//eval_command(); 
			//void eval_command()
			{
				CToken++;
				char * p_command = TryToGetString();
				if(!p_command)
					R_Gg.IntErrorCurToken("Expected command string");
				DoStringAndFree(p_command);
			}
		}
		else if(AlmostEq(CToken, "ex$it")) { 
			ExitCommand(); 
		}
		else if(AlmostEq(CToken, "f$it")) { 
			GpF.FitCommand(*this); 
		}
		else if(AlmostEq(CToken, "h$elp")) { 
			help_command(*this); }
		else if(AlmostEq(CToken, "?")) { 
			help_command(*this); }
		else if(AlmostEq(CToken, "hi$story")) { 
			history_command(*this); }
		else if(AlmostEq(CToken, "if")) { 
			IfCommand(); }
		else if(AlmostEq(CToken, "import")) { 
			ImportCommand(); 
		}
		else if(AlmostEq(CToken, "else")) { 
			ElseCommand(); 
		}
		else if(AlmostEq(CToken, "l$oad")) { 
			R_Gg.LoadCommand(*this); 
		}
		else if(AlmostEq(CToken, "pa$use")) { 
			R_Gg.PauseCommand(*this); 
		}
		else if(AlmostEq(CToken, "p$lot")) { 
			R_Gg.PlotCommand(*this); }
		else if(AlmostEq(CToken, "pr$int")) { 
			print_command(*this); 
		}
		else if(AlmostEq(CToken, "printerr$or")) { 
			//printerr_command(); 
			//void printerr_command()
			{
				FILE * save_print_out = F_PrintOut;
				F_PrintOut = stderr;
				print_command(*this);
				F_PrintOut = save_print_out;
			}
		}
		else if(AlmostEq(CToken, "pwd")) { 
			pwd_command(); 
		}
		else if(AlmostEq(CToken, "q$uit")) { 
			ExitCommand(); 
		}
		else if(AlmostEq(CToken, "ref$resh")) { 
			//refresh_command(); 
			//void refresh_command()
			{
				CToken++;
				R_Gg.RefreshRequest();
			}
		}
		else if(AlmostEq(CToken, "rep$lot")) { 
			R_Gg.ReplotCommand(term, *this); 
		}
		else if(AlmostEq(CToken, "re$read")) { 
			reread_command(); 
		}
		else if(AlmostEq(CToken, "res$et")) { 
			R_Gg.ResetCommand(*this); 
		}
		else if(AlmostEq(CToken, "sa$ve")) { 
			R_Gg.SaveCommand(*this); 
		}
		else if(AlmostEq(CToken, "scr$eendump")) { 
			screendump_command(); 
		}
		else if(AlmostEq(CToken, "se$t")) { 
			R_Gg.SetCommand(*this); 
		}
		else if(AlmostEq(CToken, "she$ll")) { 
			do_shell(); 
		}
		else if(AlmostEq(CToken, "sh$ow")) { 
			R_Gg.ShowCommand(*this); 
		}
		else if(AlmostEq(CToken, "sp$lot")) { 
			//splot_command(); 
			//void splot_command()
			{
				PlotToken = CToken++;
				GpDf.plotted_data_from_stdin = false;
				R_Gg.RefreshNPlots = 0;
				SET_CURSOR_WAIT;
			#ifdef USE_MOUSE
				plot_mode(MODE_SPLOT);
				R_Gg.Ev.AddUdvByName("MOUSE_X")->udv_value.type = NOTDEFINED;
				R_Gg.Ev.AddUdvByName("MOUSE_Y")->udv_value.type = NOTDEFINED;
				R_Gg.Ev.AddUdvByName("MOUSE_X2")->udv_value.type = NOTDEFINED;
				R_Gg.Ev.AddUdvByName("MOUSE_Y2")->udv_value.type = NOTDEFINED;
				R_Gg.Ev.AddUdvByName("MOUSE_BUTTON")->udv_value.type = NOTDEFINED;
			#endif
				R_Gg.Plot3DRequest(*this);
				SET_CURSOR_ARROW;
			}
		}
		else if(AlmostEq(CToken, "st$ats")) { 
			stats_command(); 
		}
		else if(AlmostEq(CToken, "sy$stem")) { 
			system_command(); 
		}
		else if(AlmostEq(CToken, "test")) { 
			R_Gg.TestCommand(*this); 
		}
		else if(AlmostEq(CToken, "tog$gle")) { 
			R_Gg.ToggleCommand(term, *this); 
		}
		else if(AlmostEq(CToken, "und$efine")) { 
			undefine_command(*this); 
		}
		else if(AlmostEq(CToken, "uns$et")) { 
			R_Gg.UnsetCommand(*this); 
		}
		else if(AlmostEq(CToken, "up$date")) { 
			//update_command(); 
			//void update_command()
			{
				char * opfname = NULL; // old parameter filename
				char * npfname = NULL; // new parameter filename
				CToken++;
				if(!(opfname = TryToGetString()))
					R_Gg.IntErrorCurToken("Parameter filename expected");
				if(!EndOfCommand() && !(npfname = TryToGetString()))
					R_Gg.IntErrorCurToken("New parameter filename expected");
				GpF.Update(opfname, npfname);
				free(npfname);
				free(opfname);
			}
		}
		else if(AlmostEq(CToken, "while")) { 
			WhileCommand(); 
		}
		else if(AlmostEq(CToken, "{")) { 
			BeginClause(); 
		}
		else if(AlmostEq(CToken, "}")) { 
			EndClause(); 
		}
		else if(AlmostEq(CToken, ";")) { 
			null_command(); 
		}
		else if(AlmostEq(CToken, "$")) { 
			datablock_command(*this); 
		}
		else {
			invalid_command();
		}
#endif // } 0 @construction 
	}
	return;
}
//
// process the 'raise' or 'lower' command 
//
void GpCommand::RaiseLowerCommand(int lower)
{
	++CToken;
	if(EndOfCommand()) {
		if(lower) {
#ifdef X11
			x11_lower_terminal_group();
#endif
#ifdef _Windows
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
#ifdef _Windows
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
		int negative = Eq("-");
		if(negative || Eq("+")) 
			CToken++;
		if(!EndOfCommand() && IsANumber(CToken)) {
			number = (int)RealExpression();
			if(negative)
				number = -number;
			if(lower) {
#ifdef X11
				x11_lower_terminal_window(number);
#endif
#ifdef _Windows
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
#ifdef _Windows
				win_raise_terminal_window(number);
#endif
#ifdef WXWIDGETS
				wxt_raise_terminal_window(number);
#endif
			}
			++CToken;
			return;
		}
	}
	if(lower)
		R_Gg.IntErrorCurToken("usage: lower {plot_id}");
	else
		R_Gg.IntErrorCurToken("usage: raise {plot_id}");
}

//void raise_command() { raise_lower_command(0); }
//void lower_command() { raise_lower_command(1); }

/*
 * Arrays are declared using the syntax
 *    array A[size]
 * where size is an integer and space is reserved for elements A[1] through A[size]
 * The size itself is stored in A[0].v.int_val.
 *
 * Elements in an existing array can be accessed like any other gnuplot variable.
 * Each element can be one of INTGR, CMPLX, STRING.
 * When the array is declared all elements are set to NOTDEFINED.
 */
//void array_command()
void GpCommand::ArrayCommand()
{
	int nsize = 0;
	UdvtEntry * array;
	t_value * p_values = 0;
	int i;
	// Create or recycle a udv containing an array with the requested name
	if(!IsLetter(++CToken))
		R_Gg.IntErrorCurToken("illegal variable name");
	array = R_Gg.Ev.AddUdv(*this, CToken);
	gpfree_array(&array->udv_value);
	gpfree_string(&array->udv_value);

	if(!Eq(++CToken, "["))
		R_Gg.IntErrorCurToken("expecting array[size]");
	CToken++;
	nsize = IntExpression();
	if(!Eq(CToken++, "]") || nsize <= 0)
		R_Gg.IntError(CToken-1, "expecting array[size>0]");
	array->udv_value.v.value_array = (t_value *)malloc((nsize+1) * sizeof(t_value));
	array->udv_value.type = ARRAY;
	// Element zero of the new array is not visible but contains the size
	p_values = array->udv_value.v.value_array;
	p_values[0].v.int_val = nsize;
	for(i = 0; i <= nsize; i++) {
		p_values[i].type = NOTDEFINED;
	}
	// Initializer syntax:   array A[10] = [x,y,z,,"foo",]
	if(Eq("=")) {
		if(!Eq(++CToken, "["))
			R_Gg.IntErrorCurToken("expecting Array[size] = [x,y,...]");
		CToken++;
		for(i = 1; i <= nsize; i++) {
			if(Eq("]"))
				break;
			else if(Eq(",")) {
				CToken++;
			}
			else {
				P.ConstExpress(*this, &p_values[i]);
				if(Eq("]"))
					break;
				if(Eq(","))
					CToken++;
				else
					R_Gg.IntErrorCurToken("expecting Array[size] = [x,y,...]");
			}
		}
		CToken++;
	}
}

/*
 * Check for command line beginning with
 *    Array[<expr>] = <expr>
 * This routine is modeled on command.c:define()
 */
//bool is_array_assignment()
bool GpCommand::IsArrayAssignment()
{
	UdvtEntry * udv = R_Gg.Ev.AddUdv(*this, CToken);
	t_value newvalue;
	int    index;
	bool   looks_OK = false;
	int    brackets;
	if(!IsLetter(CToken) || !Eq(CToken+1, "["))
		return false;
	/* There are other legal commands where the 2nd token is [
	 * e.g.  "plot [min:max] foo"
	 * so we check that the closing ] is immediately followed by =.
	 */
	for(index = CToken+2, brackets = 1; index < NumTokens; index++) {
		if(Eq(index, ";"))
			return false;
		if(Eq(index, "["))
			brackets++;
		if(Eq(index, "]"))
			brackets--;
		if(brackets == 0) {
			if(!Eq(index+1, "="))
				return false;
			looks_OK = true;
			break;
		}
	}
	if(looks_OK) {
		if(udv->udv_value.type != ARRAY)
			R_Gg.IntErrorCurToken("Not a known array");
		// Evaluate index
		CToken += 2;
		index = IntExpression();
		if(index <= 0 || index > udv->udv_value.v.value_array[0].v.int_val)
			R_Gg.IntErrorCurToken("array index out of range");
		if(!Eq("]") || !Eq(CToken+1, "="))
			R_Gg.IntErrorCurToken("Expecting Arrayname[<expr>] = <expr>");
		// Evaluate right side of assignment
		CToken += 2;
		P.ConstExpress(*this, &newvalue);
		udv->udv_value.v.value_array[index] = newvalue;
		return true;
	}
	else
		return false;
}

#ifdef USE_MOUSE
//
// process the 'bind' command
// EAM - rewritten 2015
//
//void bind_command()
void GpGadgets::BindCommand(GpCommand & rC)
{
	char* lhs = NULL;
	char* rhs = NULL;
	bool allwindows = false;
	++rC.CToken;
	if(rC.AlmostEq("all$windows")) {
		allwindows = true;
		rC.CToken++;
	}
	//
	// get left hand side: the key or key sequence
	// either (1) entire sequence is in quotes
	// or (2) sequence goes until the first whitespace
	//
	if(rC.EndOfCommand()) {
		; // Fall through 
	}
	else if(rC.IsStringValue(rC.CToken) && (lhs = rC.TryToGetString())) {
		FPRINTF((stderr, "Got bind quoted lhs = \"%s\"\n", lhs));
	}
	else {
		char * p_first = rC.P_InputLine + rC.P_Token[rC.CToken].start_index;
		int    size = strcspn(p_first, " \";");
		lhs = (char *)malloc(size + 1);
		strncpy(lhs, p_first, size);
		lhs[size] = '\0';
		FPRINTF((stderr, "Got bind unquoted lhs = \"%s\"\n", lhs));
		while(rC.P_InputLine + rC.P_Token[rC.CToken].start_index < p_first+size)
			rC.CToken++;
	}
	// get right hand side: the command to bind
	// either (1) quoted command
	// or (2) the rest of the line
	if(rC.EndOfCommand()) {
		; // Fall through 
	}
	else if(rC.IsStringValue(rC.CToken) && (rhs = rC.TryToGetString())) {
		FPRINTF((stderr, "Got bind quoted rhs = \"%s\"\n", rhs));
	}
	else {
		int save_token = rC.CToken;
		while(!rC.EndOfCommand())
			rC.CToken++;
		rC.MCapture(&rhs, save_token, rC.CToken-1);
		FPRINTF((stderr, "Got bind unquoted rhs = \"%s\"\n", rhs));
	}
	// bind_process() will eventually free lhs / rhs ! 
	Mse.BindProcess(lhs, rhs, allwindows);
}

#endif /* USE_MOUSE */
//
// 'break' and 'continue' commands act as in the C language.
// Skip to the end of current loop iteration and (for break)
// do not iterate further
//
//void break_command()
void GpCommand::BreakCommand()
{
	CToken++;
	if(iteration_depth) {
		// Skip to end of current iteration 
		CToken = NumTokens;
		// request that subsequent iterations should be skipped also 
		requested_break = true;
	}
}

//void continue_command()
void GpCommand::ContinueCommand()
{
	CToken++;
	if(iteration_depth) {
		// Skip to end of current clause 
		CToken = NumTokens;
		// request that remainder of this iteration be skipped also 
		requested_continue = true;
	}
}
/*
 * Command parser functions
 */
//
// process the 'call' command 
//
//void call_command()
void GpCommand::CallCommand()
{
	CToken++;
	char * save_file = TryToGetString();
	if(!save_file)
		R_Gg.IntErrorCurToken("expecting filename");
	gp_expand_tilde(&save_file);
	// Argument list follows filename 
	R_Gg.LoadFile(loadpath_fopen(save_file, "r"), save_file, 2);
}
//
// process the 'cd' command
//
//void changedir_command()
void GpGadgets::ChangeDirCommand(GpCommand & rC)
{
	char * save_file = NULL;
	rC.CToken++;
	save_file = rC.TryToGetString();
	if(!save_file)
		IntErrorCurToken("expecting directory name");
	gp_expand_tilde(&save_file);
	if(changedir(save_file))
		IntErrorCurToken("Can't change to this directory");
	else
		Ev.UpdateGpValVariables(5);
	free(save_file);
}
//
// process the 'clear' command 
//
void GpGadgets::ClearCommand(GpCommand & rC)
{
	TermStartPlot(term);
	if(IsMultiPlot && term->fillbox) {
		uint xx1 = (uint)(XOffs * term->xmax);
		uint yy1 = (uint)(YOffs * term->ymax);
		uint width = (uint)(XSz * term->xmax);
		uint height = (uint)(YSz * term->ymax);
		(*term->fillbox)(0, xx1, yy1, width, height);
	}
	TermEndPlot(term);
	screen_ok = false;
	rC.CToken++;
}
//
// process the 'exit' and 'quit' commands 
//
//void exit_command()
void GpCommand::ExitCommand()
{
	// If the command is "exit gnuplot" then do so 
	if(Eq(CToken+1, "gnuplot"))
		gp_exit(EXIT_SUCCESS);
	// exit error 'error message'  returns to the top command line 
	if(Eq(CToken+1, "error")) {
		CToken += 2;
		R_Gg.IntErrorNoCaret(TryToGetString());
	}
	// else graphics will be tidied up in main 
	command_exit_status = 1;
}

/* fit_command() is in fit.c */

/* help_command() is below */
//
// process the 'history' command 
//
void history_command(GpCommand & rC)
{
#if defined(READLINE) || defined(HAVE_LIBREADLINE) || defined(HAVE_LIBEDITLINE)
	rC.CToken++;
	if(!rC.EndOfCommand() && rC.Eq("?")) {
		static char * search_str = NULL; // @global string from command line to search for 
		// find and show the entries 
		rC.CToken++;
		rC.MCapture(&search_str, rC.CToken, rC.CToken); // reallocates memory
		printf("history ?%s\n", search_str);
		if(!rC.H.HistoryFindAll(search_str))
			GpGg.IntErrorCurToken("not in history");
		rC.CToken++;
	}
	else if(!rC.EndOfCommand() && rC.Eq("!")) {
		const char * line_to_do = NULL; // command returned by search
		rC.CToken++;
		if(rC.IsANumber(rC.CToken)) {
			int i = rC.IntExpression();
			line_to_do = rC.H.HistoryFindByNumber(i);
		}
		else {
			char * search_str = NULL; /* string from command line to search for */
			rC.MCapture(&search_str, rC.CToken, rC.CToken);
			line_to_do = rC.H.HistoryFind(search_str);
			free(search_str);
		}
		if(line_to_do == NULL)
			GpGg.IntErrorCurToken("not in history");
		// Replace current entry "history !..." in history list	
		// with the command we found by searching.		
#if defined(HAVE_LIBREADLINE)
		free(replace_history_entry(history_length-1, line_to_do, NULL)->line);
#elif defined(READLINE)
		free(rC.H.history->line);
		rC.H.history->line = (char *)line_to_do;
#endif
		printf("  Executing:\n\t%s\n", line_to_do);
		rC.DoString(line_to_do);
		rC.CToken++;
	}
	else {
		int n = 0;         /* print only <last> entries */
		char * tmp;
		bool append = false; /* rewrite output file or append it */
		static char * name = NULL; // @global name of the output file; NULL for stdout 
		bool quiet = rC.H.history_quiet;
		if(!rC.EndOfCommand() && rC.AlmostEq("q$uiet")) {
			/* option quiet to suppress history entry numbers */
			quiet = true;
			rC.CToken++;
		}
		/* show history entries */
		if(!rC.EndOfCommand() && rC.IsANumber(rC.CToken)) {
			n = rC.IntExpression();
		}
		if((tmp = rC.TryToGetString())) {
			free(name);
			name = tmp;
			if(!rC.EndOfCommand() && rC.AlmostEq("ap$pend")) {
				append = true;
				rC.CToken++;
			}
		}
		rC.H.WriteHistoryN(n, (quiet ? "" : name), (append ? "a" : "w"));
	}
#else
	rC.CToken++;
	IntWarn(NO_CARET, "This copy of gnuplot was built without support for command history.");
#endif /* defined(READLINE) || defined(HAVE_LIBREADLINE) || defined(HAVE_LIBEDITLINE) */
}

// Make a copy of an input line substring delimited by { and } 
static char * new_clause(int clause_start, int clause_end)
{
	char * clause = (char *)malloc(clause_end - clause_start);
	memcpy(clause, &GpGg.Gp__C.P_InputLine[clause_start+1], clause_end - clause_start);
	clause[clause_end - clause_start - 1] = '\0';
	return clause;
}
//
// process the 'if' command 
//
//void if_command()
void GpCommand::IfCommand()
{
	double exprval;
	int end_token;
	if(!Eq(++CToken, "("))     /* no expression */
		R_Gg.IntErrorCurToken("expecting (expression)");
	exprval = RealExpression();
	/*
	 * EAM May 2011
	 * New if {...} else {...} syntax can span multiple lines.
	 * Isolate the active clause and execute it recursively.
	 */
	if(Eq("{")) {
		/* Identify start and end position of the clause substring */
		char * clause = NULL;
		int if_start, if_end, else_start = 0, else_end = 0;
		int clause_start, clause_end;
		CToken = FindClause(&if_start, &if_end);
		if(Eq("else")) {
			if(!Eq(++CToken, "{"))
				R_Gg.IntErrorCurToken("expected {else-clause}");
			CToken = FindClause(&else_start, &else_end);
		}
		end_token = CToken;
		if(exprval != 0) {
			clause_start = if_start;
			clause_end = if_end;
			IfCondition = true;
		}
		else {
			clause_start = else_start;
			clause_end = else_end;
			IfCondition = false;
		}
		IfOpenForElse = else_start ? false : true;
		if(IfCondition || else_start != 0) {
			clause = new_clause(clause_start, clause_end);
			BeginClause();
			DoStringAndFree(clause);
			EndClause();
		}
		CToken = IterationEarlyExit() ? NumTokens : end_token;
		return;
	}
	/*
	 * EAM May 2011
	 * Old if/else syntax (no curly braces) affects the rest of the current line.
	 * Deprecate?
	 */
	if(clause_depth > 0)
		R_Gg.IntErrorCurToken("Old-style if/else statement encountered inside brackets");
	IfDepth++;
	if(exprval != 0.0) {
		// fake the condition of a ';' between commands
		int eolpos = P_Token[NumTokens - 1].start_index + P_Token[NumTokens - 1].length;
		--CToken;
		P_Token[CToken].length = 1;
		P_Token[CToken].start_index = eolpos + 2;
		P_InputLine[eolpos + 2] = ';';
		P_InputLine[eolpos + 3] = NUL;
		IfCondition = true;
	}
	else {
		while(CToken < NumTokens) {
			/* skip over until the next command */
			while(!EndOfCommand()) {
				++CToken;
			}
			if(Eq(++CToken, "else")) {
				/* break if an "else" was found */
				IfCondition = false;
				--CToken; /* go back to ';' */
				return;
			}
		}
		/* no else found */
		CToken = NumTokens = 0;
	}
}
//
// process the 'else' command 
//
void GpCommand::ElseCommand()
{
	int end_token;
	/*
	 * EAM May 2011
	 * New if/else syntax permits else clause to appear on a new line
	 */
	if(Eq(CToken+1, "{")) {
		int clause_start, clause_end;
		char * clause;
		if(IfOpenForElse)
			IfOpenForElse = false;
		else
			R_Gg.IntErrorCurToken("Invalid {else-clause}");

		CToken++; /* Advance to the opening curly brace */
		end_token = FindClause(&clause_start, &clause_end);
		if(!IfCondition) {
			clause = new_clause(clause_start, clause_end);
			BeginClause();
			DoStringAndFree(clause);
			EndClause();
		}
		CToken = IterationEarlyExit() ? NumTokens : end_token;
		return;
	}
	// EAM May 2011
	// The rest is only relevant to the old if/else syntax (no curly braces)
	if(IfDepth <= 0) {
		R_Gg.IntErrorCurToken("else without if");
		return;
	}
	else {
		IfDepth--;
	}
	if(IfCondition) {
		// First part of line was true so discard the rest of the line
		CToken = NumTokens = 0;
	}
	else {
		//REPLACE_ELSE(CToken);
		{
			int idx = P_Token[CToken].start_index;
			P_Token[CToken].length = 1;		 
			P_InputLine[idx++] = ';'; /* e */ 
			P_InputLine[idx++] = ' '; /* l */ 
			P_InputLine[idx++] = ' '; /* s */ 
			P_InputLine[idx++] = ' '; /* e */ 
		}
		IfCondition = true;
	}
}
//
// process commands of the form 'do for [i=1:N] ...'
//
//void do_command()
void GpCommand::DoCommand()
{
	t_iterator * do_iterator;
	int do_start, do_end;
	int end_token;
	char * clause;
	CToken++;
	do_iterator = CheckForIteration();
	if(!Eq("{"))
		R_Gg.IntErrorCurToken("expecting {do-clause}");
	end_token = FindClause(&do_start, &do_end);
	clause = new_clause(do_start, do_end);
	BeginClause();
	iteration_depth++;
	if(empty_iteration(do_iterator))
		strcpy(clause, ";");
	do {
		requested_continue = false;
		DoString(clause);
		if(requested_break)
			break;
	} while(next_iteration(do_iterator));
	iteration_depth--;
	free(clause);
	EndClause();
	CToken = end_token;
	do_iterator = cleanup_iteration(do_iterator);
	requested_break = false;
	requested_continue = false;
}
//
// process commands of the form 'while (foo) {...}' */
// FIXME:  For consistency there should be an iterator associated
// with this statement.
//
//void while_command()
void GpCommand::WhileCommand()
{
	int    do_start, do_end;
	char * clause;
	int    save_token, end_token;
	double exprval;

	CToken++;
	save_token = CToken;
	exprval = RealExpression();
	if(!Eq("{"))
		R_Gg.IntErrorCurToken("expecting {while-clause}");
	end_token = FindClause(&do_start, &do_end);
	clause = new_clause(do_start, do_end);
	BeginClause();
	iteration_depth++;
	while(exprval != 0) {
		requested_continue = false;
		DoString(clause);
		if(requested_break)
			break;
		CToken = save_token;
		exprval = RealExpression();
	}
	iteration_depth--;
	EndClause();
	free(clause);
	CToken = end_token;
	requested_break = false;
	requested_continue = false;
}
//
// set link [x2|y2] {via <expression1> {inverse <expression2>}}
// set nonlinear <axis> via <expression1> inverse <expression2>
// unset link [x2|y2]
// unset nonlinear <axis>
// 
//void link_command()
void GpGadgets::LinkCommand(GpCommand & rC)
{
	GpAxis * primary_axis = NULL;
	GpAxis * secondary_axis = NULL;
	bool linked = false;
	int command_token = rC.CToken;    /* points to "link" or "nonlinear" */

	rC.CToken++;
	// Set variable name accepatable for the via/inverse functions 
	strcpy(rC.P.CDummyVar[0], "x");
	strcpy(rC.P.CDummyVar[1], "y");
	if(rC.Eq("z") || rC.Eq("cb"))
		strcpy(rC.P.CDummyVar[0], "z");
	if(rC.Eq("r"))
		strcpy(rC.P.CDummyVar[0], "r");
	// 
	// "set nonlinear" will eventually apply to any visible axis
	// but for now we support x y x2 y2 cb in 2D plots; cb in 3D plots
	// 
	if(rC.Eq(command_token, "nonlinear")) {
#ifdef NONLINEAR_AXES
		AXIS_INDEX axis;
		if((axis = (AXIS_INDEX)rC.LookupTable(axisname_tbl, rC.CToken)) >= 0)
			secondary_axis = &AxA[axis];
		else
			IntErrorCurToken("not a valid nonlinear axis");
		primary_axis = get_shadow_axis(secondary_axis);
#else
		IntErrorCurToken("This copy of gnuplot does not support nonlinear axes");
#endif
		//
		// "set link" applies to either x|x2 or y|y2
		// Flag the axes as being linked, and copy the range settings
		// from the primary axis into the linked secondary axis
		// 
	}
	else {
		if(rC.AlmostEq("x$2")) {
			primary_axis = &AxA[FIRST_X_AXIS];
			secondary_axis = &AxA[SECOND_X_AXIS];
		}
		else if(rC.AlmostEq("y$2")) {
			primary_axis = &AxA[FIRST_Y_AXIS];
			secondary_axis = &AxA[SECOND_Y_AXIS];
		}
		else {
			IntErrorCurToken("expecting x2 or y2");
		}
	}
	rC.CToken++;
	// "unset link {x|y}" command 
	if(rC.Eq(command_token-1, "unset")) {
		secondary_axis->P_LinkToPrmr = 0;
		primary_axis->P_LinkToScnd = 0;
		// FIXME: could return here except for the need to free link_udf->at */
		linked = false;
	}
	else {
		secondary_axis->P_LinkToPrmr = primary_axis;
		primary_axis->P_LinkToScnd = secondary_axis;
		linked = true;
	}
	// Initialize the action tables for the mapping function[s] 
	if(!primary_axis->link_udf) {
		primary_axis->link_udf = (UdftEntry *)malloc(sizeof(UdftEntry));
		memzero(primary_axis->link_udf, sizeof(UdftEntry));
	}
	if(!secondary_axis->link_udf) {
		secondary_axis->link_udf = (UdftEntry *)malloc(sizeof(UdftEntry));
		memzero(secondary_axis->link_udf, sizeof(UdftEntry));
	}
	if(rC.Eq("via")) {
		rC.ParseLinkVia(secondary_axis->link_udf);
		if(rC.AlmostEq("inv$erse")) {
			rC.ParseLinkVia(primary_axis->link_udf);
		}
		else {
			IntWarn(rC.CToken, "inverse mapping function required");
			linked = false;
		}
	}
#ifdef NONLINEAR_AXES
	else if(rC.Eq(command_token, "nonlinear") && linked) {
		IntWarn(rC.CToken, "via mapping function required");
		linked = false;
	}
	if(rC.Eq(command_token, "nonlinear") && linked) {
		// Save current user-visible axis range (note reversed order!) 
		UdftEntry * temp = primary_axis->link_udf;
		primary_axis->link_udf = secondary_axis->link_udf;
		secondary_axis->link_udf = temp;
		clone_linked_axes(secondary_axis, primary_axis);
	}
	else
#endif
	// Clone the range information 
	if(linked) {
		clone_linked_axes(primary_axis, secondary_axis);
	}
	else {
		AtType::Destroy(secondary_axis->link_udf->at);
		secondary_axis->link_udf->at = NULL;
		AtType::Destroy(primary_axis->link_udf->at);
		primary_axis->link_udf->at = NULL;
	}
	if(secondary_axis->Index == POLAR_AXIS)
		RRangeToXY();
}
//
// process the 'load' command 
//
//void load_command()
void GpGadgets::LoadCommand(GpCommand & rC)
{
	FILE * fp;
	char * save_file;
	rC.CToken++;
	save_file = rC.TryToGetString();
	if(!save_file)
		IntErrorCurToken("expecting filename");
	gp_expand_tilde(&save_file);
	fp = strcmp(save_file, "-") ? loadpath_fopen(save_file, "r") : stdout;
	LoadFile(fp, save_file, 1);
}

/* null command */
void null_command()
{
	return;
}

/* Clauses enclosed by curly brackets:
 * do for [i = 1:N] { a; b; c; }
 * if(<test>) {
 *    line1;
 *    line2;
 * } else {
 *    ...
 * }
 */
//
// Find the start and end character positions within GpGg.Gp__C.P_InputLine
// bounding a clause delimited by {...}.
// Assumes that GpGg.Gp__C.CToken indexes the opening left curly brace.
// Returns the index of the first token after the closing curly brace.
// 
//int find_clause(int * clause_start, int * clause_end)
int GpCommand::FindClause(int * pClauseStart, int * pClauseEnd)
{
	int i, depth;
	*pClauseStart = P_Token[CToken].start_index;
	for(i = ++CToken, depth = 1; i<NumTokens; i++) {
		if(Eq(i, "{"))
			depth++;
		else if(Eq(i, "}"))
			depth--;
		if(depth == 0)
			break;
	}
	*pClauseEnd = P_Token[i].start_index;
	return (i+1);
}

//void begin_clause()
void GpCommand::BeginClause()
{
	clause_depth++;
	CToken++;
}

//void end_clause()
void GpCommand::EndClause()
{
	if(clause_depth == 0)
		R_Gg.IntErrorCurToken("unexpected }");
	else
		clause_depth--;
	CToken++;
}

//void clause_reset_after_error()
void GpCommand::ClauseResetAfterError()
{
	if(clause_depth)
		FPRINTF((stderr, "CLAUSE RESET after error at depth %d\n", clause_depth));
	clause_depth = 0;
	iteration_depth = 0;
}

/* helper routine to multiplex mouse event handling with a timed pause command */
void timed_pause(double sleep_time)
{
#if defined(HAVE_USLEEP) && defined(USE_MOUSE)
	if(term->waitforinput)          /* If the terminal supports it */
		while(sleep_time > 0.05) { /* we poll 20 times a second */
			usleep(50000);  /* Sleep for 50 msec */
			check_for_mouse_events();
			sleep_time -= 0.05;
		}
	usleep((useconds_t)(sleep_time * 1e6));
	check_for_mouse_events();
#else
	GP_SLEEP((DWORD)sleep_time);
#endif
}

/* process the 'pause' command */
#define EAT_INPUT_WITH(slurp) do {int junk = 0; do {junk = slurp; } while(junk != EOF && junk != '\n'); } while(0)

#ifdef WIN32
uint enctocodepage(enum set_encoding_id enc)
{
	switch(enc) {
		case S_ENC_CP437:  return 437;
		case S_ENC_CP850:  return 850;
		case S_ENC_CP852:  return 852;
		case S_ENC_SJIS:   return 932;
		case S_ENC_CP950:  return 950;
		case S_ENC_CP1250: return 1250;
		case S_ENC_CP1251: return 1251;
		case S_ENC_CP1252: return 1252;
		case S_ENC_CP1254: return 1254;
		case S_ENC_KOI8_R: return 20866;
		case S_ENC_KOI8_U: return 21866;
		case S_ENC_ISO8859_1:  return 28591;
		case S_ENC_ISO8859_2:  return 28592;
		case S_ENC_ISO8859_9:  return 28599;
		case S_ENC_ISO8859_15: return 28605;
		case S_ENC_UTF8: return 65001;
		default: return 0;
	}
}

/* mode == 0: => enc -> current locale (for output)
 * mode == !0: => current locale -> enc (for input)
 */
char * translate_string_encoding(char * str, int mode, enum set_encoding_id enc)
{
	char * lenc, * nstr, * locale;
	unsigned loccp, enccp, fromcp, tocp;
	int length;
	LPWSTR textw;
	if(enc == S_ENC_DEFAULT) return gp_strdup(str);
#ifdef WGP_CONSOLE
	if(mode == 0) loccp = GetConsoleOutputCP();  /* output codepage */
	else loccp = GetConsoleCP();  /* input code page */
#else
	locale = setlocale(LC_CTYPE, "");
	if(!(lenc = strchr(locale, '.')) || !sscanf(++lenc, "%i", &loccp))
		return gp_strdup(str);
#endif
	enccp = enctocodepage(enc);
	if(enccp == loccp)
		return gp_strdup(str);
	else {
		if(mode == 0) {
			fromcp = enccp;
			tocp = loccp;
		}
		else {
			fromcp = loccp;
			tocp = enccp;
		}
		length = MultiByteToWideChar(fromcp, 0, str, -1, NULL, 0);
		textw = (LPWSTR)malloc(sizeof(WCHAR) * length);
		MultiByteToWideChar(fromcp, 0, str, -1, textw, length);
		length = WideCharToMultiByte(tocp, 0, textw, -1, NULL, 0, NULL, NULL);
		nstr = (char*)malloc(length);
		WideCharToMultiByte(tocp, 0, textw, -1, nstr, length, NULL, NULL);
		free(textw);
		return nstr;
	}
}

#endif

void GpGadgets::PauseCommand(GpCommand & rC)
{
	int    text = 0;
	double sleep_time;
	static char * buf = NULL;

	rC.CToken++;

#ifdef USE_MOUSE
	paused_for_mouse = 0;
	if(rC.Eq("mouse")) {
		sleep_time = -1;
		rC.CToken++;
		// EAM FIXME - This is not the correct test; what we really want 
		// to know is whether or not the terminal supports mouse feedback 
		// if(term_initialised) { 
		if(Mse.Cfg.on && term) {
			UdvtEntry * current;
			int end_condition = 0;
			while(!(rC.EndOfCommand())) {
				if(rC.AlmostEq("key$press")) {
					end_condition |= PAUSE_KEYSTROKE;
					rC.CToken++;
				}
				else if(rC.Eq(",")) {
					rC.CToken++;
				}
				else if(rC.Eq("any")) {
					end_condition |= PAUSE_ANY;
					rC.CToken++;
				}
				else if(rC.Eq("button1")) {
					end_condition |= PAUSE_BUTTON1;
					rC.CToken++;
				}
				else if(rC.Eq("button2")) {
					end_condition |= PAUSE_BUTTON2;
					rC.CToken++;
				}
				else if(rC.Eq("button3")) {
					end_condition |= PAUSE_BUTTON3;
					rC.CToken++;
				}
				else if(rC.Eq("close")) {
					end_condition |= PAUSE_WINCLOSE;
					rC.CToken++;
				}
				else
					break;
			}
			paused_for_mouse = end_condition ? end_condition : PAUSE_CLICK;
			// Set the pause mouse return codes to -1
			current = Ev.AddUdvByName("MOUSE_KEY");
			current->udv_value.SetInt(-1);
			current = Ev.AddUdvByName("MOUSE_BUTTON");
			current->udv_value.SetInt(-1);
		}
		else
			IntWarn(NO_CARET, "Mousing not active");
	}
	else
#endif
	sleep_time = rC.RealExpression();
	if(rC.EndOfCommand()) {
		free(buf); /* remove the previous message */
		buf = gp_strdup("paused"); /* default message, used in Windows GUI pause dialog */
	}
	else {
		char * tmp = rC.TryToGetString();
		if(!tmp)
			IntErrorCurToken("expecting string");
		else {
#ifdef WIN32
			char * nbuf = translate_string_encoding(tmp, 0, encoding);
			free(tmp);
			free(buf);
			buf = nbuf;
			if(sleep_time >= 0)
				fputs(buf, stderr);
#else /* Not WIN32 or OS2 */
			free(buf);
			buf = tmp;
			fputs(buf, stderr);
#endif
			text = 1;
		}
	}

	if(sleep_time < 0) {
#if defined(WIN32)
		ctrlc_flag = false;
# if defined(WGP_CONSOLE) && defined(USE_MOUSE)
		if(!paused_for_mouse || !MousableWindowOpened()) {
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
# endif /* !WGP_CONSOLE */
		{
			if(!Pause(buf)) /* returns false if Ctrl-C or Cancel was pressed */
				bail_to_command_line();
		}
#else /* !(WIN32 || OS2) */
#ifdef USE_MOUSE
		if(term && term->waitforinput) {
			/* It does _not_ work to do EAT_INPUT_WITH(term->waitforinput()) */
			term->waitforinput(0);
		}
		else
#endif /* USE_MOUSE */
		EAT_INPUT_WITH(fgetc(stdin));

#endif /* !(WIN32 || OS2) */
	}
	if(sleep_time > 0)
		timed_pause(sleep_time);
	if(text != 0 && sleep_time >= 0)
		fputc('\n', stderr);
	screen_ok = false;
}
//
// process the 'plot' command
//
//void plot_command()
void GpGadgets::PlotCommand(GpCommand & rC)
{
	rC.PlotToken = rC.CToken++;
	GpDf.plotted_data_from_stdin = false;
	RefreshNPlots = 0;
	SET_CURSOR_WAIT;
#ifdef USE_MOUSE
	plot_mode(MODE_PLOT);
	Ev.AddUdvByName("MOUSE_X")->udv_value.type = NOTDEFINED;
	Ev.AddUdvByName("MOUSE_Y")->udv_value.type = NOTDEFINED;
	Ev.AddUdvByName("MOUSE_X2")->udv_value.type = NOTDEFINED;
	Ev.AddUdvByName("MOUSE_Y2")->udv_value.type = NOTDEFINED;
	Ev.AddUdvByName("MOUSE_BUTTON")->udv_value.type = NOTDEFINED;
	Ev.AddUdvByName("MOUSE_SHIFT")->udv_value.type = NOTDEFINED;
	Ev.AddUdvByName("MOUSE_ALT")->udv_value.type = NOTDEFINED;
	Ev.AddUdvByName("MOUSE_CTRL")->udv_value.type = NOTDEFINED;
#endif
	PlotRequest(/*rC*/);
	SET_CURSOR_ARROW;
}

//void print_set_output(char * name, bool datablock, bool append_p)
void GpCommand::PrintSetOutput(char * name, bool datablock, bool append_p)
{
	if(F_PrintOut && F_PrintOut != stderr && F_PrintOut != stdout) {
#ifdef PIPES
		if(P_PrintOutName[0] == '|') {
			if(0 > pclose(F_PrintOut))
				perror(P_PrintOutName);
		}
		else
#endif
		if(0 > fclose(F_PrintOut))
			perror(P_PrintOutName);
	}
	ZFREE(P_PrintOutName);
	if(!name) {
		F_PrintOut = stderr;
		P_PrintOutVar = NULL;
		return;
	}
	if(strcmp(name, "-") == 0) {
		F_PrintOut = stdout;
		return;
	}
#ifdef PIPES
	if(name[0] == '|') {
		restrict_popen();
		F_PrintOut = popen(name + 1, "w");
		if(!F_PrintOut)
			perror(name);
		else
			P_PrintOutName = name;
		return;
	}
#endif
	if(!datablock) {
		F_PrintOut = fopen(name, append_p ? "a" : "w");
		if(!F_PrintOut) {
			perror(name);
			return;
		}
	}
	else {
		P_PrintOutVar = R_Gg.Ev.AddUdvByName(name);
		if(P_PrintOutVar == NULL) {
			fprintf(stderr, "Error allocating datablock \"%s\"\n", name);
			return;
		}
		if(P_PrintOutVar->udv_value.type != NOTDEFINED) {
			gpfree_string(&P_PrintOutVar->udv_value);
			if(!append_p)
				gpfree_datablock(&P_PrintOutVar->udv_value);
			if(P_PrintOutVar->udv_value.type != DATABLOCK)
				P_PrintOutVar->udv_value.v.data_array = NULL;
		}
		else {
			P_PrintOutVar->udv_value.v.data_array = NULL;
		}
		P_PrintOutVar->udv_value.type = DATABLOCK;
	}
	P_PrintOutName = name;
}

//char * print_show_output()
char * GpCommand::PrintShowOutput()
{
	if(P_PrintOutName)
		return P_PrintOutName;
	else if(F_PrintOut == stdout)
		return "<stdout>";
	else if(!F_PrintOut || F_PrintOut == stderr || !P_PrintOutName)
		return "<stderr>";
	else
		return P_PrintOutName;
}
//
// process the 'print' command 
//
void print_command(GpCommand & rC)
{
	t_value a;
	// space printed between two expressions only
	bool need_space = false;
	char * dataline = NULL;
	size_t size = 256;
	size_t len = 0;
	SETIFZ(rC.F_PrintOut, stderr);
	if(rC.P_PrintOutVar != NULL) { // print to datablock 
		dataline = (char*)malloc(size);
		*dataline = NUL;
	}
	GpGg.screen_ok = false;
	do {
		++rC.CToken;
		if(rC.Eq("$") && rC.IsLetter(rC.CToken+1)) {
			char * datablock_name = rC.ParseDataBlockName();
			char ** line = get_datablock(datablock_name);
			// Printing a datablock into itself would cause infinite recursion
			if(rC.P_PrintOutVar && strcmp(datablock_name, rC.P_PrintOutName) == 0)
				continue;
			while(line && *line) {
				if(rC.P_PrintOutVar)
					append_to_datablock(&rC.P_PrintOutVar->udv_value, _strdup(*line));
				else
					fprintf(rC.F_PrintOut, "%s\n", *line);
				line++;
			}
			continue;
		}
		if(rC.TypeDdv(rC.CToken) == ARRAY && !rC.Eq(rC.CToken+1, "[")) {
			UdvtEntry * array = GpGg.Ev.AddUdv(rC, rC.CToken++);
			save_array_content(rC.F_PrintOut, array->udv_value.v.value_array);
			continue;
		}
		rC.ConstExpress(&a);
		if(a.type == STRING) {
			if(dataline != NULL)
				len = strappend(&dataline, &size, len, a.v.string_val);
			else
#ifdef WIN32
			if(rC.F_PrintOut == stderr) {
				char * nbuf = translate_string_encoding(a.v.string_val, 0, encoding);
				gpfree_string(&a);
				fputs(nbuf, rC.F_PrintOut);
				free(nbuf);
			}
			else
#endif
				fputs(a.v.string_val, rC.F_PrintOut);
			gpfree_string(&a);
			need_space = false;
		}
		else {
			if(need_space) {
				if(dataline != NULL)
					len = strappend(&dataline, &size, len, " ");
				else
					putc(' ', rC.F_PrintOut);
			}
			if(dataline != NULL)
				len = strappend(&dataline, &size, len, value_to_str(&a, false));
			else
				disp_value(rC.F_PrintOut, &a, false);
			need_space = true;
#ifdef ARRAY_COPY_ON_REFERENCE
			gpfree_string(&a); // Prevents memory leakage for ARRAY variables
#endif
		}
	} while(!rC.EndOfCommand() && rC.Eq(","));
	if(dataline) {
		append_to_datablock(&rC.P_PrintOutVar->udv_value, dataline);
	}
	else {
		putc('\n', rC.F_PrintOut);
		fflush(rC.F_PrintOut);
	}
}

/* process the 'pwd' command */
void pwd_command()
{
	char * save_file = (char*)malloc(PATH_MAX);
	if(save_file) {
		GP_GETCWD(save_file, PATH_MAX);
		fprintf(stderr, "%s\n", save_file);
		free(save_file);
	}
	GpGg.Gp__C.CToken++;
}

/* EAM April 2007
 * The "refresh" command replots the previous graph without going back to read
 * the original data. This allows zooming or other operations on data that was
 * only transiently available in the input stream.
 */
void refresh_command()
{
	GpGg.Gp__C.CToken++;
	GpGg.RefreshRequest();
}

//void refresh_request()
void GpGadgets::RefreshRequest()
{
	FPRINTF((stderr, "refresh_request\n"));
	if((!P_FirstPlot && (RefreshOk == E_REFRESH_OK_2D)) || (!P_First3DPlot && (RefreshOk == E_REFRESH_OK_3D))|| (!*GpGg.Gp__C.P_ReplotLine && (RefreshOk == E_REFRESH_NOT_OK)))
		GpGg.IntErrorNoCaret("no active plot; cannot refresh");
	if(RefreshOk == E_REFRESH_NOT_OK) {
		IntWarn(NO_CARET, "cannot refresh from this state. trying full replot");
		ReplotRequest(GpGg.Gp__C);
	}
	else {
		//
		// If the state has been reset to autoscale since the last plot, initialize the axis limits.
		//
		AxA[FIRST_X_AXIS].InitRefresh2D(true);
		AxA[FIRST_Y_AXIS].InitRefresh2D(true);
		AxA[SECOND_X_AXIS].InitRefresh2D(true);
		AxA[SECOND_Y_AXIS].InitRefresh2D(true);

		AxA[T_AXIS].UpdateRefresh2D(); // Untested: T and R want INIT2D or UPDATE2D?? 
		AxA[POLAR_AXIS].UpdateRefresh2D();
		AxA[FIRST_Z_AXIS].UpdateRefresh2D();
		AxA[COLOR_AXIS].UpdateRefresh2D();
#ifdef NONLINEAR_AXES
		// Nonlinear mapping of x or y via linkage to a hidden primary axis 
		if(AxA[FIRST_X_AXIS].P_LinkToPrmr) {
			GpAxis * primary = AxA[FIRST_X_AXIS].P_LinkToPrmr;
			primary->Range = primary->SetRange;
		}
		if(AxA[FIRST_Y_AXIS].P_LinkToPrmr) {
			GpAxis * primary = AxA[FIRST_Y_AXIS].P_LinkToPrmr;
			primary->Range = primary->SetRange;
		}
#endif
		if(RefreshOk == E_REFRESH_OK_2D) {
			RefreshBounds(term, P_FirstPlot, RefreshNPlots);
			DoPlot(P_FirstPlot, RefreshNPlots);
			GpGg.Ev.UpdateGpValVariables(1);
		}
		else if(RefreshOk == E_REFRESH_OK_3D) {
			Refresh3DBounds(P_First3DPlot, RefreshNPlots);
			Do3DPlot(term, P_First3DPlot, RefreshNPlots, 0);
			GpGg.Ev.UpdateGpValVariables(1);
		}
		else
			GpGg.IntErrorNoCaret("Internal error - refresh of unknown plot type");
	}
}
//
// process the 'replot' command 
//
//void replot_command()
void GpGadgets::ReplotCommand(GpTermEntry * pT, GpCommand & rC)
{
	if(!*rC.P_ReplotLine)
		IntErrorCurToken("no previous plot");
	if(IsVolatileData && (RefreshOk != E_REFRESH_NOT_OK) && !rC.IsReplotDisabled) {
		FPRINTF((stderr, "volatile_data %d RefreshOk %d plotted_data_from_stdin %d\n", IsVolatileData, RefreshOk, plotted_data_from_stdin));
		//refresh_command();
		//void refresh_command()
		{
			rC.CToken++;
			RefreshRequest();
		}
	}
	else {
		// Disable replot for some reason; currently used by the mouse/hotkey
		// capable terminals to avoid replotting when some data come from stdin,
		// i.e. when  plotted_data_from_stdin==1  after plot "-".
		if(rC.IsReplotDisabled) {
			rC.IsReplotDisabled = false;
			bail_to_command_line(); /* be silent --- don't mess the screen */
		}
		if(!pT) // unknown terminal
			IntErrorCurToken("use 'set pT' to set terminal type first");
		rC.CToken++;
		SET_CURSOR_WAIT;
		if(pT->flags & TERM_INIT_ON_REPLOT)
			pT->init();
		ReplotRequest(rC);
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
	GpGg.Gp__C.CToken++;
}
//
// process the 'save' command 
//
void GpGadgets::SaveCommand(GpCommand & rC)
{
	FILE * fp;
	char * save_file = NULL;
	int what;
	rC.CToken++;
	what = rC.LookupTable(&save_tbl[0], rC.CToken);
	switch(what) {
		case SAVE_FUNCS:
		case SAVE_SET:
		case SAVE_TERMINAL:
		case SAVE_VARS:
		    rC.CToken++;
		    break;
		default:
		    break;
	}
	save_file = rC.TryToGetString();
	if(!save_file)
		IntErrorCurToken("expecting filename");
#ifdef PIPES
	if(save_file[0]=='|') {
		restrict_popen();
		fp = popen(save_file+1, "w");
	}
	else
#endif
	{
		gp_expand_tilde(&save_file);
		fp = strcmp(save_file, "-") ? loadpath_fopen(save_file, "w") : stdout;
	}
	if(!fp)
		os_error(rC.CToken, "Cannot open save file");
	switch(what) {
		case SAVE_FUNCS:
		    SaveFunctions(rC, fp);
		    break;
		case SAVE_SET:
		    SaveSet(rC, fp);
		    break;
		case SAVE_TERMINAL:
		    save_term(fp);
		    break;
		case SAVE_VARS:
		    SaveVariables(rC, fp);
		    break;
		default:
		    SaveAll(rC, fp);
	}
	if(stdout != fp) {
#ifdef PIPES
		if(save_file[0] == '|')
			(void)pclose(fp);
		else
#endif
		fclose(fp);
	}
	free(save_file);
}

/* process the 'screendump' command */
void screendump_command()
{
	GpGg.Gp__C.CToken++;
#ifdef _Windows
	screen_dump();
#else
	fputs("screendump not implemented\n", stderr);
#endif
}
//
// process the 'stats' command 
//
void stats_command()
{
#ifdef USE_STATS
	GpGg.StatsRequest(GpGg.Gp__C);
#else
	GpGg.IntErrorNoCaret("This copy of gnuplot was not configured with support for the stats command");
#endif
}

// process the 'system' command 
void system_command()
{
	++GpGg.Gp__C.CToken;
	char * cmd = GpGg.Gp__C.TryToGetString();
	do_system(cmd);
	free(cmd);
}

/*
 * process the 'test palette' command
 * 1) Write a sequence of plot commands + set commands into a temp file
 * 2) Create a datablock with palette values
 * 3) Load the temp file to plot from the datablock
 *    The set commands then act to restore the initial state
 */
//static void test_palette_subcommand()
void GpGadgets::TestPaletteSubcommand(GpCommand & rC)
{
	enum {test_palette_colors = 256};

	UdvtEntry * datablock;

	char * save_replot_line;
	bool save_is_3d_plot;
	int i;

	static const char pre1[] = "reset;\
uns border; se tics scale 0;\
se cbtic 0,0.1,1 mirr format '' scale 1;\
se xr[0:1];se yr[0:1];se zr[0:1];se cbr[0:1];\
set colorbox hor user orig 0.05,0.02 size 0.925,0.12;"                                                                                                                                              ;

	static const char pre2[] =
	    "\
se lmarg scre 0.05;se rmarg scre 0.975; se bmarg scre 0.22; se tmarg scre 0.86;\
se grid; se xtics 0,0.1;se ytics 0,0.1;\
se key top right at scre 0.975,0.975 horizontal \
title 'R,G,B profiles of the current color palette';"                                                                                                                                                                                           ;

	static const char pre3[] =
	    "\
p NaN lc palette notit,\
$PALETTE u 1:2 t 'red' w l lt 1 lc rgb 'red',\
'' u 1:3 t 'green' w l lt 1 lc rgb 'green',\
'' u 1:4 t 'blue' w l lt 1 lc rgb 'blue',\
'' u 1:5 t 'NTSC' w l lt 1 lc rgb 'black'\
\n"                                                                                                                                                                                                                          ;

	FILE * f = tmpfile();

#if defined(_MSC_VER) || defined(__MINGW32__)
	/* On Vista/Windows 7 tmpfile() fails. */
	if(!f) {
		char buf[PATH_MAX];
		GetTempPath(sizeof(buf), buf);
		strcat(buf, "gnuplot-pal.tmp");
		f = fopen(buf, "w+");
	}
#endif

	while(!rC.EndOfCommand())
		rC.CToken++;
	if(!f)
		GpGg.IntErrorNoCaret("cannot write temporary file");

	/* Store R/G/B/Int curves in a datablock */
	datablock = Ev.AddUdvByName("$PALETTE");
	if(datablock->udv_value.type != NOTDEFINED)
		gpfree_datablock(&datablock->udv_value);
	datablock->udv_value.type = DATABLOCK;
	datablock->udv_value.v.data_array = NULL;

	/* Part of the purpose for writing these values into a datablock */
	/* is so that the user can read them back if desired.  But data  */
	/* will be read back using the current numeric locale, so for    */
	/* consistency we must also use the locale when creating it.     */
	set_numeric_locale();
	for(i = 0; i < test_palette_colors; i++) {
		char dataline[64];
		rgb_color rgb;
		double ntsc;
		double z = (double)i / (test_palette_colors - 1);
		double gray = (SmPalette.positive == SMPAL_NEGATIVE) ? (1.0 - z) : z;
		RGB1FromGray(gray, &rgb);
		ntsc = 0.299 * rgb.R + 0.587 * rgb.G + 0.114 * rgb.B;
		sprintf(dataline, "%0.4f %0.4f %0.4f %0.4f %0.4f %c", z, rgb.R, rgb.G, rgb.B, ntsc, '\0');
		append_to_datablock(&datablock->udv_value, _strdup(dataline));
	}
	reset_numeric_locale();

	/* commands to setup the test palette plot */
	enable_reset_palette = 0;
	save_replot_line = gp_strdup(rC.P_ReplotLine);
	save_is_3d_plot = Is3DPlot;
	fputs(pre1, f);
	fputs(pre2, f);
	fputs(pre3, f);
	// save current gnuplot 'set' status because of the tricky sets
	// for our temporary testing plot.
	SaveSet(rC, f);
	// execute all commands from the temporary file
	rewind(f);
	LoadFile(f, NULL, 1); // note: it does fclose(f)
	// enable reset_palette() and restore replot line
	enable_reset_palette = 1;
	free(rC.P_ReplotLine);
	rC.P_ReplotLine = save_replot_line;
	Is3DPlot = save_is_3d_plot;
}
//
// process the 'test' command 
//
//void test_command()
void GpGadgets::TestCommand(GpCommand & rC)
{
	int what;
	int save_token = rC.CToken++;
	if(!term) /* unknown terminal */
		IntErrorCurToken("use 'set term' to set terminal type first");
	what = rC.LookupTable(&test_tbl[0], rC.CToken);
	switch(what) {
		default:
		    if(!rC.EndOfCommand())
			    IntErrorCurToken("unrecognized test option");
		// otherwise fall through to test_term 
		case TEST_TERMINAL: TestTerm(term, rC); break;
		case TEST_PALETTE: TestPaletteSubcommand(rC); break;
	}
	// prevent annoying error messages if there was no previous plot
	// and the "test" window is resized.
	if(!rC.P_ReplotLine || !(*rC.P_ReplotLine)) {
		rC.MCapture(&rC.P_ReplotLine, save_token, rC.CToken);
	}
}
//
// toggle a single plot on/off from the command line
// (only possible for qt, wxt, x11, win)
//
//void toggle_command()
void GpGadgets::ToggleCommand(GpTermEntry * pT, GpCommand & rC)
{
	int plotno = -1;
	char * plottitle = NULL;
	bool foundit = false;
	rC.CToken++;
	if(rC.Eq("all")) {
		rC.CToken++;
	}
	else if((plottitle = rC.TryToGetString()) != NULL) {
		CurvePoints * plot;
		int length = strlen(plottitle);
		if(RefreshOk == E_REFRESH_OK_2D)
			plot = P_FirstPlot;
		else if(RefreshOk == E_REFRESH_OK_3D)
			plot = (CurvePoints *)P_First3DPlot;
		else
			plot = NULL;
		for(plotno = 0; plot; plot = plot->P_Next, plotno++) {
			if(plot->title)
				if(!strcmp(plot->title, plottitle) || (plottitle[length-1] == '*' && !strncmp(plot->title, plottitle, length-1))) {
					foundit = true;
					break;
				}
		}
		free(plottitle);
		if(!foundit) {
			IntWarn(NO_CARET, "Did not find a plot with that title");
			return;
		}
	}
	else {
		plotno = rC.IntExpression() - 1;
	}
	if(pT->modify_plots)
		pT->modify_plots(MODPLOTS_INVERT_VISIBILITIES, plotno);
}
//
// the "import" command is only implemented if support is configured for 
// using functions from external shared objects as plugins. 
//
//void import_command()
void GpCommand::ImportCommand()
{
	int start_token = CToken;
#ifdef HAVE_EXTERNAL_FUNCTIONS
	UdftEntry * udf;
	int dummy_num = 0;
	char save_dummy[MAX_NUM_VAR][MAX_ID_LEN+1];
	if(!Eq(++CToken + 1, "("))
		GpGg.IntErrorCurToken("Expecting function template");
	memcpy(save_dummy, P.CDummyVar, sizeof(save_dummy));
	do {
		CToken += 2; /* skip to the next dummy */
		CopyStr(P.CDummyVar[dummy_num++], CToken, MAX_ID_LEN);
	} while(Eq(CToken + 1, ",") && (dummy_num < MAX_NUM_VAR));
	if(Eq(++CToken, ","))
		GpGg.IntError(CToken + 1, "function contains too many parameters");
	if(!Eq(CToken++, ")"))
		GpGg.IntErrorCurToken("missing ')'");
	if(!Eq("from"))
		GpGg.IntErrorCurToken("Expecting 'from <sharedobj>'");
	CToken++;
	udf = P_DummyFunc = GpGg.Ev.AddUdf(*this, start_token+1);
	udf->dummy_num = dummy_num;
	AtType::Destroy(udf->at); /* In case there was a previous function by this name */
	udf->at = ExternalAt(udf->udf_name);
	memcpy(P.CDummyVar, save_dummy, sizeof(save_dummy));
	P_DummyFunc = NULL; /* dont let anyone else use our workspace */
	if(!udf->at)
		GpGg.IntErrorNoCaret("failed to load external function");
	// Don't copy the definition until we know it worked
	MCapture(&(udf->definition), start_token, CToken - 1);
#else
	while(!EndOfCommand())
		CToken++;
	GpGg.IntError(start_token, "This copy of gnuplot does not support plugins");
#endif
}

/* process invalid commands and, on OS/2, REXX commands */
void invalid_command()
{
	int save_token = GpGg.Gp__C.CToken;
	/* Skip the rest of the command; otherwise we're left pointing to */
	/* the middle of a command we already know is not valid.          */
	while(!GpGg.Gp__C.EndOfCommand())
		GpGg.Gp__C.CToken++;
	GpGg.IntError(save_token, "invalid command");
}

/*
 * Auxiliary routines
 */

// used by changedir_command() 
static int changedir(char * path)
{
#if defined(_WIN32)
	return !(SetCurrentDirectory(path));
#else
	return chdir(path);
#endif
}
//
// used by replot_command()
//
//void replotrequest()
void GpGadgets::ReplotRequest(GpCommand & rC)
{
	/* do not store directly into the replot_line string until the
	 * new plot line has been successfully plotted. This way,
	 * if user makes a typo in a replot line, they do not have
	 * to start from scratch. The replot_line will be committed
	 * after do_plot has returned, whence we know all is well
	 */
	if(rC.EndOfCommand()) {
		char * rest_args = &rC.P_InputLine[rC.P_Token[rC.CToken].start_index];
		size_t replot_len = strlen(rC.P_ReplotLine);
		size_t rest_len = strlen(rest_args);
		// preserve commands following 'replot ;'
		// move rest of input line to the start
		// necessary because of realloc() in extend_input_line()
		memmove(rC.P_InputLine, rest_args, rest_len+1);
		// reallocs if necessary
		while(rC.InputLineLen < replot_len+rest_len+1)
			rC.ExtendInputLine();
		// move old rest args off begin of input line to make space for replot_line
		memmove(rC.P_InputLine+replot_len, rC.P_InputLine, rest_len+1);
		// copy previous plot command to start of input line
		memcpy(rC.P_InputLine, rC.P_ReplotLine, replot_len);
	}
	else {
		char * replot_args = NULL; /* else m_capture will free it */
		int last_token = rC.NumTokens - 1;
		/* length = length of old part + length of new part + ", " + \0 */
		size_t newlen = strlen(rC.P_ReplotLine) + rC.P_Token[last_token].start_index + rC.P_Token[last_token].length - rC.P_Token[rC.CToken].start_index + 3;
		rC.MCapture(&replot_args, rC.CToken, last_token); /* might be empty */
		while(rC.InputLineLen < newlen)
			rC.ExtendInputLine();
		strcpy(rC.P_InputLine, rC.P_ReplotLine);
		strcat(rC.P_InputLine, ", ");
		strcat(rC.P_InputLine, replot_args);
		free(replot_args);
	}
	rC.PlotToken = 0; // whole line to be saved as replot line
	SetRefreshOk(E_REFRESH_NOT_OK, 0); // start of replot will destory existing data
	screen_ok = false;
	rC.NumTokens = rC.Scanner(&rC.P_InputLine, &rC.InputLineLen);
	rC.CToken = 1;    /* Skip the "plot" token */
	if(rC.AlmostEq(0, "test")) {
		rC.CToken = 0;
		TestCommand(rC);
	}
	else if(rC.AlmostEq(0, "s$plot"))
		Plot3DRequest(rC);
	else
		PlotRequest(/*rC*/);
}
//
// This routine is called at the beginning of 'splot'. It sets up some splot
// parameters needed to present the 'set view map'.
//
//void splot_map_activate()
void GpGadgets::SplotMapActivate(GpCommand & rC)
{
	if(!rC.splot_map_active) {
		rC.splot_map_active = 1;
		// save current values
		rC.splot_map_surface_rot_x = surface_rot_x;
		rC.splot_map_surface_rot_z = surface_rot_z;
		rC.splot_map_surface_scale = surface_scale;
		// set new values
		surface_rot_x = 180;
		surface_rot_z = 0;
		// version 4 had constant value surface_scale = 1.3
		surface_scale = 1.425f * mapview_scale;
		// The Y axis runs backwards from a normal 2D plot
		Exchange(&AxA[FIRST_Y_AXIS].Range.low, &AxA[FIRST_Y_AXIS].Range.upp);
	}
}
//
// This routine is called at the end of 3D plot evaluation to undo the
// changes needed for 'set view map'.
//
//void splot_map_deactivate()
void GpGadgets::SplotMapDeactivate(GpCommand & rC)
{
	if(rC.splot_map_active) {
		rC.splot_map_active = 0;
		// restore the original values
		surface_rot_x = rC.splot_map_surface_rot_x;
		surface_rot_z = rC.splot_map_surface_rot_z;
		surface_scale = rC.splot_map_surface_scale;
		// The Y axis runs backwards from a normal 2D plot
		Exchange(&AxA[FIRST_Y_AXIS].Range.low, &AxA[FIRST_Y_AXIS].Range.upp);
	}
}

/* Support for input, shell, and help for various systems */

#ifdef VMS

# include <descrip.h>
# include <rmsdef.h>
# include <smgdef.h>
# include <smgmsg.h>
# include <ssdef.h>

extern lib$get_input(), lib$put_output();
extern smg$read_composed_line();
extern sys$putmsg();
extern lbr$output_help();
extern lib$spawn();

int vms_len;

uint status[2] = { 1, 0 };

static char Help[MAX_LINE_LEN+1] = "gnuplot";

$DESCRIPTOR(prompt_desc, PROMPT);
/* temporary fix until change to variable length */
struct dsc$descriptor_s line_desc = {0, DSC$K_DTYPE_T, DSC$K_CLASS_S, NULL};

$DESCRIPTOR(help_desc, Help);
$DESCRIPTOR(helpfile_desc, "GNUPLOT$HELP");

/* HBB 990829: confirmed this to be used on VMS, only --> moved into
 * the VMS-specific section */
void done(int status)
{
	term_reset();
	gp_exit(status);
}

// VMS-only version of read_line 
int GpCommand::ReadLine(const char * prompt, int start)
{
	int more;
	char expand_prompt[40];
	current_prompt = prompt; /* HBB NEW 20040727 */
	prompt_desc.dsc$w_length = strlen(prompt);
	prompt_desc.dsc$a_pointer = (char*)prompt;
	strcpy(expand_prompt, "_");
	strncat(expand_prompt, prompt, sizeof(expand_prompt) - 2);
	do {
		line_desc.dsc$w_length = MAX_LINE_LEN - start;
		line_desc.dsc$a_pointer = &P_InputLine[start];
		switch(status[1] = smg$read_composed_line(&vms_vkid, &vms_ktid, &line_desc, &prompt_desc, &vms_len)) {
			case SMG$_EOF:
			    done(EXIT_SUCCESS); /* ^Z isn't really an error */
			    break;
			case RMS$_TNS: /* didn't press return in time */
			    vms_len--; /* skip the last character */
			    break; /* and parse anyway */
			case RMS$_BES: /* Bad Escape Sequence */
			case RMS$_PES: /* Partial Escape Sequence */
			    sys$putmsg(status);
			    vms_len = 0; /* ignore the line */
			    break;
			case SS$_NORMAL:
			    break; /* everything's fine */
			default:
			    done(status[1]); /* give the error message */
		}
		start += vms_len;
		P_InputLine[start] = NUL;
		inline_num++;
		if(P_InputLine[start - 1] == '\\') {
			/* Allow for a continuation line. */
			prompt_desc.dsc$w_length = strlen(expand_prompt);
			prompt_desc.dsc$a_pointer = expand_prompt;
			more = 1;
			--start;
		}
		else {
			line_desc.dsc$w_length = strlen(P_InputLine);
			line_desc.dsc$a_pointer = P_InputLine;
			more = 0;
		}
	} while(more);
	return 0;
}

#ifdef NO_GIH
void help_command(GpCommand & rC)
{
	int first = rC.CToken;
	while(!rC.EndOfCommand())
		++rC.CToken;
	strcpy(Help, "GNUPLOT ");
	rC.Capture(Help + 8, first, rC.CToken - 1, sizeof(Help) - 9);
	help_desc.dsc$w_length = strlen(Help);
	if((vaxc$errno = lbr$output_help(lib$put_output, 0, &help_desc, &helpfile_desc, 0, lib$get_input)) != SS$_NORMAL)
		os_error(NO_CARET, "can't open GNUPLOT$HELP");
}
#endif /* NO_GIH */

void do_shell()
{
	screen_ok = false;
	GpGg.Gp__C.CToken++;
	if((vaxc$errno = lib$spawn()) != SS$_NORMAL) {
		os_error(NO_CARET, "spawn error");
	}
}

static void do_system(const char * cmd)
{
	if(cmd){
		/* GpGg.Gp__C.P_InputLine is filled by read_line or load_file, but
		* line_desc length is set only by read_line; adjust now
		*/
		line_desc.dsc$w_length = strlen(cmd);
		line_desc.dsc$a_pointer = (char*)cmd;
		if((vaxc$errno = lib$spawn(&line_desc)) != SS$_NORMAL)
			os_error(NO_CARET, "spawn error");
		putc('\n', stderr);
	}
}

#endif /* VMS */

#ifdef NO_GIH
#if defined(_Windows)

#pragma comment (lib, "htmlhelp.lib")

void help_command(GpCommand & rC)
{
	rC.CToken++;
	HWND   parent = GetDesktopWindow();
	/* open help file if necessary */
	help_window = HtmlHelp(parent, winhelpname, HH_GET_WIN_HANDLE, (DWORD_PTR)NULL);
	if(help_window == NULL) {
		help_window = HtmlHelp(parent, winhelpname, HH_DISPLAY_TOPIC, (DWORD_PTR)NULL);
		if(help_window == NULL) {
			fprintf(stderr, "Error: Could not open help file \"%s\"\n", winhelpname);
			return;
		}
	}
	if(rC.EndOfCommand()) {
		/* show table of contents */
		HtmlHelp(parent, winhelpname, HH_DISPLAY_TOC, (DWORD_PTR)NULL);
	}
	else {
		/* lookup topic in index */
		HH_AKLINK link;
		char buf[128];
		int start = rC.CToken;
		while(!(rC.EndOfCommand()))
			rC.CToken++;
		rC.Capture(buf, start, rC.CToken - 1, 128);
		link.cbStruct =     sizeof(HH_AKLINK);
		link.fReserved =    false;
		link.pszKeywords =  buf;
		link.pszUrl =       NULL;
		link.pszMsgText =   NULL;
		link.pszMsgTitle =  NULL;
		link.pszWindow =    NULL;
		link.fIndexOnFail = true;
		HtmlHelp(parent, winhelpname, HH_KEYWORD_LOOKUP, (DWORD_PTR)&link);
	}
}

#else  /* !_Windows */
#ifndef VMS
void help_command(GpCommand & rC)
{
	while(!(rC.EndOfCommand()))
		rC.CToken++;
	fputs("This gnuplot was not built with inline help\n", stderr);
}

#endif /* VMS */
#endif /* _Windows */
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
 * FIXME - helpbuf is never free()'d
 */

#ifndef NO_GIH
void help_command(GpCommand & rC)
{
	static char * helpbuf = NULL;
	static char * prompt = NULL;
	static int toplevel = 1;
	int base;               /* index of first char AFTER help string */
	int len;                /* length of current help string */
	bool more_help;
	bool only;          /* true if only printing subtopics */
	bool subtopics;     /* 0 if no subtopics for this topic */
	int start;              /* starting token of help string */
	char * help_ptr;        /* name of help file */
# if defined(SHELFIND)
	static char help_fname[256] = ""; /* keep helpfilename across calls */
# endif

	if((help_ptr = getenv("GNUHELP")) == (char*)NULL)
# ifndef SHELFIND
	/* if can't find environment variable then just use HELPFILE */

/* patch by David J. Liu for getting GNUHELP from home directory */
#ifdef __DJGPP__
		help_ptr = HelpFile;
#else
		help_ptr = HELPFILE;
#endif
/* end of patch  - DJL */

# else /* !SHELFIND */
		/* try whether we can find the helpfile via shell_find. If not, just
		   use the default. (tnx Andreas) */
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
# endif                         /* !SHELFIND */

	/* Since MSDOS DGROUP segment is being overflowed we can not allow such  */
	/* huge static variables (1k each). Instead we dynamically allocate them */
	/* on the first call to this function...                                 */
	if(helpbuf == NULL) {
		helpbuf = malloc(MAX_LINE_LEN);
		prompt = malloc(MAX_LINE_LEN);
		helpbuf[0] = prompt[0] = 0;
	}
	if(toplevel)
		helpbuf[0] = prompt[0] = 0;  /* in case user hit ^c last time */

	/* if called recursively, toplevel == 0; toplevel must == 1 if called
	 * from command() to get the same behaviour as before when toplevel
	 * supplied as function argument
	 */
	toplevel = 1;
	len = base = strlen(helpbuf);
	start = ++rC.CToken;
	/* find the end of the help command */
	while(!(rC.EndOfCommand()))
		rC.CToken++;
	/* copy new help input into helpbuf */
	if(len > 0)
		helpbuf[len++] = ' ';  /* add a space */
	rC.Capture(helpbuf + len, start, rC.CToken - 1, MAX_LINE_LEN - len);
	squash_spaces(helpbuf + base);  /* only bother with new stuff */
	len = strlen(helpbuf);

	/* now, a lone ? will print subtopics only */
	if(strcmp(helpbuf + (base ? base + 1 : 0), "?") == 0) {
		/* subtopics only */
		subtopics = 1;
		only = true;
		helpbuf[base] = NUL; /* cut off question mark */
	}
	else {
		/* normal help request */
		subtopics = 0;
		only = false;
	}

	switch(help(helpbuf, help_ptr, &subtopics)) {
		case H_FOUND: {
		    /* already printed the help info */
		    /* subtopics now is true if there were any subtopics */
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
				    rC.ReadLine(prompt, 0);
				    rC.NumTokens = rC.Scanner(&rC.P_InputLine, &rC.InputLineLen);
				    rC.CToken = 0;
				    more_help = !(rC.EndOfCommand());
				    if(more_help) {
					    rC.CToken--;
					    toplevel = 0;
					    // base for next level is all of current helpbuf 
					    help_command(rC);
				    }
			    }
			    else
				    more_help = false;
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
		    GpGg.IntErrorNoCaret("Impossible case in switch");
		    break;
	}

	helpbuf[base] = NUL;    /* cut it off where we started */
}

#endif /* !NO_GIH */

#ifndef VMS

static void do_system(const char * cmd)
{
/* (am, 19980929)
 * OS/2 related note: cmd.exe returns 255 if called w/o argument.
 * i.e. calling a shell by "!" will always end with an error message.
 * A workaround has to include checking for EMX,OS/2, two environment
 *  variables,...
 */
	if(cmd) {
		restrict_popen();
		system(cmd);
	}
}

#if defined(READLINE) || defined(HAVE_LIBREADLINE) || defined(HAVE_LIBEDITLINE)
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
	/* skip leading whitespace */
	while(oneof2(line[start], ' ', '\t'))
		start++;
	/* find end of "token" */
	while(line[start + length] != NUL && !oneof2(line[start + length], ' ', '\t'))
		length++;
	for(i = 0; i < length + after; i++) {
		if(str[i] != line[start + i]) {
			if(str[i] != '$')
				return false;
			else {
				after = 1;
				start--; /* back up token ptr */
			}
		}
	}
	// i now beyond end of token string
	return (after || str[i] == '$' || str[i] == NUL);
}

char * GpCommand::RlGets(char * s, size_t n, const char * prompt)
{
	static char * line = (char*)NULL;
	static int leftover = -1; /* index of 1st char leftover from last call */
	if(leftover == -1) {
		ZFREE(line);
		line = ReadLine_ipc(GpGg.IsInteractive ? prompt : "");
		leftover = 0;
		/* If it's not an EOF */
		if(line && *line) {
#  if defined(HAVE_LIBREADLINE)
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
							free(removed->line);
							free(removed->data);
							free(removed);
						}
					}
				}
				add_history(line);
			}
#elif defined(HAVE_LIBEDITLINE)
			if(!is_history_command(line)) {
				// deleting history entries does not work, so suppress adjacent duplicates only 
				int found = 0;
				using_history();
				if(!history_full)
					found = history_search(line, -1);
				if(found <= 0)
					add_history(line);
			}
#else // builtin readline 
			if(!is_history_command(line))
				H.AddHistory(line);
#endif
		}
	}
	if(line) {
		/* s will be NUL-terminated here */
		strnzcpy(s, line + leftover, n);
		leftover += strlen(s);
		if(line[leftover] == NUL)
			leftover = -1;
		return s;
	}
	return NULL;
}

#endif                         /* READLINE || HAVE_LIBREADLINE || HAVE_LIBEDITLINE */

#if defined(_Windows)
void do_shell()
{
	GpGg.screen_ok = false;
	GpGg.Gp__C.CToken++;
	if(GpGg.user_shell) {
#if defined(_Windows)
		if(WinExec(GpGg.user_shell, SW_SHOWNORMAL) <= 32)
#else
		if(spawnl(P_WAIT, user_shell, NULL) == -1)
#endif
			os_error(NO_CARET, "unable to spawn shell");
	}
}
#else /* !OS2 */

/* plain old Unix */

#define EXEC "exec "
void do_shell()
{
	static char exec[100] = EXEC;
	screen_ok = false;
	GpGg.Gp__C.CToken++;
	if(user_shell) {
		if(system(strnzcpy(&exec[sizeof(EXEC) - 1], user_shell, sizeof(exec) - sizeof(EXEC) - 1)))
			os_error(NO_CARET, "system() failed");
	}
	putc('\n', stderr);
}
#endif // !MSDOS 

// read from stdin, everything except VMS 

#if !defined(READLINE) && !defined(HAVE_LIBREADLINE) && !defined(HAVE_LIBEDITLINE)
	#define PUT_STRING(s) fputs(s, stderr)
	#define GET_STRING(s, l) fgets(s, l, stdin)
#endif // !READLINE && !HAVE_LIBREADLINE && !HAVE_LIBEDITLINE 

/* this function is called for non-interactive operation. Its usage is
 * like fgets(), but additionally it checks for ipc events from the
 * terminals waitforinput() (if USE_MOUSE, and terminal is
 * mouseable). This function will be used when reading from a pipe.
 * fgets() reads in at most one less than size characters from stream and
 * stores them into the buffer pointed to by s.
 * Reading stops after an EOF or a newline.  If a newline is read, it is
 * stored into the buffer.  A '\0' is stored  after the last character in
 * the buffer. */
static char * fgets_ipc(GpTermEntry * pT, char * dest /* string to fill */, int len /* size of it */)
{
#ifdef USE_MOUSE
	if(pT && pT->waitforinput) {
		/* This a mouseable terminal --- must expect input from it */
		int    i = 0;
		dest[0] = '\0';
		for(i = 0; i < len-1; i++) {
			const int c = pT->waitforinput(0);
			if('\n' == c) {
				dest[i] = '\n';
				i++;
				break;
			}
			else if(EOF == c) {
				dest[i] = '\0';
				return (char*)0;
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
//
// get a line from stdin, and display a prompt if interactive 
//
static char * gp_get_string(char * buffer, size_t len, const char * prompt)
{
# if defined(READLINE) || defined(HAVE_LIBREADLINE) || defined(HAVE_LIBEDITLINE)
	return GpGg.IsInteractive ? GpGg.Gp__C.RlGets(buffer, len, prompt) : fgets_ipc(term, buffer, len);
# else
	if(GpGg.IsInteractive)
		PUT_STRING(prompt);
	return GET_STRING(buffer, len);
# endif
}

/* Non-VMS version */
//static int read_line(const char * prompt, int start)
int GpCommand::ReadLine(const char * prompt, int start)
{
	bool more = false;
	int last = 0;
	current_prompt = prompt;
	/* Once we start to read a new line, the tokens pointing into the old */
	/* line are no longer valid.  We used to _not_ clear things here, but */
	/* that lead to errors when a mouse-triggered replot request came in  */
	/* while a new line was being read.   Bug 3602388 Feb 2013.           */
	/* FIXME: If this causes problems, push it down into fgets_ipc().     */
	if(start == 0) {
		CToken = NumTokens = 0;
		P_InputLine[0] = '\0';
	}
	do {
		// grab some input 
		if(gp_get_string(P_InputLine + start, InputLineLen - start, ((more) ? ">" : prompt)) == (char*)NULL) {
			// end-of-file 
			if(GpGg.IsInteractive)
				putc('\n', stderr);
			P_InputLine[start] = NUL;
			InlineNum++;
			if(start > 0 && curly_brace_count == 0) /* don't quit yet - process what we have */
				more = false;
			else
				return (1);  /* exit gnuplot */
		}
		else {
			/* normal line input */
			/* P_InputLine must be NUL-terminated for strlen not to pass the
			 * the bounds of this array */
			last = strlen(P_InputLine) - 1;
			if(last >= 0) {
				if(P_InputLine[last] == '\n') { // remove any newline
					P_InputLine[last] = NUL;
					if(last > 0 && P_InputLine[last-1] == '\r') {
						P_InputLine[--last] = NUL;
					}
					// Watch out that we don't backup beyond 0 (1-1-1)
					if(last > 0)
						--last;
				}
				else if((last + 2) >= (int)InputLineLen) {
					ExtendInputLine();
					// read rest of line, don't print "> "
					start = last + 1;
					more = true;
					continue;
					// else fall through to continuation handling
				}
				if(P_InputLine[last] == '\\') {
					// line continuation 
					start = last;
					more = true;
				}
				else
					more = false;
			}
			else
				more = false;
		}
	} while(more);
	return (0);
}

#endif /* !VMS */

/*
 * Walk through the input line looking for string variables preceded by @.
 * Replace the characters @<varname> with the contents of the string.
 * Anything inside quotes is not expanded.
 * Allow up to 3 levels of nested macros.
 */
//void string_expand_macros()
void GpCommand::StringExpandMacros()
{
	if(Expand1LevelMacros() && Expand1LevelMacros() && Expand1LevelMacros() && Expand1LevelMacros())
		GpGg.IntErrorNoCaret("Macros nested too deeply");
}

#define COPY_CHAR GpGg.Gp__C.P_InputLine[o++] = *c; after_backslash = false;

//int expand_1level_macros()
int GpCommand::Expand1LevelMacros()
{
	bool in_squote = false;
	bool in_dquote = false;
	bool after_backslash = false;
	bool in_comment = false;
	int len;
	int o = 0;
	int nfound = 0;
	char * c;
	char * temp_string;
	char temp_char;
	char * m;
	UdvtEntry * udv;
	// Most lines have no macros
	if(!strchr(P_InputLine, '@'))
		return(0);
	temp_string = (char *)malloc(InputLineLen);
	len = strlen(P_InputLine);
	if(len >= (int)InputLineLen)
		len = InputLineLen-1;
	strncpy(temp_string, P_InputLine, len);
	temp_string[len] = '\0';
	for(c = temp_string; len && c && *c; c++, len--) {
		switch(*c) {
			case '@': /* The only tricky bit */
			    if(!in_squote && !in_dquote && !in_comment && isalpha((uchar)c[1])) {
				    /* Isolate the udv key as a null-terminated substring */
				    m = ++c;
				    while(isalnum((uchar )*c) || (*c=='_'))
						c++;
				    temp_char = *c; *c = '\0';
				    /* Look up the key and restore the original following char */
				    udv = GpGg.Ev.GetUdvByName(m);
				    if(udv && udv->udv_value.type == STRING) {
					    nfound++;
					    m = udv->udv_value.v.string_val;
					    FPRINTF((stderr, "Replacing @%s with \"%s\"\n", udv->udv_name, m));
					    while((strlen(m) + o + len) > InputLineLen)
						    ExtendInputLine();
					    while(*m)
						    P_InputLine[o++] = (*m++);
				    }
				    else {
					    GpGg.IntWarn(NO_CARET, "%s is not a string variable", m);
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
			    P_InputLine[o++] = *c; break;
			case '#':
			    if(!in_squote && !in_dquote)
				    in_comment = true;
			default:
			    COPY_CHAR; break;
		}
	}
	P_InputLine[o] = '\0';
	free(temp_string);
	if(nfound)
		FPRINTF((stderr, "After string substitution command line is:\n\t%s\n", P_InputLine));
	return(nfound);
}

/* much more than what can be useful */
#define MAX_TOTAL_LINE_LEN (1024 * MAX_LINE_LEN)

int do_system_func(const char * cmd, char ** output)
{
#if defined(VMS) || defined(PIPES)
	int c;
	FILE * f;
	int result_allocated, result_pos;
	char* result;
	int ierr = 0;
# if defined(VMS)
	int chan, one = 1;
	struct dsc$descriptor_s pgmdsc = {0, DSC$K_DTYPE_T, DSC$K_CLASS_S, 0};
	static $DESCRIPTOR(lognamedsc, "PLOT$MAILBOX");
# endif /* VMS */

	/* open stream */
# ifdef VMS
	pgmdsc.dsc$a_pointer = cmd;
	pgmdsc.dsc$w_length = strlen(cmd);
	if(!((vaxc$errno = sys$crembx(0, &chan, 0, 0, 0, 0, &lognamedsc)) & 1))
		os_error(NO_CARET, "sys$crembx failed");
	if(!((vaxc$errno = lib$spawn(&pgmdsc, 0, &lognamedsc, &one)) & 1))
		os_error(NO_CARET, "lib$spawn failed");
	if((f = fopen("PLOT$MAILBOX", "r")) == NULL)
		os_error(NO_CARET, "mailbox open failed");
# else  /* everyone else */
	restrict_popen();
	if((f = popen(cmd, "r")) == NULL)
		os_error(NO_CARET, "popen failed");
# endif /* everyone else */

	/* get output */
	result_pos = 0;
	result_allocated = MAX_LINE_LEN;
	result = malloc(MAX_LINE_LEN);
	result[0] = NUL;
	while(1) {
		if((c = getc(f)) == EOF)
			break;
		/* result <- c */
		result[result_pos++] = c;
		if(result_pos == result_allocated) {
			if(result_pos >= MAX_TOTAL_LINE_LEN) {
				result_pos--;
				IntWarn(NO_CARET, "*very* long system call output has been truncated");
				break;
			}
			else {
				result = gp_realloc(result, result_allocated + MAX_LINE_LEN, "extend in do_system_func");
				result_allocated += MAX_LINE_LEN;
			}
		}
	}
	result[result_pos] = NUL;

	/* close stream */
	ierr = pclose(f);

	result = gp_realloc(result, strlen(result)+1, "do_system_func");
	*output = result;
	return ierr;

#else /* VMS || PIPES */

	GpGg.IntWarn(NO_CARET, "system() requires support for pipes");
	*output = gp_strdup("");
	return 0;

#endif /* VMS || PIPES */
}

