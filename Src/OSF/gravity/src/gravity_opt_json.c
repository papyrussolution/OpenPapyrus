//  gravity_opt_json.c
//  gravity
//
//  Created by Marco Bambini on 20/08/2019.
//  Copyright © 2019 CreoLabs. All rights reserved.
//
#include <gravity_.h>
#pragma hdrstop

#define GRAVITY_JSON_STRINGIFY_NAME     "stringify"
#define GRAVITY_JSON_PARSE_NAME         "parse"
#define STATIC_BUFFER_SIZE              8192

// MARK: - Implementation -

static bool JSON_stringify(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
{
	if(nargs < 2) 
		return vm->ReturnNull(rindex);
	// extract value
	GravityValue value = args[1];
	// special case for string because it can be huge (and must be quoted)
	if(value.IsString()) {
		const int nchars = 5;
		const char * v = static_cast<gravity_string_t *>(value)->cptr();
		size_t vlen = static_cast<gravity_string_t *>(value)->len;
		// string must be quoted
		if(vlen < 4096-nchars) {
			char vbuffer2[4096];
			vlen = snprintf(vbuffer2, sizeof(vbuffer2), "\"%s\"", v);
			return vm->ReturnValue(VALUE_FROM_STRING(vm, vbuffer2, (uint32)vlen), rindex);
		}
		else {
			char * vbuffer2 = static_cast<char *>(mem_alloc(NULL, vlen + nchars));
			vlen = snprintf(vbuffer2, vlen + nchars, "\"%s\"", v);
			return vm->ReturnValue(GravityValue::from_object(reinterpret_cast<gravity_class_t *>(gravity_string_new(vm, vbuffer2, (uint32)vlen, 0))), rindex);
		}
	}
	// primitive cases supported by JSON (true, false, null, number)
	char vbuffer[512];
	const char * v = NULL;
	if(value.IsNull() || value.IsUndefined()) 
		v = "null";
	// was %g but we don't like scientific notation nor the missing .0 in case of float number with no decimals
	else if(value.IsFloat()) {
		snprintf(vbuffer, sizeof(vbuffer), "%f", value.f); v = vbuffer;
	}
	else if(value.IsBool()) 
		v = (value.n) ? "true" : "false";
	else if(value.IsInt()) {
	#if GRAVITY_ENABLE_INT64
		snprintf(vbuffer, sizeof(vbuffer), "%" PRId64 "", value.n);
	#else
		snprintf(vbuffer, sizeof(vbuffer), "%d", value.n);
	#endif
		v = vbuffer;
	}
	if(v) return vm->ReturnValue(gravity_zstring_to_value(vm, v), rindex);

	// more complex object case (list, map, class, closure, instance/object)
	GravityJson * json = json_new();
	json_set_option(json, json_opt_no_maptype);
	json_set_option(json, json_opt_no_undef);
	json_set_option(json, json_opt_prettify);
	gravity_value_serialize(NULL, value, json);
	size_t len = 0;
	const char * jbuffer = json_buffer(json, &len);
	const char * buffer = string_ndup(jbuffer, len);
	gravity_string_t * s = gravity_string_new(vm, (char *)buffer, (uint32)len, 0);
	json_free(json);
	return vm->ReturnValue(GravityValue::from_object(reinterpret_cast<gravity_class_t *>(s)), rindex);
}

static GravityValue JSON_value(gravity_vm * vm, json_value * json) 
{
	switch(json->type) {
		case json_none:
		case json_null: return GravityValue::from_null();
		case json_object: 
			{
				gravity_class_t * obj = gravity_object_deserialize(vm, json);
				GravityValue objv = (obj) ? GravityValue::from_object(obj) : GravityValue::from_null();
				return objv;
			}
		case json_array: 
			{
				uint length = json->u.array.length;
				gravity_list_t * list = gravity_list_new(vm, length);
				for(uint i = 0; i < length; ++i) {
					GravityValue value = JSON_value(vm, json->u.array.values[i]);
					list->array.insert(value);
				}
				return GravityValue::from_object(reinterpret_cast<gravity_class_t *>(list));
			}
		case json_integer: return GravityValue::from_int(json->u.integer);
		case json_double: return GravityValue::from_float(json->u.dbl);
		case json_string: return VALUE_FROM_STRING(vm, json->u.string.ptr, json->u.string.length);
		case json_boolean: return GravityValue::from_bool(json->u.boolean);
	}
	return GravityValue::from_null();
}

static bool JSON_parse(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
{
	if(nargs < 2) 
		return vm->ReturnNull(rindex);
	// value to parse
	GravityValue value = args[1];
	if(!value.IsString()) 
		return vm->ReturnNull(rindex);
	gravity_string_t * string = static_cast<gravity_string_t *>(value);
	json_value * json = json_parse(string->cptr(), string->len);
	if(!json) 
		return vm->ReturnNull(rindex);
	return vm->ReturnValue(JSON_value(vm, json), rindex);
}

//static bool JSON_begin_object (gravity_vm *vm, GravityValue *args, uint16 nargs, uint32 rindex) { return false; }
//static bool json_end_object (gravity_vm *vm, GravityValue *args, uint16 nargs, uint32 rindex) {}
//static bool json_begin_array (gravity_vm *vm, GravityValue *args, uint16 nargs, uint32 rindex) {}
//static bool json_end_array (gravity_vm *vm, GravityValue *args, uint16 nargs, uint32 rindex) {}
//static bool json_add_object (gravity_vm *vm, GravityValue *args, uint16 nargs, uint32 rindex) {}

class GravityClassImplementation_Json : public GravityClassImplementation {
public:
	GravityClassImplementation_Json() : GravityClassImplementation(GRAVITY_CLASS_JSON_NAME, fCore)
	{
	}
	virtual int Bind(gravity_class_t * pMeta)
	{
		int    ok = 1;
		//gravity_class_bind(pMeta, GRAVITY_INTERNAL_EXEC_NAME, NEW_CLOSURE_VALUE(JSON_exec));
		gravity_class_bind(pMeta, GRAVITY_JSON_STRINGIFY_NAME, NEW_CLOSURE_VALUE(JSON_stringify));
		gravity_class_bind(pMeta, GRAVITY_JSON_PARSE_NAME, NEW_CLOSURE_VALUE(JSON_parse));
		//gravity_class_bind(gravity_class_json, "begin", NEW_CLOSURE_VALUE(JSON_begin_object));
		return ok;
	}
	virtual void DestroyMeta(gravity_class_t * pMeta)
	{
	}
};

static GravityClassImplementation_Json gravity_clsimp_json;

bool gravity_isjson_class(const gravity_class_t * c) { return (c && c == gravity_clsimp_json.P_Cls); }
const char * gravity_json_name() { return gravity_clsimp_json.P_Name/*GRAVITY_CLASS_JSON_NAME*/; }
void gravity_json_register(gravity_vm * vm) { gravity_clsimp_json.Register(vm); }
void gravity_json_free() { gravity_clsimp_json.UnRegister(); }

#if 0 // {

static gravity_class_t * gravity_class_json = NULL;
static uint32 refcount = 0;

// MARK: - Internals -
static void create_optional_class() 
{
	gravity_class_json = gravity_class_new_pair(NULL, GRAVITY_CLASS_JSON_NAME, NULL, 0, 0);
	gravity_class_t * json_meta = gravity_class_get_meta(gravity_class_json);
	//gravity_class_bind(json_meta, GRAVITY_INTERNAL_EXEC_NAME, NEW_CLOSURE_VALUE(JSON_exec));
	gravity_class_bind(json_meta, GRAVITY_JSON_STRINGIFY_NAME, NEW_CLOSURE_VALUE(JSON_stringify));
	gravity_class_bind(json_meta, GRAVITY_JSON_PARSE_NAME, NEW_CLOSURE_VALUE(JSON_parse));
	//gravity_class_bind(gravity_class_json, "begin", NEW_CLOSURE_VALUE(JSON_begin_object));
	SETMETA_INITED(gravity_class_json);
}

// MARK: - Commons -

bool gravity_isjson_class(const gravity_class_t * c) { return (c == gravity_class_json); }
const char * gravity_json_name() { return GRAVITY_CLASS_JSON_NAME; }

void gravity_json_register(gravity_vm * vm) 
{
	if(!gravity_class_json) {
		create_optional_class();
	}
	++refcount;
	if(!vm || gravity_vm_ismini(vm)) 
		return;
	gravity_vm_setvalue(vm, GRAVITY_CLASS_JSON_NAME, GravityValue::from_object(gravity_class_json));
}

void gravity_json_free() 
{
	if(gravity_class_json) {
		if(--refcount) 
			return;
		else {
			gravity_class_free_core(NULL, gravity_class_get_meta(gravity_class_json));
			gravity_class_free_core(NULL, gravity_class_json);
			gravity_class_json = NULL;
		}
	}
}
#endif