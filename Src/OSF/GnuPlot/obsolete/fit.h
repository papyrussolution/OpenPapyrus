/*
 * $Id: fit.h,v 1.31 2015/02/26 18:43:58 sfeam Exp $
 */

/* GNUPLOT - fit.h */

/*  NOTICE: Change of Copyright Status
 *
 *  The author of this module, Carsten Grammes, has expressed in
 *  personal email that he has no more interest in this code, and
 *  doesn't claim any copyright. He has agreed to put this module
 *  into the public domain.
 *
 *  Lars Hecking  15-02-1999
 */

/*
 *	Header file: public functions in fit.c
 *
 *
 *	Previous copyright of this module:   Carsten Grammes, 1993
 *      Experimental Physics, University of Saarbruecken, Germany
 *
 *	Internet address: cagr@rz.uni-sb.de
 *
 *	Permission to use, copy, and distribute this software and its
 *	documentation for any purpose with or without fee is hereby granted,
 *	provided that the above copyright notice appear in all copies and
 *	that both that copyright notice and this permission notice appear
 *	in supporting documentation.
 *
 *      This software is provided "as is" without express or implied warranty.
 */


#ifndef GNUPLOT_FIT_H		/* avoid multiple inclusions */
#define GNUPLOT_FIT_H

/* #if... / #include / #define collection: */

//#include "syscfg.h"
#include "stdfn.h"
//#include "gp_types.h"

#define DEF_FIT_LIMIT 1e-5
#define LAMBDA_UP_FACTOR 10
#define LAMBDA_DOWN_FACTOR 10

// error interrupt for fitting routines 
#define Eex(a)       { ErrorEx(NO_CARET, (a)); }
#define Eex2(a,b)    { ErrorEx(NO_CARET, (a), (b)); }
#define Eex3(a,b,c)  { ErrorEx(NO_CARET, (a), (b), (c)); }
#define Eexc(c,a)    { ErrorEx((c), (a)); }
#define Eexc2(c,a,b) { ErrorEx((c), (a), (b)); }

/* Type definitions */

enum verbosity_level {
    QUIET, 
	RESULTS, 
	BRIEF, 
	VERBOSE
};

typedef char fixstr[MAX_ID_LEN+1];

class GpFit {
public:
	GpFit()
	{
		fitlogfile = NULL;
		fit_script = NULL;
		fit_wrap = 0;
		fit_verbosity = BRIEF;
		fit_suppress_log = false;
		fit_errorvariables = true;
		fit_covarvariables = false;
		fit_errorscaling = true;
		fit_prescale = true;
		fit_v4compatible = false;
		FITLIMIT = "FIT_LIMIT";
		FITSTARTLAMBDA = "FIT_START_LAMBDA";
		FITLAMBDAFACTOR = "FIT_LAMBDA_FACTOR";
		FITMAXITER = "FIT_MAXITER";
		epsilon_abs = 0.0; // default to zero non-relative limit
		maxiter = 0;
		//
		epsilon = DEF_FIT_LIMIT;
		startup_lambda = 0.0;
		lambda_down_factor = LAMBDA_DOWN_FACTOR;
		lambda_up_factor = LAMBDA_UP_FACTOR;
		fitlogfile_default = "fit.log";
		GNUFITLOG = "FIT_LOG";
		log_f = NULL;
		fit_show_lambda = true;
		GP_FIXED = "# FIXED";
		FITSCRIPT = "FIT_SCRIPT";
		DEFAULT_CMD = "replot";
		num_data = 0;
		num_params = 0;
		num_indep = 0;
		num_errors = 0;
		memzero(err_cols, sizeof(err_cols));
		columns = 0;
		fit_x = 0;
		fit_z = 0;
		err_data = 0;
		P_A = 0;
		regress_C = 0;
		regress_cleanup = NULL;
		user_stop = false;
		scale_params = 0;
		MEMSZERO(func);
		par_name = 0;
		last_par_name = NULL;
		last_num_params = 0;
		memzero(last_dummy_var, sizeof(last_dummy_var));
		last_fit_command = NULL;
		memzero(fit_dummy_udvs, sizeof(fit_dummy_udvs));
	}
	static void   FitCommand();

	enum marq_res_t {
		OK, 
		ML_ERROR, 
		BETTER, 
		WORSE
	};
	void   Init();
	void   Update(char * pfile, char * npfile);
	marq_res_t Marquardt(double a[], double ** C, double * chisq, double * lambda);
	void   Calculate(double * zfunc, double ** dzda, double a[]);
	void   Analyze(double a[], double ** C, double d[], double * chisq, double ** deriv);
	void   CallGnuplot(const double * par, double * data);
	bool   Regress(double a[]);
	bool   RegressCheckStop(int iter, double chisq, double last_chisq, double lambda);
	void   RegressFinalize(int iter, double chisq, double last_chisq, double lambda, double ** covar);
	void   FitShow(int i, double chisq, double last_chisq, double* a, double lambda, FILE * device);
	void   FitShowBrief(int iter, double chisq, double last_chisq, double* parms, double lambda, FILE * device);
	void   FitProgress(int i, double chisq, double last_chisq, double* a, double lambda, FILE * device);
	void   ShowResults(double chisq, double last_chisq, double * a, double * dpar, double ** corel);
	char * GetFitScript();
	char * GetLogfile();
	bool   FitInterrupt();
	void   ErrorEx(int t_num, const char * str, ...);
#if defined(VA_START) && defined(STDC_HEADERS)
	void   Dblfn(const char * fmt, ...);
#else
	void   Dblfn(const char * fmt, va_dcl);
#endif
	size_t WriToFilLastFitCmd(FILE * fp);

	const char * FITLIMIT;
	const char * FITSTARTLAMBDA;
	const char * FITLAMBDAFACTOR;
	const char * FITMAXITER;
	char * fitlogfile;
	char * fit_script;
	int    maxiter;
	int    fit_wrap;
	double epsilon_abs;  /* absolute convergence criterion */
	verbosity_level fit_verbosity;
	bool   fit_errorscaling;
	bool   fit_prescale;
	bool   fit_suppress_log;
	bool   fit_errorvariables;
	bool   fit_covarvariables;
	bool   fit_v4compatible;

	double ** regress_C;  // global copy of C matrix in regress 
private:
	void   Implement_FitCommand();
	double EffectiveError(double ** deriv, int i);
	void   CalcEerivatives(const double * par, double * data, double ** deriv);
	void   RegressInit();
	void   BackupFile(char * tofile, const char * fromfile);

	double epsilon; // relative convergence limit
	double startup_lambda;
	double lambda_down_factor;
	double lambda_up_factor;
	const char * fitlogfile_default;
	const char * GNUFITLOG;
	FILE * log_f;
	bool fit_show_lambda;
	const char * GP_FIXED;
	const char * FITSCRIPT;
	const char * DEFAULT_CMD;      /* if no fitscript spec. */
	int num_data;
	int num_params;
	int num_indep;    /* # independent variables in fit function */
	int num_errors;   /* # error columns */
	bool err_cols[MAX_NUM_VAR+1];    /* true if variable has an associated error */
	int columns;      /* # values read from data file for each point */
	double * fit_x;       // all independent variable values, e.g. value of the ith variable from the jth data point is in fit_x[j*num_indep+i] 
	double * fit_z;       /* dependent data values */
	double * err_data;    /* standard deviations of indep. and dependent data */
	double * P_A;         // array of fitting parameters 
	void (* regress_cleanup)(); // memory cleanup function callback 
	bool user_stop;
	double * scale_params; // scaling values for parameters 
	UdftEntry func;
	fixstr * par_name;
	fixstr * last_par_name;
	int last_num_params;
	char * last_dummy_var[MAX_NUM_VAR];
	char * last_fit_command;;
	//
	// Mar 2014 - the single hottest call path in fit was looking up the
	// dummy parameters by name (4 billion times in fit.dem).
	// A total waste, since they don't change.  Look up once and store here.
	//
	UdvtEntry * fit_dummy_udvs[MAX_NUM_VAR];
};

extern GpFit GpF; // @global
//
// Exported Variables of fit.c 
//
/*
extern const char *FITLIMIT;
extern const char *FITSTARTLAMBDA;
extern const char *FITLAMBDAFACTOR;
extern const char *FITMAXITER;
extern char *fitlogfile;
extern bool fit_suppress_log;
extern bool fit_errorvariables;
extern bool fit_covarvariables;
extern verbosity_level fit_verbosity;
extern bool fit_errorscaling;
extern bool fit_prescale;
extern char *fit_script;
extern double epsilon_abs;  // absolute convergence criterion
extern int maxiter;
extern int fit_wrap;
extern bool fit_v4compatible;
*/

// Prototypes of functions exported by fit.c 
/*
#if defined(__GNUC__)
	void error_ex(int t_num, const char *str, ...) __attribute__((noreturn));
#elif defined(_MSC_VER)
	__declspec(noreturn) void error_ex(int t_num, const char *str, ...);
#else
	void error_ex(int t_num, const char *str, ...);
#endif
*/
//void init_fit();
//void update(char * pfile, char *npfile);
//void fit_command();
//size_t wri_to_fil_last_fit_cmd __PROTO((FILE *fp));
//char *getfitlogfile();
//char *getfitscript();

//void call_gnuplot(const double *par, double *data);
//bool regress_check_stop(int iter, double chisq, double last_chisq, double lambda);
//void fit_progress(int i, double chisq, double last_chisq, double* a, double lambda, FILE *device);

#endif /* GNUPLOT_FIT_H */
