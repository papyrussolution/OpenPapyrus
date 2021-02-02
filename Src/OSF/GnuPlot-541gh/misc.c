// GNUPLOT - misc.c 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
#include <gnuplot.h>
#pragma hdrstop

//static void prepare_call(int calltype);

/* State information for load_file(), to recover from errors
 * and properly handle recursive load_file calls
 */
LFS * lf_head = NULL;            /* NULL if not in load_file */

/* these are global so that plot.c can load them for the -c option */
int call_argc;
char * call_args[10] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
static const char * argname[] = {"ARG0", "ARG1", "ARG2", "ARG3", "ARG4", "ARG5", "ARG6", "ARG7", "ARG8", "ARG9"};

/* Used by postscript terminal if a font file is found by loadpath_fopen() */
char * loadpath_fontname = NULL;

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
		argval[argindex].type = NOTDEFINED;
	if(calltype == 2) {
		call_argc = 0;
		while(!Pgm.EndOfCommand() && call_argc < 9) {
			call_args[call_argc] = TryToGetString();
			if(!call_args[call_argc]) {
				int save_token = Pgm.GetCurTokenIdx();
				// This catches call "file" STRINGVAR (expression) 
				if(Pgm.TypeUdv(Pgm.GetCurTokenIdx()) == STRING) {
					call_args[call_argc] = gp_strdup(add_udv(Pgm.GetCurTokenIdx())->udv_value.v.string_val);
					Pgm.Shift();
					// Evaluate a parenthesized expression or a bare numeric user variable and store the result in a string
				}
				else if(Pgm.EqualsCur("(") || (Pgm.TypeUdv(Pgm.GetCurTokenIdx()) == INTGR || Pgm.TypeUdv(Pgm.GetCurTokenIdx()) == CMPLX)) {
					char val_as_string[32];
					GpValue a;
					ConstExpress(&a);
					argval[call_argc] = a;
					switch(a.type) {
						case CMPLX: /* FIXME: More precision? Some way to provide a format? */
						    sprintf(val_as_string, "%g", a.v.cmplx_val.real);
						    call_args[call_argc] = gp_strdup(val_as_string);
						    break;
						default:
						    IntError(save_token, "Unrecognized argument type");
						    break;
						case INTGR:
						    sprintf(val_as_string, PLD, a.v.int_val);
						    call_args[call_argc] = gp_strdup(val_as_string);
						    break;
					}
					// Old (pre version 5) style wrapping of bare tokens as strings
					// is still used for storing numerical constants ARGn but not ARGV[n]
				}
				else {
					double temp;
					char * endptr;
					Pgm.MCapture(&call_args[call_argc], Pgm.GetCurTokenIdx(), Pgm.GetCurTokenIdx());
					Pgm.Shift();
					temp = strtod(call_args[call_argc], &endptr);
					if(endptr != call_args[call_argc] && *endptr == '\0')
						Gcomplex(&argval[call_argc], temp, 0.0);
				}
			}
			call_argc++;
		}
		lf_head->_CToken = Pgm.GetCurTokenIdx();
		if(!Pgm.EndOfCommand()) {
			Pgm.Shift();
			IntErrorCurToken("too many arguments for 'call <file>'");
		}
	}
	else if(calltype == 5) {
		// lf_push() moved our call arguments from call_args[] to lf->call_args[] 
		// call_argc was determined at program entry 
		for(argindex = 0; argindex < 10; argindex++) {
			call_args[argindex] = lf_head->call_args[argindex];
			lf_head->call_args[argindex] = NULL; // just to be safe 
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
	Gstring(&(udv->udv_value), gp_strdup(lf_head->name));
	udv = Ev.AddUdvByName("ARGV");
	udv->udv_value.Destroy();
	argv_size = MIN(call_argc, 9);
	udv->udv_value.type = ARRAY;
	ARGV = udv->udv_value.v.value_array = (GpValue *)gp_alloc((argv_size + 1) * sizeof(GpValue), "array state");
	ARGV[0].v.int_val = argv_size;
	ARGV[0].type = NOTDEFINED;
	for(argindex = 1; argindex <= 9; argindex++) {
		char * argstring = call_args[argindex-1];
		udv = Ev.AddUdvByName(argname[argindex]);
		gpfree_string(&(udv->udv_value));
		Gstring(&(udv->udv_value), argstring ? gp_strdup(argstring) : gp_strdup(""));
		if(argindex > argv_size)
			continue;
		if(argval[argindex-1].type == NOTDEFINED)
			Gstring(&ARGV[argindex], gp_strdup(udv->udv_value.v.string_val));
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
void GpProgram::LoadFile(FILE * fp, char * pName, int calltype)
{
	int len;
	int start, left;
	int more;
	int stop = FALSE;
	udvt_entry * gpval_lineno = NULL;
	char ** datablock_input_line = NULL;
	// Support for "load $datablock" 
	if(calltype == 6)
		datablock_input_line = get_datablock(pName);
	if(!fp && !datablock_input_line)
		GPO.IntError(NO_CARET, "Cannot load input from '%s'", pName);
	// Provide a user-visible copy of the current line number in the input file 
	gpval_lineno = GPO.Ev.AddUdvByName("GPVAL_LINENO");
	Ginteger(&gpval_lineno->udv_value, 0);
	LfPush(fp, pName, NULL); // save state for errors and recursion 
	if(fp == stdin) {
		// DBT 10-6-98  go interactive if "-" named as load file 
		interactive = TRUE;
		while(!com_line())
			;
		LfPop();
		return;
	}
	else {
		// We actually will read from a file 
		GPO.PrepareCall(calltype);
		// things to do after lf_push 
		inline_num = 0;
		// go into non-interactive mode during load 
		// will be undone below, or in load_file_error 
		interactive = FALSE;
		while(!stop) {  // read all lines in file 
			left = gp_input_line_len;
			start = 0;
			more = TRUE;
			// read one logical line 
			while(more) {
				if(fp && fgets(&(gp_input_line[start]), left, fp) == (char*)NULL) {
					// EOF in input file 
					stop = TRUE;
					gp_input_line[start] = '\0';
					more = FALSE;
				}
				else if(!fp && datablock_input_line && (*datablock_input_line == NULL)) {
					// End of input datablock 
					stop = TRUE;
					gp_input_line[start] = '\0';
					more = FALSE;
				}
				else {
					/* Either we successfully read a line from input file fp
					 * or we are about to copy a line from a datablock.
					 * Either way we have to process line-ending '\' as a
					 * continuation request.
					 */
					if(!fp && datablock_input_line) {
						strncpy(&(gp_input_line[start]), *datablock_input_line, left);
						datablock_input_line++;
					}
					inline_num++;
					gpval_lineno->udv_value.v.int_val = inline_num; /* User visible copy */
					if((len = strlen(gp_input_line)) == 0)
						continue;
					--len;
					if(gp_input_line[len] == '\n') { /* remove any newline */
						gp_input_line[len] = '\0';
						// Look, len was 1-1 = 0 before, take care here! 
						if(len > 0)
							--len;
						if(gp_input_line[len] == '\r') { /* remove any carriage return */
							gp_input_line[len] = NUL;
							if(len > 0)
								--len;
						}
					}
					else if(len + 2 >= left) {
						extend_input_line();
						left = gp_input_line_len - len - 1;
						start = len + 1;
						continue; // don't check for '\' 
					}
					if(gp_input_line[len] == '\\') {
						// line continuation 
						start = len;
						left = gp_input_line_len - start;
					}
					else {
						/* EAM May 2011 - handle multi-line bracketed clauses {...}.
						 * Introduces a requirement for scanner.c and scanner.h
						 * This code is redundant with part of do_line(),
						 * but do_line() assumes continuation lines come from stdin.
						 */
						/* macros in a clause are problematic, as they are */
						/* only expanded once even if the clause is replayed */
						string_expand_macros();
						// Strip off trailing comment and count curly braces 
						NumTokens = Scanner(&gp_input_line, &gp_input_line_len);
						if(gp_input_line[P_Token[NumTokens].start_index] == '#') {
							gp_input_line[P_Token[NumTokens].start_index] = NUL;
							start = P_Token[NumTokens].start_index;
							left = gp_input_line_len - start;
						}
						/* Read additional lines if necessary to complete a
						 * bracketed clause {...}
						 */
						if(curly_brace_count < 0)
							GPO.IntError(NO_CARET, "Unexpected }");
						if(curly_brace_count > 0) {
							if((len + 4) > gp_input_line_len)
								extend_input_line();
							strcat(gp_input_line, ";\n");
							start = strlen(gp_input_line);
							left = gp_input_line_len - start;
							continue;
						}
						more = FALSE;
					}
				}
			}
			// If we hit a 'break' or 'continue' statement in the lines just processed 
			if(iteration_early_exit())
				continue;
			// process line 
			if(strlen(gp_input_line) > 0) {
				screen_ok = FALSE; /* make sure command line is echoed on error */
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
// called by load_file and load_file_error 
//
//bool lf_pop()
bool GpProgram::LfPop()
{
	if(lf_head == NULL)
		return false;
	else {
		int argindex;
		udvt_entry * udv;
		LFS * lf = lf_head;
		if(!lf->fp || lf->fp == stdin)
			; // Do not close stdin in the case that "-" is named as a load file 
	#if defined(PIPES)
		else if(lf->name != NULL && lf->name[0] == '<')
			pclose(lf->fp);
	#endif
		else
			fclose(lf->fp);
		// call arguments are not relevant when invoked from do_string_and_free 
		if(!lf->cmdline) {
			for(argindex = 0; argindex < 10; argindex++) {
				SAlloc::F(call_args[argindex]);
				call_args[argindex] = lf->call_args[argindex];
			}
			call_argc = lf->call_argc;
			// Restore ARGC and ARG0 ... ARG9 
			if((udv = GPO.Ev.GetUdvByName("ARGC"))) {
				Ginteger(&(udv->udv_value), call_argc);
			}
			if((udv = GPO.Ev.GetUdvByName("ARG0"))) {
				gpfree_string(&(udv->udv_value));
				Gstring(&(udv->udv_value), (lf->prev && lf->prev->name) ? gp_strdup(lf->prev->name) : gp_strdup(""));
			}
			for(argindex = 1; argindex <= 9; argindex++) {
				if((udv = GPO.Ev.GetUdvByName(argname[argindex]))) {
					gpfree_string(&(udv->udv_value));
					if(!call_args[argindex-1])
						udv->udv_value.type = NOTDEFINED;
					else
						Gstring(&(udv->udv_value), gp_strdup(call_args[argindex-1]));
				}
			}
			if((udv = GPO.Ev.GetUdvByName("ARGV")) && udv->udv_value.type == ARRAY) {
				GpValue * ARGV;
				int argv_size = lf->argv[0].v.int_val;
				gpfree_array(&(udv->udv_value));
				udv->udv_value.type = ARRAY;
				ARGV = udv->udv_value.v.value_array = (GpValue *)gp_alloc((argv_size + 1) * sizeof(GpValue), "array state");
				for(argindex = 0; argindex <= argv_size; argindex++)
					ARGV[argindex] = lf->argv[argindex];
			}
		}
		interactive = lf->interactive;
		inline_num = lf->inline_num;
		GPO.Ev.AddUdvByName("GPVAL_LINENO")->udv_value.v.int_val = inline_num;
		if_open_for_else = lf->if_open_for_else;
		// Restore saved input state and free the copy 
		if(lf->P_Tokens) {
			NumTokens = lf->_NumTokens;
			SetTokenIdx(lf->_CToken);
			assert(TokenTableSize >= lf->_NumTokens+1);
			memcpy(P_Token, lf->P_Tokens, (lf->_NumTokens+1) * sizeof(lexical_unit));
			SAlloc::F(lf->P_Tokens);
		}
		if(lf->input_line) {
			strcpy(gp_input_line, lf->input_line);
			SAlloc::F(lf->input_line);
		}
		SAlloc::F(lf->name);
		SAlloc::F(lf->cmdline);
		lf_head = lf->prev;
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
void GpProgram::LfPush(FILE * fp, char * pName, char * pCmdLine)
{
	int    argindex;
	LFS  * lf = (LFS *)gp_alloc(sizeof(LFS), (char*)NULL);
	if(!lf) {
		SFile::ZClose(&fp); // it won't be otherwise 
		GPO.IntErrorCurToken("not enough memory to load file");
	}
	lf->fp = fp;            /* save this file pointer */
	lf->name = pName;
	lf->cmdline = pCmdLine;
	lf->interactive = interactive;  /* save current state */
	lf->inline_num = inline_num;    /* save current line number */
	lf->call_argc = call_argc;
	// Call arguments are irrelevant if invoked from do_string_and_free 
	if(!pCmdLine) {
		udvt_entry * udv;
		// Save ARG0 through ARG9 
		for(argindex = 0; argindex < 10; argindex++) {
			lf->call_args[argindex] = call_args[argindex];
			call_args[argindex] = NULL; /* initially no args */
		}
		// Save ARGV[] 
		lf->argv[0].v.int_val = 0;
		lf->argv[0].type = NOTDEFINED;
		if((udv = GPO.Ev.GetUdvByName("ARGV")) && udv->udv_value.type == ARRAY) {
			for(argindex = 0; argindex <= call_argc; argindex++) {
				lf->argv[argindex] = udv->udv_value.v.value_array[argindex];
				if(lf->argv[argindex].type == STRING)
					lf->argv[argindex].v.string_val = gp_strdup(lf->argv[argindex].v.string_val);
			}
		}
	}
	lf->depth = lf_head ? lf_head->depth+1 : 0; /* recursion depth */
	if(lf->depth > STACK_DEPTH)
		GPO.IntError(NO_CARET, "load/eval nested too deeply");
	lf->if_open_for_else = if_open_for_else;
	lf->_CToken = GetCurTokenIdx();
	lf->_NumTokens = NumTokens;
	lf->P_Tokens = (lexical_unit *)gp_alloc((NumTokens+1) * sizeof(lexical_unit), "lf tokens");
	memcpy(lf->P_Tokens, P_Token, (NumTokens+1) * sizeof(lexical_unit));
	lf->input_line = gp_strdup(gp_input_line);
	lf->prev = lf_head; // link to stack 
	lf_head = lf;
}
//
// used for reread  vsnyder@math.jpl.nasa.gov 
//
FILE * lf_top()
{
	return lf_head ? (lf_head->fp) : 0;
}
//
// called from main 
//
void load_file_error()
{
	// clean up from error in load_file 
	// pop off everything on stack 
	while(GPO.Pgm.LfPop())
		;
}

FILE * loadpath_fopen(const char * filename, const char * mode)
{
	FILE * fp;
	/* The global copy of fullname is only for the benefit of post.trm's
	 * automatic fontfile conversion via a constructed shell command.
	 * FIXME: There was a Feature Request to export the directory path
	 * in which a loaded file was found to a user-visible variable for the
	 * lifetime of that load.  This is close but without the lifetime.
	 */
	ZFREE(loadpath_fontname);
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
	if(fp != NULL)
		_setmode(_fileno(fp), _O_BINARY);
#endif
	return fp;
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
		SAlloc::F(push_term_name);
		SAlloc::F(push_term_opts);
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

/* Pop the terminal.
 * Called anytime by user command "set term pop".
 */
void pop_terminal()
{
	if(push_term_name != NULL) {
		char * s;
		int i = strlen(push_term_name) + 11;
		if(push_term_opts) {
			/* do_string() does not like backslashes -- thus remove them */
			for(s = push_term_opts; *s; s++)
				if(*s=='\\' || *s=='\n') 
					*s = ' ';
			i += strlen(push_term_opts);
		}
		s = (char *)gp_alloc(i, "pop");
		i = interactive;
		interactive = 0;
		sprintf(s, "set term %s %s", push_term_name, (push_term_opts ? push_term_opts : ""));
		GPO.DoStringAndFree(s);
		interactive = i;
		if(interactive)
			fprintf(stderr, "   restored terminal is %s %s\n", term->name, ((*term_options) ? term_options : ""));
	}
	else
		fprintf(stderr, "No terminal has been pushed yet\n");
}
// 
// Parse a plot style. Used by 'set style {data|function}' and by (s)plot.  
// 
enum PLOT_STYLE get_style()                
{
	/* defined in plot.h */
	enum PLOT_STYLE ps;
	GPO.Pgm.Shift();
	ps = (enum PLOT_STYLE)GPO.Pgm.LookupTableForCurrentToken(&plotstyle_tbl[0]);
	GPO.Pgm.Shift();
	if(ps == PLOT_STYLE_NONE)
		GPO.IntErrorCurToken("unrecognized plot type");
	return ps;
}
// 
// Parse options for style filledcurves and fill fco accordingly.
// If no option given, set it to FILLEDCURVES_DEFAULT.
// 
void get_filledcurves_style_options(filledcurves_opts * fco)
{
	enum filledcurves_opts_id p;
	fco->closeto = FILLEDCURVES_DEFAULT;
	fco->oneside = 0;
	while((p = (enum filledcurves_opts_id)GPO.Pgm.LookupTableForCurrentToken(&filledcurves_opts_tbl[0])) != -1) {
		fco->closeto = p;
		GPO.Pgm.Shift();
		if(p == FILLEDCURVES_ABOVE) {
			fco->oneside = 1;
			continue;
		}
		else if(p == FILLEDCURVES_BELOW) {
			fco->oneside = -1;
			continue;
		}
		fco->at = 0;
		if(!GPO.Pgm.EqualsCur("="))
			return;
		/* parameter required for filledcurves x1=... and friends */
		if(p < FILLEDCURVES_ATXY)
			fco->closeto = (filledcurves_opts_id)(fco->closeto + 4);
		GPO.Pgm.Shift();
		fco->at = GPO.RealExpression();
		if(p != FILLEDCURVES_ATXY)
			return;
		/* two values required for FILLEDCURVES_ATXY */
		if(!GPO.Pgm.EqualsCur(","))
			GPO.IntErrorCurToken("syntax is xy=<x>,<y>");
		GPO.Pgm.Shift();
		fco->aty = GPO.RealExpression();
	}
}

/* Print filledcurves style options to a file (used by 'show' and 'save'
 * commands).
 */
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

bool FASTCALL need_fill_border(const fill_style_type * fillstyle)
{
	lp_style_type p;
	p.pm3d_color = fillstyle->border_color;
	if(p.pm3d_color.type == TC_LT) {
		// Doesn't want a border at all 
		if(p.pm3d_color.lt == LT_NODRAW)
			return FALSE;
		load_linetype(term, &p, p.pm3d_color.lt+1);
	}
	// Wants a border in a new color 
	if(p.pm3d_color.type != TC_DEFAULT)
		GPO.ApplyPm3DColor(term, &p.pm3d_color);
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
			pDashTyp->pattern[j++] = RealExpression(); // The solid portion 
			if(!Pgm.EqualsCurShift(","))
				IntErrorCurToken("expecting comma");
			pDashTyp->pattern[j++] = RealExpression(); // The empty portion 
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
#define DSCALE 10.
		while(dash_str[j] && (k < DASHPATTERN_LENGTH || dash_str[j] == ' ')) {
			/* .      Dot with short space
			 * -      Dash with regular space
			 * _      Long dash with regular space
			 * space  Don't add new dash, just increase last space */
			switch(dash_str[j]) {
				case '.':
				    pDashTyp->pattern[k++] = 0.2 * DSCALE;
				    pDashTyp->pattern[k++] = 0.5 * DSCALE;
				    break;
				case '-':
				    pDashTyp->pattern[k++] = 1.0 * DSCALE;
				    pDashTyp->pattern[k++] = 1.0 * DSCALE;
				    break;
				case '_':
				    pDashTyp->pattern[k++] = 2.0 * DSCALE;
				    pDashTyp->pattern[k++] = 1.0 * DSCALE;
				    break;
				case ' ':
				    if(k == 0)
					    leading_space++;
				    else
					    pDashTyp->pattern[k-1] += 1.0 * DSCALE;
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
		safe_strncpy(pDashTyp->dstring, dash_str, sizeof(pDashTyp->dstring));
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
int GnuPlot::LpParse(lp_style_type * lp, lp_class destination_class, bool allow_point)
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
		lp_use_properties(lp, IntExpression());
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
			/* both syntaxes allowed: 'with lt pal' as well as 'with pal' */
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
				if(prefer_line_styles && (destination_class != LP_STYLE))
					lp_use_properties(lp, new_lt);
				else
					load_linetype(term, lp, new_lt);
			}
		} /* linetype, lt */

		/* both syntaxes allowed: 'with lt pal' as well as 'with pal' */
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
				udvt_entry * colormap = get_colormap(Pgm.GetCurTokenIdx()+1);
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
					load_linetype(term, &temp, IntExpression());
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
			newlp.l_width = GPO.RealExpression();
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
					newlp.p_type = PT_CHARACTER;
					truncate_to_one_utf8_char(symbol);
					safe_strncpy(newlp.p_char, symbol, sizeof(newlp.p_char));
					SAlloc::F(symbol);
				}
				else if(Pgm.AlmostEqualsCur("var$iable") && (destination_class == LP_ADHOC)) {
					newlp.p_type = PT_VARIABLE;
					Pgm.Shift();
				}
				else
					newlp.p_type = IntExpression() - 1;
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
					newlp.p_size = PTSZ_VARIABLE;
					Pgm.Shift();
				}
				else if(Pgm.AlmostEqualsCur("def$ault")) {
					newlp.p_size = PTSZ_DEFAULT;
					Pgm.Shift();
				}
				else {
					newlp.p_size = GPO.RealExpression();
					if(newlp.p_size < 0)
						newlp.p_size = 0;
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
			tmp = ParseDashType(&newlp.custom_dash_pattern);
			/* Pull the dashtype from the list of already defined dashtypes, */
			/* but only if it we didn't get an explicit one back from parse_dashtype */
			if(tmp == DASHTYPE_AXIS)
				lp->l_type = LT_AXIS;
			if(tmp >= 0)
				tmp = load_dashtype(&newlp.custom_dash_pattern, tmp + 1);
			newlp.d_type = tmp;
			continue;
		}

		/* caught unknown option -> quit the while(1) loop */
		break;
	}

	if(set_lt > 1 || set_pal > 1 || set_lw > 1 || set_pt > 1 || set_ps > 1 || set_dt > 1
	    || (set_pi + set_pn > 1))
		IntErrorCurToken("duplicate or conflicting arguments in style specification");

	if(set_pal) {
		lp->pm3d_color = newlp.pm3d_color;
		/* hidden3d uses this to decide that a single color surface is wanted */
		lp->flags |= LP_EXPLICIT_COLOR;
	}
	else {
		lp->flags &= ~LP_EXPLICIT_COLOR;
	}
	if(set_lw)
		lp->l_width = newlp.l_width;
	if(set_pt) {
		lp->p_type = newlp.p_type;
		memcpy(lp->p_char, newlp.p_char, sizeof(newlp.p_char));
	}
	if(set_ps)
		lp->p_size = newlp.p_size;
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
		lp->custom_dash_pattern = newlp.custom_dash_pattern;
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
		load_linetype(term, &lptemp, tc->lt + 1);
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
			tc->lt = parse_color_name();
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
		tc->lt = parse_color_name();
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

long parse_color_name()
{
	char * string;
	long color;
	// Terminal drivers call this after seeing a "background" option 
	if(GPO.Pgm.AlmostEqualsCur("rgb$color") && GPO.Pgm.AlmostEquals(GPO.Pgm.GetPrevTokenIdx(), "back$ground"))
		GPO.Pgm.Shift();
	if((string = GPO.TryToGetString())) {
		color = lookup_color_name(string);
		SAlloc::F(string);
		if(color == -2)
			GPO.IntErrorCurToken("unrecognized color name and not a string \"#AARRGGBB\" or \"0xAARRGGBB\"");
	}
	else
		color = GPO.IntExpression();
	return (ulong)(color);
}

/* arrow parsing...
 *
 * allow_as controls whether we are allowed to accept arrowstyle in
 * the current context [ie not when doing a  set style arrow command]
 */

void arrow_use_properties(struct arrow_style_type * arrow, int tag)
{
	/*  This function looks for an arrowstyle defined by 'tag' and
	 *  copies its data into the structure 'ap'. */
	struct arrowstyle_def * p_this;
	/* If a color has already been set for p_this arrow, keep it */
	struct t_colorspec save_colorspec = arrow->lp_properties.pm3d_color;
	/* Default if requested style is not found */
	default_arrow_style(arrow);
	p_this = first_arrowstyle;
	while(p_this != NULL) {
		if(p_this->tag == tag) {
			*arrow = p_this->arrow_properties;
			break;
		}
		else {
			p_this = p_this->next;
		}
	}
	/* tag not found: */
	if(!p_this || p_this->tag != tag)
		GPO.IntWarn(NO_CARET, "arrowstyle %d not found", tag);
	// Restore original color if the style doesn't specify one 
	if(arrow->lp_properties.pm3d_color.type == TC_DEFAULT)
		arrow->lp_properties.pm3d_color = save_colorspec;
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
		else {
			arrow_use_properties(arrow, IntExpression());
		}
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
			struct GpPosition hsize;
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
			LpParse(&arrow->lp_properties, LP_ADHOC, FALSE);
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

void get_image_options(t_image * image)
{
	if(GPO.Pgm.AlmostEqualsCur("pix$els") || GPO.Pgm.EqualsCur("failsafe")) {
		GPO.Pgm.Shift();
		image->fallback = TRUE;
	}
}
/*
 * Try to interpret the next token in a command line as the name of a colormap.
 * If it seems to belong to a valid colormap, return a pointer.
 * Otherwise return NULL.  The caller is responsible for reporting an error.
 */
struct udvt_entry * get_colormap(int token)                    
{
	udvt_entry * colormap = NULL;
	if(GPO.Pgm.TypeUdv(token) == ARRAY) {
		udvt_entry * udv = add_udv(token);
		if((udv->udv_value.v.value_array[0].type == COLORMAP_ARRAY) && (udv->udv_value.v.value_array[0].v.int_val >= 2))
			colormap = udv;
	}
	return colormap;
}
/*
 * Create a pixmap containing an existing colormap palette.
 * This can be used to produce a colorbox for a named palette
 * separate from the automatic colorbox generated for the main palette.
 */
void pixmap_from_colormap(t_pixmap * pixmap)
{
	udvt_entry * colormap = get_colormap(GPO.Pgm.GetCurTokenIdx());
	uint rgb;
	int size, i, ip;
	if(!colormap)
		GPO.IntErrorCurToken("not a colormap");
	GPO.Pgm.Shift();
	SAlloc::F(pixmap->colormapname);
	pixmap->colormapname = gp_strdup(colormap->udv_name);
	size = colormap->udv_value.v.value_array[0].v.int_val;
	pixmap->image_data = (coordval *)gp_realloc(pixmap->image_data, size * 4. * sizeof(coordval), "pixmap");
	/* Unpack ARGB colormap entry into 4 separate values R G B A */
	for(i = 1, ip = 0; i <= size; i++) {
		rgb = colormap->udv_value.v.value_array[i].v.int_val;
		pixmap->image_data[ip++] = ((rgb >> 16) & 0xff) / 255.;
		pixmap->image_data[ip++] = ((rgb >> 8) & 0xff) / 255.;
		pixmap->image_data[ip++] = ((rgb) & 0xff) / 255.;
		pixmap->image_data[ip++] = 255-((rgb >> 24) & 0xff);
	}
	pixmap->ncols = 1;
	pixmap->nrows = size;
}
