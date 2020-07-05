/**
 * @file
 * This file provides the environment class (`ENV`).
 * It utilizes a couple of custom overloads to enhance usage and mimic the
 * usage within other scripting and programming languages.
 */
#include <gravity_.h>
#pragma hdrstop

#if defined(_WIN32)
	#define setenv(_key, _value_, _unused)      _putenv_s(_key, _value_)
	#define unsetenv(_key)                      _putenv_s(_key, "")
#endif
/**
 * Wraps `getenv()` to be used with Gravity.
 *
 */
static bool gravity_env_get(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
{
	if(!args[1].IsString()) {
		return vm->ReturnError(rindex, "Environment variable key must be a string.");
	}
	const char * key = args[1].GetZString();
	const char * value = getenv(key);
	GravityValue rt = GravityValue::from_undefined();
	// GRAVITY_DEBUG_PRINT("[ENV::GET args : %i] %s => %s\n", nargs, key, value);
	if(value) {
		rt = VALUE_FROM_STRING(vm, value, (uint32)strlen(value));
	}
	return vm->ReturnValue(rt, rindex);
}

/**
 * @brief  Wraps putenv() into a Gravity callable function
 * @param  vm The Gravity Virtual Maschine this function is associated with.
 * @param  args List of arguments passed to this function
 * @param  nargs Number of arguments passed to this function
 * @param  rindex Slot-index for the return value to be stored in.
 * @retval  Weather this function was successful or not.
 */
static bool gravity_env_set(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
{
	if(!args[1].IsString() || (!args[2].IsString() && !args[2].IsNull())) {
		return vm->ReturnError(rindex, "Environment variable key and value must both be strings.");
	}
	gravity_string_t * key = static_cast<gravity_string_t *>(args[1]);
	gravity_string_t * value = args[2].IsString() ? static_cast<gravity_string_t *>(args[2]) : NULL;
	// GRAVITY_DEBUG_PRINT("[ENV::SET args : %i] %s => %s\n", nargs, key, value);
	int rt = (value) ? setenv(key->cptr(), value->cptr(), 1) : unsetenv(key->cptr());
	return vm->ReturnValue(GravityValue::from_int(rt), rindex);
}

static bool gravity_env_keys(gravity_vm * vm, GravityValue * args, uint16 nparams, uint32 rindex) 
{
	extern char ** environ;
	gravity_list_t * keys = gravity_list_new(vm, 16);
	for(char ** env = environ; *env; ++env) {
		char * entry = *env;
		// env is in the form key=value
		uint32 len = 0;
		for(uint32 i = 0; entry[len]; ++i, ++len) {
			if(entry[i] == '=') 
				break;
		}
		GravityValue key = VALUE_FROM_STRING(vm, entry, len);
		keys->array.insert(key);
	}
	return vm->ReturnValue(GravityValue::from_object(reinterpret_cast<gravity_class_t *>(keys)), rindex);
}

class GravityClassImplementation_Env : public GravityClassImplementation {
public:
	GravityClassImplementation_Env() : GravityClassImplementation(GRAVITY_CLASS_ENV_NAME, fCore)
	{
	}
	virtual int Bind(gravity_class_t * pMeta)
	{
		int    ok = 1;
		// .get(key) and .set(key, value)
		gravity_class_bind_outerproc(pMeta, "get", gravity_env_get);
		gravity_class_bind_outerproc(pMeta, "set", gravity_env_set);
		gravity_class_bind_outerproc(pMeta, "keys", gravity_env_keys);
		// Allow map-access
		gravity_class_bind_outerproc(pMeta, GRAVITY_INTERNAL_LOADAT_NAME, gravity_env_get);
		gravity_class_bind_outerproc(pMeta, GRAVITY_INTERNAL_STOREAT_NAME, gravity_env_set);
		return ok;
	}
	virtual void DestroyMeta(gravity_class_t * pMeta)
	{
	}
};

static GravityClassImplementation_Env gravity_clsimp_env;

bool gravity_isenv_class(const gravity_class_t * c) { return (c && c == gravity_clsimp_env.P_Cls); }
const char * gravity_env_name() { return /*GRAVITY_CLASS_ENV_NAME*/gravity_clsimp_env.P_Name; }

void gravity_env_register(gravity_vm * vm) { gravity_clsimp_env.Register(vm); }
void gravity_env_free() { gravity_clsimp_env.UnRegister(); }

#if 0 // {
// MARK: - Internals -
static gravity_class_t * gravity_class_env = NULL;
static uint32 refcount = 0;

static void create_optional_class() 
{
	gravity_class_env = gravity_class_new_pair(NULL, GRAVITY_CLASS_ENV_NAME, NULL, 0, 0);
	gravity_class_t * meta = gravity_class_get_meta(gravity_class_env);
	// .get(key) and .set(key, value)
	gravity_class_bind(meta, "get", NEW_CLOSURE_VALUE(gravity_env_get));
	gravity_class_bind(meta, "set", NEW_CLOSURE_VALUE(gravity_env_set));
	gravity_class_bind(meta, "keys", NEW_CLOSURE_VALUE(gravity_env_keys));
	// Allow map-access
	gravity_class_bind(meta, GRAVITY_INTERNAL_LOADAT_NAME, NEW_CLOSURE_VALUE(gravity_env_get));
	gravity_class_bind(meta, GRAVITY_INTERNAL_STOREAT_NAME, NEW_CLOSURE_VALUE(gravity_env_set));
	SETMETA_INITED(gravity_class_env);
}

// MARK: - Commons -

void gravity_env_register(gravity_vm * vm) 
{
	if(!gravity_class_env) {
		create_optional_class();
	}
	++refcount;
	if(!vm || gravity_vm_ismini(vm)) 
		return;
	gravity_vm_setvalue(vm, GRAVITY_CLASS_ENV_NAME, GravityValue::from_object(gravity_class_env));
}

void gravity_env_free() 
{
	if(gravity_class_env) {
		if(--refcount) 
			return;
		else {
			gravity_class_t * meta = gravity_class_get_meta(gravity_class_env);
			gravity_class_free_core(NULL, meta);
			gravity_class_free_core(NULL, gravity_class_env);
			gravity_class_env = NULL;
		}
	}
}

bool gravity_isenv_class(const gravity_class_t * c) { return (c == gravity_class_env); }
const char * gravity_env_name() { return GRAVITY_CLASS_ENV_NAME; }
#endif // } 0