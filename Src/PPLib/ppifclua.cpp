// PPIFCLUA.CPP
// Copyright (c) A.Sobolev 2018
// @codepage UTF-8
// Экспериментальный модуль интерфейса с LUA
//
#include <pp.h>
#pragma hdrstop

//extern "C" {
	#include <..\osf\lua\lua.h>
	#include <..\osf\lua\lualib.h>
	#include <..\osf\lua\lauxlib.h>
//}

template <class T> class Luna {
public:
	static void Register(lua_State *L) 
	{
		lua_pushcfunction(L, &Luna<T>::constructor);
		lua_setglobal(L, T::GetClsName());
		luaL_newmetatable(L, T::GetClsName());
		lua_pushstring(L, "__gc");
		lua_pushcfunction(L, &Luna<T>::gc_obj);
		lua_settable(L, -3);
	}
	static int constructor(lua_State *L) 
	{
		T * obj = new T(L);
		lua_newtable(L);
		lua_pushnumber(L, 0);
		T ** a = (T**)lua_newuserdata(L, sizeof(T*));
		*a = obj;
		luaL_getmetatable(L, T::GetClsName());
		lua_setmetatable(L, -2);
		lua_settable(L, -3); // table[0] = obj;
		for(int i = 0; T::LuaRegTab[i].name; i++) {
			lua_pushstring(L, T::LuaRegTab[i].name);
			lua_pushnumber(L, i);
			lua_pushcclosure(L, &Luna<T>::thunk, 1);
			lua_settable(L, -3);
		}
		return 1;
	}
	static int thunk(lua_State *L) 
	{
		int i = (int)lua_tonumber(L, lua_upvalueindex(1));
		lua_pushnumber(L, 0);
		lua_gettable(L, 1);
		T ** obj = static_cast<T**>(luaL_checkudata(L, -1, T::GetClsName()));
		lua_remove(L, -1);
		return ((*obj)->*(T::LuaRegTab[i].mfunc))(L);
	}
	static int gc_obj(lua_State * L) 
	{
		T** obj = static_cast <T**>(luaL_checkudata(L, -1, T::GetClsName()));
		delete (*obj);
		return 0;
	}
	struct RegType {
		const char * name;
		int (T::*mfunc)(lua_State *);
	};
};

class PPLuaModule {
public:
	static const char * GetClsName() { return "LuaModule"; }
	static const Luna <PPLuaModule>::RegType LuaRegTab[];
	//
	SLAPI  PPLuaModule (lua_State * pL)
	{
	}
	SLAPI ~PPLuaModule ()
	{
	}
	int Version(lua_State * pL)
	{
		SString result_buf;
		char   ver_text[128];
		PPVersionInfo vi = DS.GetVersionInfo();
		vi.GetProductName(result_buf);
		vi.GetVersionText(ver_text, sizeof(ver_text));
		result_buf.Space().Cat(ver_text);
		lua_pushstring(pL, result_buf);
		return 1;
	}
};

const Luna <PPLuaModule>::RegType PPLuaModule ::LuaRegTab[] = {
  { "Version", &PPLuaModule ::Version },
  { 0 }
};

#if 0 // test lua script {

local pm = LuaModule()\nreturn pm:Version()\n

#endif // } test lua script

/*int SLAPI RegisterLuaModules()
{
	Luna <LuaModule>::Register(L);
}*/

int SLAPI ExecuteLuaScript(const char * pScriptText)
{
	int    ok = 1;
	SString result_buf;
    lua_State * p_ctx = luaL_newstate();
	luaL_openlibs(p_ctx);
	Luna <PPLuaModule>::Register(p_ctx);
	//luaL_newlib(p_ctx, PapyrusLib);
	//lua_setglobal(p_ctx, "PapyrusLib");
	THROW(luaL_loadstring(p_ctx, pScriptText) == 0);
	THROW(lua_pcall(p_ctx, 0, LUA_MULTRET, 0) == 0);
	{
		int ret_count = lua_gettop(p_ctx);
		for(int next_ret_idx = -ret_count; next_ret_idx < 0; next_ret_idx++) 
			result_buf = lua_tostring(p_ctx, next_ret_idx);
	}
    lua_pcall(p_ctx, 0, 0, 0);
	CATCHZOK
	return ok;
}

int SLAPI LuaTest()
{
	int    ok = 1;
	const char * p_lua_script = "local pm = LuaModule()\nreturn pm:Version()\n";
	ExecuteLuaScript(p_lua_script);
	return ok;
}

#if 0 // {
static const struct luaL_Reg PapyrusLib[] = {
	//{ "Version", Lua_PapyrusVersion },
	{ NULL, NULL }
};

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
#endif // } 0
