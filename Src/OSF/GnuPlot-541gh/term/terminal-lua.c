// GNUPLOT - lua.trm
// Copyright 2008   Peter Hedwig <peter@affenbande.org>
//
#include <gnuplot.h>
#pragma hdrstop
#include "driver.h"

// @experimental {
#define TERM_BODY
#define TERM_PUBLIC static
#define TERM_TABLE
#define TERM_TABLE_START(x) GpTermEntry x {
#define TERM_TABLE_END(x)   };
// } @experimental

#define LUA_TERM_REVISION "$Rev: Jun 2020$"
#define GNUPLOT_LUA_DIR   "share/lua" // @sobolev

#ifdef TERM_REGISTER
	register_term(lua)
#endif

//#ifdef TERM_PROTO
TERM_PUBLIC void LUA_options(GpTermEntry * pThis, GnuPlot * pGp);
TERM_PUBLIC void LUA_init(GpTermEntry * pThis);
TERM_PUBLIC void LUA_reset(GpTermEntry * pThis);
TERM_PUBLIC void LUA_text(GpTermEntry * pThis);
// scale 
TERM_PUBLIC void LUA_graphics(GpTermEntry * pThis);
TERM_PUBLIC void LUA_move(GpTermEntry * pThis, uint x, uint y);
TERM_PUBLIC void LUA_vector(GpTermEntry * pThis, uint ux, uint uy);
TERM_PUBLIC void LUA_linetype(GpTermEntry * pThis, int linetype);
TERM_PUBLIC void LUA_dashtype(GpTermEntry * pThis, int type, t_dashtype * custom_dash_type);
TERM_PUBLIC void LUA_put_text(GpTermEntry * pThis, uint x, uint y, const char str[]);
TERM_PUBLIC int  LUA_text_angle(GpTermEntry * pThis, int ang);
TERM_PUBLIC int  LUA_justify_text(GpTermEntry * pThis, enum JUSTIFY mode);
TERM_PUBLIC void LUA_point(GpTermEntry * pThis, uint x, uint y, int number);
TERM_PUBLIC void LUA_arrow(GpTermEntry * pThis, uint sx, uint sy, uint ex, uint ey, int head);
TERM_PUBLIC int  LUA_set_font(GpTermEntry * pThis, const char * font);
TERM_PUBLIC void LUA_pointsize(GpTermEntry * pThis, double ptsize);
TERM_PUBLIC void LUA_boxfill(GpTermEntry * pThis, int style, uint x1, uint y1, uint width, uint height);
TERM_PUBLIC void LUA_linewidth(GpTermEntry * pThis, double width);
TERM_PUBLIC int  LUA_make_palette(GpTermEntry * pThis, t_sm_palette *);
TERM_PUBLIC void LUA_previous_palette(GpTermEntry * pThis);
TERM_PUBLIC void LUA_set_color(GpTermEntry * pThis, const t_colorspec *);
TERM_PUBLIC void LUA_filled_polygon(GpTermEntry * pThis, int, gpiPoint *);
TERM_PUBLIC void LUA_image(GpTermEntry * pThis, uint, uint, coordval *, const gpiPoint *, t_imagecolor);
TERM_PUBLIC void LUA_path(GpTermEntry * pThis, int p);
TERM_PUBLIC void LUA_boxed_text(GpTermEntry * pThis, uint, uint, int);

// defaults 
#define LUA_XMAX 10000.0
#define LUA_YMAX 10000.0

#define LUA_HTIC        100
#define LUA_VTIC        100
#define LUA_HCHAR       160
#define LUA_VCHAR       420
#define LUA_TERM_DESCRIPTION "Lua generic terminal driver"

// gnuplot 4.3, term->tscale 
#define LUA_TSCALE   1.0
//#endif /* TERM_PROTO */

#ifndef TERM_PROTO_ONLY
#ifdef TERM_BODY
#ifdef HAVE_CAIROPDF
	#include "wxterminal/gp_cairo.h"
	#include "wxterminal/gp_cairo_helpers.h"
	#define LUA_EXTERNAL_IMAGES 1
#elif defined(HAVE_GD_PNG) && (GD2_VERS >= 2)
	#include "gd.h"
	#define LUA_EXTERNAL_IMAGES 1
#endif

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

static lua_State * /*L*/P_LuaS = NULL;
static char * LUA_script = NULL;
static int lua_term_status, lua_term_result;
static int tb, luaterm, image_cnt, image_extern;

#if LUA_VERSION_NUM > 501
/*
 * two helper functions to ease transitioning to lua 5.2
 */

/*
 * same as lua_getfield(pL, LUA_GLOBALINDEXS, f) in lua 5.1
 */
static void LUA_getfield_global(lua_State * pL, const char * f)
{
	lua_rawgeti(pL, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
	lua_getfield(pL, -1, f);
	lua_replace(pL, -2);
}
/*
 * approximately the same as luaL_register(pL, libname, l) in lua 5.1
 */
static void LUA_register(lua_State * pL, const char * libname, const luaL_Reg * l)
{
	if(!libname)
		luaL_setfuncs(pL, l, 0);
	else {
		LUA_getfield_global(pL, "package");
		lua_getfield(pL, -1, "loaded");
		lua_newtable(pL);
		luaL_setfuncs(pL, l, 0);
		lua_pushvalue(pL, -1);
		lua_setglobal(pL, libname);
		lua_setfield(pL, -2, libname);
		lua_pop(pL, 2);
		lua_getglobal(pL, libname);
	}
}

#endif /* LUA_VERSION_NUM > 501 */

/*
 * static buffer, because we cannot free memory after
 * calling int_error that never returns
 */
static char last_error_msg[MAX_LINE_LEN+1] = "";

/*
 * Handle Lua functions
 */
#define LUA_GP_FNC "gp"
// 
// returns a table with the coords of
// the plot's bounding box
// 
static int LUA_GP_get_boundingbox(lua_State * pL) 
{
	lua_newtable(pL);
	lua_pushstring(pL, "xleft");
	lua_pushinteger(pL, GPO.V.BbPlot.xleft);
	lua_rawset(pL, -3);
	lua_pushstring(pL, "xright");
	lua_pushinteger(pL, GPO.V.BbPlot.xright);
	lua_rawset(pL, -3);
	lua_pushstring(pL, "ybot");
	lua_pushinteger(pL, GPO.V.BbPlot.ybot);
	lua_rawset(pL, -3);
	lua_pushstring(pL, "ytop");
	lua_pushinteger(pL, GPO.V.BbPlot.ytop);
	lua_rawset(pL, -3);
	return 1;
}
//
// gp.term_options(char *str) 
//
static int LUA_GP_term_options(lua_State * pL) 
{
	int n = lua_gettop(pL); // Number of arguments 
	const char * opt_str;
	if(n != 1)
		return luaL_error(pL, "Got %d arguments expected 1", n);
	opt_str = luaL_checkstring(pL, 1);
	n = strlen(opt_str);
	if(n > MAX_LINE_LEN)
		return luaL_error(pL, "Option string consists of %d characters but only %d are allowed", n, MAX_LINE_LEN);
	GPT._TermOptions.CatN(opt_str, MAX_LINE_LEN);
	return 0;
}

// close Lua context and clean up 
static void LUA_close() 
{
	if(P_LuaS) {
		lua_close(P_LuaS);
		P_LuaS = NULL;
	}
	ZFREE(LUA_script);
}
//
// gp.write(char *str) 
//
static int LUA_GP_write(lua_State * pL) 
{
	int n = lua_gettop(pL); // Number of arguments 
	const char * out_str;
	if(n != 1)
		return luaL_error(pL, "Got %d arguments expected 1", n);
	out_str = luaL_checkstring(pL, 1);
	fputs(out_str, GPT.P_GpOutFile);
	return 0;
}
/*
   gp.GPO.IntError(int t_num, char *msg)
   gp.GPO.IntError(char *msg)
 */
static int LUA_GP_int_error(lua_State * pL) 
{
	int t_num = NO_CARET;
	const char * msg = "";
	int n = lua_gettop(pL); // Number of arguments 
	switch(n) {
		case 1:
		    msg = luaL_checkstring(pL, 1);
		    break;
		case 2:
		    t_num = static_cast<int>(luaL_checkinteger(pL, 1));
		    msg  = luaL_checkstring(pL, 2);
		    break;
		default:
		    return luaL_error(pL, "Got %d arguments expected 1 or 2", n);
		    break;
	}
	snprintf(last_error_msg, MAX_LINE_LEN, "%s Lua context closed.", msg);
	// close Lua context on fatal errors 
	LUA_close();
	GPO.IntError(t_num, last_error_msg);
	return 0;
}
/*
   gp.GPO.IntWarn(int t_num, char *errmsg)
   gp.GPO.IntWarn(char *errmsg)
*/
static int LUA_GP_int_warn(lua_State * pL) 
{
	int t_num = NO_CARET;
	const char * msg = "";
	int n = lua_gettop(pL); // Number of arguments 
	switch(n) {
		case 1:
		    msg = luaL_checkstring(pL, 1);
		    break;
		case 2:
		    t_num = static_cast<int>(luaL_checkinteger(pL, 1));
		    msg = luaL_checkstring(pL, 2);
		    break;
		default:
		    return luaL_error(pL, "Got %d arguments expected 1 or 2", n);
		    break;
	}
	GPO.IntWarn(t_num, msg);
	return 0;
}
/*
   gp.term_out(char *terminal_msg)

   Print user messages, e.g. help messages
 */
static int LUA_GP_term_out(lua_State * pL) 
{
	char c; // dummy input char 
	char * line, * last;
	int pagelines = 0;
	const char * msg = "";
	int n = lua_gettop(pL); // Number of arguments 
	switch(n) {
		case 1:
		    msg = luaL_checkstring(pL, 1);
		    break;
		default:
		    return luaL_error(pL, "Got %d arguments expected 1", n);
		    break;
	}
	last = (char *)msg;
	while((line = strchr(last, '\n'))) {
		*line = '\0';
		if(pagelines >= 22) {
			fputs("Press return for more: ", stderr);
#if defined(ATARI) || defined(MTOS)
			do
				c = tos_getch();
			while(c != '\x04' && c != '\r' && c != '\n');
#elif defined(_WIN32)
			do {
				c = getchar();
			} while(c != EOF && c != '\n' && c != '\r');
#else
			do {
				c = getchar();
			} while(c != EOF && c != '\n');
#endif
			pagelines = 0;
		}
		fputs(last, stderr);
		fputs("\n", stderr);
		pagelines++;
		last = line+1;
	}
	if(*last)
		fputs(last, stderr);
	return 0;
}
//
// gp.is_multiplot()
//
static int LUA_GP_is_multiplot(lua_State * pL) 
{
	lua_pushboolean(pL, BIN(GPT.Flags & GpTerminalBlock::fMultiplot));
	return 1;
}
/*
   returns all internal and userdefined variables
   [name] = {type, val1 [, val2]},
   [name] = {type, val1 [, val2]},
   ...
 */
static int LUA_GP_get_all_variables(lua_State * pL) 
{
	udvt_entry * udv = GPO.Ev.P_FirstUdv;
	GpValue * val;
	lua_newtable(pL);
	while(udv) {
		// ignore mouse related variables 
		if(!strncmp(udv->udv_name, "MOUSE_", 6)) {
			udv = udv->next_udv;
			continue;
		}
		lua_newtable(pL);
		if(udv->udv_value.Type == NOTDEFINED) {
			lua_pushnil(pL);
			lua_rawseti(pL, -2, 2);
		}
		else {
			val = &(udv->udv_value);
			switch(val->Type) {
				case INTGR:
				    lua_pushstring(pL, "int");
				    lua_rawseti(pL, -2, 2);
				    lua_pushinteger(pL, val->v.int_val);
				    lua_rawseti(pL, -2, 3);
				    break;
				case CMPLX:
				    if(val->v.cmplx_val.imag  != 0.0) {
					    lua_pushstring(pL, "cmplx");
					    lua_rawseti(pL, -2, 2);
					    lua_pushnumber(pL, val->v.cmplx_val.imag);
					    lua_rawseti(pL, -2, 4);
				    }
				    else {
					    lua_pushstring(pL, "real");
					    lua_rawseti(pL, -2, 2);
				    }
#ifdef HAVE_ISNAN
				    if(isnan(val->v.cmplx_val.real)) {
					    lua_pushnil(pL);
					    lua_rawseti(pL, -2, 3);
				    }
				    else
#endif
				    {
					    lua_pushnumber(pL, val->v.cmplx_val.real);
					    lua_rawseti(pL, -2, 3);
				    }
				    break;
#ifdef GP_STRING_VARS
				case STRING:
				    lua_pushstring(pL, "string");
				    lua_rawseti(pL, -2, 2);
				    if(val->v.string_val) {
					    lua_pushstring(pL, val->v.string_val);
					    lua_rawseti(pL, -2, 3);
				    }
				    break;
#endif
				default:
				    lua_pushstring(pL, "unknown");
				    lua_rawseti(pL, -2, 2);
			}
		}
		lua_setfield(pL, -2, udv->udv_name);
		udv = udv->next_udv;
	}
	return 1;
}
/*
 * based on the parse_color_name() function in misc.c
 * returns rgb triplet based on color name or hex values
 */
static int LUA_GP_parse_color_name(lua_State * pL)
{
	int color = -1, token_cnt;
	int n = lua_gettop(pL); /* Number of arguments */
	const char * opt_str;
	if(n != 2)
		return luaL_error(pL, "Got %d arguments expected 2", n);
	token_cnt = static_cast<int>(luaL_checkinteger(pL, 1));
	opt_str = luaL_checkstring(pL, 2);
	color = lookup_table_nth(pm3d_color_names_tbl, opt_str);
	if(color >= 0)
		color = pm3d_color_names_tbl[color].value;
	else
		sscanf(opt_str, "#%x", &color);
	if((color & 0xff000000) != 0)
		GPO.IntError(token_cnt, "not recognized as a color name or a string of form \"#RRGGBB\"");
	lua_createtable(pL, 3, 0);
	n = lua_gettop(pL);
	lua_pushnumber(pL, (double)((color >> 16 ) & 255) / 255.0); lua_rawseti(pL, n, 1);
	lua_pushnumber(pL, (double)((color >> 8 ) & 255) / 255.0); lua_rawseti(pL, n, 2);
	lua_pushnumber(pL, (double)(color & 255) / 255.0); lua_rawseti(pL, n, 3);
	return 1;
}

#if LUA_VERSION_NUM > 500
static const luaL_Reg gp_methods[] = {
#else
static const luaL_reg gp_methods[] = {
#endif
	{"write", LUA_GP_write},
	{"int_error", LUA_GP_int_error},
	{"int_warn", LUA_GP_int_warn},
	{"term_out", LUA_GP_term_out},
	{"get_boundingbox", LUA_GP_get_boundingbox},
	{"is_multiplot", LUA_GP_is_multiplot},
	{"get_all_variables", LUA_GP_get_all_variables},
	{"term_options", LUA_GP_term_options},
	{"parse_color_name", LUA_GP_parse_color_name},
	{NULL, NULL}
};

static void LUA_register_gp_fnc(void)
{
#if LUA_VERSION_NUM > 501
	LUA_register(P_LuaS, LUA_GP_FNC, gp_methods);
#else
	luaL_register(P_LuaS, LUA_GP_FNC, gp_methods);
#endif
}
//
// read variables from script
//
static void LUA_get_term_vars(GpTermEntry * pTerm)
{
	lua_getfield(P_LuaS, luaterm, "description");
	pTerm->description = (lua_isstring(P_LuaS, -1)) ? lua_tostring(P_LuaS, -1) : LUA_TERM_DESCRIPTION;
	lua_pop(P_LuaS, 1);
	lua_getfield(P_LuaS, luaterm, "xmax");
	pTerm->MaxX = static_cast<uint>((lua_isnumber(P_LuaS, -1)) ? (uint)lua_tonumber(P_LuaS, -1) : LUA_XMAX);
	lua_pop(P_LuaS, 1);
	lua_getfield(P_LuaS, luaterm, "ymax");
	pTerm->MaxY = static_cast<uint>((lua_isnumber(P_LuaS, -1)) ? (uint)lua_tonumber(P_LuaS, -1) : LUA_YMAX);
	lua_pop(P_LuaS, 1);
	lua_getfield(P_LuaS, luaterm, "ChrV");
	pTerm->ChrV = (lua_isnumber(P_LuaS, -1)) ? (uint)lua_tonumber(P_LuaS, -1) : LUA_VCHAR;
	lua_pop(P_LuaS, 1);
	lua_getfield(P_LuaS, luaterm, "ChrH");
	pTerm->ChrH = (lua_isnumber(P_LuaS, -1)) ? (uint)lua_tonumber(P_LuaS, -1) : LUA_HCHAR;
	lua_pop(P_LuaS, 1);
	lua_getfield(P_LuaS, luaterm, "TicV");
	pTerm->TicV = (lua_isnumber(P_LuaS, -1)) ? (uint)lua_tonumber(P_LuaS, -1) : LUA_VTIC;
	lua_pop(P_LuaS, 1);
	lua_getfield(P_LuaS, luaterm, "TicH");
	pTerm->TicH = (lua_isnumber(P_LuaS, -1)) ? (uint)lua_tonumber(P_LuaS, -1) : LUA_HTIC;
	lua_pop(P_LuaS, 1);
	lua_getfield(P_LuaS, luaterm, "flags");
	pTerm->flags = (lua_isnumber(P_LuaS, -1)) ? (uint)lua_tonumber(P_LuaS, -1) : TERM_BINARY;
	lua_pop(P_LuaS, 1);
	lua_getfield(P_LuaS, luaterm, "tscale");
	pTerm->tscale = (lua_isnumber(P_LuaS, -1)) ? (double)lua_tonumber(P_LuaS, -1) : LUA_TSCALE;
	lua_pop(P_LuaS, 1);
	lua_getfield(P_LuaS, luaterm, "external_images");
	image_extern = lua_toboolean(P_LuaS, -1);
	lua_pop(P_LuaS, 1);
}

static void LUA_set_term_vars(void) 
{
	/* set term.version */
	lua_pushstring(P_LuaS, gnuplot_version);
	lua_setfield(P_LuaS, luaterm, "gp_version");
	/* set term.patchlevel */
	lua_pushstring(P_LuaS, gnuplot_patchlevel);
	lua_setfield(P_LuaS, luaterm, "gp_patchlevel");
	/* set term.patchlevel */
	lua_pushstring(P_LuaS, LUA_TERM_REVISION);
	lua_setfield(P_LuaS, luaterm, "lua_term_revision");
	/* set term.lua_ident */
	lua_pushstring(P_LuaS, LUA_RELEASE);
	lua_setfield(P_LuaS, luaterm, "lua_ident");
	/* set flag if terminal supports external images */
#ifdef LUA_EXTERNAL_IMAGES
	image_extern = 0;
	lua_pushboolean(P_LuaS, image_extern);
	lua_setfield(P_LuaS, luaterm, "external_images");
#endif
	/* some static definitions from term_api.h */
	lua_pushinteger(P_LuaS, TERM_CAN_MULTIPLOT); /* tested if stdout not redirected */
	lua_setfield(P_LuaS, luaterm, "TERM_CAN_MULTIPLOT");
	lua_pushinteger(P_LuaS, TERM_CANNOT_MULTIPLOT); /* tested if stdout is redirected  */
	lua_setfield(P_LuaS, luaterm, "TERM_CANNOT_MULTIPLOT");
	lua_pushinteger(P_LuaS, TERM_BINARY); /* open output file with "b"       */
	lua_setfield(P_LuaS, luaterm, "TERM_BINARY");
	lua_pushinteger(P_LuaS, TERM_INIT_ON_REPLOT); /* call term->init() on replot     */
	lua_setfield(P_LuaS, luaterm, "TERM_INIT_ON_REPLOT");
	lua_pushinteger(P_LuaS, TERM_IS_POSTSCRIPT); /* post, next, pslatex, etc        */
	lua_setfield(P_LuaS, luaterm, "TERM_IS_POSTSCRIPT");
	lua_pushinteger(P_LuaS, TERM_IS_LATEX);
	lua_setfield(P_LuaS, luaterm, "TERM_IS_LATEX");
	lua_pushinteger(P_LuaS, TERM_ENHANCED_TEXT); /* enhanced text mode is enabled   */
	lua_setfield(P_LuaS, luaterm, "TERM_ENHANCED_TEXT");
	lua_pushinteger(P_LuaS, TERM_NO_OUTPUTFILE); /* terminal doesn't write to a file */
	lua_setfield(P_LuaS, luaterm, "TERM_NO_OUTPUTFILE");
	lua_pushinteger(P_LuaS, TERM_CAN_CLIP); /* terminal does its own clipping  */
	lua_setfield(P_LuaS, luaterm, "TERM_CAN_CLIP");
	lua_pushinteger(P_LuaS, TERM_CAN_DASH); /* terminal knows dashed lines */
	lua_setfield(P_LuaS, luaterm, "TERM_CAN_DASH");
	lua_pushinteger(P_LuaS, TERM_ALPHA_CHANNEL); /* alpha channel transparency      */
	lua_setfield(P_LuaS, luaterm, "TERM_ALPHA_CHANNEL");
	lua_pushinteger(P_LuaS, TERM_MONOCHROME); /* term is running in mono mode    */
	lua_setfield(P_LuaS, luaterm, "TERM_MONOCHROME");
	lua_pushinteger(P_LuaS, TERM_LINEWIDTH); /* support for set term linewidth  */
	lua_setfield(P_LuaS, luaterm, "TERM_LINEWIDTH");
#ifdef TERM_FONTSCALE
	lua_pushinteger(P_LuaS, TERM_FONTSCALE); // terminal supports fontscale
	lua_setfield(P_LuaS, luaterm, "TERM_FONTSCALE");
#endif
}

static int LUA_init_luaterm_function(GpTermEntry * pThis, const char * fnc) 
{
	GnuPlot * p_gp = pThis->P_Gp;
	if(!P_LuaS)
		p_gp->IntError(NO_CARET, "Missing Lua context! No script?");
	lua_getfield(P_LuaS, luaterm, fnc);
	if(lua_isfunction(P_LuaS, -1)) {
		return 1;
	}
	else {
		p_gp->IntWarn(NO_CARET, "Script lacks function `%s'!", fnc);
		lua_pop(P_LuaS, 1); /* clean stack */
	}
	return 0;
}

static int LUA_call_report(int status)
{
	if(status) {
		const char * msg = lua_tostring(P_LuaS, -1);
		SETIFZ(msg, "(error with no message)");
		snprintf(last_error_msg, MAX_LINE_LEN, "%s. Lua context closed.", msg);
		LUA_close();
		GPO.IntError(NO_CARET, last_error_msg);
	}
	return status;
}

#if defined(_WIN32)
	extern LPSTR RelativePathToGnuplot(const char * path);
#endif

static int LUA_init_lua(void)
{
	int sf; /* Lua script "function" */
	struct stat stat_buf;
	char * script_fqn;
	char * gp_lua_dir;
#if defined(_WIN32)
	char * free_lua_dir = NULL;
#endif
	// 
	// Close old Lua context and open a new one.
	// 
	if(P_LuaS)
		lua_close(P_LuaS);
#if LUA_VERSION_NUM > 500
	P_LuaS = luaL_newstate();
#else
	P_LuaS = lua_open();
#endif
	luaL_openlibs(P_LuaS); /* Load Lua libraries */
	luaopen_debug(P_LuaS);
	gp_lua_dir = getenv("GNUPLOT_LUA_DIR");
#if defined(_WIN32)
	if(!gp_lua_dir)
		gp_lua_dir = free_lua_dir = RelativePathToGnuplot(GNUPLOT_LUA_DIR);
#else
	SETIFZ(gp_lua_dir, GNUPLOT_LUA_DIR);
#endif
	if(stat(LUA_script, &stat_buf) || !S_ISREG(stat_buf.st_mode)) {
		script_fqn = (char *)SAlloc::M(strlen(gp_lua_dir) + strlen(LUA_script) + 2);
		sprintf(script_fqn, "%s%c%s", gp_lua_dir, DIRSEP1, LUA_script);
	}
	else {
		script_fqn = sstrdup(LUA_script);
	}
#if defined(_WIN32)
	SAlloc::F(free_lua_dir);
#endif
	/* Load the file containing the script we are going to run */
	lua_term_status = luaL_loadfile(P_LuaS, script_fqn);
	if(lua_term_status) {
		fprintf(stderr, "error: %s. Lua context closed.\n", lua_tostring(P_LuaS, -1));
		LUA_close();
		SAlloc::F(script_fqn);
		return 0;
	}
	SAlloc::F(script_fqn);
	/* remember script "function" */
	sf = lua_gettop(P_LuaS);
	/*  lua_settop(P_LuaS, 0);*/ /* clear stack */
#if LUA_VERSION_NUM > 501
	LUA_getfield_global(P_LuaS, "debug");
#else
	lua_getfield(P_LuaS, LUA_GLOBALSINDEX, "debug");
#endif
	lua_getfield(P_LuaS, -1, "traceback");
	lua_remove(P_LuaS, -2); /* rm debug */
	tb = lua_gettop(P_LuaS); /* store "traceback" */
	/* create table `term' */
	lua_newtable(P_LuaS);
	lua_setglobal(P_LuaS, "term");
#if LUA_VERSION_NUM > 501
	LUA_getfield_global(P_LuaS, "term");
#else
	lua_getfield(P_LuaS, LUA_GLOBALSINDEX, "term");
#endif
	luaterm = lua_gettop(P_LuaS); /* store `term' */
	LUA_register_gp_fnc(); /* register gp functions */
	LUA_set_term_vars(); /* set terminal variables */
	/* put script "function" on top and call */
	lua_pushvalue(P_LuaS, sf);
	LUA_call_report(lua_pcall(P_LuaS, 0, LUA_MULTRET, tb));
	return 1;
}

/* see color.h */
static const char* LUA_get_colorstyle(int style) 
{
	const char * style_str = "unknown";
	switch(style) {
		case TC_LT: /* Use the color of linetype <n> */
		    style_str = "LT";
		    break;
		case TC_LINESTYLE:/* Use the color of line style <n> (only for "internal" use or unused?) */
		    style_str = "LINESTYLE";
		    break;
		case TC_RGB: /* Explicit RGBA values provided by user */
		    style_str = "RGBA";
		    break;
		case TC_CB: /* "palette cb <value>" (only for "internal" use or unused?) */
		    style_str = "CB";
		    break;
		case TC_FRAC: /* "palette frac <value>" */
		    style_str = "FRAC";
		    break;
		case TC_Z: /* "palette z" (only for "internal" use or unused?) */
		    style_str = "Z";
		    break;
		case TC_DEFAULT: /* Use default color, set separately (only for "internal" use or unused?) */
		    style_str = "DEFAULT";
		    break;
	}
	return(style_str);
}

/* see term_api.h */
static const char* LUA_get_fillstyle(int style) 
{
	const char * style_str = "unknown";
	if(style == FS_OPAQUE) {
		/* FIXME: not quite sure how to handle this, since it is only used by the
		   postscript terminal */
		style_str = "OPAQUE";
	}
	else {
		switch(style & 0xf) {
			case FS_EMPTY: style_str = "EMPTY"; break;
			case FS_SOLID: style_str = "SOLID"; break;
			case FS_PATTERN: style_str = "PATTERN"; break;
			case FS_TRANSPARENT_SOLID: style_str = "TRANSPARENT_SOLID"; break;
			case FS_TRANSPARENT_PATTERN: style_str = "TRANSPARENT_PATTERN"; break;
			case FS_DEFAULT: style_str = "DEFAULT"; break;
		}
	}
	return(style_str);
}

/*
 * Handle options
 */
TERM_PUBLIC void LUA_options(GpTermEntry * pThis, GnuPlot * pGp)
{
	char * opt_str = NULL;
	char * s;
	int tc_off = pGp->Pgm.GetCurTokenIdx()+1; // token counter offset 
	int need_init = 1;
	/* 'set term tikz' is short for 'set term lua tikz' */
	pGp->Pgm.Rollback(); /* see how we got here */
	if(!pGp->Pgm.EqualsCur("tikz")) {
		if(pGp->Pgm.AlmostEqualsCur("termop$tions")) {
			if(!LUA_script) {
				pGp->IntError(NO_CARET, "No Lua context for setting terminal options!");
				return;
			}
			need_init = 0;
		}
		/*  else should always be lua, so just count up ... */
		pGp->Pgm.Shift();
	}
	opt_str = pGp->Pgm.P_InputLine + pGp->Pgm.GetCurTokenStartIndex();
	if(need_init) {
		if(!pGp->Pgm.EndOfCommand()) {
			if(*opt_str == '"' || *opt_str == '\'') {
				s = pGp->TryToGetString();
				pGp->GpExpandTilde(&s);
			}
			else {
				s = (char *)SAlloc::M(pGp->Pgm.CurTokenLen()+strlen("gnuplot-.lua")+1);
				memcpy(s, "gnuplot-", 8);
				memcpy(s+8, opt_str, pGp->Pgm.CurTokenLen());
				memcpy(s+8+pGp->Pgm.CurTokenLen(), ".lua\0", 5);
				pGp->Pgm.Shift();
			}
			if(LUA_script) {
				if(strcmp(LUA_script, s)) {
					SAlloc::F(LUA_script);
					LUA_script = s;
					need_init = 1;
				}
				else {
					SAlloc::F(s);
					need_init = 0;
				}
			}
			else {
				LUA_script = s;
			}
			opt_str = pGp->Pgm.P_InputLine + pGp->Pgm.GetCurTokenStartIndex();
		}
		else {
			LUA_close();
			pGp->IntError(NO_CARET, "No Lua driver name or file name given!");
		}
		/* init lua when opening the terminal or on script change */
		if(!LUA_init_lua()) {
			return;
		}
	}
	// since options are tokenized again in the script we always "finish" this here 
	while(!pGp->Pgm.EndOfCommand())
		pGp->Pgm.Shift();
	if(LUA_init_luaterm_function(pThis, "options")) {
		// isolate the "set term ...;" part of the command line 
		opt_str = sstrdup(opt_str);
		opt_str[ strcspn(opt_str, ";") ] = '\0';
		lua_pushstring(P_LuaS, opt_str);
		lua_pushinteger(P_LuaS, need_init);
		lua_pushinteger(P_LuaS, tc_off);
		LUA_call_report(lua_pcall(P_LuaS, 3, 1, tb));
		lua_term_result = (int)lua_tonumber(P_LuaS, -1);
		lua_pop(P_LuaS, 1);
		SAlloc::F(opt_str);
	}
	LUA_get_term_vars(pThis);
	// Treat "set term tikz mono" as "set term tikz; set mono" 
	if(GPT._TermOptions.Search("monochrome", 0, 0, 0)) {
		GPT.Flags |= GpTerminalBlock::fMonochrome;
		pGp->InitMonochrome();
	}
}

TERM_PUBLIC void LUA_init(GpTermEntry * pThis)
{
	if(GPT.P_GpOutFile != stdout) {
		fseek(GPT.P_GpOutFile, 0, SEEK_SET);
		// ignore compiler warnings here, because `gpoutfile' is already open 
		if(fflush(GPT.P_GpOutFile) || ftruncate(_fileno(GPT.P_GpOutFile), 0))
			pThis->P_Gp->IntWarn(NO_CARET, "Error re-writing output file: %s", strerror(errno));
	}
	image_cnt = 0; // reset image counter 
	LUA_linetype(pThis, -1);
	if(LUA_init_luaterm_function(pThis, "init")) {
		LUA_call_report(lua_pcall(P_LuaS, 0, 1, tb));
		lua_term_result = (int)lua_tonumber(P_LuaS, -1);
		lua_pop(P_LuaS, 1);
	}
}

TERM_PUBLIC void LUA_graphics(GpTermEntry * pThis)
{
	if(LUA_init_luaterm_function(pThis, "graphics")) {
		LUA_call_report(lua_pcall(P_LuaS, 0, 1, tb));
		lua_term_result = (int)lua_tonumber(P_LuaS, -1);
		lua_pop(P_LuaS, 1);
	}
}

TERM_PUBLIC void LUA_text(GpTermEntry * pThis)
{
	if(LUA_init_luaterm_function(pThis, "text")) {
		LUA_call_report(lua_pcall(P_LuaS, 0, 1, tb));
		lua_term_result = (int)lua_tonumber(P_LuaS, -1);
		lua_pop(P_LuaS, 1);
	}
}

TERM_PUBLIC void LUA_linetype(GpTermEntry * pThis, int linetype)
{
	if(LUA_init_luaterm_function(pThis, "linetype")) {
		lua_pushinteger(P_LuaS, linetype);
		LUA_call_report(lua_pcall(P_LuaS, 1, 1, tb));
		lua_term_result = (int)lua_tonumber(P_LuaS, -1);
		lua_pop(P_LuaS, 1);
	}
}

TERM_PUBLIC void LUA_dashtype(GpTermEntry * pThis, int type, t_dashtype * custom_dash_type)
{
	int i = 0;
	if(LUA_init_luaterm_function(pThis, "dashtype")) {
		lua_pushinteger(P_LuaS, type);
		lua_newtable(P_LuaS);
		if(type == DASHTYPE_CUSTOM) {
			while(custom_dash_type->pattern[i] > 0) {
				lua_pushnumber(P_LuaS, custom_dash_type->pattern[i]);
				i++;
				lua_rawseti(P_LuaS, -2, i);
			}
		}
		else {
			lua_pushinteger(P_LuaS, 0);
			lua_rawseti(P_LuaS, -2, 1);
		}
		LUA_call_report(lua_pcall(P_LuaS, 2, 1, tb));
		lua_term_result = (int)lua_tonumber(P_LuaS, -1);
		lua_pop(P_LuaS, 1);
	}
}

TERM_PUBLIC void LUA_move(GpTermEntry * pThis, uint x, uint y)
{
	if(LUA_init_luaterm_function(pThis, "move")) {
		lua_pushinteger(P_LuaS, (int)x);
		lua_pushinteger(P_LuaS, (int)y);
		LUA_call_report(lua_pcall(P_LuaS, 2, 1, tb));
		lua_term_result = (int)lua_tonumber(P_LuaS, -1);
		lua_pop(P_LuaS, 1);
	}
}

TERM_PUBLIC void LUA_point(GpTermEntry * pThis, uint x, uint y, int number)
{
	lua_term_result = 0;
	if(LUA_init_luaterm_function(pThis, "point")) {
		lua_pushinteger(P_LuaS, (int)x);
		lua_pushinteger(P_LuaS, (int)y);
		lua_pushinteger(P_LuaS, number);
		LUA_call_report(lua_pcall(P_LuaS, 3, 1, tb));
		lua_term_result = (int)lua_tonumber(P_LuaS, -1);
		lua_pop(P_LuaS, 1);
	}
	if(!lua_term_result) 
		GnuPlot::DoPoint(pThis, x, y, number);
}

TERM_PUBLIC void LUA_pointsize(GpTermEntry * pThis, double ptsize)
{
	if(LUA_init_luaterm_function(pThis, "pointsize")) {
		lua_pushnumber(P_LuaS, ptsize);
		LUA_call_report(lua_pcall(P_LuaS, 1, 1, tb));
		lua_term_result = (int)lua_tonumber(P_LuaS, -1);
		lua_pop(P_LuaS, 1);
	}
}

TERM_PUBLIC void LUA_vector(GpTermEntry * pThis, uint ux, uint uy)
{
	if(LUA_init_luaterm_function(pThis, "vector")) {
		lua_pushinteger(P_LuaS, (int)ux);
		lua_pushinteger(P_LuaS, (int)uy);
		LUA_call_report(lua_pcall(P_LuaS, 2, 1, tb));
		lua_term_result = (int)lua_tonumber(P_LuaS, -1);
		lua_pop(P_LuaS, 1);
	}
}

TERM_PUBLIC void LUA_arrow(GpTermEntry * pThis, uint sx, uint sy, uint ex, uint ey, int head)
{
	// if the script does not provide an `arrow' functions
	// or if it returns `0' we fall back to `do_arrow'
	lua_term_result = 0;
	if(LUA_init_luaterm_function(pThis, "arrow")) {
		lua_pushinteger(P_LuaS, (int)sx);
		lua_pushinteger(P_LuaS, (int)sy);
		lua_pushinteger(P_LuaS, (int)ex);
		lua_pushinteger(P_LuaS, (int)ey);
		// hidden3d uses a non-standard method of indicating NOHEAD 
		if(GPT.CArw.HeadAngle == 0 && GPT.CArw.HeadLength > 0)
			head = NOHEAD;
		lua_pushinteger(P_LuaS, head);
		// additional vars  
		lua_pushinteger(P_LuaS, GPT.CArw.HeadLength); // access head length + angle (int)
		lua_pushnumber(P_LuaS, GPT.CArw.HeadAngle); // angle in degrees (double)
		lua_pushnumber(P_LuaS, GPT.CArw.HeadBackAngle); // angle in degrees (double)
		lua_pushinteger(P_LuaS, GPT.CArw.HeadFilled); // arrow head filled or not
		LUA_call_report(lua_pcall(P_LuaS, 9, 1, tb));
		lua_term_result = (int)lua_tonumber(P_LuaS, -1);
		lua_pop(P_LuaS, 1);
	}
	if(!lua_term_result) 
		GnuPlot::DoArrow(pThis, sx, sy, ex, ey, head);
}

TERM_PUBLIC void LUA_put_text(GpTermEntry * pThis, uint x, uint y, const char str[])
{
	if(LUA_init_luaterm_function(pThis, "put_text")) {
		lua_pushinteger(P_LuaS, (int)x);
		lua_pushinteger(P_LuaS, (int)y);
		lua_pushstring(P_LuaS, str);
		LUA_call_report(lua_pcall(P_LuaS, 3, 1, tb));
		lua_term_result = (int)lua_tonumber(P_LuaS, -1);
		lua_pop(P_LuaS, 1);
	}
}

TERM_PUBLIC int LUA_justify_text(GpTermEntry * pThis, enum JUSTIFY mode)
{
	if(LUA_init_luaterm_function(pThis, "justify_text")) {
		const char * m;
		switch(mode) {
			case LEFT: m = "left"; break;
			default:
			case CENTRE: m = "center"; break;
			case RIGHT: m = "right"; break;
		}
		lua_pushstring(P_LuaS, m);
		LUA_call_report(lua_pcall(P_LuaS, 1, 1, tb));
		lua_term_result = (int)lua_tonumber(P_LuaS, -1);
		lua_pop(P_LuaS, 1);
		return (lua_term_result ? TRUE : FALSE);
	}
	return(FALSE);
}

TERM_PUBLIC int LUA_text_angle(GpTermEntry * pThis, int ang)
{
	if(LUA_init_luaterm_function(pThis, "text_angle")) {
		lua_pushinteger(P_LuaS, ang);
		LUA_call_report(lua_pcall(P_LuaS, 1, 1, tb));
		lua_term_result = (int)lua_tonumber(P_LuaS, -1);
		lua_pop(P_LuaS, 1);
		return(((lua_term_result || !ang ) ? TRUE : FALSE));
	}
	return((ang ? FALSE : TRUE)); /* return TRUE if called with ang==0 */
}

TERM_PUBLIC int LUA_set_font(GpTermEntry * pThis, const char * font)
{
	if(LUA_init_luaterm_function(pThis, "set_font")) {
		lua_pushstring(P_LuaS, font);
		LUA_call_report(lua_pcall(P_LuaS, 1, 1, tb));
		lua_term_result = (int)lua_tonumber(P_LuaS, -1);
		lua_pop(P_LuaS, 1);
		if(lua_term_result) {
			lua_getfield(P_LuaS, luaterm, "ChrV");
			pThis->ChrV = (lua_isnumber(P_LuaS, -1)) ? (uint)lua_tonumber(P_LuaS, -1) : LUA_VCHAR;
			lua_pop(P_LuaS, 1);
			lua_getfield(P_LuaS, luaterm, "ChrH");
			pThis->ChrH = (lua_isnumber(P_LuaS, -1)) ? (uint)lua_tonumber(P_LuaS, -1) : LUA_HCHAR;
			lua_pop(P_LuaS, 1);
			return TRUE;
		}
	}
	return(FALSE);
}

TERM_PUBLIC void LUA_boxfill(GpTermEntry * pThis, int style, uint x1, uint y1, uint width, uint height)
{
	if(LUA_init_luaterm_function(pThis, "boxfill")) {
		lua_pushstring(P_LuaS, LUA_get_fillstyle(style));
		lua_pushinteger(P_LuaS, style >> 4);
		lua_pushinteger(P_LuaS, (int)x1);
		lua_pushinteger(P_LuaS, (int)y1);
		lua_pushinteger(P_LuaS, (int)width);
		lua_pushinteger(P_LuaS, (int)height);
		LUA_call_report(lua_pcall(P_LuaS, 6, 1, tb));
		lua_term_result = (int)lua_tonumber(P_LuaS, -1);
		lua_pop(P_LuaS, 1);
	}
}

TERM_PUBLIC void LUA_linewidth(GpTermEntry * pThis, double width)
{
	if(LUA_init_luaterm_function(pThis, "linewidth")) {
		lua_pushnumber(P_LuaS, width);
		LUA_call_report(lua_pcall(P_LuaS, 1, 1, tb));
		lua_term_result = (int)lua_tonumber(P_LuaS, -1);
		lua_pop(P_LuaS, 1);
	}
}

TERM_PUBLIC void LUA_previous_palette(GpTermEntry * pThis)
{
	if(LUA_init_luaterm_function(pThis, "previous_palette")) {
		LUA_call_report(lua_pcall(P_LuaS, 0, 1, tb));
		lua_term_result = (int)lua_tonumber(P_LuaS, -1);
		lua_pop(P_LuaS, 1);
	}
}

TERM_PUBLIC void LUA_reset(GpTermEntry * pThis)
{
	if(LUA_init_luaterm_function(pThis, "reset")) {
		LUA_call_report(lua_pcall(P_LuaS, 0, 1, tb));
		lua_term_result = (int)lua_tonumber(P_LuaS, -1);
		lua_pop(P_LuaS, 1);
	}
}

TERM_PUBLIC int LUA_make_palette(GpTermEntry * pThis, t_sm_palette * palette)
{
	if(LUA_init_luaterm_function(pThis, "make_palette")) {
		LUA_call_report(lua_pcall(P_LuaS, 0, 1, tb));
		lua_term_result = (int)lua_tonumber(P_LuaS, -1);
		lua_pop(P_LuaS, 1);
		return lua_term_result;
	}
	return 0; // continuous number of colours 
}

TERM_PUBLIC void LUA_set_color(GpTermEntry * pThis, const t_colorspec * colorspec)
{
	GnuPlot * p_gp = pThis->P_Gp;
	double gray = colorspec->value;
	rgb_color color = {0., 0., 0.};
	double opacity = 1.0;
	if(LUA_init_luaterm_function(pThis, "set_color")) {
		if(colorspec->type == TC_FRAC) {
			if(p_gp->SmPltt.Colors != 0) /* finite nb of colors explicitly requested */
				gray = (gray >= ((double)(p_gp->SmPltt.Colors-1)) / p_gp->SmPltt.Colors) ? 1 : floor(gray * p_gp->SmPltt.Colors) / p_gp->SmPltt.Colors;
			p_gp->Rgb1FromGray(gray, &color);
		}
		else if(colorspec->type == TC_RGB) {
			opacity = 1.0 - (double)(colorspec->lt >> 24 & 255) / 255.0;
			color.r = (double)((colorspec->lt >> 16 ) & 255) / 255.0;
			color.g = (double)((colorspec->lt >> 8 ) & 255) / 255.0;
			color.b = (double)(colorspec->lt & 255) / 255.0;
		}
		if(color.r < 1e-4) color.r = 0;
		if(color.g < 1e-4) color.g = 0;
		if(color.b < 1e-4) color.b = 0;
		lua_pushstring(P_LuaS, LUA_get_colorstyle(colorspec->type));
		lua_pushinteger(P_LuaS, colorspec->lt);
		lua_pushnumber(P_LuaS, colorspec->value);
		lua_pushnumber(P_LuaS, opacity);
		lua_pushnumber(P_LuaS, color.r);
		lua_pushnumber(P_LuaS, color.g);
		lua_pushnumber(P_LuaS, color.b);
		LUA_call_report(lua_pcall(P_LuaS, 7, 1, tb));
		lua_term_result = (int)lua_tonumber(P_LuaS, -1);
		lua_pop(P_LuaS, 1);
	}
}

TERM_PUBLIC void LUA_filled_polygon(GpTermEntry * pThis, int points, gpiPoint * corners)
{
	if(LUA_init_luaterm_function(pThis, "filled_polygon")) {
		int i;
		lua_pushstring(P_LuaS, LUA_get_fillstyle(corners->style));
		lua_pushinteger(P_LuaS, corners->style >> 4);
		// put all coords into a simple table 
		lua_newtable(P_LuaS);
		for(i = 0; i < points; i++) {
			lua_newtable(P_LuaS);
			lua_pushinteger(P_LuaS, corners[i].x);
			lua_rawseti(P_LuaS, -2, 1);
			lua_pushinteger(P_LuaS, corners[i].y);
			lua_rawseti(P_LuaS, -2, 2);
			lua_rawseti(P_LuaS, -2, i+1); /* add "subtable" */
		}
		LUA_call_report(lua_pcall(P_LuaS, 3, 1, tb));
		lua_term_result = (int)lua_tonumber(P_LuaS, -1);
		lua_pop(P_LuaS, 1);
	}
}

TERM_PUBLIC void LUA_layer(GpTermEntry * pThis, t_termlayer syncpoint)
{
	if(LUA_init_luaterm_function(pThis, "layer")) {
		const char * m;
		switch(syncpoint) {
			case TERM_LAYER_RESET: m = "reset"; break; // Start of plot; reset flag 
			case TERM_LAYER_BACKTEXT: m = "backtext"; break; // Start of "back" text layer 
			case TERM_LAYER_FRONTTEXT: m = "fronttext"; break; // Start of "front" text layer 
			case TERM_LAYER_END_TEXT: m = "end_text"; break; // Close off front or back macro before leaving 
			case TERM_LAYER_BEFORE_PLOT: m = "before_plot"; break; // Close off front or back macro before leaving 
			case TERM_LAYER_AFTER_PLOT: m = "after_plot"; break; // Close off front or back macro before leaving 
			case TERM_LAYER_BEGIN_GRID: m = "begin_grid"; break;
			case TERM_LAYER_END_GRID: m = "end_grid"; break;
			default: m = ""; break;
		}
		lua_pushstring(P_LuaS, m);
		LUA_call_report(lua_pcall(P_LuaS, 1, 1, tb));
		lua_term_result = (int)lua_tonumber(P_LuaS, -1);
		lua_pop(P_LuaS, 1);
	}
}

TERM_PUBLIC void LUA_path(GpTermEntry * pThis, int path)
{
	if(LUA_init_luaterm_function(pThis, "path")) {
		lua_pushinteger(P_LuaS, path);
		LUA_call_report(lua_pcall(P_LuaS, 1, 1, tb));
		lua_term_result = (int)lua_tonumber(P_LuaS, -1);
		lua_pop(P_LuaS, 1);
	}
}
// 
// Lua table structure for the image pixel:
// pixel = {{r, g, b, [, a]}, {r, g, b [, a]}, ... , {r, g, b [, a]}}
// 
TERM_PUBLIC void LUA_image(GpTermEntry * pThis, uint m, uint n, coordval * image, const gpiPoint * corner, t_imagecolor color_mode) 
{
	if(LUA_init_luaterm_function(pThis, "image")) {
		int i;
		rgb_color rgb1;
		coordval alpha = 0;
		char * image_file = NULL;
#ifdef LUA_EXTERNAL_IMAGES
		// "externalize" if transparent images are used or on user request 
		if(GPT.P_OutStr && ((color_mode == IC_RGBA) || image_extern)) {
			char * idx;
			// cairo based png images with alpha channel 
			if((idx = strrchr(GPT.P_OutStr, '.')) == NULL)
				idx = strchr(GPT.P_OutStr, '\0');
			image_file = (char *)SAlloc::M((idx-GPT.P_OutStr)+10);
			strncpy(image_file, GPT.P_OutStr, (idx-GPT.P_OutStr) + 1);
			snprintf(image_file+(idx-GPT.P_OutStr), 9, ".%03d.png", (uchar)(++image_cnt));
			write_png_image(pThis, m, n, image, color_mode, image_file);
		}
#endif
		lua_pushinteger(P_LuaS, m);
		lua_pushinteger(P_LuaS, n);
		lua_newtable(P_LuaS); /* pixel table */
		for(i = 0; i < static_cast<int>(m*n); i++) {
			if(color_mode == IC_PALETTE) {
				// FIXME: Is this correct? Needs a testcase. Would be nice to map it correctly to RGB.
				pThis->P_Gp->Rgb1MaxColorsFromGray(*image++, &rgb1);
			}
			else { /* IC_RGB and IC_RGBA*/
				rgb1.r = *image++;
				rgb1.g = *image++;
				rgb1.b = *image++;
				if(color_mode == IC_RGBA) 
					alpha = (*image++)/255.0;
			}
			lua_newtable(P_LuaS); /* pixel color */
			lua_pushnumber(P_LuaS, rgb1.r);
			lua_rawseti(P_LuaS, -2, 1);
			lua_pushnumber(P_LuaS, rgb1.g);
			lua_rawseti(P_LuaS, -2, 2);
			lua_pushnumber(P_LuaS, rgb1.b);
			lua_rawseti(P_LuaS, -2, 3);
			if(color_mode == IC_RGBA) {
				lua_pushnumber(P_LuaS, alpha);
				lua_rawseti(P_LuaS, -2, 4);
			}
			lua_rawseti(P_LuaS, -2, i+1); /* add "pixel" */
		}
		lua_newtable(P_LuaS); // "corner" table 
		for(i = 0; i < 4; i++) {
			lua_newtable(P_LuaS);
			lua_pushinteger(P_LuaS, (int)corner[i].x);
			lua_rawseti(P_LuaS, -2, 1);
			lua_pushinteger(P_LuaS, (int)corner[i].y);
			lua_rawseti(P_LuaS, -2, 2);
			lua_rawseti(P_LuaS, -2, i+1); /* add "subtable" */
		}
		switch(color_mode) {
			case IC_PALETTE:
			case IC_RGB: lua_pushstring(P_LuaS, "RGB"); break;
			case IC_RGBA: lua_pushstring(P_LuaS, "RGBA"); break;
		}
		if(image_file) {
			lua_pushstring(P_LuaS, image_file);
			SAlloc::F(image_file);
		}
		else {
			lua_pushnil(P_LuaS);
		}
		LUA_call_report(lua_pcall(P_LuaS, 6, 1, tb));
		lua_term_result = (int)lua_tonumber(P_LuaS, -1);
		lua_pop(P_LuaS, 1);
	}
}

TERM_PUBLIC void LUA_boxed_text(GpTermEntry * pThis, uint x, uint y, int option)
{
	const char * option_str = "UNKNOWN";
	switch(option) {
		case TEXTBOX_INIT: option_str = "INIT"; break;
		case TEXTBOX_OUTLINE: option_str = "OUTLINE"; break;
		case TEXTBOX_BACKGROUNDFILL: option_str = "BACKGROUNDFILL"; break;
		case TEXTBOX_MARGINS: option_str = "MARGINS"; break;
		default: break;
	}
	if(LUA_init_luaterm_function(pThis, "boxed_text")) {
		lua_pushinteger(P_LuaS, x);
		lua_pushinteger(P_LuaS, y);
		lua_pushstring(P_LuaS, option_str);
		LUA_call_report(lua_pcall(P_LuaS, 3, 1, tb));
		lua_term_result = (int)lua_tonumber(P_LuaS, -1);
		lua_pop(P_LuaS, 1);
	}
}

#endif /* TERM_BODY */

#ifdef TERM_TABLE

TERM_TABLE_START(lua_driver)
	"lua", 
	LUA_TERM_DESCRIPTION,
	static_cast<uint>(LUA_XMAX),
	static_cast<uint>(LUA_YMAX),
	LUA_VCHAR, 
	LUA_HCHAR,
	LUA_VTIC, 
	LUA_HTIC, 
	LUA_options, 
	LUA_init, 
	LUA_reset,
	LUA_text, 
	GnuPlot::NullScale, 
	LUA_graphics, 
	LUA_move, 
	LUA_vector,
	LUA_linetype, 
	LUA_put_text, 
	LUA_text_angle,
	LUA_justify_text, 
	LUA_point, 
	LUA_arrow, 
	LUA_set_font, 
	LUA_pointsize,
	TERM_BINARY /*flags*/, 
	0 /*suspend*/, 
	0 /*resume*/,
	LUA_boxfill, 
	LUA_linewidth,
	#ifdef USE_MOUSE
		0, 
		0, 
		0, 
		0, 
		0,
	#endif
	LUA_make_palette, 
	LUA_previous_palette,  
	LUA_set_color,
	LUA_filled_polygon,
	LUA_image,
	0, 
	0, 
	0,
	LUA_layer,
	LUA_path,
	LUA_TSCALE,
	NULL,     /* hypertext */
	LUA_boxed_text,
	NULL,     /* modify plots */
	LUA_dashtype 
TERM_TABLE_END(lua_driver)

#undef LAST_TERM
#define LAST_TERM lua_driver

TERM_TABLE_START(tikz_driver)
	"tikz", 
	"TeX TikZ graphics macros via the lua script driver",
	static_cast<uint>(LUA_XMAX),
	static_cast<uint>(LUA_YMAX),
	LUA_VCHAR, 
	LUA_HCHAR,
	LUA_VTIC, 
	LUA_HTIC, 
	LUA_options, 
	LUA_init, 
	LUA_reset,
	LUA_text, 
	GnuPlot::NullScale, 
	LUA_graphics, 
	LUA_move, 
	LUA_vector,
	LUA_linetype, 
	LUA_put_text, 
	LUA_text_angle,
	LUA_justify_text, 
	LUA_point, 
	LUA_arrow, 
	LUA_set_font, 
	LUA_pointsize,
	TERM_BINARY /*flags*/, 
	0 /*suspend*/, 
	0 /*resume*/,
	LUA_boxfill, 
	LUA_linewidth,
	#ifdef USE_MOUSE
		0, 
		0, 
		0, 
		0, 
		0,
	#endif
	LUA_make_palette, 
	LUA_previous_palette,  
	LUA_set_color,
	LUA_filled_polygon,
	LUA_image,
	0, 
	0, 
	0,
	LUA_layer,
	LUA_path,
	LUA_TSCALE,
	NULL,     /* hypertext */
	LUA_boxed_text,
	NULL, /* modify plots */
	LUA_dashtype 
TERM_TABLE_END(tikz_driver)

#undef LAST_TERM
#define LAST_TERM tikz_driver

#endif /* TERM_TABLE */
#endif /* TERM_PROTO_ONLY */

#ifdef TERM_HELP
START_HELP(lua)
"1 lua",
"?commands set terminal lua",
"?set terminal lua",
"?set term lua",
"?terminal lua",
"?term lua",
"?lua",
" The `lua` generic terminal driver works in conjunction with an",
" external Lua script to create a target-specific plot file.",
" Currently the only supported target is TikZ -> pdflatex.",
"",
" Information about Lua is available at http://www.lua.org .",
"",
" Syntax:",
"    set terminal lua <target name> | \"<file name>\"",
"                        {<script_args> ...}",
"                        {help}",
"",
" A 'target name' or 'file name' (in quotes) for a script is mandatory.",
" If a 'target name' for the script is given, the terminal will look for",
" \"gnuplot-<target name>.lua\" in the local directory and on failure in",
" the environmental variable GNUPLOT_LUA_DIR.",
"",
" All arguments will be provided to the selected script for further",
" evaluation. E.g. 'set term lua tikz help' will cause the script itself",
" to print additional help on options and choices for the script.",
#ifdef HAVE_LUA
#include "gnuplot-tikz.help"
#endif
""
END_HELP(lua)
START_HELP(tikz)
"1 tikz",
"?commands set terminal tikz",
"?set terminal tikz",
"?set term tikz",
"?terminal tikz",
"?term tikz",
"?tikz",
" This driver creates output for use with the TikZ package of graphics macros",
" in TeX.  It is currently implemented via an external lua script, and ",
" `set term tikz` is a short form of the command `set term lua tikz`.",
" See `term lua` for more information.  Use the command `set term tikz help`",
" to print terminal options."
END_HELP(tikz)
#endif /* TERM_HELP */
