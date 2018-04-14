// PPIFCLUA.CPP
// Copyright (c) A.Sobolev 2018
// @codepage UTF-8
// Экспериментальный модуль интерфейса с LUA
//
#include <pp.h>
#pragma hdrstop

extern "C" {
	#include <..\osf\lua\lua.h>
	#include <..\osf\lua\lualib.h>
	#include <..\osf\lua\lauxlib.h>
}

static int Lua_PapyrusVersion(lua_State * luaVM)
{
	SString result_buf;
	char   ver_text[128];
	PPVersionInfo vi = DS.GetVersionInfo();
	vi.GetProductName(result_buf);
	vi.GetVersionText(ver_text, sizeof(ver_text));
	result_buf.Space().Cat(ver_text);
	lua_pushstring(luaVM, result_buf);
	return 1;
}

static const struct luaL_Reg PapyrusLib[] = {
	{ "Version", Lua_PapyrusVersion },
	{ NULL, NULL }
};

int SLAPI ExecuteLuaScript(const char * pScriptText)
{
	int    ok = 1;
    lua_State * p_ctx = luaL_newstate();
	luaL_openlibs(p_ctx);
	luaL_newlib(p_ctx, PapyrusLib);
	lua_setglobal(p_ctx, "PapyrusLib");
	THROW(luaL_loadstring(p_ctx, pScriptText) == 0);
	THROW(lua_pcall(p_ctx, 0, LUA_MULTRET, 0) == 0);
    lua_pcall(p_ctx, 0, 0, 0);
	CATCHZOK
	return ok;
}

int SLAPI ExecuteLuaScriptFile(const char * pFileName)
{
	int    ok = 1;
    lua_State * p_ctx = luaL_newstate();
	luaL_openlibs(p_ctx);
	luaL_newlib(p_ctx, PapyrusLib);
	lua_setglobal(p_ctx, "PapyrusLib");
	THROW(luaL_loadfile(p_ctx, pFileName) == 0);
	THROW(lua_pcall(p_ctx, 0, LUA_MULTRET, 0) == 0);
    lua_pcall(p_ctx, 0, 0, 0);
	CATCHZOK
	return ok;
}