// GNUPLOT - external.c 
// Copyright 2002 Stephan Boettcher
//
#include <gnuplot.h>
#pragma hdrstop

#ifdef HAVE_EXTERNAL_FUNCTIONS

#ifdef _WIN32
	#include "win/winmain.h"
#endif

typedef GpValue (* exfn_t)(int, GpValue *, void *);
typedef void * (* infn_t)(exfn_t);
typedef void (* fifn_t)(void *);

struct exft_entry {
	exfn_t exfn;
	fifn_t fifn;
	udft_entry * args;
	void * P_Private;
};

//void f_calle(union argument * x)
void GnuPlot::F_Calle(union argument * x)
{
	GpValue r = x->exf_arg->exfn(x->exf_arg->args->dummy_num, x->exf_arg->args->dummy_values, x->exf_arg->P_Private);
	if(r.Type == INVALID_VALUE)
		r = Ev.P_UdvNaN->udv_value;
	EvStk.Push(&r);
}

#ifdef _WIN32
	static void * dll_open_w(const char * f)
	{
		LPWSTR w = UnicodeText((f), encoding);
		void * dl = (void*)LoadLibraryW(w);
		SAlloc::F(w);
		return dl;
	}
#endif
// 
// Parse the string argument for a dll filename and function.  Create a
// one-item action list that calls a plugin function.  Call the _init,
// if present.  _init may return a pointer to private data that is
// handed over to the external function for each call.
// 
//at_type * external_at(const char * func_name)
at_type * GnuPlot::ExternalAt(const char * func_name)
{
	char * file = NULL;
	char * func;
	gp_dll_t dl;
	exfn_t exfn;
	infn_t infn;
	fifn_t fifn;
	at_type * at = NULL;
	if(!Pgm.IsString(Pgm.GetCurTokenIdx()))
		IntErrorCurToken("expecting external function filename");
	// NB: cannot use TryToGetString() inside an expression evaluation 
	Pgm.MQuoteCapture(&file, Pgm.GetCurTokenIdx(), Pgm.GetCurTokenIdx());
	if(!file)
		IntErrorCurToken("expecting external function filename");
	GpExpandTilde(&file);
	func = strrchr(file, ':');
	if(func) {
		func[0] = 0;
		func++;
	}
	else {
		func = (char *)func_name;
	}
	/* 1st try:  "file" */
	dl = DLL_OPEN(file);
	if(!dl) {
		char * err = DLL_ERROR(dl);
		char * s;
#ifdef DLL_PATHSEP
		int no_path = !(s = strrchr(file, DLL_PATHSEP[0]));
#else
		int no_path = 0;
#endif
#ifdef DLL_EXT
		int no_ext  = !strrchr(no_path ? file : s, '.');
#else
		int no_ext = 0;
#endif
		if(no_path || no_ext) {
			char * nfile = (char *)SAlloc::M(strlen(file)+7);
#ifdef DLL_EXT
			if(no_ext) {
				strcpy(nfile, file);
				strcat(nfile, DLL_EXT);
				/* 2nd try:  "file.so" */
				dl = DLL_OPEN(nfile);
			}
#endif
#ifdef DLL_PATHSEP
			if(!dl && no_path) {
				strcpy(nfile, "." DLL_PATHSEP);
				strcat(nfile, file);
				/* 3rd try:  "./file" */
				dl = DLL_OPEN(nfile);
#ifdef DLL_EXT
				if(!dl && no_ext) {
					strcat(nfile, DLL_EXT);
					/* 4th try:  "./file.so" */
					dl = DLL_OPEN(nfile);
				}
#endif
			}
#endif
			SAlloc::F(nfile);
		}
		if(!dl) {
			if(!err || !*err)
				err = "cannot load external function";
			IntWarnCurToken(err);
			goto bailout;
		}
	}
	exfn = (exfn_t)DLL_SYM(dl, func);
	if(!exfn) {
		char * err = DLL_ERROR(dl);
		SETIFZ(err, "external function not found");
		IntWarnCurToken(err);
		goto bailout;
	}
	infn = (infn_t)DLL_SYM(dl, "gnuplot_init");
	fifn = (fifn_t)DLL_SYM(dl, "gnuplot_fini");
	if(!(at = (at_type *)SAlloc::M(sizeof(at_type))))
		goto bailout;
	memzero(at, sizeof(*at));       /* reset action table !!! */
	at->a_count = 1;
	at->actions[0].index = CALLE;
	at->actions[0].arg.exf_arg = (exft_entry *)SAlloc::M(sizeof(exft_entry));
	if(!at->actions[0].arg.exf_arg) {
		ZFREE(at);
		goto bailout;
	}
	at->actions[0].arg.exf_arg->exfn = exfn; /* must be freed later */
	at->actions[0].arg.exf_arg->fifn = fifn;
	at->actions[0].arg.exf_arg->args = Pgm.dummy_func;
	if(!infn)
		at->actions[0].arg.exf_arg->P_Private = 0x0;
	else
		at->actions[0].arg.exf_arg->P_Private = (*infn)(exfn);
bailout:
	Pgm.Shift();
	SAlloc::F(file);
	return at;
}
/*
   Called with the at of a UDF about to be redefined.  Test if the
   function was external, and call its _fini, if any.
 */
void external_free(at_type * at)
{
	if(at && at->a_count == 1 && at->actions[0].index == CALLE && at->actions[0].arg.exf_arg->fifn)
		(*at->actions[0].arg.exf_arg->fifn)(at->actions[0].arg.exf_arg->P_Private);
}

#endif /* HAVE_EXTERNAL_FUNCTIONS */
