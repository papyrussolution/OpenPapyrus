/*
 * $Id: external.c,v 1.1 2014/02/28 00:24:20 sfeam Exp $
 */
/* GNUPLOT - external.c */

/*[
 * Copyright 2002 Stephan Boettcher
 *
 * Gnuplot license:
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
 *
 * Alternative license:
 *
 * As an alternative to distributing code in this file under the gnuplot license,
 * you may instead comply with the terms below. In this case, redistribution and
 * use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.  Redistributions in binary
 * form must reproduce the above copyright notice, this list of conditions and
 * the following disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
   ]*/
#include <gnuplot.h>
#pragma hdrstop
#ifdef HAVE_EXTERNAL_FUNCTIONS

typedef t_value (*exfn_t)(int, t_value *, void *);
typedef void * (*infn_t)(exfn_t);
typedef void (*fifn_t)(void *);

struct exft_entry {
	exfn_t exfn;
	fifn_t fifn;
	UdftEntry * args;
	void * P_Private;
};

void f_calle(GpArgument * x)
{
	t_value r = x->exf_arg->exfn(x->exf_arg->args->dummy_num, x->exf_arg->args->dummy_values, x->exf_arg->P_Private);
	if(r.type == INVALID_VALUE)
		r = GpGg.Ev.udv_NaN->udv_value;
	GpGg.Ev.Push(&r);
}

/*
   Parse the sring argument for a dll filename and function.  Create a
   one-item action list that calls a plugin function.  Call the _init,
   if present.  _init may return a pointer to private data that is
   handed over to the external function for each call.
 */

AtType * GpCommand::ExternalAt(const char * pFuncName)                 
{
	char * file = NULL;
	char * func;
	gp_dll_t dl;
	exfn_t exfn;
	infn_t infn;
	fifn_t fifn;
	AtType * at = NULL;
	if(!IsString(CToken))
		GpGg.IntErrorCurToken("expecting external function filename");
	// NB: cannot use TryToGetString() inside an expression evaluation 
	MQuoteCapture(&file, CToken, CToken);
	if(!file)
		GpGg.IntErrorCurToken("expecting external function filename");
	gp_expand_tilde(&file);
	func = strrchr(file, ':');
	if(func) {
		func[0] = 0;
		func++;
	}
	else {
		func = (char*)pFuncName;
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
			char * nfile = (char *)malloc(strlen(file)+7);
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
			free(nfile);
		}

		if(!dl) {
			if(!err || !*err)
				err = "cannot load external function";
			int_warn(CToken, err);
			goto bailout;
		}
	}
	exfn = (exfn_t)DLL_SYM(dl, func);
	if(!exfn) {
		char * err = DLL_ERROR(dl);
		SETIFZ(err, "external function not found");
		int_warn(CToken, err);
		goto bailout;
	}
	infn = (infn_t)DLL_SYM(dl, "gnuplot_init");
	fifn = (fifn_t)DLL_SYM(dl, "gnuplot_fini");
	if(!(at = (AtType *)malloc(sizeof(AtType))))
		goto bailout;
	memzero(at, sizeof(*at));     /* reset action table !!! */
	at->a_count = 1;
	at->actions[0].Index = CALLE;
	at->actions[0].arg.exf_arg = (struct exft_entry *)malloc(sizeof(struct exft_entry));
	if(!at->actions[0].arg.exf_arg) {
		free(at);
		at = NULL;
		goto bailout;
	}
	at->actions[0].arg.exf_arg->exfn = exfn; /* must be freed later */
	at->actions[0].arg.exf_arg->fifn = fifn;
	at->actions[0].arg.exf_arg->args = P_DummyFunc;
	at->actions[0].arg.exf_arg->P_Private = infn ? (*infn)(exfn) : 0;
bailout:
	CToken++;
	free(file);
	return at;
}
//
// Called with the at of a UDF about to be redefined.  Test if the
// function was external, and call its _fini, if any.
//
void external_free(AtType * at)
{
	if(at && at->a_count == 1 && at->actions[0].Index == CALLE && at->actions[0].arg.exf_arg->fifn)
		(*at->actions[0].arg.exf_arg->fifn)(at->actions[0].arg.exf_arg->P_Private);
}

#endif /* HAVE_EXTERNAL_FUNCTIONS */
