// GNUPLOT - eval.c 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
#include <gnuplot.h>
#pragma hdrstop

// Internal prototypes 
static RETSIGTYPE fpe(int an_int);
//
// The stack this operates on 
//
GpStack::GpStack() : Sp(-1), JumpOffset(0)
{
	memzero(St, sizeof(St));
}

GpValue & GpStack::Top()
{
	return St[Sp];
}

void GpStack::Reset()
{
	Sp = -1;
}

// make sure stack's empty 
void GpStack::Check() const
{ 
	if(Sp != -1)
		fprintf(stderr, "\nwarning:  internal error--stack not empty!\n(function called with too many parameters?)\n");
}

bool GpStack::MoreOnStack() const
{
	return (Sp >= 0);
}

void FASTCALL GnuPlot::Push(GpValue * x)
{
	if(EvStk.Sp == STACK_DEPTH - 1)
		IntError(NO_CARET, "stack overflow");
	EvStk.St[++EvStk.Sp] = *x;
	// WARNING - This is a memory leak if the string is not later freed 
	if(x->Type == STRING && x->v.string_val)
		EvStk.St[EvStk.Sp].v.string_val = sstrdup(x->v.string_val);
}

GpValue * FASTCALL GnuPlot::Pop(GpValue * x) 
{
	if(EvStk.Sp < 0)
		IntError(NO_CARET, "stack underflow (function call with missing parameters?)");
	*x = EvStk.St[EvStk.Sp--];
	return (x);
}

#if 0 // (replaced with _FuncTab2) {
//
// The table of built-in functions */
// These must strictly parallel enum operators in eval.h 
//
const struct ft_entry _FuncTab[] = {
	// internal functions: 
	{"push",         f_push},
	{"pushc",   	 f_pushc},
	{"pushd1",  	 f_pushd1},
	{"pushd2",  	 f_pushd2},
	{"pushd",   	 f_pushd},
	{"pop",     	 f_pop},
	{"call",    	 f_call},
	{"calln",   	 f_calln},
	{"sum",     	 f_sum},
	{"lnot",    	 f_lnot},
	{"bnot",    	 f_bnot},
	{"uminus",  	 f_uminus},
	{"lor",     	 f_lor},
	{"land",    	 f_land},
	{"bor",     	 f_bor},
	{"xor",     	 f_xor},
	{"band",    	 f_band},
	{"eq",      	 f_eq},
	{"ne",      	 f_ne},
	{"gt",      	 f_gt},
	{"lt",           f_lt},
	{"ge",           f_ge},
	{"le",           f_le},
	{"leftshift",    f_leftshift},
	{"rightshift",   f_rightshift},
	{"plus",         f_plus},
	{"minus",        f_minus},
	{"mult",         f_mult},
	{"div",          f_div},
	{"mod",          f_mod},
	{"power",        f_power},
	{"factorial",    f_factorial},
	{"bool",         f_bool},
	{"dollars",      f_dollars},             //  for usespec 
	{"concatenate",  f_concatenate},         //  for string variables only 
	{"eqs",          f_eqs},                 //  for string variables only 
	{"nes",          f_nes},                 //  for string variables only 
	{"[]",           f_range},               //  for string variables only 
	{"[]",           f_index},               //  for array variables only 
	{"||",           f_cardinality},         //  for array variables only 
	{"assign",       f_assign},              //  assignment operator '=' 
	{"jump",         f_jump},
	{"jumpz",        f_jumpz},
	{"jumpnz",       f_jumpnz},
	{"jtern",        f_jtern},
	{"",             NULL},                  // Placeholder for SF_START 
#ifdef HAVE_EXTERNAL_FUNCTIONS
	{"",             f_calle},
#endif
	// legal in using spec only 
	{"column",        f_column},
	{"stringcolumn",  f_stringcolumn},       //  for using specs 
	{"strcol",        f_stringcolumn},       //  shorthand form 
	{"columnhead",    f_columnhead},
	{"columnheader",  f_columnhead},
	{"valid",         f_valid},
	{"timecolumn",    f_timecolumn},
	// standard functions: 
	{"real",          f_real},
	{"imag",       	  f_imag},
	{"arg",        	  f_arg},
	{"conj",       	  f_conjg},
	{"conjg",      	  f_conjg},
	{"sin",        	  f_sin},
	{"cos",        	  f_cos},
	{"tan",        	  f_tan},
	{"asin",       	  f_asin},
	{"acos",       	  f_acos},
	{"atan",       	  f_atan},
	{"atan2",      	  f_atan2},
	{"sinh",       	  f_sinh},
	{"cosh",       	  f_cosh},
	{"tanh",       	  f_tanh},
	{"EllipticK",  	  f_ellip_first},
	{"EllipticE",  	  f_ellip_second},
	{"EllipticPi", 	  f_ellip_third},
	{"int",        	  f_int},
	{"round",         f_round},
	{"abs",    		  f_abs},
	{"sgn",    		  f_sgn},
	{"sqrt",   		  f_sqrt},
	{"exp",    		  f_exp},
	{"log10",  		  f_log10},
	{"log",    		  f_log},
	{"besi0",  		  f_besi0},
	{"besi1",  		  f_besi1},
	{"besin",  		  f_besin},
	{"besj0",  		  f_besj0},
	{"besj1",  		  f_besj1},
	{"besjn",  		  f_besjn},
	{"besy0",  		  f_besy0},
	{"besy1",  		  f_besy1},
	{"besyn",  		  f_besyn},
	{"erf",    		  f_erf},
	{"erfc",   		  f_erfc},
	{"gamma",  		  f_gamma},
	{"lgamma", 		  f_lgamma},
	{"ibeta",  		  f_ibeta},
	{"voigt",  		  f_voigt},
	{"rand",   		  f_rand},
	{"floor",  		  f_floor},
	{"ceil",   		  f_ceil},
	{"norm",   		  f_normal},
	{"inverf",        f_inverse_erf},
	{"invnorm",   	  f_inverse_normal},
	{"invigamma", 	  f_inverse_igamma},
	{"invibeta",  	  f_inverse_ibeta},
	{"asinh",     	  f_asinh},
	{"acosh",     	  f_acosh},
	{"atanh",     	  f_atanh},
	{"lambertw",   	  f_lambertw},     //  HBB, from G.Kuhnle 20001107
	{"airy",      	  f_airy},         //  cephes library version 
#ifdef HAVE_AMOS
	{"Ai",            f_amos_Ai},      //  Amos version from libopenspecfun  
	{"Bi",        	  f_amos_Bi},      //  Amos version from libopenspecfun 
	{"BesselI",   	  f_amos_BesselI}, //  Amos version from libopenspecfun 
	{"BesselJ",   	  f_amos_BesselJ}, //  Amos version from libopenspecfun 
	{"BesselK",   	  f_amos_BesselK}, //  Amos version from libopenspecfun 
	{"BesselY",   	  f_amos_BesselY}, //  Amos version from libopenspecfun 
	{"Hankel1",   	  f_Hankel1},      //  Amos version from libopenspecfun 
	{"Hankel2",   	  f_Hankel2},      //  Amos version from libopenspecfun 
#endif
#ifdef HAVE_CEXINT
	{"expint",        f_amos_cexint},  //  Amos algorithm 683 from libamos 
#else
	{"expint",        f_expint},       //  Jim Van Zandt, 20101010 
#endif
#ifdef HAVE_COMPLEX_FUNCS
	{"igamma",        f_Igamma},       //  Complex igamma(a,z) 
	{"LambertW",  	  f_LambertW},     //  Complex W(z,k) 
	{"lnGamma",   	  f_lnGamma},      //  Complex lnGamma(z) 
	{"Sign",      	  f_Sign},         //  Complex sign function 
#else
	{"igamma",    	  f_igamma},       //  Jos van der Woude 1992 
#endif
#ifdef HAVE_LIBCERF
	{"cerf",          f_cerf},         //  complex error function  
	{"cdawson",   	  f_cdawson},      //  complex Dawson's integral 
	{"erfi",      	  f_erfi},         //  imaginary error function 
	{"VP",        	  f_voigtp},       //  Voigt profile 
	{"VP_fwhm",   	  f_VP_fwhm},      //  Voigt profile full width at half maximum 
	{"faddeeva",  	  f_faddeeva},     //  Faddeeva rescaled complex error function "w_of_z" 
	{"FresnelC",  	  f_FresnelC},     //  Fresnel integral cosine term calculated from cerf 
	{"FresnelS",  	  f_FresnelS},     //  Fresnel integral sine term calculated from cerf 
#endif
	{"SynchrotronF",  f_SynchrotronF}, //  Synchrotron F 
	{"tm_sec",        f_tmsec},        //  for timeseries 
	{"tm_min",        f_tmmin},   	   //  for timeseries 
	{"tm_hour",   	  f_tmhour},  	   //  for timeseries 
	{"tm_mday",   	  f_tmmday},  	   //  for timeseries 
	{"tm_mon",    	  f_tmmon},   	   //  for timeseries 
	{"tm_year",   	  f_tmyear},  	   //  for timeseries 
	{"tm_wday",   	  f_tmwday},  	   //  for timeseries 
	{"tm_yday",   	  f_tmyday},  	   //  for timeseries 
	{"tm_week",   	  f_tmweek},  	   //  for timeseries 
	{"sprintf",   	  f_sprintf}, 	   //  for string variables only 
	{"gprintf",   	  f_gprintf}, 	   //  for string variables only 
	{"strlen",    	  f_strlen},  	   //  for string variables only 
	{"strstrt",   	  f_strstrt}, 	   //  for string variables only 
	{"substr",    	  f_range},   	   //  for string variables only 
	{"trim",      	  f_trim},         //  for string variables only 
	{"word",      	  f_word},         //  for string variables only 
	{"words",     	  f_words},        //  implemented as word(s,-1) 
	{"strftime",  	  f_strftime},     //  time to string 
	{"strptime",  	  f_strptime},     //  string to time 
	{"time",      	  f_time},         //  get current time 
	{"system",    	  f_system},       //  "dynamic backtics" 
	{"exist",     	  f_exists},       //  exists("foo") replaces defined(foo) 
	{"exists",    	  f_exists},       //  exists("foo") replaces defined(foo) 
	{"value",     	  f_value},        //  retrieve value of variable known by name 
	{"hsv2rgb",   	  f_hsv2rgb},      //  color conversion 
	{"palette",   	  f_palette},      //  palette color lookup 
	{"rgbcolor",  	  f_rgbcolor},     //  32bit ARGB color lookup by name or string 
#ifdef VOXEL_GRID_SUPPORT
	{"voxel",         f_voxel},        //  extract value of single voxel 
#endif
	{NULL,            NULL}
};
#endif // } 0

enum {
	gpfunc_NONE_,
	gpfunc_PUSH,
	gpfunc_PUSHC,
	gpfunc_PUSHD1,
	gpfunc_PUSHD2,
	gpfunc_PUSHD,
	gpfunc_POP,
	gpfunc_CALL,
	gpfunc_CALLN,
	gpfunc_SUM,
	gpfunc_LNOT,
	gpfunc_BNOT,
	gpfunc_UMINUS,
	gpfunc_LOR,
	gpfunc_LAND,
	gpfunc_BOR,
	gpfunc_XOR,
	gpfunc_BAND,
	gpfunc_EQ,
	gpfunc_NE,
	gpfunc_GT,
	gpfunc_LT,
	gpfunc_GE,
	gpfunc_LE,
	gpfunc_LEFTSHIFT,
	gpfunc_RIGHTSHIFT,
	gpfunc_PLUS,
	gpfunc_MINUS,
	gpfunc_MULT,
	gpfunc_DIV,
	gpfunc_MOD,
	gpfunc_POWER,
	gpfunc_FACTORIAL,
	gpfunc_BOOL,
	gpfunc_DOLLARS,       
	gpfunc_CONCATENATE,   
	gpfunc_EQS,           
	gpfunc_NES,           
	gpfunc_RANGE,         
	gpfunc_INDEX,         
	gpfunc_CARDINALITY,   
	gpfunc_ASSIGN,        
	gpfunc_JUMP,
	gpfunc_JUMPZ,
	gpfunc_JUMPNZ,
	gpfunc_JTERN,
	gpfunc_CALLE,
	gpfunc_COLUMN,
	gpfunc_STRINGCOLUMN, 
	gpfunc_COLUMNHEAD,
	gpfunc_VALID,
	gpfunc_TIMECOLUMN,
	gpfunc_REAL,
	gpfunc_IMAG,
	gpfunc_ARG,
	gpfunc_CONJG,
	gpfunc_SIN,
	gpfunc_COS,
	gpfunc_TAN,
	gpfunc_ASIN,
	gpfunc_ACOS,
	gpfunc_ATAN,
	gpfunc_ATAN2,
	gpfunc_SINH,
	gpfunc_COSH,
	gpfunc_TANH,
	gpfunc_ELLIP_FIRST,
	gpfunc_ELLIP_SECOND,
	gpfunc_ELLIP_THIRD,
	gpfunc_INT,
	gpfunc_ROUND,
	gpfunc_ABS,
	gpfunc_SGN,
	gpfunc_SQRT,
	gpfunc_EXP,
	gpfunc_LOG10,
	gpfunc_LOG,
	gpfunc_BESI0,
	gpfunc_BESI1,
	gpfunc_BESIN,
	gpfunc_BESJ0,
	gpfunc_BESJ1,
	gpfunc_BESJN,
	gpfunc_BESY0,
	gpfunc_BESY1,
	gpfunc_BESYN,
	gpfunc_ERF,
	gpfunc_ERFC,
	gpfunc_GAMMA,
	gpfunc_LGAMMA,
	gpfunc_IBETA,
	gpfunc_VOIGT,
	gpfunc_RAND,
	gpfunc_FLOOR,
	gpfunc_CEIL,
	gpfunc_NORMAL,
	gpfunc_INVERSE_ERF,
	gpfunc_INVERSE_NORMAL,
	gpfunc_INVERSE_IGAMMA,
	gpfunc_INVERSE_IBETA,
	gpfunc_ASINH,
	gpfunc_ACOSH,
	gpfunc_ATANH,
	gpfunc_LAMBERTW,     
	gpfunc_AIRY,         
	gpfunc_AMOS_AI,      
	gpfunc_AMOS_BI,      
	gpfunc_AMOS_BESSELI, 
	gpfunc_AMOS_BESSELJ, 
	gpfunc_AMOS_BESSELK, 
	gpfunc_AMOS_BESSELY, 
	gpfunc_HANKEL1,      
	gpfunc_HANKEL2,      
	gpfunc_AMOS_CEXINT,  
	gpfunc_EXPINT,       
	gpfunc_IGAMMA,       
	gpfunc_LNGAMMA,      
	gpfunc_SIGN,         
	gpfunc_CERF,         
	gpfunc_CDAWSON,      
	gpfunc_ERFI,         
	gpfunc_VOIGTP,       
	gpfunc_VP_FWHM,      
	gpfunc_FADDEEVA,     
	gpfunc_FRESNELC,     
	gpfunc_FRESNELS,     
	gpfunc_SYNCHROTRONF, 
	gpfunc_TMSEC,        
	gpfunc_TMMIN,   	   
	gpfunc_TMHOUR,  	   
	gpfunc_TMMDAY,  	   
	gpfunc_TMMON,   	   
	gpfunc_TMYEAR,  	   
	gpfunc_TMWDAY,  	   
	gpfunc_TMYDAY,  	   
	gpfunc_TMWEEK,  	   
	gpfunc_SPRINTF, 	   
	gpfunc_GPRINTF, 	   
	gpfunc_STRLEN,  	   
	gpfunc_STRSTRT, 	   
	gpfunc_TRIM,         
	gpfunc_WORD,         
	gpfunc_WORDS,        
	gpfunc_STRFTIME,     
	gpfunc_STRPTIME,     
	gpfunc_TIME,         
	gpfunc_SYSTEM,       
	gpfunc_EXISTS,       
	gpfunc_VALUE,        
	gpfunc_HSV2RGB,      
	gpfunc_PALETTE,      
	gpfunc_RGBCOLOR,     
	gpfunc_VOXEL,        
};

const GpFuncEntry _FuncTab2[] = {
	// internal functions: 
	{ gpfunc_PUSH,        "push",    0/*f_push*/},
	{ gpfunc_PUSHC,       "pushc",   0/*f_pushc*/},
	{ gpfunc_PUSHD1,      "pushd1",  0/*f_pushd1*/},
	{ gpfunc_PUSHD2		, "pushd2",  0/*f_pushd2*/},
	{ gpfunc_PUSHD 		, "pushd",   0/*f_pushd*/},
	{ gpfunc_POP   		, "pop",     0/*f_pop*/},
	{ gpfunc_CALL  		, "call",    0/*f_call*/  		   },
	{ gpfunc_CALLN 		, "calln",   0/*f_calln*/ 		   },
	{ gpfunc_SUM   		, "sum",     0/*f_sum*/   		   },
	{ gpfunc_LNOT  		, "lnot",    0/*f_lnot*/  		   },
	{ gpfunc_BNOT  		, "bnot",    0/*f_bnot*/  		   },
	{ gpfunc_UMINUS		, "uminus",  0/*f_uminus*/		   },
	{ gpfunc_LOR   		, "lor",     0/*f_lor*/   		   },
	{ gpfunc_LAND  		, "land",    0/*f_land*/  		   },
	{ gpfunc_BOR   		, "bor",     0/*f_bor*/   		   },
	{ gpfunc_XOR   		, "xor",     0/*f_xor*/   		   },
	{ gpfunc_BAND  		, "band",    0/*f_band*/  		   },
	{ gpfunc_EQ    		, "eq",      0/*f_eq*/    		   },
	{ gpfunc_NE    		, "ne",      0/*f_ne*/    		   },
	{ gpfunc_GT    		, "gt",      0/*f_gt*/    		   },
	{ gpfunc_LT    		, "lt",           0/*f_lt*/    		   },
	{ gpfunc_GE    		, "ge",           0/*f_ge*/    		   },
	{ gpfunc_LE    		, "le",           0/*f_le*/    		   },
	{ gpfunc_LEFTSHIFT  , "leftshift",    0/*f_leftshift*/       },
	{ gpfunc_RIGHTSHIFT , "rightshift",   0/*f_rightshift*/ 	   },
	{ gpfunc_PLUS       , "plus",         0/*f_plus*/       	   },
	{ gpfunc_MINUS      , "minus",        0/*f_minus*/      	   },
	{ gpfunc_MULT       , "mult",         0/*f_mult*/       	   },
	{ gpfunc_DIV        , "div",          0/*f_div*/        	   },
	{ gpfunc_MOD        , "mod",          0/*f_mod*/        	   },
	{ gpfunc_POWER      , "power",        0/*f_power*/      	   },
	{ gpfunc_FACTORIAL  , "factorial",    0/*f_factorial*/  	   },
	{ gpfunc_BOOL       , "bool",         0/*f_bool*/       	   },
	{ gpfunc_DOLLARS    , "dollars",      0/*f_dollars*/         },        //  for usespec 
	{ gpfunc_CONCATENATE, "concatenate",  0/*f_concatenate*/     },    //  for string variables only 
	{ gpfunc_EQS        , "eqs",          0/*f_eqs*/             },            //  for string variables only 
	{ gpfunc_NES        , "nes",          0/*f_nes*/             },               //  for string variables only 
	{ gpfunc_RANGE      , "[]",           0/*f_range*/           },          //  for string variables only 
	{ gpfunc_INDEX      , "[]",           0/*f_index*/           },          //  for array variables only 
	{ gpfunc_CARDINALITY, "||",           0/*f_cardinality*/     },    //  for array variables only 
	{ gpfunc_ASSIGN     , "assign",       0/*f_assign*/          },         //  assignment operator '=' 
	{ gpfunc_JUMP       , "jump",         0/*f_jump*/       	   },
	{ gpfunc_JUMPZ      , "jumpz",        0/*f_jumpz*/      	   },
	{ gpfunc_JUMPNZ     , "jumpnz",       0/*f_jumpnz*/     	   },
	{ gpfunc_JTERN      , "jtern",        0/*f_jtern*/      	   },
	{ gpfunc_NONE_      , "",             NULL              },             // Placeholder for SF_START 
#ifdef HAVE_EXTERNAL_FUNCTIONS
	{ gpfunc_CALLE, "", 0/*f_calle*/ },
#endif
	// legal in using spec only 
	{ gpfunc_COLUMN      , "column",        0/*f_column*/ },
	{ gpfunc_STRINGCOLUMN, "stringcolumn",  0/*f_stringcolumn*/   },     //  for using specs 
	{ gpfunc_STRINGCOLUMN, "strcol",        0/*f_stringcolumn*/   },     //  shorthand form 
	{ gpfunc_COLUMNHEAD  , "columnhead",    0/*f_columnhead*/     },
	{ gpfunc_COLUMNHEAD  , "columnheader",  0/*f_columnhead*/     },
	{ gpfunc_VALID       , "valid",         0/*f_valid*/          },
	{ gpfunc_TIMECOLUMN  , "timecolumn",    0/*f_timecolumn*/     },
	// standard functions: 
	{ gpfunc_REAL         , "real",       0/*f_real*/           },
	{ gpfunc_IMAG 		  , "imag",       0/*f_imag*/ 		   },
	{ gpfunc_ARG  		  , "arg",        0/*f_arg*/ },
	{ gpfunc_CONJG		  , "conj",       0/*f_conjg*/ },
	{ gpfunc_CONJG		  , "conjg",      0/*f_conjg*/		   },
	{ gpfunc_SIN  		  , "sin",        0/*f_sin*/  		   },
	{ gpfunc_COS  		  , "cos",        0/*f_cos*/  		   },
	{ gpfunc_TAN  		  , "tan",        0/*f_tan*/  		   },
	{ gpfunc_ASIN 		  , "asin",       0/*f_asin*/ 		   },
	{ gpfunc_ACOS 		  , "acos",       0/*f_acos*/ 		   },
	{ gpfunc_ATAN 		  , "atan",       0/*f_atan*/ },
	{ gpfunc_ATAN2		  , "atan2",      0/*f_atan2*/		   },
	{ gpfunc_SINH 		  , "sinh",       0/*f_sinh*/ 		   },
	{ gpfunc_COSH 		  , "cosh",       0/*f_cosh*/ 		   },
	{ gpfunc_TANH 		  , "tanh",       0/*f_tanh*/ 		   },
	{ gpfunc_ELLIP_FIRST  , "EllipticK",  0/*f_ellip_first*/    },
	{ gpfunc_ELLIP_SECOND , "EllipticE",  0/*f_ellip_second*/   },
	{ gpfunc_ELLIP_THIRD  , "EllipticPi", 0/*f_ellip_third*/    },
	{ gpfunc_INT          , "int",        0/*f_int*/            },
	{ gpfunc_ROUND 		  , "round",      0/*f_round*/ 		   },
	{ gpfunc_ABS   		  , "abs",    0/*f_abs*/   		   },
	{ gpfunc_SGN   		  , "sgn",    0/*f_sgn*/   		   },
	{ gpfunc_SQRT  		  , "sqrt",   0/*f_sqrt*/  		   },
	{ gpfunc_EXP   		  , "exp",    0/*f_exp*/   		   },
	{ gpfunc_LOG10 		  , "log10",  0/*f_log10*/ 		   },
	{ gpfunc_LOG   		  , "log",    0/*f_log*/   		   },
	{ gpfunc_BESI0 		  , "besi0",  0/*f_besi0*/ 		   },
	{ gpfunc_BESI1 		  , "besi1",  0/*f_besi1*/ 		   },
	{ gpfunc_BESIN 		  , "besin",  0/*f_besin*/ 		   },
	{ gpfunc_BESJ0 		  , "besj0",  0/*f_besj0*/ 		   },
	{ gpfunc_BESJ1 		  , "besj1",  0/*f_besj1*/ 		   },
	{ gpfunc_BESJN 		  , "besjn",  0/*f_besjn*/ 		   },
	{ gpfunc_BESY0 		  , "besy0",  0/*f_besy0*/ 		   },
	{ gpfunc_BESY1 		  , "besy1",  0/*f_besy1*/ 		   },
	{ gpfunc_BESYN 		  , "besyn",  0/*f_besyn*/ 		   },
	{ gpfunc_ERF   		  , "erf",    0/*f_erf*/   		   },
	{ gpfunc_ERFC  		  , "erfc",   0/*f_erfc*/  		   },
	{ gpfunc_GAMMA 		  , "gamma",  0/*f_gamma*/ 		   },
	{ gpfunc_LGAMMA		  , "lgamma", 0/*f_lgamma*/		   },
	{ gpfunc_IBETA 		  , "ibeta",  0/*f_ibeta*/ 		   },
	{ gpfunc_VOIGT 		  , "voigt",  0/*f_voigt*/ 		   },
	{ gpfunc_RAND  		  , "rand",   0/*f_rand*/  		   },
	{ gpfunc_FLOOR 		  , "floor",  0/*f_floor*/ 		   },
	{ gpfunc_CEIL  		  , "ceil",   0/*f_ceil*/  		   },
	{ gpfunc_NORMAL		  , "norm",   0/*f_normal*/		   },
	{ gpfunc_INVERSE_ERF   , "inverf",    0/*f_inverse_erf*/    },
	{ gpfunc_INVERSE_NORMAL, "invnorm",   0/*f_inverse_normal*/ },
	{ gpfunc_INVERSE_IGAMMA, "invigamma", 0/*f_inverse_igamma*/ },
	{ gpfunc_INVERSE_IBETA , "invibeta",  0/*f_inverse_ibeta*/  },
	{ gpfunc_ASINH         , "asinh",     0/*f_asinh*/          },
	{ gpfunc_ACOSH         , "acosh",     0/*f_acosh*/          },
	{ gpfunc_ATANH         , "atanh",     0/*f_atanh*/          },
	{ gpfunc_LAMBERTW      , "lambertw",  0/*f_lambertw*/       },     //  HBB, from G.Kuhnle 20001107
	{ gpfunc_AIRY          , "airy",      0/*f_airy*/           },         //  cephes library version 
#ifdef HAVE_AMOS
	{ gpfunc_AMOS_AI     , "Ai",        0/*f_amos_Ai*/        }, //  Amos version from libopenspecfun  
	{ gpfunc_AMOS_BI     , "Bi",        0/*f_amos_Bi*/        }, //  Amos version from libopenspecfun
	{ gpfunc_AMOS_BESSELI, "BesselI",   0/*f_amos_BesselI*/   }, //  Amos version from libopenspecfun
	{ gpfunc_AMOS_BESSELJ, "BesselJ",   0/*f_amos_BesselJ*/   }, //  Amos version from libopenspecfun
	{ gpfunc_AMOS_BESSELK, "BesselK",   0/*f_amos_BesselK*/   }, //  Amos version from libopenspecfun
	{ gpfunc_AMOS_BESSELY, "BesselY",   0/*f_amos_BesselY*/   }, //  Amos version from libopenspecfun
	{ gpfunc_HANKEL1     , "Hankel1",   0/*f_Hankel1*/        }, //  Amos version from libopenspecfun
	{ gpfunc_HANKEL2     , "Hankel2",   0/*f_Hankel2*/        }, //  Amos version from libopenspecfun
#endif
#ifdef HAVE_CEXINT
	{ gpfunc_AMOS_CEXINT , "expint",    0/*f_amos_cexint*/    },  //  Amos algorithm 683 from libamos 
#else
	{ gpfunc_EXPINT, "expint",        0/*f_expint*/         },       //  Jim Van Zandt, 20101010 
#endif
#ifdef HAVE_COMPLEX_FUNCS
	{ gpfunc_IGAMMA  , "igamma",    0/*f_Igamma*/         },       //  Complex igamma(a,z) 
	{ gpfunc_LAMBERTW, "LambertW",  0/*f_LambertW*/       },     //  Complex W(z,k) 
	{ gpfunc_LNGAMMA , "lnGamma",   0/*f_lnGamma*/        },      //  Complex lnGamma(z) 
	{ gpfunc_SIGN    , "Sign",      0/*f_Sign*/           },         //  Complex sign function 
#else
	{ gpfunc_IGAMMA, "igamma",    0/*f_igamma*/         },       //  Jos van der Woude 1992 
#endif
#ifdef HAVE_LIBCERF
	{ gpfunc_CERF    , "cerf",      0/*f_cerf*/           },   //  complex error function  
	{ gpfunc_CDAWSON , "cdawson",   0/*f_cdawson*/        },   //  complex Dawson's integral 
	{ gpfunc_ERFI    , "erfi",      0/*f_erfi*/           },   //  imaginary error function 
	{ gpfunc_VOIGTP  , "VP",        0/*f_voigtp*/         },   //  Voigt profile 
	{ gpfunc_VP_FWHM , "VP_fwhm",   0/*f_VP_fwhm*/        },   //  Voigt profile full width at half maximum 
	{ gpfunc_FADDEEVA, "faddeeva",  0/*f_faddeeva*/       },   //  Faddeeva rescaled complex error function "w_of_z" 
	{ gpfunc_FRESNELC, "FresnelC",  0/*f_FresnelC*/       },   //  Fresnel integral cosine term calculated from cerf 
	{ gpfunc_FRESNELS, "FresnelS",  	  0/*f_FresnelS*/ },   //  Fresnel integral sine term calculated from cerf 
#endif
	{ gpfunc_SYNCHROTRONF, "SynchrotronF", 0/*f_SynchrotronF*/   },   //  Synchrotron F 
	{ gpfunc_TMSEC       , "tm_sec",       0/*f_tmsec*/          },   //  for timeseries 
	{ gpfunc_TMMIN       , "tm_min",       0/*f_tmmin*/      	   },   //  for timeseries 
	{ gpfunc_TMHOUR      , "tm_hour",   0/*f_tmhour*/    	   },   //  for timeseries 
	{ gpfunc_TMMDAY      , "tm_mday",   0/*f_tmmday*/    	   },   //  for timeseries 
	{ gpfunc_TMMON      , "tm_mon",    0/*f_tmmon*/      	   },   //  for timeseries 
	{ gpfunc_TMYEAR     , "tm_year",   0/*f_tmyear*/    	   },   //  for timeseries 
	{ gpfunc_TMWDAY     , "tm_wday",   0/*f_tmwday*/    	   },   //  for timeseries 
	{ gpfunc_TMYDAY     , "tm_yday",   0/*f_tmyday*/    	   },   //  for timeseries 
	{ gpfunc_TMWEEK     , "tm_week",   0/*f_tmweek*/    	   },   //  for timeseries 
	{ gpfunc_SPRINTF    , "sprintf",   0/*f_sprintf*/  },   //  for string variables only 
	{ gpfunc_GPRINTF    , "gprintf",   0/*f_gprintf*/  },   //  for string variables only 
	{ gpfunc_STRLEN     , "strlen",    0/*f_strlen*/   },   //  for string variables only 
	{ gpfunc_STRSTRT    , "strstrt",   0/*f_strstrt*/  },   //  for string variables only 
	{ gpfunc_RANGE      , "substr",    0/*f_range*/    },   //  for string variables only 
	{ gpfunc_TRIM        , "trim",     0/*f_trim*/     },   //  for string variables only 
	{ gpfunc_WORD        , "word",     0/*f_word*/     },   //  for string variables only 
	{ gpfunc_WORDS       , "words",    0/*f_words*/    },   //  implemented as word(s,-1) 
	{ gpfunc_STRFTIME    , "strftime", 0/*f_strftime*/ },   //  time to string 
	{ gpfunc_STRPTIME    , "strptime", 0/*f_strptime*/ },   //  string to time 
	{ gpfunc_TIME        , "time",     0/*f_time*/     },   //  get current time 
	{ gpfunc_SYSTEM      , "system",   0/*f_system*/   },   //  "dynamic backtics" 
	{ gpfunc_EXISTS      , "exist",    0/*f_exists*/   },   //  exists("foo") replaces defined(foo) 
	{ gpfunc_EXISTS      , "exists",   0/*f_exists*/   },   //  exists("foo") replaces defined(foo) 
	{ gpfunc_VALUE       , "value",    0/*f_value*/    },   //  retrieve value of variable known by name 
	{ gpfunc_HSV2RGB     , "hsv2rgb",  0/*f_hsv2rgb*/  },   //  color conversion 
	{ gpfunc_PALETTE     , "palette",  0/*f_palette*/  },   //  palette color lookup 
	{ gpfunc_RGBCOLOR    , "rgbcolor", 0/*f_rgbcolor*/ },   //  32bit ARGB color lookup by name or string 
#ifdef VOXEL_GRID_SUPPORT
	{ gpfunc_VOXEL,      "voxel", 0/*f_voxel*/ },   //  extract value of single voxel 
#endif
	{ gpfunc_NONE_, NULL, NULL}
};
//
// Module-local variables:
//
static JMP_BUF fpe_env;
//
// Internal helper functions: 
//
static RETSIGTYPE fpe(int /*an_int*/)
{
#if defined(MSDOS) && !defined(__EMX__) && !defined(DJGPP)
	_fpreset(); // thanks to lotto@wjh12.UUCP for telling us about this 
#endif
	signal(SIGFPE, (sigfunc)fpe);
	GPO.Ev.IsUndefined_ = true;
	LONGJMP(fpe_env, TRUE);
}

/* Exported functions */

/* First, some functions that help other modules use 'GpValue' ---
 * these might justify a separate module, but I'll stick with this,
 * for now */
//
// returns the real part of val 
//
double FASTCALL GnuPlot::Real(const GpValue * pVal)
{
	switch(pVal->Type) {
		case INTGR: return static_cast<double>(pVal->v.int_val);
		case CMPLX: return (pVal->v.cmplx_val.real);
		case STRING: return satof(pVal->v.string_val); // is this ever used? 
		case NOTDEFINED: return fgetnan();
		default: IntError(NO_CARET, "unknown type in real()");
	}
	return 0.0; // NOTREACHED 
}
//
// returns the imag part of val 
//
double FASTCALL GnuPlot::Imag(const GpValue * pVal)
{
	switch(pVal->Type) {
		case INTGR: return (0.0);
		case CMPLX: return (pVal->v.cmplx_val.imag);
		case STRING:
		    // This is where we end up if the user tries: 
		    //     x = 2;  plot sprintf(format,x)         
		    IntWarn(NO_CARET, "encountered a string when expecting a number");
		    IntError(NO_CARET, "Did you try to generate a file name using dummy variable x or y?");
		case NOTDEFINED: return fgetnan();
		default: IntError(NO_CARET, "unknown type in imag()");
	}
	return 0.0; // NOTREACHED 
}
//
// returns the magnitude of val 
//
double FASTCALL GnuPlot::Magnitude(const GpValue * pVal)
{
	switch(pVal->Type) {
		case INTGR:
		    return fabs((double)pVal->v.int_val);
		case CMPLX:
	    {
		    // The straightforward implementation sqrt(r*r+i*i)
		    // over-/underflows if either r or i is very large or very
		    // small. This implementation avoids over-/underflows from
		    // squaring large/small numbers whenever possible.  It
		    // only over-/underflows if the correct result would, too.
		    // CAVEAT: sqrt(1+x*x) can still have accuracy problems. 
		    double abs_r = fabs(pVal->v.cmplx_val.real);
		    double abs_i = fabs(pVal->v.cmplx_val.imag);
		    double quotient;
		    if(abs_i == 0)
			    return abs_r;
		    if(abs_r > abs_i) {
			    quotient = abs_i / abs_r;
			    return abs_r * sqrt(1 + quotient*quotient);
		    }
		    else {
			    quotient = abs_r / abs_i;
			    return abs_i * sqrt(1 + quotient*quotient);
		    }
	    }
		default:
		    IntError(NO_CARET, "unknown type in magnitude()");
	}
	return 0.0; // NOTREACHED 
}
//
// returns the angle of val 
//
//double angle(const GpValue * pVal)
double FASTCALL GnuPlot::Angle(const GpValue * pVal)
{
	switch(pVal->Type) {
		case INTGR:
		    return ((pVal->v.int_val >= 0) ? 0.0 : SMathConst::Pi);
		case CMPLX:
		    if(pVal->v.cmplx_val.imag == 0.0) {
			    if(pVal->v.cmplx_val.real >= 0.0)
				    return (0.0);
			    else
				    return (SMathConst::Pi);
		    }
		    return (atan2(pVal->v.cmplx_val.imag, pVal->v.cmplx_val.real));
		default:
		    IntError(NO_CARET, "unknown type in angle()");
	}
	return 0.0; // NOTREACHED 
}

GpValue * Gcomplex(GpValue * a, double realpart, double imagpart) 
{
	a->Type = CMPLX;
	a->v.cmplx_val.real = realpart;
	a->v.cmplx_val.imag = imagpart;
	return (a);
}

GpValue * FASTCALL Ginteger(GpValue * a, intgr_t i) 
{
	a->Type = INTGR;
	a->v.int_val = i;
	return (a);
}

GpValue * FASTCALL Gstring(GpValue * a, char * s) 
{
	a->Type = STRING;
	a->v.string_val = s ? s : sstrdup("");
	return (a);
}
//
// Common interface for freeing data structures attached to a GpValue.
// Each of the type-specific routines will ignore values of other types.
//
void GpValue::Destroy()
{
	gpfree_string(this);
	gpfree_datablock(this);
	gpfree_array(this);
	Type = NOTDEFINED;
}

int GpValue::IntCheck() const
{
	if(Type != INTGR) {
		GPO.IntError(NO_CARET, "non-integer passed to boolean operator");
		return 0;
	}
	else
		return 1;
}
//
// Common interface for freeing data structures attached to a GpValue.
// Each of the type-specific routines will ignore values of other types.
//
/* (replaced with GpValue::Destroy()) void FASTCALL free_value(GpValue * a)
{
	gpfree_string(a);
	gpfree_datablock(a);
	gpfree_array(a);
	a->Type = NOTDEFINED;
}*/
// 
// It is always safe to call gpfree_string with a->Type is INTGR or CMPLX.
// However it would be fatal to call it with a->Type = STRING if a->string_val
// was not obtained by a previous call to SAlloc::M(), or has already been freed.
// Thus 'a->Type' is set to NOTDEFINED afterwards to make subsequent calls safe.
// 
void FASTCALL gpfree_string(GpValue * a)
{
	if(a->Type == STRING) {
		SAlloc::F(a->v.string_val);
		a->Type = NOTDEFINED;
	}
}

void gpfree_array(GpValue * a)
{
	if(a->Type == ARRAY) {
		const int size = a->v.value_array[0].v.int_val;
		for(int i = 1; i <= size; i++)
			gpfree_string(&(a->v.value_array[i]));
		SAlloc::F(a->v.value_array);
		a->Type = NOTDEFINED;
	}
}
// 
// some machines have trouble with exp(-x) for large x
// if E_MINEXP is defined at compile time, use gp_exp(x) instead,
// which returns 0 for exp(x) with x < E_MINEXP
// exp(x) will already have been defined as gp_exp(x) in plot.h
// 
double gp_exp(double x)
{
#ifdef E_MINEXP
	return (x < (E_MINEXP)) ? 0.0 : exp(x);
#else  /* E_MINEXP */
	int old_errno = errno;
	double result = exp(x);
	/* exp(-large) quite uselessly raises ERANGE --- stop that */
	if(result == 0.0)
		errno = old_errno;
	return result;
#endif /* E_MINEXP */
}

//void reset_stack() { s_p = -1; }

// make sure stack's empty 
//void check_stack() { if(s_p != -1) fprintf(stderr, "\nwarning:  internal error--stack not empty!\n(function called with too many parameters?)\n"); }
//bool more_on_stack() { return (s_p >= 0); }

// 
// Allow autoconversion of string variables to floats if they
// are dereferenced in a numeric context.
// 
//GpValue * FASTCALL pop_or_convert_from_string(GpValue * v) 
GpValue * FASTCALL GnuPlot::PopOrConvertFromString(GpValue * v)
{
	Pop(v);
	// FIXME: Test for INVALID_VALUE? Other corner cases? 
	if(v->Type == INVALID_NAME)
		IntError(NO_CARET, "invalid dummy variable name");
	if(v->Type == STRING) {
		char * eov;
		if(*(v->v.string_val) && strspn(v->v.string_val, "0123456789 ") == strlen(v->v.string_val)) {
			int64 li = atoll(v->v.string_val);
			gpfree_string(v);
			Ginteger(v, static_cast<intgr_t>(li));
		}
		else {
			double d = strtod(v->v.string_val, &eov);
			if(v->v.string_val == eov) {
				gpfree_string(v);
				IntError(NO_CARET, "Non-numeric string found where a numeric expression was expected");
				// Note: This also catches syntax errors like "set term ''*0 " 
			}
			gpfree_string(v);
			Gcomplex(v, d, 0.);
			FPRINTF((stderr, "converted string to CMPLX value %g\n", Real(v)));
		}
	}
	return v;
}

//void FASTCALL int_check(const GpValue * v) { if(v->type != INTGR) GPO.IntError(NO_CARET, "non-integer passed to boolean operator"); }
// 
// Internal operators of the stack-machine, not directly represented
// by any user-visible operator, or using private status variables directly 
//
// converts top-of-stack to boolean 
//void f_bool(union argument * /*x*/)
void GnuPlot::F_Bool(union argument * /*x*/)
{
	EvStk.Top().IntCheck();
	EvStk.Top().v.int_val = !!EvStk.Top().v.int_val;
}

//void f_jump(union argument * x)
void GnuPlot::F_Jump(union argument * x)
{
	EvStk.SetJumpOffset(x->j_arg);
}

//void f_jumpz(union argument * x)
void GnuPlot::F_Jumpz(union argument * x)
{
	GpValue a;
	EvStk.Top().IntCheck();
	if(EvStk.Top().v.int_val) { // non-zero --> no jump
		Pop(&a);
	}
	else
		EvStk.SetJumpOffset(x->j_arg); // leave the argument on TOS 
}

//void f_jumpnz(union argument * x)
void GnuPlot::F_Jumpnz(union argument * x)
{
	GpValue a;
	EvStk.Top().IntCheck();
	if(EvStk.Top().v.int_val) // non-zero 
		EvStk.SetJumpOffset(x->j_arg); // leave the argument on TOS 
	else {
		Pop(&a);
	}
}

//void f_jtern(union argument * x)
void GnuPlot::F_Jtern(union argument * x)
{
	GpValue a;
	Pop(&a)->IntCheck();
	if(!a.v.int_val)
		EvStk.SetJumpOffset(x->j_arg); // go jump to FALSE code 
}
// 
// This is the heart of the expression evaluation module: the stack program execution loop.
// 
// 'ft' is a table containing C functions within this program.
// 
// An 'action_table' contains pointers to these functions and
// arguments to be passed to them.
// 
// at_ptr is a pointer to the action table which must be executed (evaluated).
// 
// so the iterated line executes the function indexed by the at_ptr
// and passes the address of the argument which is pointed to by the arg_ptr
// 
void FASTCALL GnuPlot::_ExecuteAt2(at_type * pAt)
{
	const int saved_jump_offset = EvStk.GetJumpOffset();
	int count = pAt->a_count;
	for(int instruction_index = 0; instruction_index < count;) {
		const int op_ = (int)pAt->actions[instruction_index].index;
		EvStk.SetJumpOffset(1); // jump operators can modify this 
		argument * p_arg = &pAt->actions[instruction_index].arg;
		switch(_FuncTab2[op_].FuncId) {
			case gpfunc_PUSH:            F_Push(p_arg); break; 
			case gpfunc_PUSHC:			 F_Pushc(p_arg); break;
			case gpfunc_PUSHD1:			 F_Pushd1(p_arg); break;
			case gpfunc_PUSHD2:			 F_Pushd2(p_arg); break;
			case gpfunc_PUSHD:			 F_Pushd(p_arg); break;
			case gpfunc_POP:			 F_Pop(p_arg); break;
			case gpfunc_CALL:			 F_Call(p_arg); break;
			case gpfunc_CALLN:			 F_Calln(p_arg); break;
			case gpfunc_SUM:			 F_Sum(p_arg); break;
			case gpfunc_LNOT:			 F_LNot(p_arg); break;
			case gpfunc_BNOT:			 F_BNot(p_arg); break;
			case gpfunc_UMINUS:			 F_UMinus(p_arg); break;
			case gpfunc_LOR:			 F_LOr(p_arg); break;
			case gpfunc_LAND:			 F_LAnd(p_arg); break;
			case gpfunc_BOR:			 F_BOr(p_arg); break;
			case gpfunc_XOR:			 F_XOr(p_arg); break;
			case gpfunc_BAND:			 F_BAnd(p_arg); break;
			case gpfunc_EQ:				 F_Eq(p_arg); break;
			case gpfunc_NE:				 F_Ne(p_arg); break;
			case gpfunc_GT:				 F_Gt(p_arg); break;
			case gpfunc_LT:				 F_Lt(p_arg); break;
			case gpfunc_GE:				 F_Ge(p_arg); break;
			case gpfunc_LE:				 F_Le(p_arg); break;
			case gpfunc_LEFTSHIFT:		 F_LeftShift(p_arg); break;
			case gpfunc_RIGHTSHIFT:		 F_RightShift(p_arg); break;
			case gpfunc_PLUS:			 F_Plus(p_arg); break;
			case gpfunc_MINUS:			 F_Minus(p_arg); break;
			case gpfunc_MULT:			 F_Mult(p_arg); break;
			case gpfunc_DIV:			 F_Div(p_arg); break;
			case gpfunc_MOD:			 F_Mod(p_arg); break;
			case gpfunc_POWER:			 F_Power(p_arg); break;
			case gpfunc_FACTORIAL:		 F_Factorial(p_arg); break;
			case gpfunc_BOOL:			 F_Bool(p_arg); break;
			case gpfunc_DOLLARS:       	 F_Dollars(p_arg); break;       
			case gpfunc_CONCATENATE:   	 F_Concatenate(p_arg); break;   
			case gpfunc_EQS:           	 F_Eqs(p_arg); break;           
			case gpfunc_NES:           	 F_Nes(p_arg); break;           
			case gpfunc_RANGE:         	 F_Range(p_arg); break;         
			case gpfunc_INDEX:         	 F_Index(p_arg); break;         
			case gpfunc_CARDINALITY:   	 F_Cardinality(p_arg); break;   
			case gpfunc_ASSIGN:        	 F_Assign(p_arg); break;        
			case gpfunc_JUMP:			 F_Jump(p_arg); break;
			case gpfunc_JUMPZ:			 F_Jumpz(p_arg); break;
			case gpfunc_JUMPNZ:			 F_Jumpnz(p_arg); break;
			case gpfunc_JTERN:			 F_Jtern(p_arg); break;
#ifdef HAVE_EXTERNAL_FUNCTIONS
			case gpfunc_CALLE:			 F_Calle(p_arg); break;
#endif
			case gpfunc_COLUMN:			 F_Column(p_arg); break;
			case gpfunc_STRINGCOLUMN: 	 F_StringColumn(p_arg); break; 
			case gpfunc_COLUMNHEAD:		 F_Columnhead(p_arg); break;
			case gpfunc_VALID:			 F_Valid(p_arg); break;
			case gpfunc_TIMECOLUMN:		 F_TimeColumn(p_arg); break;
			case gpfunc_REAL:			 F_Real(p_arg); break;
			case gpfunc_IMAG:			 F_Imag(p_arg); break;
			case gpfunc_ARG:			 F_Arg(p_arg); break;
			case gpfunc_CONJG:			 F_Conjg(p_arg); break;
			case gpfunc_SIN:			 F_Sin(p_arg); break;
			case gpfunc_COS:			 F_Cos(p_arg); break;
			case gpfunc_TAN:			 F_Tan(p_arg); break;
			case gpfunc_ASIN:			 F_ASin(p_arg); break;
			case gpfunc_ACOS:			 F_ACos(p_arg); break;
			case gpfunc_ATAN:			 F_ATan(p_arg); break;
			case gpfunc_ATAN2:			 F_ATan2(p_arg); break;
			case gpfunc_SINH:			 F_Sinh(p_arg); break;
			case gpfunc_COSH:			 F_Cosh(p_arg); break;
			case gpfunc_TANH:			 F_Tanh(p_arg); break;
			case gpfunc_ELLIP_FIRST:	 F_EllipFirst(p_arg); break;
			case gpfunc_ELLIP_SECOND:	 F_EllipSecond(p_arg); break;
			case gpfunc_ELLIP_THIRD:	 F_EllipThird(p_arg); break;
			case gpfunc_INT:			 F_Int(p_arg); break;
			case gpfunc_ROUND:			 F_Round(p_arg); break;
			case gpfunc_ABS:			 F_Abs(p_arg); break;
			case gpfunc_SGN:			 F_Sgn(p_arg); break;
			case gpfunc_SQRT:			 F_Sqrt(p_arg); break;
			case gpfunc_EXP:			 F_Exp(p_arg); break;
			case gpfunc_LOG10:			 F_Log10(p_arg); break;
			case gpfunc_LOG:			 F_Log(p_arg); break;
			case gpfunc_BESI0:			 F_Besi0(p_arg); break;
			case gpfunc_BESI1:			 F_Besi1(p_arg); break;
			case gpfunc_BESIN:			 F_Besin(p_arg); break;
			case gpfunc_BESJ0:			 F_Besj0(p_arg); break;
			case gpfunc_BESJ1:			 F_Besj1(p_arg); break;
			case gpfunc_BESJN:			 F_Besjn(p_arg); break;
			case gpfunc_BESY0:			 F_Besy0(p_arg); break;
			case gpfunc_BESY1:			 F_Besy1(p_arg); break;
			case gpfunc_BESYN:			 F_Besyn(p_arg); break;
			case gpfunc_ERF:			 F_Erf(p_arg); break;
			case gpfunc_ERFC:			 F_Erfc(p_arg); break;
			case gpfunc_GAMMA:			 F_Gamma(p_arg); break;
			case gpfunc_LGAMMA:			 F_LGamma(p_arg); break;
			case gpfunc_IBETA:			 F_IBeta(p_arg); break;
			case gpfunc_VOIGT:			 F_Voigt(p_arg); break;
			case gpfunc_RAND:			 F_Rand(p_arg); break;
			case gpfunc_FLOOR:			 F_Floor(p_arg); break;
			case gpfunc_CEIL:			 F_Ceil(p_arg); break;
			case gpfunc_NORMAL:			 F_Normal(p_arg); break;
			case gpfunc_INVERSE_ERF:	 F_InverseErf(p_arg); break;
			case gpfunc_INVERSE_NORMAL:	 F_InverseNormal(p_arg); break;
			case gpfunc_INVERSE_IGAMMA:	 F_InverseIGamma(p_arg); break;
			case gpfunc_INVERSE_IBETA:	 F_InverseIBeta(p_arg); break;
			case gpfunc_ASINH:			 F_ASinh(p_arg); break;
			case gpfunc_ACOSH:			 F_ACosh(p_arg); break;
			case gpfunc_ATANH:			 F_ATanh(p_arg); break;
			case gpfunc_LAMBERTW:     	 F_Lambertw(p_arg); break;
			case gpfunc_AIRY:         	 F_Airy(p_arg); break;
#ifdef HAVE_AMOS
			case gpfunc_AMOS_AI:      	 F_amos_Ai(p_arg); break;      
			case gpfunc_AMOS_BI:      	 F_amos_Bi(p_arg); break;      
			case gpfunc_AMOS_BESSELI: 	 F_amos_BesselI(p_arg); break; 
			case gpfunc_AMOS_BESSELJ: 	 F_amos_BesselJ(p_arg); break; 
			case gpfunc_AMOS_BESSELK: 	 F_amos_BesselK(p_arg); break; 
			case gpfunc_AMOS_BESSELY: 	 F_amos_BesselY(p_arg); break; 
			case gpfunc_HANKEL1:      	 F_Hankel1(p_arg); break;      
			case gpfunc_HANKEL2:      	 F_Hankel2(p_arg); break;      
#endif
#ifdef HAVE_CEXINT
			case gpfunc_AMOS_CEXINT:  	 F_amos_cexint(p_arg); break;  
#else
			case gpfunc_EXPINT:       	 F_ExpInt(p_arg); break;       
#endif
#ifdef HAVE_COMPLEX_FUNCS
			case gpfunc_IGAMMA:       	 F_IGamma(p_arg); break;       
			case gpfunc_LAMBERTW:     	 F_LambertW(p_arg); break; 
			case gpfunc_LNGAMMA:      	 F_lnGamma(p_arg); break;      
			case gpfunc_SIGN:         	 F_Sign(p_arg); break;         
#else
			case gpfunc_IGAMMA:          F_IGamma(p_arg); break;
#endif
#ifdef HAVE_LIBCERF
			case gpfunc_CERF:         	 F_Cerf(p_arg); break;         
			case gpfunc_CDAWSON:      	 F_CDawson(p_arg); break;      
			case gpfunc_ERFI:         	 F_Erfi(p_arg); break;         
			case gpfunc_VOIGTP:       	 F_Voigtp(p_arg); break;       
			case gpfunc_VP_FWHM:      	 F_VP_Fwhm(p_arg); break;      
			case gpfunc_FADDEEVA:     	 F_Faddeeva(p_arg); break;     
			case gpfunc_FRESNELC:     	 F_FresnelC(p_arg); break;     
			case gpfunc_FRESNELS:     	 F_FresnelS(p_arg); break;     
#endif
			case gpfunc_SYNCHROTRONF: 	 F_SynchrotronF(p_arg); break; 
			case gpfunc_TMSEC:        	 F_TmSec(p_arg); break;        
			case gpfunc_TMMIN:   	   	 F_TmMin(p_arg); break;   	   
			case gpfunc_TMHOUR:  	   	 F_TmHour(p_arg); break;  	   
			case gpfunc_TMMDAY:  	   	 F_TmMDay(p_arg); break;  	   
			case gpfunc_TMMON:   	   	 F_TmMon(p_arg); break;   	   
			case gpfunc_TMYEAR:  	   	 F_TmYear(p_arg); break;  	   
			case gpfunc_TMWDAY:  	   	 F_TmWDay(p_arg); break;  	   
			case gpfunc_TMYDAY:  	   	 F_TmYDay(p_arg); break;  	   
			case gpfunc_TMWEEK:  	   	 F_TmWeek(p_arg); break;  	   
			case gpfunc_SPRINTF: 	   	 F_SPrintf(p_arg); break; 	   
			case gpfunc_GPRINTF: 	   	 F_GPrintf(p_arg); break; 	   
			case gpfunc_STRLEN:  	   	 F_Strlen(p_arg); break;  	   
			case gpfunc_STRSTRT: 	   	 F_Strstrt(p_arg); break; 	   
			case gpfunc_TRIM:         	 F_Trim(p_arg); break;         
			case gpfunc_WORD:         	 F_Word(p_arg); break;         
			case gpfunc_WORDS:        	 F_Words(p_arg); break;        
			case gpfunc_STRFTIME:     	 F_StrFTime(p_arg); break;     
			case gpfunc_STRPTIME:     	 F_StrPTime(p_arg); break;     
			case gpfunc_TIME:         	 F_Time(p_arg); break;         
			case gpfunc_SYSTEM:       	 F_System(p_arg); break;       
			case gpfunc_EXISTS:       	 F_Exists(p_arg); break;       
			case gpfunc_VALUE:        	 F_Value(p_arg); break;
			case gpfunc_HSV2RGB:      	 F_Hsv2Rgb(p_arg); break;
			case gpfunc_PALETTE:      	 F_Palette(p_arg); break;
			case gpfunc_RGBCOLOR:     	 F_RgbColor(p_arg); break;
#ifdef VOXEL_GRID_SUPPORT
			case gpfunc_VOXEL:        	 F_Voxel(p_arg); break;
#endif
		}
		assert(is_jump(op_) || (EvStk.GetJumpOffset() == 1));
		instruction_index += EvStk.GetJumpOffset();
	}
	EvStk.SetJumpOffset(saved_jump_offset);
}
//
// As of May 2013 input of Inf/NaN values through evaluation is treated 
// equivalently to direct input of a formatted value.  See imageNaN.dem. 
//
//void evaluate_at(at_type * at_ptr, GpValue * val_ptr)
void GnuPlot::EvaluateAt(at_type * pAt, GpValue * pVal)
{
	// A test for if (undefined) is allowed only immediately following
	// evalute_at() or eval_link_function().  Both must clear it on entry
	// so that the value on return reflects what really happened.
	Ev.IsUndefined_ = false;
	errno = 0;
	//reset_stack();
	EvStk.Reset();
	if(!_Df.evaluate_inside_using || !_Df.df_nofpe_trap) {
		if(SETJMP(fpe_env, 1))
			return;
		signal(SIGFPE, (sigfunc)fpe);
	}
	_ExecuteAt2(pAt);
	if(!_Df.evaluate_inside_using || !_Df.df_nofpe_trap)
		signal(SIGFPE, SIG_DFL);
	if(oneof2(errno, EDOM, ERANGE))
		Ev.IsUndefined_ = true;
	else if(!Ev.IsUndefined_) {
		Pop(pVal);
		//check_stack();
		EvStk.Check();
	}
	if(!Ev.IsUndefined_ && pVal->Type == ARRAY) {
		// Aug 2016: error rather than warning because too many places
		// cannot deal with UNDEFINED or NaN where they were expecting a number
		// E.g. load_one_range()
		pVal->Type = NOTDEFINED;
		if(!_Pb.string_result_only)
			IntError(NO_CARET, "evaluate_at: unsupported array operation");
	}
}

void FASTCALL real_free_at(at_type * at_ptr)
{
	// All string constants belonging to this action table have to be freed before destruction.
	if(at_ptr) {
		for(int i = 0; i<at_ptr->a_count; i++) {
			at_entry * a = &(at_ptr->actions[i]);
			// if union a->arg is used as a->arg.v_arg free potential string 
			if(oneof2(a->index, PUSHC, DOLLARS))
				gpfree_string(&(a->arg.v_arg));
			// a summation contains its own action table wrapped in a private udf 
			if(a->index == SUM) {
				real_free_at(a->arg.udf_arg->at);
				SAlloc::F(a->arg.udf_arg);
			}
#ifdef HAVE_EXTERNAL_FUNCTIONS
			// external function calls contain a parameter list 
			if(a->index == CALLE)
				SAlloc::F(a->arg.exf_arg);
#endif
		}
		SAlloc::F(at_ptr);
	}
}
// 
// EAM July 2003 - Return pointer to udv with this name; if the key does not
// match any existing udv names, create a new one and return a pointer to it.
// 
//udvt_entry * add_udv_by_name(const char * key) 
udvt_entry * GpEval::AddUdvByName(const char * pKey)
{
	udvt_entry ** udv_ptr = &P_FirstUdv;
	// check if it's already in the table... 
	while(*udv_ptr) {
		if(sstreq(pKey, (*udv_ptr)->udv_name))
			return (*udv_ptr);
		udv_ptr = &((*udv_ptr)->next_udv);
	}
	*udv_ptr = (udvt_entry *)SAlloc::M(sizeof(udvt_entry));
	(*udv_ptr)->next_udv = NULL;
	(*udv_ptr)->udv_name = sstrdup(pKey);
	(*udv_ptr)->udv_value.SetNotDefined();
	return (*udv_ptr);
}

//udvt_entry * get_udv_by_name(const char * key) 
udvt_entry * GpEval::GetUdvByName(const char * pKey)
{
	udvt_entry * udv = P_FirstUdv;
	while(udv) {
		if(sstreq(pKey, udv->udv_name))
			return udv;
		udv = udv->next_udv;
	}
	return 0;
}
//
// This doesn't really delete, it just marks the udv as undefined 
//
//void del_udv_by_name(char * key, bool wildcard)
void GnuPlot::DelUdvByName(const char * pKey, bool wildcard)
{
	udvt_entry * udv_ptr = *Ev.PP_UdvUserHead;
	while(udv_ptr) {
		// Forbidden to delete GPVAL_* 
		if(!strncmp(udv_ptr->udv_name, "GPVAL", 5))
			;
		else if(!strncmp(udv_ptr->udv_name, "GNUTERM", 7))
			;
		// exact match 
		else if(!wildcard && sstreq(pKey, udv_ptr->udv_name)) {
			_VG.FreeGrid(udv_ptr);
			udv_ptr->udv_value.Destroy();
			udv_ptr->udv_value.SetNotDefined();
			break;
		}
		// wildcard match: prefix matches 
		else if(wildcard && !strncmp(pKey, udv_ptr->udv_name, strlen(pKey)) ) {
			_VG.FreeGrid(udv_ptr);
			udv_ptr->udv_value.Destroy();
			udv_ptr->udv_value.SetNotDefined();
			// no break - keep looking! 
		}
		udv_ptr = udv_ptr->next_udv;
	}
}
//
// Clear (delete) all user defined functions 
//
void GpEval::ClearUdfList()
{
	udft_entry * udf_ptr = P_FirstUdf;
	udft_entry * udf_next;
	while(udf_ptr) {
		SAlloc::F(udf_ptr->udf_name);
		SAlloc::F(udf_ptr->definition);
		free_at(udf_ptr->at);
		udf_next = udf_ptr->next_udf;
		SAlloc::F(udf_ptr);
		udf_ptr = udf_next;
	}
	P_FirstUdf = NULL;
}

void GnuPlot::SetGpValAxisSthDouble(const char * pPrefix, AXIS_INDEX axIdx, const char * pSuffix, double value)
{
	udvt_entry * v;
	char * cc, s[24];
	sprintf(s, "%s_%s_%s", pPrefix, axis_name(axIdx), pSuffix);
	for(cc = s; *cc; cc++)
		*cc = toupper((uchar)*cc); /* make the name uppercase */
	v = Ev.AddUdvByName(s);
	if(!v)
		return; /* should not happen */
	Gcomplex(&v->udv_value, value, 0);
}

void GnuPlot::FillGpValAxis(AXIS_INDEX axIdx)
{
	const char * prefix = "GPVAL";
	const GpAxis * ap = &AxS[axIdx];
	SetGpValAxisSthDouble(prefix, axIdx, "MIN", ap->min);
	SetGpValAxisSthDouble(prefix, axIdx, "MAX", ap->max);
	SetGpValAxisSthDouble(prefix, axIdx, "LOG", ap->base);
	if(axIdx < POLAR_AXIS) {
		SetGpValAxisSthDouble("GPVAL_DATA", axIdx, "MIN", ap->data_min);
		SetGpValAxisSthDouble("GPVAL_DATA", axIdx, "MAX", ap->data_max);
	}
}
// 
// Fill variable "var" visible by "show var" or "show var all" ("GPVAL_*")
// by the given value (string, integer, float, complex).
// 
void FASTCALL GpEval::FillGpValString(const char * var, const char * pValue)
{
	udvt_entry * v = AddUdvByName(var);
	if(v) {
		if(v->udv_value.Type == STRING && sstreq(v->udv_value.v.string_val, pValue))
			return;
		else
			gpfree_string(&v->udv_value);
		Gstring(&v->udv_value, sstrdup(pValue));
	}
}

void FASTCALL GpEval::FillGpValInteger(const char * var, intgr_t value)
{
	udvt_entry * v = AddUdvByName(var);
	if(v)
		Ginteger(&v->udv_value, value);
}

void GpEval::FillGpValFoat(const char * var, double value)
{
	udvt_entry * v = AddUdvByName(var);
	if(v)
		Gcomplex(&v->udv_value, value, 0);
}

void GpEval::FillGpValComplex(const char * var, double areal, double aimag)
{
	udvt_entry * v = AddUdvByName(var);
	if(v)
		Gcomplex(&v->udv_value, areal, aimag);
}
// 
// Export axis bounds in terminal coordinates from previous plot.
// This allows offline mapping of pixel coordinates onto plot coordinates.
// 
void GnuPlot::UpdatePlotBounds(GpTermEntry * pTerm)
{
	Ev.FillGpValInteger("GPVAL_TERM_XMIN", static_cast<intgr_t>(AxS[FIRST_X_AXIS].term_lower / pTerm->tscale));
	Ev.FillGpValInteger("GPVAL_TERM_XMAX", static_cast<intgr_t>(AxS[FIRST_X_AXIS].term_upper / pTerm->tscale));
	Ev.FillGpValInteger("GPVAL_TERM_YMIN", static_cast<intgr_t>(AxS[FIRST_Y_AXIS].term_lower / pTerm->tscale));
	Ev.FillGpValInteger("GPVAL_TERM_YMAX", static_cast<intgr_t>(AxS[FIRST_Y_AXIS].term_upper / pTerm->tscale));
	Ev.FillGpValInteger("GPVAL_TERM_XSIZE", V.BbCanvas.xright+1);
	Ev.FillGpValInteger("GPVAL_TERM_YSIZE", V.BbCanvas.ytop+1);
	Ev.FillGpValInteger("GPVAL_TERM_SCALE", static_cast<intgr_t>(pTerm->tscale));
	// May be useful for debugging font problems 
	Ev.FillGpValInteger("GPVAL_TERM_HCHAR", pTerm->ChrH);
	Ev.FillGpValInteger("GPVAL_TERM_VCHAR", pTerm->ChrV);
}
// 
// Put all the handling for GPVAL_* variables in this one routine.
// We call it from one of several contexts:
// 0: following a successful set/unset command
// 1: following a successful plot/splot
// 2: following an unsuccessful command (int_error)
// 3: program entry
// 4: explicit reset of error status
// 5: directory changed
// 6: X11 Window ID changed
// 
void GnuPlot::UpdateGpvalVariables(GpTermEntry * pTerm, int context)
{
	// These values may change during a plot command due to auto range 
	if(context == 1) {
		FillGpValAxis(FIRST_X_AXIS);
		FillGpValAxis(FIRST_Y_AXIS);
		FillGpValAxis(SECOND_X_AXIS);
		FillGpValAxis(SECOND_Y_AXIS);
		FillGpValAxis(FIRST_Z_AXIS);
		FillGpValAxis(COLOR_AXIS);
		FillGpValAxis(T_AXIS);
		FillGpValAxis(U_AXIS);
		FillGpValAxis(V_AXIS);
		Ev.FillGpValFoat("GPVAL_R_MIN", AxS.__R().min);
		Ev.FillGpValFoat("GPVAL_R_MAX", AxS.__R().max);
		Ev.FillGpValFoat("GPVAL_R_LOG", AxS.__R().base);
		UpdatePlotBounds(pTerm);
		Ev.FillGpValInteger("GPVAL_PLOT", Gg.Is3DPlot ? 0 : 1);
		Ev.FillGpValInteger("GPVAL_SPLOT", Gg.Is3DPlot ? 1 : 0);
		Ev.FillGpValInteger("GPVAL_VIEW_MAP", _3DBlk.splot_map ? 1 : 0);
		Ev.FillGpValFoat("GPVAL_VIEW_ROT_X", _3DBlk.SurfaceRotX);
		Ev.FillGpValFoat("GPVAL_VIEW_ROT_Z", _3DBlk.SurfaceRotZ);
		Ev.FillGpValFoat("GPVAL_VIEW_SCALE", _3DBlk.SurfaceScale);
		Ev.FillGpValFoat("GPVAL_VIEW_ZSCALE", _3DBlk.SurfaceZScale);
		Ev.FillGpValFoat("GPVAL_VIEW_AZIMUTH", _3DBlk.Azimuth);
		// Screen coordinates of 3D rotational center and radius of the sphere */
		// in which x/y axes are drawn after 'set view equal xy[z]' */
		Ev.FillGpValFoat("GPVAL_VIEW_XCENT", (double)(V.BbCanvas.xright+1 - _3DBlk.Middle.x)/(double)(V.BbCanvas.xright+1));
		Ev.FillGpValFoat("GPVAL_VIEW_YCENT", 1.0 - (double)(V.BbCanvas.ytop+1 - _3DBlk.Middle.y)/(double)(V.BbCanvas.ytop+1));
		Ev.FillGpValFoat("GPVAL_VIEW_RADIUS", 0.5 * _3DBlk.SurfaceScale * _3DBlk.Scaler.x/(double)(V.BbCanvas.xright+1));
		return;
	}
	// These are set after every "set" command, which is kind of silly
	// because they only change after 'set term' 'set output' ...
	if(oneof3(context, 0, 2, 3)) {
		// This prevents a segfault if term==NULL, which can 
		// happen if set_terminal() exits via IntError().
		if(!pTerm)
			Ev.FillGpValString("GPVAL_TERM", "unknown");
		else
			Ev.FillGpValString("GPVAL_TERM", pTerm->name);
		Ev.FillGpValString("GPVAL_TERMOPTIONS", GPT._TermOptions);
		Ev.FillGpValString("GPVAL_OUTPUT", NZOR(GPT.P_OutStr, ""));
		Ev.FillGpValString("GPVAL_ENCODING", encoding_names[GPT._Encoding]);
		Ev.FillGpValString("GPVAL_MINUS_SIGN", NZOR(GpU.minus_sign, "-"));
		Ev.FillGpValString("GPVAL_MICRO", NZOR(GpU.micro, "u"));
		Ev.FillGpValString("GPVAL_DEGREE_SIGN", GpU.degree_sign);
	}
	// If we are called from IntError() then set the error state 
	if(context == 2)
		Ev.FillGpValInteger("GPVAL_ERRNO", 1);
	// These initializations need only be done once, on program entry 
	if(context == 3) {
		udvt_entry * v = Ev.AddUdvByName("GPVAL_VERSION");
		char * tmp;
		if(v && v->udv_value.Type == NOTDEFINED)
			Gcomplex(&v->udv_value, satof(gnuplot_version), 0);
		v = Ev.AddUdvByName("GPVAL_PATCHLEVEL");
		if(v && v->udv_value.Type == NOTDEFINED)
			Ev.FillGpValString("GPVAL_PATCHLEVEL", gnuplot_patchlevel);
		v = Ev.AddUdvByName("GPVAL_COMPILE_OPTIONS");
		if(v && v->udv_value.Type == NOTDEFINED)
			Ev.FillGpValString("GPVAL_COMPILE_OPTIONS", compile_options);
		// Start-up values 
		Ev.FillGpValInteger("GPVAL_MULTIPLOT", 0);
		Ev.FillGpValInteger("GPVAL_PLOT", 0);
		Ev.FillGpValInteger("GPVAL_SPLOT", 0);
		tmp = get_terminals_names();
		Ev.FillGpValString("GPVAL_TERMINALS", tmp);
		SAlloc::F(tmp);
		Ev.FillGpValString("GPVAL_ENCODING", encoding_names[GPT._Encoding]);
		// Permanent copy of user-clobberable variables pi and NaN 
		Ev.FillGpValFoat("GPVAL_pi", SMathConst::Pi);
		Ev.FillGpValFoat("GPVAL_NaN", fgetnan());
		FillGpValSysInfo(); // System information 
	}
	if(oneof2(context, 3, 4)) {
		Ev.FillGpValInteger("GPVAL_ERRNO", 0);
		Ev.FillGpValString("GPVAL_ERRMSG", "");
		Ev.FillGpValInteger("GPVAL_SYSTEM_ERRNO", 0);
		Ev.FillGpValString("GPVAL_SYSTEM_ERRMSG", "");
	}
	// GPVAL_PWD is unreliable.  If the current directory becomes invalid,
	// GPVAL_PWD does not reflect this.  If this matters, the user can
	// instead do something like    MY_PWD = "`pwd`"
	if(oneof2(context, 3, 5)) {
		char * save_file = (char *)SAlloc::M(PATH_MAX);
		int ierror = (GP_GETCWD(save_file, PATH_MAX) == NULL);
		Ev.FillGpValString("GPVAL_PWD", ierror ? "" : save_file);
		SAlloc::F(save_file);
	}
	if(context == 6) {
		Ev.FillGpValInteger("GPVAL_TERM_WINDOWID", Gg.current_x11_windowid);
	}
}
// 
// System information is stored in GPVAL_BITS GPVAL_MACHINE GPVAL_SYSNAME 
// 
#ifdef HAVE_UNAME
	#include <sys/utsname.h>
#elif defined(_WIN32)
	//#include <windows.h>
#endif

void GnuPlot::FillGpValSysInfo()
{
// For linux/posix systems with uname 
#ifdef HAVE_UNAME
	struct utsname uts;
	if(uname(&uts) < 0)
		return;
	Ev.FillGpValString("GPVAL_SYSNAME", uts.sysname);
	Ev.FillGpValString("GPVAL_MACHINE", uts.machine);
// For Windows systems 
#elif defined(_WIN32)
	SYSTEM_INFO stInfo;
	OSVERSIONINFO osvi;
	char s[30];
	memzero(&osvi, sizeof(osvi));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);
	snprintf(s, 30, "Windows_NT-%ld.%ld", osvi.dwMajorVersion, osvi.dwMinorVersion);
	Ev.FillGpValString("GPVAL_SYSNAME", s);
	GetSystemInfo(&stInfo);
	switch(stInfo.wProcessorArchitecture) {
		case PROCESSOR_ARCHITECTURE_INTEL: Ev.FillGpValString("GPVAL_MACHINE", "x86"); break;
		case PROCESSOR_ARCHITECTURE_IA64: Ev.FillGpValString("GPVAL_MACHINE", "ia64"); break;
		case PROCESSOR_ARCHITECTURE_AMD64: Ev.FillGpValString("GPVAL_MACHINE", "x86_64"); break;
		default: Ev.FillGpValString("GPVAL_MACHINE", "unknown");
	}
#endif
	// For all systems 
	Ev.FillGpValInteger("GPVAL_BITS", 8 * sizeof(void *));
}
//
// Callable wrapper for the words() internal function 
//
int GnuPlot::Gp_Words(char * string)
{
	GpValue a;
	Push(Gstring(&a, string));
	F_Words(0);
	Pop(&a);
	return a.v.int_val;
}
// 
// Callable wrapper for the word() internal function 
// 
char * GnuPlot::Gp_Word(char * string, int i)
{
	GpValue a;
	Push(Gstring(&a, string));
	Push(Ginteger(&a, (intgr_t)i));
	F_Word((union argument *)NULL);
	Pop(&a);
	return a.v.string_val;
}
