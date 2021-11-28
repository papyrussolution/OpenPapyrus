// GNUPLOT - misc.c 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
#include <gnuplot.h>
#pragma hdrstop

//static void prepare_call(int calltype);

// State information for load_file(), to recover from errors and properly handle recursive load_file calls
//LFS  * lf_head = NULL; // NULL if not in load_file 

// these are global so that plot.c can load them for the -c option 
int    call_argc;
char * call_args[10] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
char * loadpath_fontname = NULL; // Used by postscript terminal if a font file is found by GnuPlot::LoadPath_fopen() 
static const char * argname[] = {"ARG0", "ARG1", "ARG2", "ARG3", "ARG4", "ARG5", "ARG6", "ARG7", "ARG8", "ARG9"};

//static void prepare_call(int calltype)
void GnuPlot::PrepareCall(int calltype)
{
	udvt_entry * udv;
	GpValue * ARGV;
	int argindex;
	int argv_size;
	// call_args[] will hold arguments as strings.
	// argval[] will be a private copy of numeric arguments as udvs.
	// Later we will fill ARGV[] from one or the other.
	GpValue argval[9];
	for(argindex = 0; argindex < 9; argindex++)
		argval[argindex].SetNotDefined();
	if(calltype == 2) {
		call_argc = 0;
		while(!Pgm.EndOfCommand() && call_argc < 9) {
			call_args[call_argc] = TryToGetString();
			if(!call_args[call_argc]) {
				int save_token = Pgm.GetCurTokenIdx();
				// This catches call "file" STRINGVAR (expression) 
				if(TypeUdv(Pgm.GetCurTokenIdx()) == STRING) {
					call_args[call_argc] = sstrdup(AddUdv(Pgm.GetCurTokenIdx())->udv_value.v.string_val);
					Pgm.Shift();
					// Evaluate a parenthesized expression or a bare numeric user variable and store the result in a string
				}
				else if(Pgm.EqualsCur("(") || (TypeUdv(Pgm.GetCurTokenIdx()) == INTGR || TypeUdv(Pgm.GetCurTokenIdx()) == CMPLX)) {
					char val_as_string[32];
					GpValue a;
					ConstExpress(&a);
					argval[call_argc] = a;
					switch(a.Type) {
						case CMPLX: /* FIXME: More precision? Some way to provide a format? */
						    sprintf(val_as_string, "%g", a.v.cmplx_val.real);
						    call_args[call_argc] = sstrdup(val_as_string);
						    break;
						default:
						    IntError(save_token, "Unrecognized argument type");
						    break;
						case INTGR:
						    sprintf(val_as_string, PLD, a.v.int_val);
						    call_args[call_argc] = sstrdup(val_as_string);
						    break;
					}
					// Old (pre version 5) style wrapping of bare tokens as strings
					// is still used for storing numerical constants ARGn but not ARGV[n]
				}
				else {
					char * endptr;
					Pgm.MCapture(&call_args[call_argc], Pgm.GetCurTokenIdx(), Pgm.GetCurTokenIdx());
					Pgm.Shift();
					double temp = strtod(call_args[call_argc], &endptr);
					if(endptr != call_args[call_argc] && *endptr == '\0')
						Gcomplex(&argval[call_argc], temp, 0.0);
				}
			}
			call_argc++;
		}
		P_LfHead->_CToken = Pgm.GetCurTokenIdx();
		if(!Pgm.EndOfCommand()) {
			Pgm.Shift();
			IntErrorCurToken("too many arguments for 'call <file>'");
		}
	}
	else if(calltype == 5) {
		// lf_push() moved our call arguments from call_args[] to lf->call_args[] 
		// call_argc was determined at program entry 
		for(argindex = 0; argindex < 10; argindex++) {
			call_args[argindex] = P_LfHead->call_args[argindex];
			P_LfHead->call_args[argindex] = NULL; // just to be safe 
		}
	}
	else {
		// "load" command has no arguments 
		call_argc = 0;
	}
	// Old-style "call" arguments were referenced as $0 ... $9 and $#.
	// New-style has ARG0 = script-name, ARG1 ... ARG9 and ARGC
	// Version 5.3 adds ARGV[n]
	udv = Ev.AddUdvByName("ARGC");
	Ginteger(&(udv->udv_value), call_argc);
	udv = Ev.AddUdvByName("ARG0");
	gpfree_string(&(udv->udv_value));
	Gstring(&(udv->udv_value), sstrdup(P_LfHead->name));
	udv = Ev.AddUdvByName("ARGV");
	udv->udv_value.Destroy();
	argv_size = MIN(call_argc, 9);
	udv->udv_value.Type = ARRAY;
	ARGV = udv->udv_value.v.value_array = (GpValue *)SAlloc::M((argv_size + 1) * sizeof(GpValue));
	ARGV[0].v.int_val = argv_size;
	ARGV[0].SetNotDefined();
	for(argindex = 1; argindex <= 9; argindex++) {
		char * argstring = call_args[argindex-1];
		udv = Ev.AddUdvByName(argname[argindex]);
		gpfree_string(&(udv->udv_value));
		Gstring(&(udv->udv_value), argstring ? sstrdup(argstring) : sstrdup(""));
		if(argindex > argv_size)
			continue;
		if(argval[argindex-1].Type == NOTDEFINED)
			Gstring(&ARGV[argindex], sstrdup(udv->udv_value.v.string_val));
		else
			ARGV[argindex] = argval[argindex-1];
	}
}
/*
 * calltype indicates whether load_file() is called from
 * (1) the "load" command, no arguments substitution is done
 * (2) the "call" command, arguments are substituted for $0, $1, etc.
 * (3) on program entry to load initialization files (acts like "load")
 * (4) to execute script files given on the command line (acts like "load")
 * (5) to execute a single script file given with -c (acts like "call")
 * (6) "load $datablock"
 */
//void load_file(FILE * fp, char * name, int calltype)
void GnuPlot::LoadFile(FILE * fp, char * pName, int calltype)
{
	int len;
	int start, left;
	int more;
	int stop = FALSE;
	udvt_entry * gpval_lineno = NULL;
	char ** datablock_input_line = NULL;
	// Support for "load $datablock" 
	if(calltype == 6)
		datablock_input_line = GetDatablock(pName);
	if(!fp && !datablock_input_line)
		IntError(NO_CARET, "Cannot load input from '%s'", pName);
	// Provide a user-visible copy of the current line number in the input file 
	gpval_lineno = Ev.AddUdvByName("GPVAL_LINENO");
	Ginteger(&gpval_lineno->udv_value, 0);
	LfPush(fp, pName, NULL); // save state for errors and recursion 
	if(fp == stdin) {
		// DBT 10-6-98  go interactive if "-" named as load file 
		_Plt.interactive = true;
		while(!ComLine())
			;
		LfPop();
		return;
	}
	else {
		// We actually will read from a file 
		PrepareCall(calltype);
		// things to do after lf_push 
		Pgm.inline_num = 0;
		// go into non-interactive mode during load 
		// will be undone below, or in LoadFileError
		_Plt.interactive = false;
		while(!stop) {  // read all lines in file 
			left = Pgm.InputLineLen;
			start = 0;
			more = TRUE;
			// read one logical line 
			while(more) {
				if(fp && fgets(&(Pgm.P_InputLine[start]), left, fp) == (char *)NULL) {
					// EOF in input file 
					stop = TRUE;
					Pgm.P_InputLine[start] = '\0';
					more = FALSE;
				}
				else if(!fp && datablock_input_line && (*datablock_input_line == NULL)) {
					// End of input datablock 
					stop = TRUE;
					Pgm.P_InputLine[start] = '\0';
					more = FALSE;
				}
				else {
					/* Either we successfully read a line from input file fp
					 * or we are about to copy a line from a datablock.
					 * Either way we have to process line-ending '\' as a
					 * continuation request.
					 */
					if(!fp && datablock_input_line) {
						strncpy(&(Pgm.P_InputLine[start]), *datablock_input_line, left);
						datablock_input_line++;
					}
					Pgm.inline_num++;
					gpval_lineno->udv_value.v.int_val = Pgm.inline_num; // User visible copy 
					if((len = strlen(Pgm.P_InputLine)) == 0)
						continue;
					--len;
					if(Pgm.P_InputLine[len] == '\n') { /* remove any newline */
						Pgm.P_InputLine[len] = '\0';
						// Look, len was 1-1 = 0 before, take care here! 
						if(len > 0)
							--len;
						if(Pgm.P_InputLine[len] == '\r') { /* remove any carriage return */
							Pgm.P_InputLine[len] = '\0';
							if(len > 0)
								--len;
						}
					}
					else if(len + 2 >= left) {
						ExtendInputLine();
						left = Pgm.InputLineLen - len - 1;
						start = len + 1;
						continue; // don't check for '\' 
					}
					if(Pgm.P_InputLine[len] == '\\') {
						// line continuation 
						start = len;
						left = Pgm.InputLineLen - start;
					}
					else {
						/* EAM May 2011 - handle multi-line bracketed clauses {...}.
						 * Introduces a requirement for scanner.c and scanner.h
						 * This code is redundant with part of do_line(),
						 * but do_line() assumes continuation lines come from stdin.
						 */
						/* macros in a clause are problematic, as they are */
						/* only expanded once even if the clause is replayed */
						StringExpandMacros();
						// Strip off trailing comment and count curly braces 
						Pgm.NumTokens = Scanner(&Pgm.P_InputLine, &Pgm.InputLineLen);
						if(Pgm.P_InputLine[Pgm.P_Token[Pgm.NumTokens].StartIdx] == '#') {
							Pgm.P_InputLine[Pgm.P_Token[Pgm.NumTokens].StartIdx] = '\0';
							start = Pgm.P_Token[Pgm.NumTokens].StartIdx;
							left = Pgm.InputLineLen - start;
						}
						// Read additional lines if necessary to complete a bracketed clause {...}
						if(Pgm.CurlyBraceCount < 0)
							IntError(NO_CARET, "Unexpected }");
						if(Pgm.CurlyBraceCount > 0) {
							if((len + 4) > static_cast<int>(Pgm.InputLineLen))
								ExtendInputLine();
							strcat(Pgm.P_InputLine, ";\n");
							start = strlen(Pgm.P_InputLine);
							left = Pgm.InputLineLen - start;
							continue;
						}
						more = FALSE;
					}
				}
			}
			// If we hit a 'break' or 'continue' statement in the lines just processed 
			if(IterationEarlyExit())
				continue;
			// process line 
			if(!isempty(Pgm.P_InputLine)) {
				GpU.screen_ok = FALSE; // make sure command line is echoed on error 
				if(DoLine())
					stop = TRUE;
			}
		}
		// pop state 
		LfPop(); // also closes file fp 
	}
}
// 
// pop from load_file state stack FALSE if stack was empty
// called by load_file and LoadFileError
//
//bool lf_pop()
bool GnuPlot::LfPop()
{
	if(!P_LfHead)
		return false;
	else {
		int argindex;
		udvt_entry * udv;
		LFS * lf = P_LfHead;
		if(!lf->fp || lf->fp == stdin)
			; // Do not close stdin in the case that "-" is named as a load file 
	#if defined(PIPES)
		else if(lf->name && lf->name[0] == '<')
			pclose(lf->fp);
	#endif
		else
			fclose(lf->fp);
		// call arguments are not relevant when invoked from do_string_and_free 
		if(!lf->cmdline) {
			for(argindex = 0; argindex < 10; argindex++) {
				FREEANDASSIGN(call_args[argindex], lf->call_args[argindex]);
			}
			call_argc = lf->call_argc;
			// Restore ARGC and ARG0 ... ARG9 
			if((udv = Ev.GetUdvByName("ARGC"))) {
				Ginteger(&(udv->udv_value), call_argc);
			}
			if((udv = Ev.GetUdvByName("ARG0"))) {
				gpfree_string(&(udv->udv_value));
				Gstring(&(udv->udv_value), (lf->prev && lf->prev->name) ? sstrdup(lf->prev->name) : sstrdup(""));
			}
			for(argindex = 1; argindex <= 9; argindex++) {
				if((udv = Ev.GetUdvByName(argname[argindex]))) {
					gpfree_string(&(udv->udv_value));
					if(!call_args[argindex-1])
						udv->udv_value.SetNotDefined();
					else
						Gstring(&(udv->udv_value), sstrdup(call_args[argindex-1]));
				}
			}
			if((udv = Ev.GetUdvByName("ARGV")) && udv->udv_value.Type == ARRAY) {
				int argv_size = lf->argv[0].v.int_val;
				gpfree_array(&(udv->udv_value));
				udv->udv_value.Type = ARRAY;
				GpValue * ARGV = (GpValue *)SAlloc::M((argv_size + 1) * sizeof(GpValue));
				udv->udv_value.v.value_array = ARGV;
				for(argindex = 0; argindex <= argv_size; argindex++)
					ARGV[argindex] = lf->argv[argindex];
			}
		}
		_Plt.interactive = lf->interactive;
		Pgm.inline_num = lf->inline_num;
		Ev.AddUdvByName("GPVAL_LINENO")->udv_value.v.int_val = Pgm.inline_num;
		Pgm.if_open_for_else = lf->if_open_for_else;
		// Restore saved input state and free the copy 
		if(lf->P_Tokens) {
			Pgm.NumTokens = lf->_NumTokens;
			Pgm.SetTokenIdx(lf->_CToken);
			assert(Pgm.TokenTableSize >= lf->_NumTokens+1);
			memcpy(Pgm.P_Token, lf->P_Tokens, (lf->_NumTokens+1) * sizeof(GpLexicalUnit));
			SAlloc::F(lf->P_Tokens);
		}
		if(lf->input_line) {
			strcpy(Pgm.P_InputLine, lf->input_line);
			SAlloc::F(lf->input_line);
		}
		SAlloc::F(lf->name);
		SAlloc::F(lf->cmdline);
		P_LfHead = lf->prev;
		SAlloc::F(lf);
		return true;
	}
}
// 
// lf_push is called from two different contexts:
//   load_file passes fp and file name (3rd param NULL)
//   do_string_and_free passes cmdline (1st and 2nd params NULL)
// In either case the routines lf_push/lf_pop save and restore state
// information that may be changed by executing commands from a file
// or from the passed command line.
// 
//void lf_push(FILE * fp, char * name, char * cmdline)
void GnuPlot::LfPush(FILE * fp, char * pName, char * pCmdLine)
{
	LFS  * lf = (LFS *)SAlloc::M(sizeof(LFS));
	if(!lf) {
		SFile::ZClose(&fp); // it won't be otherwise 
		IntErrorCurToken("not enough memory to load file");
	}
	else {
		lf->fp = fp; // save this file pointer 
		lf->name = pName;
		lf->cmdline = pCmdLine;
		lf->interactive = _Plt.interactive; // save current state 
		lf->inline_num = Pgm.inline_num; // save current line number 
		lf->call_argc = call_argc;
		// Call arguments are irrelevant if invoked from do_string_and_free 
		if(!pCmdLine) {
			// Save ARG0 through ARG9 
			for(int argindex = 0; argindex < 10; argindex++) {
				lf->call_args[argindex] = call_args[argindex];
				call_args[argindex] = NULL; /* initially no args */
			}
			// Save ARGV[] 
			lf->argv[0].v.int_val = 0;
			lf->argv[0].SetNotDefined();
			const udvt_entry * udv = Ev.GetUdvByName("ARGV");
			if(udv && udv->udv_value.Type == ARRAY) {
				for(int argindex = 0; argindex <= call_argc; argindex++) {
					lf->argv[argindex] = udv->udv_value.v.value_array[argindex];
					if(lf->argv[argindex].Type == STRING)
						lf->argv[argindex].v.string_val = sstrdup(lf->argv[argindex].v.string_val);
				}
			}
		}
		lf->depth = P_LfHead ? (P_LfHead->depth+1) : 0; // recursion depth 
		if(lf->depth > STACK_DEPTH)
			IntError(NO_CARET, "load/eval nested too deeply");
		lf->if_open_for_else = Pgm.if_open_for_else;
		lf->_CToken = Pgm.GetCurTokenIdx();
		lf->_NumTokens = Pgm.NumTokens;
		lf->P_Tokens = (GpLexicalUnit *)SAlloc::M((Pgm.NumTokens+1) * sizeof(GpLexicalUnit));
		memcpy(lf->P_Tokens, Pgm.P_Token, (Pgm.NumTokens+1) * sizeof(GpLexicalUnit));
		lf->input_line = sstrdup(Pgm.P_InputLine);
		lf->prev = P_LfHead; // link to stack 
		P_LfHead = lf;
	}
}
//
// used for reread  vsnyder@math.jpl.nasa.gov 
//
//FILE * lf_top()
FILE * GnuPlot::LfTop()
{
	return P_LfHead ? (P_LfHead->fp) : 0;
}
//
// called from main 
//
//void load_file_error()
void GnuPlot::LoadFileError()
{
	// clean up from error in load_file 
	// pop off everything on stack 
	while(LfPop())
		;
}

//FILE * loadpath_fopen(const char * filename, const char * mode)
FILE * GnuPlot::LoadPath_fopen(const char * filename, const char * mode)
{
	FILE * fp = 0;
	// The global copy of fullname is only for the benefit of post.trm's
	// automatic fontfile conversion via a constructed shell command.
	// FIXME: There was a Feature Request to export the directory path
	// in which a loaded file was found to a user-visible variable for the
	// lifetime of that load.  This is close but without the lifetime.
	ZFREE(loadpath_fontname);
#if defined(PIPES)
	if(*filename == '<') {
		RestrictPOpen();
		if((fp = popen(filename + 1, "r")) == (FILE*)NULL)
			return (FILE*)0;
	}
	else
#endif
	fp = fopen(filename, mode);
	if(!fp) {
		// try 'loadpath' variable 
		char * fullname = 0;
		char * path = 0;
		while((path = get_loadpath()) != 0) {
			// length of path, dir separator, filename, \0 
			fullname = (char *)SAlloc::R(fullname, strlen(path) + 1 + strlen(filename) + 1);
			strcpy(fullname, path);
			PATH_CONCAT(fullname, filename);
			if((fp = fopen(fullname, mode)) != NULL) {
				/* SAlloc::F(fullname); */
				loadpath_fontname = fullname;
				fullname = NULL;
				/* reset loadpath internals!
				 * maybe this can be replaced by calling get_loadpath with
				 * a NULL argument and some loadpath_handler internal logic */
				while(get_loadpath())
					;
				break;
			}
		}
		SAlloc::F(fullname);
	}
#ifdef _WIN32
	if(fp)
		_setmode(_fileno(fp), _O_BINARY);
#endif
	return fp;
}
// 
// Push current terminal.
// Called 
//   1. in main(), just after init_terminal(),
//   2. from load_rcfile(),
//   3. anytime by user command "set term push".
// 
//static char * push_term_name = NULL;
//static char * push_term_opts = NULL;

//void push_terminal(int isInteractive)
void GnuPlot::PushTerminal(int isInteractive)
{
	if(GPT.P_Term) {
		FREEANDASSIGN(P_PushTermName, sstrdup(GPT.P_Term->name));
		FREEANDASSIGN(P_PushTermOpts, sstrdup(GPT._TermOptions));
		if(isInteractive)
			fprintf(stderr, "   pushed terminal %s %s\n", P_PushTermName, P_PushTermOpts);
	}
	else {
		if(isInteractive)
			fputs("\tcurrent terminal type is unknown\n", stderr);
	}
}
// 
// Pop the terminal.
// Called anytime by user command "set term pop".
// 
//void pop_terminal()
void GnuPlot::PopTerminal()
{
	if(P_PushTermName) {
		char * s;
		int i = strlen(P_PushTermName) + 11;
		if(P_PushTermOpts) {
			// do_string() does not like backslashes -- thus remove them 
			for(s = P_PushTermOpts; *s; s++)
				if(*s=='\\' || *s=='\n') 
					*s = ' ';
			i += strlen(P_PushTermOpts);
		}
		s = (char *)SAlloc::M(i);
		i = _Plt.interactive;
		_Plt.interactive = false;
		sprintf(s, "set term %s %s", P_PushTermName, NZOR(P_PushTermOpts, ""));
		DoStringAndFree(s);
		_Plt.interactive = LOGIC(i);
		if(_Plt.interactive)
			fprintf(stderr, "   restored terminal is %s %s\n", GPT.P_Term->name, (GPT._TermOptions.NotEmpty() ? GPT._TermOptions.cptr() : ""));
	}
	else
		fprintf(stderr, "No terminal has been pushed yet\n");
}
// 
// Parse a plot style. Used by 'set style {data|function}' and by (s)plot.  
// 
//enum PLOT_STYLE get_style()
enum PLOT_STYLE GnuPlot::GetStyle()
{
	// defined in plot.h 
	enum PLOT_STYLE ps;
	Pgm.Shift();
	ps = (enum PLOT_STYLE)Pgm.LookupTableForCurrentToken(&plotstyle_tbl[0]);
	Pgm.Shift();
	if(ps == PLOT_STYLE_NONE)
		IntErrorCurToken("unrecognized plot type");
	return ps;
}
// 
// Parse options for style filledcurves and fill fco accordingly.
// If no option given, set it to FILLEDCURVES_DEFAULT.
// 
//void get_filledcurves_style_options(filledcurves_opts * fco)
void GnuPlot::GetFilledCurvesStyleOptions(filledcurves_opts * fco)
{
	enum filledcurves_opts_id p;
	fco->closeto = FILLEDCURVES_DEFAULT;
	fco->oneside = 0;
	while((p = (enum filledcurves_opts_id)Pgm.LookupTableForCurrentToken(&filledcurves_opts_tbl[0])) != -1) {
		fco->closeto = p;
		Pgm.Shift();
		if(p == FILLEDCURVES_ABOVE) {
			fco->oneside = 1;
			continue;
		}
		else if(p == FILLEDCURVES_BELOW) {
			fco->oneside = -1;
			continue;
		}
		fco->at = 0;
		if(!Pgm.EqualsCur("="))
			return;
		/* parameter required for filledcurves x1=... and friends */
		if(p < FILLEDCURVES_ATXY)
			fco->closeto = (filledcurves_opts_id)(fco->closeto + 4);
		Pgm.Shift();
		fco->at = RealExpression();
		if(p != FILLEDCURVES_ATXY)
			return;
		/* two values required for FILLEDCURVES_ATXY */
		if(!Pgm.EqualsCur(","))
			IntErrorCurToken("syntax is xy=<x>,<y>");
		Pgm.Shift();
		fco->aty = RealExpression();
	}
}
//
// Print filledcurves style options to a file (used by 'show' and 'save' commands).
//
void filledcurves_options_tofile(filledcurves_opts * fco, FILE * fp)
{
	if(fco->closeto == FILLEDCURVES_DEFAULT)
		return;
	if(fco->oneside)
		fputs(fco->oneside > 0 ? "above " : "below ", fp);
	if(fco->closeto == FILLEDCURVES_CLOSED) {
		fputs("closed", fp);
		return;
	}
	if(fco->closeto <= FILLEDCURVES_Y2) {
		fputs(filledcurves_opts_tbl[fco->closeto].key, fp);
		return;
	}
	if(fco->closeto <= FILLEDCURVES_ATY2) {
		fprintf(fp, "%s=%g", filledcurves_opts_tbl[fco->closeto - 4].key, fco->at);
		return;
	}
	if(fco->closeto == FILLEDCURVES_ATXY) {
		fprintf(fp, "xy=%g,%g", fco->at, fco->aty);
		return;
	}
}

//bool FASTCALL need_fill_border(const fill_style_type * pFillStyle)
bool GnuPlot::NeedFillBorder(GpTermEntry * pTerm, const fill_style_type * pFillStyle)
{
	lp_style_type p;
	p.pm3d_color = pFillStyle->border_color;
	if(p.pm3d_color.type == TC_LT) {
		// Doesn't want a border at all 
		if(p.pm3d_color.lt == LT_NODRAW)
			return FALSE;
		LoadLineType(pTerm, &p, p.pm3d_color.lt+1);
	}
	// Wants a border in a new color 
	if(p.pm3d_color.type != TC_DEFAULT)
		ApplyPm3DColor(pTerm, &p.pm3d_color);
	return TRUE;
}

//int parse_dashtype(t_dashtype * dt)
int GnuPlot::ParseDashType(t_dashtype * pDashTyp)
{
	int res = DASHTYPE_SOLID;
	int j = 0;
	int k = 0;
	char * dash_str = NULL;
	// Erase any previous contents 
	memzero(pDashTyp, sizeof(t_dashtype));
	// Fill in structure based on keyword ... 
	if(Pgm.EqualsCur("solid")) {
		res = DASHTYPE_SOLID;
		Pgm.Shift();
		// Or numerical pattern consisting of pairs solid,empty,solid,empty... 
	}
	else if(Pgm.EqualsCur("(")) {
		Pgm.Shift();
		while(!Pgm.EndOfCommand()) {
			if(j >= DASHPATTERN_LENGTH) {
				IntErrorCurToken("too many pattern elements");
			}
			pDashTyp->pattern[j++] = FloatExpression(); // The solid portion 
			if(!Pgm.EqualsCurShift(","))
				IntErrorCurToken("expecting comma");
			pDashTyp->pattern[j++] = FloatExpression(); // The empty portion 
			if(Pgm.EqualsCur(")"))
				break;
			if(!Pgm.EqualsCurShift(","))
				IntErrorCurToken("expecting comma");
		}
		if(!Pgm.EqualsCur(")"))
			IntErrorCurToken("expecting , or )");
		Pgm.Shift();
		res = DASHTYPE_CUSTOM;
		// Or string representing pattern elements ... 
	}
	else if((dash_str = TryToGetString())) {
		int leading_space = 0;
#define DSCALE 10.0f
		while(dash_str[j] && (k < DASHPATTERN_LENGTH || dash_str[j] == ' ')) {
			/* .      Dot with short space
			 * -      Dash with regular space
			 * _      Long dash with regular space
			 * space  Don't add new dash, just increase last space */
			switch(dash_str[j]) {
				case '.':
				    pDashTyp->pattern[k++] = 0.2f * DSCALE;
				    pDashTyp->pattern[k++] = 0.5f * DSCALE;
				    break;
				case '-':
				    pDashTyp->pattern[k++] = 1.0f * DSCALE;
				    pDashTyp->pattern[k++] = 1.0f * DSCALE;
				    break;
				case '_':
				    pDashTyp->pattern[k++] = 2.0f * DSCALE;
				    pDashTyp->pattern[k++] = 1.0f * DSCALE;
				    break;
				case ' ':
				    if(k == 0)
					    leading_space++;
				    else
					    pDashTyp->pattern[k-1] += 1.0f * DSCALE;
				    break;
				default:
				    IntError(Pgm.GetPrevTokenIdx(), "expecting one of . - _ or space");
			}
			j++;
		}
		// Move leading space, if any, to the end 
		if(k > 0)
			pDashTyp->pattern[k-1] += leading_space * DSCALE;
#undef  DSCALE
		// truncate dash_str if we ran out of space in the array representation 
		dash_str[j] = '\0';
		strnzcpy(pDashTyp->dstring, dash_str, sizeof(pDashTyp->dstring));
		SAlloc::F(dash_str);
		if(k == 0)
			res = DASHTYPE_SOLID;
		else
			res = DASHTYPE_CUSTOM;
		// Or index of previously defined dashtype 
	}
	else {
		res = IntExpression();
		if(res < 0)
			IntError(Pgm.GetPrevTokenIdx(), "dashtype must be non-negative");
		if(res == 0)
			res = DASHTYPE_AXIS;
		else
			res = res - 1;
	}
	return res;
}
// 
// destination_class tells us whether we are filling in a line style ('set style line'),
// a persistent linetype ('set linetype') or an ad hoc set of properties for a single
// use ('plot ... lc foo lw baz').
// allow_point controls whether we accept a point attribute in this lp_style.
// 
//int lp_parse(lp_style_type * lp, lp_class destination_class, bool allow_point)
int GnuPlot::LpParse(GpTermEntry * pTerm, lp_style_type * lp, lp_class destination_class, bool allow_point)
{
	// keep track of which options were set during this call 
	int set_lt = 0, set_pal = 0, set_lw = 0;
	int set_pt = 0, set_ps  = 0, set_pi = 0;
	int set_pn = 0;
	int set_dt = 0;
	int new_lt = 0;
	int set_colormap = 0;
	// EAM Mar 2010 - We don't want properties from a user-defined default
	// linetype to override properties explicitly set here.  So fill in a
	// local lp_style_type as we go and then copy over the specifically
	// requested properties on top of the default ones.
	lp_style_type newlp = *lp;
	if((destination_class == LP_ADHOC) && (Pgm.AlmostEqualsCur("lines$tyle") || Pgm.EqualsCur("ls"))) {
		Pgm.Shift();
		LpUseProperties(pTerm, lp, IntExpression());
	}
	while(!Pgm.EndOfCommand()) {
		// This special case is to flag an attempt to "set object N lt <lt>",
		// which would otherwise be accepted but ignored, leading to confusion
		// FIXME:  Couldn't this be handled at a higher level?
		if((destination_class == LP_NOFILL) && (Pgm.EqualsCur("lt") || Pgm.AlmostEqualsCur("linet$ype")))
			IntErrorCurToken("object linecolor must be set using fillstyle border");
		if(Pgm.AlmostEqualsCur("linet$ype") || Pgm.EqualsCur("lt")) {
			if(set_lt++)
				break;
			if(destination_class == LP_TYPE)
				IntErrorCurToken("linetype definition cannot use linetype");
			Pgm.Shift();
			if(Pgm.AlmostEqualsCur("rgb$color")) {
				if(set_pal++)
					break;
				Pgm.Rollback();
				ParseColorSpec(&(newlp.pm3d_color), TC_RGB);
			}
			else
			// both syntaxes allowed: 'with lt pal' as well as 'with pal' 
			if(Pgm.AlmostEqualsCur("pal$ette")) {
				if(set_pal++)
					break;
				Pgm.Rollback();
				ParseColorSpec(&(newlp.pm3d_color), TC_Z);
			}
			else if(Pgm.EqualsCur("bgnd")) {
				*lp = background_lp;
				Pgm.Shift();
			}
			else if(Pgm.EqualsCur("black")) {
				*lp = default_border_lp;
				Pgm.Shift();
			}
			else if(Pgm.EqualsCur("nodraw")) {
				lp->l_type = LT_NODRAW;
				Pgm.Shift();
			}
			else {
				// These replace the base style 
				new_lt = IntExpression();
				lp->l_type = new_lt - 1;
				// user may prefer explicit line styles 
				if(Gg.PreferLineStyles && destination_class != LP_STYLE)
					LpUseProperties(pTerm, lp, new_lt);
				else
					LoadLineType(pTerm, lp, new_lt);
			}
		} /* linetype, lt */
		// both syntaxes allowed: 'with lt pal' as well as 'with pal' 
		if(Pgm.AlmostEqualsCur("pal$ette")) {
			if(set_pal++)
				break;
			Pgm.Rollback();
			ParseColorSpec(&(newlp.pm3d_color), TC_Z);
			continue;
		}
		/* This is so that "set obj ... lw N fc <colorspec>" doesn't eat up the
		 * fc colorspec as a line property.  We need to parse it later as a
		 * _fill_ property. Also prevents "plot ... fc <col1> fs <foo> lw <baz>"
		 * from generating an error claiming redundant line properties.
		 */
		if(oneof2(destination_class, LP_NOFILL, LP_ADHOC) && (Pgm.EqualsCur("fc") || Pgm.AlmostEqualsCur("fillc$olor")) && 
			(!Pgm.AlmostEquals(Pgm.GetCurTokenIdx()+1, "pal$ette")))
			break;
		if(Pgm.EqualsCur("lc") || Pgm.AlmostEqualsCur("linec$olor") || Pgm.EqualsCur("fc") || Pgm.AlmostEqualsCur("fillc$olor")) {
			if(set_pal++)
				break;
			Pgm.Shift();
			if(Pgm.AlmostEqualsCur("rgb$color") || Pgm.IsString(Pgm.GetCurTokenIdx())) {
				Pgm.Rollback();
				ParseColorSpec(&(newlp.pm3d_color), TC_RGB);
			}
			else if(Pgm.AlmostEqualsCur("pal$ette")) {
				/* The next word could be any of {z|cb|frac|<colormap-name>}.
				 * Check first for a colormap name.
				 */
				udvt_entry * colormap = GetColorMap(Pgm.GetCurTokenIdx()+1);
				if(colormap) {
					newlp.pm3d_color.type = TC_COLORMAP;
					newlp.P_Colormap = colormap;
					set_colormap++;
					Pgm.Shift();
					Pgm.Shift();
				}
				else {
					Pgm.Rollback();
					ParseColorSpec(&(newlp.pm3d_color), TC_Z);
				}
			}
			else if(Pgm.EqualsCur("bgnd")) {
				newlp.pm3d_color.type = TC_LT;
				newlp.pm3d_color.lt = LT_BACKGROUND;
				Pgm.Shift();
			}
			else if(Pgm.EqualsCur("black")) {
				newlp.pm3d_color.type = TC_LT;
				newlp.pm3d_color.lt = LT_BLACK;
				Pgm.Shift();
			}
			else if(Pgm.AlmostEqualsCur("var$iable")) {
				Pgm.Shift();
				newlp.l_type = LT_COLORFROMCOLUMN;
				newlp.pm3d_color.type = TC_LINESTYLE;
			}
			else {
				/* Pull the line colour from a default linetype, but */
				/* only if we are not in the middle of defining one! */
				if(destination_class != LP_STYLE) {
					lp_style_type temp;
					LoadLineType(pTerm, &temp, IntExpression());
					newlp.pm3d_color = temp.pm3d_color;
				}
				else {
					newlp.pm3d_color.type = TC_LT;
					newlp.pm3d_color.lt = IntExpression() - 1;
				}
			}
			continue;
		}
		if(Pgm.AlmostEqualsCur("linew$idth") || Pgm.EqualsCur("lw")) {
			if(set_lw++)
				break;
			Pgm.Shift();
			newlp.l_width = RealExpression();
			if(newlp.l_width < 0)
				newlp.l_width = 0;
			continue;
		}
		if(Pgm.EqualsCur("bgnd")) {
			if(set_lt++)
				break; ;
			Pgm.Shift();
			*lp = background_lp;
			continue;
		}
		if(Pgm.EqualsCur("black")) {
			if(set_lt++)
				break; ;
			Pgm.Shift();
			*lp = default_border_lp;
			continue;
		}
		if(Pgm.AlmostEqualsCur("pointt$ype") || Pgm.EqualsCur("pt")) {
			if(allow_point) {
				char * symbol;
				if(set_pt++)
					break;
				Pgm.Shift();
				if((symbol = TryToGetString())) {
					newlp.PtType = PT_CHARACTER;
					truncate_to_one_utf8_char(symbol);
					strnzcpy(newlp.p_char, symbol, sizeof(newlp.p_char));
					SAlloc::F(symbol);
				}
				else if(Pgm.AlmostEqualsCur("var$iable") && (destination_class == LP_ADHOC)) {
					newlp.PtType = PT_VARIABLE;
					Pgm.Shift();
				}
				else
					newlp.PtType = IntExpression() - 1;
			}
			else {
				IntWarnCurToken("No pointtype specifier allowed, here");
				Pgm.Shift();
				Pgm.Shift();
			}
			continue;
		}
		if(Pgm.AlmostEqualsCur("points$ize") || Pgm.EqualsCur("ps")) {
			if(allow_point) {
				if(set_ps++)
					break;
				Pgm.Shift();
				if(Pgm.AlmostEqualsCur("var$iable")) {
					newlp.PtSize = PTSZ_VARIABLE;
					Pgm.Shift();
				}
				else if(Pgm.AlmostEqualsCur("def$ault")) {
					newlp.PtSize = PTSZ_DEFAULT;
					Pgm.Shift();
				}
				else {
					newlp.PtSize = RealExpression();
					SETMAX(newlp.PtSize, 0.0);
				}
			}
			else {
				IntWarnCurToken("No pointsize specifier allowed, here");
				Pgm.Shift();
				Pgm.Shift();
			}
			continue;
		}
		if(Pgm.AlmostEqualsCur("pointi$nterval") || Pgm.EqualsCur("pi")) {
			Pgm.Shift();
			if(allow_point) {
				newlp.p_interval = IntExpression();
				set_pi = 1;
			}
			else {
				IntWarnCurToken("No pointinterval specifier allowed here");
				IntExpression();
			}
			continue;
		}
		if(Pgm.AlmostEqualsCur("pointn$umber") || Pgm.EqualsCur("pn")) {
			Pgm.Shift();
			if(allow_point) {
				newlp.p_number = IntExpression();
				set_pn = 1;
			}
			else {
				IntWarnCurToken("No pointnumber specifier allowed here)");
				IntExpression();
			}
			continue;
		}
		if(Pgm.AlmostEqualsCur("dasht$ype") || Pgm.EqualsCur("dt")) {
			int tmp;
			if(set_dt++)
				break;
			Pgm.Shift();
			tmp = ParseDashType(&newlp.CustomDashPattern);
			// Pull the dashtype from the list of already defined dashtypes, 
			// but only if it we didn't get an explicit one back from parse_dashtype 
			if(tmp == DASHTYPE_AXIS)
				lp->l_type = LT_AXIS;
			if(tmp >= 0)
				tmp = LoadDashType(&newlp.CustomDashPattern, tmp + 1);
			newlp.d_type = tmp;
			continue;
		}
		// caught unknown option -> quit the while(1) loop 
		break;
	}
	if(set_lt > 1 || set_pal > 1 || set_lw > 1 || set_pt > 1 || set_ps > 1 || set_dt > 1 || (set_pi + set_pn > 1))
		IntErrorCurToken("duplicate or conflicting arguments in style specification");
	if(set_pal) {
		lp->pm3d_color = newlp.pm3d_color;
		// hidden3d uses this to decide that a single color surface is wanted 
		lp->flags |= LP_EXPLICIT_COLOR;
	}
	else {
		lp->flags &= ~LP_EXPLICIT_COLOR;
	}
	if(set_lw)
		lp->l_width = newlp.l_width;
	if(set_pt) {
		lp->PtType = newlp.PtType;
		memcpy(lp->p_char, newlp.p_char, sizeof(newlp.p_char));
	}
	if(set_ps)
		lp->PtSize = newlp.PtSize;
	if(set_pi) {
		lp->p_interval = newlp.p_interval;
		lp->p_number = 0;
	}
	if(set_pn) {
		lp->p_number = newlp.p_number;
		lp->p_interval = 0;
	}
	if(newlp.l_type == LT_COLORFROMCOLUMN)
		lp->l_type = LT_COLORFROMCOLUMN;
	if(set_dt) {
		lp->d_type = newlp.d_type;
		lp->CustomDashPattern = newlp.CustomDashPattern;
	}
	if(set_colormap) {
		lp->P_Colormap = newlp.P_Colormap;
	}
	return new_lt;
}

/* <fillstyle> = {empty | solid {<density>} | pattern {<n>}} {noborder | border {<lt>}} */
const struct gen_table fs_opt_tbl[] = {
	{"e$mpty", FS_EMPTY},
	{"s$olid", FS_SOLID},
	{"p$attern", FS_PATTERN},
	{NULL, -1}
};

//void parse_fillstyle(fill_style_type * fs)
void GnuPlot::ParseFillStyle(fill_style_type * fs)
{
	bool set_fill = FALSE;
	bool set_border = FALSE;
	bool transparent = FALSE;
	if(Pgm.EndOfCommand())
		return;
	if(!Pgm.EqualsCur("fs") && !Pgm.AlmostEqualsCur("fill$style"))
		return;
	Pgm.Shift();
	while(!Pgm.EndOfCommand()) {
		int i;
		if(Pgm.AlmostEqualsCur("trans$parent")) {
			transparent = TRUE;
			fs->filldensity = 50;
			Pgm.Shift();
			continue;
		}
		i = Pgm.LookupTableForCurrentToken(fs_opt_tbl);
		switch(i) {
			default:
			    break;
			case FS_EMPTY:
			case FS_SOLID:
			case FS_PATTERN:
			    if(set_fill && fs->fillstyle != i)
				    IntErrorCurToken("conflicting option");
			    fs->fillstyle = i;
			    set_fill = TRUE;
			    Pgm.Shift();
			    if(!transparent)
				    fs->filldensity = 100;
			    if(MightBeNumeric(Pgm.GetCurTokenIdx())) {
				    if(fs->fillstyle == FS_SOLID) {
					    // user sets 0...1, but is stored as an integer 0..100 
					    fs->filldensity = static_cast<int>(100.0 * RealExpression() + 0.5);
					    if(fs->filldensity < 0)
						    fs->filldensity = 0;
					    if(fs->filldensity > 100)
						    fs->filldensity = 100;
				    }
				    else if(fs->fillstyle == FS_PATTERN) {
					    fs->fillpattern = IntExpression();
					    if(fs->fillpattern < 0)
						    fs->fillpattern = 0;
				    }
				    else
					    IntErrorCurToken("this fill style does not have a parameter");
			    }
			    continue;
		}
		if(Pgm.AlmostEqualsCur("bo$rder")) {
			if(set_border && fs->border_color.lt == LT_NODRAW)
				IntErrorCurToken("conflicting option");
			fs->border_color.type = TC_DEFAULT;
			set_border = TRUE;
			Pgm.Shift();
			if(Pgm.EndOfCommand())
				continue;
			if(Pgm.EqualsCur("-") || Pgm.IsANumber(Pgm.GetCurTokenIdx())) {
				fs->border_color.type = TC_LT;
				fs->border_color.lt = IntExpression() - 1;
			}
			else if(Pgm.EqualsCur("lc") || Pgm.AlmostEqualsCur("linec$olor")) {
				ParseColorSpec(&fs->border_color, TC_Z);
			}
			else if(Pgm.EqualsCur("rgb") || Pgm.EqualsCur("lt") || Pgm.AlmostEqualsCur("linet$ype")) {
				Pgm.Rollback();
				ParseColorSpec(&fs->border_color, TC_Z);
			}
			continue;
		}
		else if(Pgm.AlmostEqualsCur("nobo$rder")) {
			if(set_border && fs->border_color.lt != LT_NODRAW)
				IntErrorCurToken("conflicting option");
			fs->border_color.type = TC_LT;
			fs->border_color.lt = LT_NODRAW;
			set_border = TRUE;
			Pgm.Shift();
			continue;
		}

		/* Keyword must belong to someone else */
		break;
	}
	if(transparent) {
		if(fs->fillstyle == FS_SOLID)
			fs->fillstyle = FS_TRANSPARENT_SOLID;
		else if(fs->fillstyle == FS_PATTERN)
			fs->fillstyle = FS_TRANSPARENT_PATTERN;
	}
}
// 
// Parse the sub-options of text color specification
//   { def$ault | lt <linetype> | pal$ette { cb <val> | frac$tion <val> | z }
// The ordering of alternatives shown in the line above is kept in the symbol definitions
// TC_DEFAULT TC_LT TC_LINESTYLE TC_RGB TC_CB TC_FRAC TC_Z TC_VARIABLE (0 1 2 3 4 5 6 7)
// and the "options" parameter to parse_colorspec limits legal input to the
// corresponding point in the series. So TC_LT allows only default or linetype
// coloring, while TC_Z allows all coloring options up to and including pal z
// 
//void parse_colorspec(t_colorspec * tc, int options)
void GnuPlot::ParseColorSpec(t_colorspec * tc, int options)
{
	Pgm.Shift();
	if(Pgm.EndOfCommand())
		IntErrorCurToken("expected colorspec");
	if(Pgm.AlmostEqualsCur("def$ault")) {
		Pgm.Shift();
		tc->type = TC_DEFAULT;
	}
	else if(Pgm.EqualsCur("bgnd")) {
		Pgm.Shift();
		tc->type = TC_LT;
		tc->lt = LT_BACKGROUND;
	}
	else if(Pgm.EqualsCur("black")) {
		Pgm.Shift();
		tc->type = TC_LT;
		tc->lt = LT_BLACK;
	}
	else if(Pgm.EqualsCur("lt")) {
		lp_style_type lptemp;
		Pgm.Shift();
		if(Pgm.EndOfCommand())
			IntErrorCurToken("expected linetype");
		tc->type = TC_LT;
		tc->lt = IntExpression()-1;
		if(tc->lt < LT_BACKGROUND) {
			tc->type = TC_DEFAULT;
			IntWarnCurToken("illegal linetype");
		}
		//
		// July 2014 - translate linetype into user-defined linetype color.
		// This is a CHANGE!
		//
		LoadLineType(GPT.P_Term, &lptemp, tc->lt + 1);
		*tc = lptemp.pm3d_color;
	}
	else if(options <= TC_LT) {
		tc->type = TC_DEFAULT;
		IntErrorCurToken("only tc lt <n> possible here");
	}
	else if(Pgm.EqualsCur("ls") || Pgm.AlmostEqualsCur("lines$tyle")) {
		Pgm.Shift();
		tc->type = TC_LINESTYLE;
		tc->lt = static_cast<int>(RealExpression());
	}
	else if(Pgm.AlmostEqualsCur("rgb$color")) {
		Pgm.Shift();
		tc->type = TC_RGB;
		if(Pgm.AlmostEqualsCur("var$iable")) {
			tc->value = -1.0;
			Pgm.Shift();
		}
		else {
			tc->value = 0.0;
			tc->lt = ParseColorName();
		}
	}
	else if(Pgm.AlmostEqualsCur("pal$ette")) {
		Pgm.Shift();
		if(Pgm.EqualsCur("z")) {
			/* The actual z value is not yet known, fill it in later */
			if(options >= TC_Z) {
				tc->type = TC_Z;
			}
			else {
				tc->type = TC_DEFAULT;
				IntErrorCurToken("palette z not possible here");
			}
			Pgm.Shift();
		}
		else if(Pgm.EqualsCur("cb")) {
			tc->type = TC_CB;
			Pgm.Shift();
			if(Pgm.EndOfCommand())
				IntErrorCurToken("expected cb value");
			tc->value = RealExpression();
		}
		else if(Pgm.AlmostEqualsCur("frac$tion")) {
			tc->type = TC_FRAC;
			Pgm.Shift();
			if(Pgm.EndOfCommand())
				IntErrorCurToken("expected palette fraction");
			tc->value = RealExpression();
			if(tc->value < 0. || tc->value > 1.0)
				IntErrorCurToken("palette fraction out of range");
		}
		else {
			/* Pgm.EndOfCommand() or palette <blank> */
			if(options >= TC_Z)
				tc->type = TC_Z;
		}
	}
	else if(options >= TC_VARIABLE && Pgm.AlmostEqualsCur("var$iable")) {
		tc->type = TC_VARIABLE;
		Pgm.Shift();
		/* New: allow to skip the rgb keyword, as in  'plot $foo lc "blue"' */
	}
	else if(Pgm.IsString(Pgm.GetCurTokenIdx())) {
		tc->type = TC_RGB;
		tc->lt = ParseColorName();
	}
	else {
		IntErrorCurToken("colorspec option not recognized");
	}
}

long lookup_color_name(char * string)
{
	long color = -2;
	int iret = lookup_table_nth(pm3d_color_names_tbl, string);
	if(iret >= 0)
		color = pm3d_color_names_tbl[iret].value;
	else if(string[0] == '#')
		iret = sscanf(string, "#%lx", &color);
	else if(string[0] == '0' && (string[1] == 'x' || string[1] == 'X'))
		iret = sscanf(string, "%lx", &color);
	return color;
}

//long parse_color_name()
long GnuPlot::ParseColorName()
{
	char * string;
	long color;
	// Terminal drivers call this after seeing a "background" option 
	if(Pgm.AlmostEqualsCur("rgb$color") && Pgm.AlmostEquals(Pgm.GetPrevTokenIdx(), "back$ground"))
		Pgm.Shift();
	if((string = TryToGetString())) {
		color = lookup_color_name(string);
		SAlloc::F(string);
		if(color == -2)
			IntErrorCurToken("unrecognized color name and not a string \"#AARRGGBB\" or \"0xAARRGGBB\"");
	}
	else
		color = IntExpression();
	return (ulong)(color);
}
// 
// arrow parsing...
// 
// allow_as controls whether we are allowed to accept arrowstyle in
// the current context [ie not when doing a  set style arrow command]
// 
//void arrow_use_properties(arrow_style_type * pArrow, int tag)
void GnuPlot::ArrowUseProperties(arrow_style_type * pArrow, int tag)
{
	// This function looks for an arrowstyle defined by 'tag' and
	// copies its data into the structure 'ap'. 
	// If a color has already been set for p_this arrow, keep it 
	t_colorspec save_colorspec = pArrow->lp_properties.pm3d_color;
	// Default if requested style is not found 
	default_arrow_style(pArrow);
	arrowstyle_def * p_this = Gg.P_FirstArrowStyle; 
	while(p_this) {
		if(p_this->tag == tag) {
			*pArrow = p_this->arrow_properties;
			break;
		}
		else
			p_this = p_this->next;
	}
	// tag not found: 
	if(!p_this || p_this->tag != tag)
		IntWarn(NO_CARET, "arrowstyle %d not found", tag);
	// Restore original color if the style doesn't specify one 
	if(pArrow->lp_properties.pm3d_color.type == TC_DEFAULT)
		pArrow->lp_properties.pm3d_color = save_colorspec;
}

//void arrow_parse(arrow_style_type * arrow, bool allow_as)
void GnuPlot::ArrowParse(arrow_style_type * arrow, bool allow_as)
{
	int    set_layer = 0;
	int    set_line = 0;
	int    set_head = 0;
	int    set_headsize = 0;
	int    set_headfilled = 0;
	// Use predefined arrow style 
	if(allow_as && (Pgm.AlmostEqualsCur("arrows$tyle") || Pgm.EqualsCur("as"))) {
		Pgm.Shift();
		if(Pgm.AlmostEqualsCur("var$iable")) {
			arrow->tag = AS_VARIABLE;
			Pgm.Shift();
		}
		else
			ArrowUseProperties(arrow, IntExpression());
		return;
	}
	// No predefined arrow style; read properties from command line 
	// avoid duplicating options 
	while(!Pgm.EndOfCommand()) {
		if(Pgm.EqualsCur("nohead")) {
			if(set_head++)
				break;
			Pgm.Shift();
			arrow->head = NOHEAD;
			continue;
		}
		if(Pgm.EqualsCur("head")) {
			if(set_head++)
				break;
			Pgm.Shift();
			arrow->head = END_HEAD;
			continue;
		}
		if(Pgm.EqualsCur("backhead")) {
			if(set_head++)
				break;
			Pgm.Shift();
			arrow->head = BACKHEAD;
			continue;
		}
		if(Pgm.EqualsCur("heads")) {
			if(set_head++)
				break;
			Pgm.Shift();
			arrow->head = (t_arrow_head)(BACKHEAD | END_HEAD);
			continue;
		}
		if(Pgm.AlmostEqualsCur("nobo$rder")) {
			if(set_headfilled++)
				break;
			Pgm.Shift();
			arrow->headfill = AS_NOBORDER;
			continue;
		}
		if(Pgm.AlmostEqualsCur("fill$ed")) {
			if(set_headfilled++)
				break;
			Pgm.Shift();
			arrow->headfill = AS_FILLED;
			continue;
		}
		if(Pgm.AlmostEqualsCur("empty")) {
			if(set_headfilled++)
				break;
			Pgm.Shift();
			arrow->headfill = AS_EMPTY;
			continue;
		}
		if(Pgm.AlmostEqualsCur("nofill$ed")) {
			if(set_headfilled++)
				break;
			Pgm.Shift();
			arrow->headfill = AS_NOFILL;
			continue;
		}
		if(Pgm.EqualsCur("size")) {
			GpPosition hsize;
			if(set_headsize++)
				break;
			hsize.scalex = hsize.scaley = hsize.scalez = first_axes;
			/* only scalex used; scaley is angle of the head in [deg] */
			Pgm.Shift();
			if(Pgm.EndOfCommand())
				IntErrorCurToken("head size expected");
			GetPosition(&hsize);
			arrow->head_length = hsize.x;
			arrow->head_lengthunit = hsize.scalex;
			arrow->head_angle = hsize.y;
			arrow->head_backangle = hsize.z;
			// invalid backangle --> default of 90.0 degrees 
			if(arrow->head_backangle <= arrow->head_angle)
				arrow->head_backangle = 90.0;
			// Assume adjustable size but check for 'fixed' instead 
			arrow->head_fixedsize = FALSE;
			continue;
		}
		if(Pgm.AlmostEqualsCur("fix$ed")) {
			arrow->head_fixedsize = TRUE;
			Pgm.Shift();
			continue;
		}
		if(Pgm.EqualsCur("back")) {
			if(set_layer++)
				break;
			Pgm.Shift();
			arrow->layer = LAYER_BACK;
			continue;
		}
		if(Pgm.EqualsCur("front")) {
			if(set_layer++)
				break;
			Pgm.Shift();
			arrow->layer = LAYER_FRONT;
			continue;
		}
		// pick up a line spec - allow ls, but no point. 
		{
			int stored_token = Pgm.GetCurTokenIdx();
			LpParse(GPT.P_Term, &arrow->lp_properties, LP_ADHOC, FALSE);
			if(stored_token == Pgm.GetCurTokenIdx() || set_line++)
				break;
			continue;
		}
		// unknown option caught -> quit the while(1) loop 
		break;
	}
	if(set_layer>1 || set_line>1 || set_head>1 || set_headsize>1 || set_headfilled>1)
		IntErrorCurToken("duplicated arguments in style specification");
}

//void get_image_options(t_image * image)
void GnuPlot::GetImageOptions(t_image * image)
{
	if(Pgm.AlmostEqualsCur("pix$els") || Pgm.EqualsCur("failsafe")) {
		Pgm.Shift();
		image->fallback = TRUE;
	}
}
// 
// Try to interpret the next token in a command line as the name of a colormap.
// If it seems to belong to a valid colormap, return a pointer.
// Otherwise return NULL.  The caller is responsible for reporting an error.
// 
//udvt_entry * get_colormap(int token)
udvt_entry * GnuPlot::GetColorMap(int token)
{
	udvt_entry * colormap = NULL;
	if(TypeUdv(token) == ARRAY) {
		udvt_entry * udv = AddUdv(token);
		if((udv->udv_value.v.value_array[0].Type == COLORMAP_ARRAY) && (udv->udv_value.v.value_array[0].v.int_val >= 2))
			colormap = udv;
	}
	return colormap;
}
// 
// Create a pixmap containing an existing colormap palette.
// This can be used to produce a colorbox for a named palette
// separate from the automatic colorbox generated for the main palette.
// 
//void pixmap_from_colormap(t_pixmap * pixmap)
void GnuPlot::PixMapFromColorMap(t_pixmap * pPixmap)
{
	udvt_entry * colormap = GetColorMap(Pgm.GetCurTokenIdx());
	uint rgb;
	int size, i, ip;
	if(!colormap)
		IntErrorCurToken("not a colormap");
	Pgm.Shift();
	FREEANDASSIGN(pPixmap->colormapname, sstrdup(colormap->udv_name));
	size = colormap->udv_value.v.value_array[0].v.int_val;
	pPixmap->image_data = (coordval *)SAlloc::R(pPixmap->image_data, static_cast<size_t>(size * 4.0 * sizeof(coordval)));
	// Unpack ARGB colormap entry into 4 separate values R G B A 
	for(i = 1, ip = 0; i <= size; i++) {
		rgb = colormap->udv_value.v.value_array[i].v.int_val;
		pPixmap->image_data[ip++] = ((rgb >> 16) & 0xff) / 255.;
		pPixmap->image_data[ip++] = ((rgb >> 8) & 0xff) / 255.;
		pPixmap->image_data[ip++] = ((rgb) & 0xff) / 255.;
		pPixmap->image_data[ip++] = 255-((rgb >> 24) & 0xff);
	}
	pPixmap->ncols = 1;
	pPixmap->nrows = size;
}
