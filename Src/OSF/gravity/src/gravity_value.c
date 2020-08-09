//  gravity_value.c
//  gravity
//
//  Created by Marco Bambini on 11/12/14.
//  Copyright (c) 2014 CreoLabs. All rights reserved.
//
#include <gravity_.h>
#pragma hdrstop

// mark object visited to avoid infinite loop
//#define SET_OBJECT_VISITED_FLAG(_obj, _flag)    (reinterpret_cast<gravity_class_t *>(_obj)->GcVisited = _flag)
#define SET_OBJECT_VISITED_FLAG(_obj, _flag)      SETFLAG(/*reinterpret_cast<gravity_class_t *>*/(_obj)->Flags, GravityObjectBase::fGcVisited, _flag)

const char * GravityValue::GetZString() { return reinterpret_cast<gravity_string_t *>(Ptr)->cptr(); }

// MARK: -

static void gravity_function_special_serialize(gravity_function_t * f, const char * key, GravityJson * json);
static gravity_map_t * gravity_map_deserialize(gravity_vm * vm, json_value * json);

static void gravity_hash_serialize(gravity_hash_t * table, GravityValue key, GravityValue value, void * data) 
{
	GravityJson * json = (GravityJson*)data;
	if(value.IsClosure()) 
		value = GravityValue::from_object(reinterpret_cast<gravity_class_t *>(VALUE_AS_CLOSURE(value)->f));
	if(value.IsFunction()) {
		gravity_function_t * f = VALUE_AS_FUNCTION(value);
		if(f->tag == EXEC_TYPE_SPECIAL) 
			gravity_function_special_serialize(f, key.GetZString(), json);
		else {
			// there was an issue here due to the fact that when a subclass needs to use a $init from a
			// superclass
			// internally it has a unique name (key) but f->identifier continue to be called $init
			// without this fix the subclass would continue to have 2 or more $init functions
			gravity_string_t * s = static_cast<gravity_string_t *>(key);
			bool is_super_function = ((s->len > 5) && (string_casencmp(s->cptr(), CLASS_INTERNAL_INIT_NAME, 5) == 0));
			const char * saved = f->identifier;
			if(is_super_function) 
				f->identifier = s->cptr();
			gravity_function_serialize(f, json);
			if(is_super_function) 
				f->identifier = saved;
		}
	}
	else if(value.IsClass()) {
		gravity_class_serialize(static_cast<gravity_class_t *>(value), json);
	}
	else
		assert(0);
}

void gravity_hash_keyvaluefree(gravity_hash_t * table, GravityValue key, GravityValue value, void * data) 
{
	gravity_vm * vm = static_cast<gravity_vm *>(data);
	gravity_value_free(vm, key);
	gravity_value_free(vm, value);
}

void gravity_hash_keyfree(gravity_hash_t * table, GravityValue key, GravityValue value, void * data) 
{
	gravity_vm * vm = static_cast<gravity_vm *>(data);
	gravity_value_free(vm, key);
}

void gravity_hash_finteralfree(gravity_hash_t * table, GravityValue key, GravityValue value, void * data) 
{
	if(value.IsObject()) {
		gravity_class_t * obj = VALUE_AS_OBJECT(value);
		if(obj->IsClosure()) {
			gravity_closure_t * closure = (gravity_closure_t *)obj;
			if(closure->f && closure->f->tag == EXEC_TYPE_INTERNAL) 
				gravity_function_free(NULL, closure->f);
		}
	}
}

void gravity_hash_valuefree(gravity_hash_t * table, GravityValue key, GravityValue value, void * data) 
{
	gravity_vm * vm = static_cast<gravity_vm *>(data);
	gravity_value_free(vm, value);
}

static void gravity_hash_internalsize(gravity_hash_t * table, GravityValue key, GravityValue value, void * data1, void * data2) 
{
	uint32 * size = static_cast<uint32 *>(data1);
	gravity_vm * vm = static_cast<gravity_vm *>(data2);
	*size = gravity_value_size(vm, key);
	*size += gravity_value_size(vm, value);
}

static void gravity_hash_gray(gravity_hash_t * table, GravityValue key, GravityValue value, void * data1) 
{
	gravity_vm * vm = static_cast<gravity_vm *>(data1);
	gravity_gray_value(vm, key);
	gravity_gray_value(vm, value);
}

// MARK: -

gravity_module_t * gravity_module_new(gravity_vm * vm, const char * identifier) 
{
	gravity_module_t * m = (gravity_module_t*)mem_alloc(NULL, sizeof(gravity_module_t));
	assert(m);
	m->isa = GravityEnv.P_ClsModule;
	m->identifier = sstrdup(identifier);
	m->htable = gravity_hash_create(0, gravity_value_hash, gravity_value_equals, gravity_hash_keyvaluefree, (void*)vm);
	gravity_vm_transfer(vm, (gravity_class_t *)m);
	return m;
}

void gravity_module_free(gravity_vm * vm, gravity_module_t * m) 
{
	if(m) {
		mem_free(m->identifier);
		gravity_hash_free(m->htable);
		mem_free(m);
	}
}

uint32 gravity_module_size(gravity_vm * vm, gravity_module_t * m) 
{
	SET_OBJECT_VISITED_FLAG(m, true);
	uint32 hash_size = 0;
	gravity_hash_iterate2(m->htable, gravity_hash_internalsize, (void*)&hash_size, (void*)vm);
	uint32 module_size = (sizeof(gravity_module_t)) + sstrlen(m->identifier) + hash_size + gravity_hash_memsize(m->htable);
	SET_OBJECT_VISITED_FLAG(m, false);
	return module_size;
}

void gravity_module_blacken(gravity_vm * vm, gravity_module_t * m) 
{
	gravity_vm_memupdate(vm, gravity_module_size(vm, m));
	gravity_hash_iterate(m->htable, gravity_hash_gray, (void*)vm);
}

// MARK: -

void FASTCALL gravity_class_bind(gravity_class_t * c, const char * key, GravityValue value) 
{
	if(value.IsClass()) {
		// set has_outer when bind a class inside another class
		gravity_class_t * obj = static_cast<gravity_class_t *>(value);
		obj->Flags |= GravityObjectBase::fHasOuter; //obj->has_outer = true;
	}
	gravity_hash_insert(c->htable, gravity_zstring_to_value(NULL, key), value);
}

void FASTCALL gravity_class_bind_outerproc(gravity_class_t * c, const char * key, gravity_c_internal proc)
{
	gravity_class_bind(c, key, NEW_CLOSURE_VALUE(proc));
}

void FASTCALL gravity_class_bind_property_outerproc(gravity_class_t * c, const char * key, gravity_c_internal getProc, gravity_c_internal setProc)
{
	gravity_closure_t * p_closure = computed_property_create(NULL, (getProc ? NEW_FUNCTION(getProc) : 0), (setProc ? NEW_FUNCTION(setProc) : 0));
	gravity_class_bind(c, key, GravityValue::from_object(reinterpret_cast<gravity_class_t *>(p_closure)));
}

gravity_class_t * gravity_class_getsuper(gravity_class_t * c) { return c->superclass; }

bool gravity_class_grow(gravity_class_t * c, uint32 n) 
{
	mem_free(c->ivars);
	if((c->nivars + n) >= MAX_IVARS) 
		return false;
	else {
		c->nivars += n;
		c->ivars = static_cast<GravityValue *>(mem_alloc(NULL, c->nivars * sizeof(GravityValue)));
		for(uint32 i = 0; i < c->nivars; ++i) 
			c->ivars[i] = GravityValue::from_null();
		return true;
	}
}

bool gravity_class_setsuper(gravity_class_t * baseclass, gravity_class_t * superclass) 
{
	if(!superclass) 
		return true;
	baseclass->superclass = superclass;
	// check meta class first
	gravity_class_t * supermeta = superclass ? gravity_class_get_meta(superclass) : NULL;
	uint32 n1 = supermeta ? supermeta->nivars : 0;
	if(n1) 
		if(!gravity_class_grow(gravity_class_get_meta(baseclass), n1)) 
			return false;
	// then check real class
	uint32 n2 = (superclass) ? superclass->nivars : 0;
	if(n2) 
		if(!gravity_class_grow(baseclass, n2)) 
			return false;
	return true;
}

bool gravity_class_setsuper_extern(gravity_class_t * baseclass, const char * identifier) 
{
	if(identifier) 
		baseclass->superlook = sstrdup(identifier);
	return true;
}

gravity_class_t * FASTCALL gravity_class_new_single(gravity_vm * vm, const char * identifier, uint32 nivar) 
{
	gravity_class_t * c = static_cast<gravity_class_t *>(mem_alloc(NULL, sizeof(gravity_class_t)));
	assert(c);
	c->isa = GravityEnv.P_ClsClass;
	c->identifier = sstrdup(identifier);
	c->superclass = NULL;
	c->nivars = nivar;
	c->htable = gravity_hash_create(0, gravity_value_hash, gravity_value_equals, gravity_hash_keyfree, NULL);
	if(nivar) {
		c->ivars = static_cast<GravityValue *>(mem_alloc(NULL, nivar * sizeof(GravityValue)));
		for(uint32 i = 0; i < nivar; ++i) 
			c->ivars[i] = GravityValue::from_null();
	}
	gravity_vm_transfer(vm, c);
	return c;
}

gravity_class_t * FASTCALL gravity_class_new_pair(gravity_vm * vm, const char * identifier, gravity_class_t * superclass, uint32 nivar, uint32 nsvar) 
{
	gravity_class_t * c = 0;
	// each class must have a valid identifier
	if(identifier) {
		char buffer[512];
		snprintf(buffer, sizeof(buffer), "%s meta", identifier);
		// ivar count/grow is managed by gravity_class_setsuper
		gravity_class_t * meta = gravity_class_new_single(vm, buffer, nsvar);
		meta->objclass = GravityEnv.P_ClsObj;
		gravity_class_setsuper(meta, GravityEnv.P_ClsClass);
		c = gravity_class_new_single(vm, identifier, nivar);
		c->objclass = meta;
		// a class without a superclass in a subclass of Object
		gravity_class_setsuper(c, (superclass) ? superclass : GravityEnv.P_ClsObj);
	}
	return c;
}

gravity_class_t * FASTCALL gravity_class_get_meta(gravity_class_t * c) 
{
	// meta classes have objclass set to class object
	return (c->objclass == GravityEnv.P_ClsObj) ? c : c->objclass;
}

const gravity_class_t * FASTCALL gravity_class_get_meta_const(const gravity_class_t * c) 
{
	// meta classes have objclass set to class object
	return (c->objclass == GravityEnv.P_ClsObj) ? c : c->objclass;
}

bool gravity_class_is_meta(const gravity_class_t * c) 
{
	// meta classes have objclass set to class object
	return (c->objclass == GravityEnv.P_ClsObj);
}

bool gravity_class_is_anon(const gravity_class_t * c) 
{
	return (string_casencmp(c->identifier, GRAVITY_VM_ANONYMOUS_PREFIX, strlen(GRAVITY_VM_ANONYMOUS_PREFIX)) == 0);
}

uint32 gravity_class_count_ivars(const gravity_class_t * c) 
{
	return static_cast<uint32>(c->nivars);
}

int16 gravity_class_add_ivar(gravity_class_t * c, const char * identifier) 
{
	// TODO: add identifier in array (for easier debugging)
	++c->nivars;
	return static_cast<int16>(c->nivars-1); // its a C array so index is 0 based
}

void gravity_class_dump(gravity_class_t * c) 
{
	gravity_hash_dump(c->htable);
}

void gravity_class_setxdata(gravity_class_t * c, void * xdata) 
{
	c->xdata = xdata;
}

void gravity_class_serialize(gravity_class_t * c, GravityJson * json) 
{
	const char * label = json_get_label(json, c->identifier);
	json_begin_object(json, label);
	json_add_cstring(json, GRAVITY_JSON_LABELTYPE, GRAVITY_JSON_CLASS); // MANDATORY 1st FIELD
	json_add_cstring(json, GRAVITY_JSON_LABELIDENTIFIER, c->identifier); // MANDATORY 2nd FIELD
	// avoid write superclass name if it is the default Object one
	if(c->superclass && c->superclass->identifier && strcmp(c->superclass->identifier, GRAVITY_CLASS_OBJECT_NAME) != 0) {
		json_add_cstring(json, GRAVITY_JSON_LABELSUPER, c->superclass->identifier);
	}
	else if(c->superlook) {
		json_add_cstring(json, GRAVITY_JSON_LABELSUPER, c->superlook);
	}
	// get c meta class
	gravity_class_t * meta = gravity_class_get_meta(c);
	// number of instance (and static) variables
	json_add_int(json, GRAVITY_JSON_LABELNIVAR, c->nivars);
	if((c != meta) && (meta->nivars > 0)) json_add_int(json, GRAVITY_JSON_LABELSIVAR, meta->nivars);
	// struct flag
	if(c->Flags & GravityObjectBase::fIsStruct/*c->is_struct*/) 
		json_add_bool(json, GRAVITY_JSON_LABELSTRUCT, true);
	// serialize htable
	if(c->htable) {
		gravity_hash_iterate(c->htable, gravity_hash_serialize, (void*)json);
	}
	// serialize meta class
	if(c != meta) {
		// further proceed only if it has something to be serialized
		if((meta->htable) && (gravity_hash_count(meta->htable) > 0)) {
			json_begin_array(json, GRAVITY_JSON_LABELMETA);
			gravity_hash_iterate(meta->htable, gravity_hash_serialize, (void*)json);
			json_end_array(json);
		}
	}
	json_end_object(json);
}

gravity_class_t * gravity_class_deserialize(gravity_vm * vm, json_value * json) 
{
	// sanity check
	if(json->type != json_object) 
		return NULL;
	if(json->u.object.length < 3) 
		return NULL;
	// scan identifier
	json_value * p_value = json->u.object.values[1].value;
	const char * key = json->u.object.values[1].name;
	// sanity check identifier
	if(string_casencmp(key, GRAVITY_JSON_LABELIDENTIFIER, strlen(key)) != 0) return NULL;
	assert(p_value->type == json_string);
	// create class and meta
	gravity_class_t * c = gravity_class_new_pair(vm, p_value->u.string.ptr, NULL, 0, 0);
	DEBUG_DESERIALIZE("DESERIALIZE CLASS: %p %s\n", c, value->u.string.ptr);
	// get its meta class
	gravity_class_t * meta = gravity_class_get_meta(c);
	uint32 n = json->u.object.length;
	for(uint32 i = 2; i<n; ++i) { // from 2 to skip type and identifier
		// parse values
		p_value = json->u.object.values[i].value;
		key = json->u.object.values[i].name;
		if(p_value->type != json_object) {
			// super
			if(string_casencmp(key, GRAVITY_JSON_LABELSUPER, strlen(key)) == 0) {
				// the trick here is to re-use a runtime field to store a temporary static data like
				// superclass name
				// (only if different than the default Object one)
				if(strcmp(p_value->u.string.ptr, GRAVITY_CLASS_OBJECT_NAME) != 0) {
					c->xdata = (void *)sstrdup(p_value->u.string.ptr);
				}
				continue;
			}
			// nivar
			if(string_casencmp(key, GRAVITY_JSON_LABELNIVAR, strlen(key)) == 0) {
				gravity_class_grow(c, (uint32)p_value->u.integer);
				continue;
			}
			// sivar
			if(string_casencmp(key, GRAVITY_JSON_LABELSIVAR, strlen(key)) == 0) {
				gravity_class_grow(meta, (uint32)p_value->u.integer);
				continue;
			}
			// struct
			if(string_casencmp(key, GRAVITY_JSON_LABELSTRUCT, strlen(key)) == 0) {
				c->Flags |= GravityObjectBase::fIsStruct;//c->is_struct = true;
				continue;
			}
			// meta
			if(string_casencmp(key, GRAVITY_JSON_LABELMETA, strlen(key)) == 0) {
				uint32 m = p_value->u.array.length;
				for(uint32 j = 0; j < m; ++j) {
					json_value * r = p_value->u.array.values[j];
					if(r->type != json_object) 
						continue;
					gravity_class_t * obj = gravity_object_deserialize(vm, r);
					if(!obj) 
						goto abort_load;
					const char * identifier = obj->identifier;
					if(obj->IsFunction()) 
						obj = (gravity_class_t *)gravity_closure_new(vm, (gravity_function_t *)obj);
					if(obj) 
						gravity_class_bind(meta, identifier, GravityValue::from_object(obj));
					else goto abort_load;
				}
				continue;
			}
			// error here
			goto abort_load;
		}
		if(p_value->type == json_object) {
			gravity_class_t * obj = gravity_object_deserialize(vm, p_value);
			if(!obj) 
				goto abort_load;
			const char * identifier = obj->identifier;
			if(obj->IsFunction()) 
				obj = (gravity_class_t *)gravity_closure_new(vm, (gravity_function_t *)obj);
			gravity_class_bind(c, identifier, GravityValue::from_object(obj));
		}
	}
	return c;
abort_load:
	// do not free c here because it is already garbage collected
	return NULL;
}

static void gravity_class_free_internal(gravity_vm * vm, gravity_class_t * c, bool skip_base) 
{
	if(!skip_base || (!gravity_iscore_class(c) && !gravity_isopt_class(c))) {
		DEBUG_FREE("FREE %s", gravity_object_debug(c, true));
		// check if bridged data needs to be freed too
		if(c->xdata && vm) {
			gravity_delegate_t * p_delegate = gravity_vm_delegate(vm);
			if(p_delegate->bridge_free) 
				p_delegate->bridge_free(vm, c);
		}
		mem_free((void*)c->identifier);
		mem_free((void*)c->superlook);
		if(!skip_base) {
			// base classes have functions not registered inside VM so manually free all of them
			gravity_hash_iterate(c->htable, gravity_hash_finteralfree, NULL);
			gravity_hash_iterate(c->htable, gravity_hash_valuefree, NULL);
		}
		gravity_hash_free(c->htable);
		mem_free((void*)c->ivars);
		mem_free((void*)c);
	}
}

void gravity_class_free_core(gravity_vm * vm, gravity_class_t * c) { gravity_class_free_internal(vm, c, false); }
void gravity_class_free(gravity_vm * vm, gravity_class_t * c) { gravity_class_free_internal(vm, c, true); }

/*inline*/gravity_class_t * FASTCALL gravity_class_lookup(gravity_class_t * c, GravityValue key) 
{
	while(c) {
		GravityValue * v = gravity_hash_lookup(c->htable, key);
		if(v) 
			return v->Ptr;
		c = c->superclass;
	}
	return NULL;
}

gravity_class_t * gravity_class_lookup_class_identifier(gravity_class_t * c, const char * identifier) 
{
	while(c) {
		if(sstreq(c->identifier, identifier)) 
			return c;
		c = c->superclass;
	}
	return NULL;
}

/*inline*/gravity_closure_t * FASTCALL gravity_class_lookup_closure(gravity_class_t * c, GravityValue key) 
{
	gravity_class_t * obj = gravity_class_lookup(c, key);
	return (obj && obj->IsClosure()) ? (gravity_closure_t *)obj : NULL;
}

/*inline*/gravity_closure_t * gravity_class_lookup_constructor(gravity_class_t * c, uint32 nparams) 
{
	if(c->xdata) {
		// bridged class so check for special $initN function
		if(nparams == 0) {
			STATICVALUE_FROM_STRING(key, CLASS_INTERNAL_INIT_NAME, strlen(CLASS_INTERNAL_INIT_NAME));
			return (gravity_closure_t *)gravity_class_lookup(c, key);
		}
		else {
			// for bridged classed (which can have more than one init constructor like in objc) the convention is
			// to map each bridged init with a special $initN function (where N>0 is num params)
			char name[256]; 
			snprintf(name, sizeof(name), "%s%d", CLASS_INTERNAL_INIT_NAME, nparams);
			STATICVALUE_FROM_STRING(key, name, strlen(name));
			return (gravity_closure_t *)gravity_class_lookup(c, key);
		}
	}
	else {
		// for non bridge classes just check for constructor
		STATICVALUE_FROM_STRING(key, CLASS_CONSTRUCTOR_NAME, strlen(CLASS_CONSTRUCTOR_NAME));
		return (gravity_closure_t *)gravity_class_lookup(c, key);
	}
}

uint32 gravity_class_size(gravity_vm * vm, gravity_class_t * c) 
{
	SET_OBJECT_VISITED_FLAG(c, true);
	uint32 class_size = sizeof(gravity_class_t) + (c->nivars * sizeof(GravityValue)) + sstrlen(c->identifier);
	uint32 hash_size = 0;
	gravity_hash_iterate2(c->htable, gravity_hash_internalsize, (void*)&hash_size, (void*)vm);
	hash_size += gravity_hash_memsize(c->htable);
	gravity_delegate_t * delegate = gravity_vm_delegate(vm);
	if(c->xdata && delegate->bridge_size)
		class_size += delegate->bridge_size(vm, reinterpret_cast<gravity_class_t *>(c->xdata));
	SET_OBJECT_VISITED_FLAG(c, false);
	return class_size;
}

void gravity_class_blacken(gravity_vm * vm, gravity_class_t * c) 
{
	gravity_vm_memupdate(vm, gravity_class_size(vm, c));
	gravity_gray_object(vm, c->objclass); // metaclass
	gravity_gray_object(vm, c->superclass); // superclass
	gravity_hash_iterate(c->htable, gravity_hash_gray, (void*)vm); // internals
	// ivars
	for(uint32 i = 0; i<c->nivars; ++i) {
		gravity_gray_value(vm, c->ivars[i]);
	}
}

// MARK: -

gravity_function_t * gravity_function_new(gravity_vm * vm, const char * identifier, uint16 nparams, uint16 nlocals, uint16 ntemps, void * code) 
{
	gravity_function_t * f = new gravity_function_t(nparams, nlocals, ntemps);//(gravity_function_t *)mem_alloc(NULL, sizeof(gravity_function_t));
	assert(f);
	// @ctr f->isa = GravityEnv.P_ClsFunc;
	f->identifier = sstrdup(identifier);
	// @ctr f->tag = EXEC_TYPE_NATIVE;
	// @ctr f->nparams = nparams;
	// @ctr f->nlocals = nlocals;
	// @ctr f->ntemps = ntemps;
	// @ctr f->nupvalues = 0;
	// only available in EXEC_TYPE_NATIVE case
	// code is != NULL when EXEC_TYPE_NATIVE
	if(code) {
		f->U.Nf.useargs = false;
		f->U.Nf.bytecode = static_cast<uint32 *>(code);
		marray_init(f->U.Nf.cpool);
		marray_init(f->U.Nf.pvalue);
		marray_init(f->U.Nf.pname);
	}
	gravity_vm_transfer(vm, reinterpret_cast<gravity_class_t *>(f));
	return f;
}

gravity_function_t * gravity_function_new_internal(gravity_vm * vm, const char * identifier, gravity_c_internal exec, uint16 nparams) 
{
	gravity_function_t * f = gravity_function_new(vm, identifier, nparams, 0, 0, NULL);
	f->tag = EXEC_TYPE_INTERNAL;
	f->U.internal = exec;
	return f;
}

gravity_function_t * gravity_function_new_special(gravity_vm * vm, const char * identifier, uint16 index, void * getter, void * setter) 
{
	gravity_function_t * f = gravity_function_new(vm, identifier, 0, 0, 0, NULL);
	f->tag = EXEC_TYPE_SPECIAL;
	f->U.Sf.index = index;
	f->U.Sf.special[0] = getter;
	f->U.Sf.special[1] = setter;
	return f;
}

gravity_function_t * gravity_function_new_bridged(gravity_vm * vm, const char * identifier, void * xdata) 
{
	gravity_function_t * f = gravity_function_new(vm, identifier, 0, 0, 0, NULL);
	f->tag = EXEC_TYPE_BRIDGED;
	f->xdata = xdata;
	return f;
}

uint16 FASTCALL gravity_function_cpool_add(gravity_vm * vm, gravity_function_t * f, GravityValue v) 
{
	assert(f->tag == EXEC_TYPE_NATIVE);
	size_t n = f->U.Nf.cpool.getCount();
	for(size_t i = 0; i < n; i++) {
		GravityValue v2 = f->U.Nf.cpool.at(i);
		if(gravity_value_equals(v, v2)) {
			gravity_value_free(NULL, v);
			return (uint16)i;
		}
	}
	// vm is required here because I cannot know in advance if v is already in the pool or not
	// and value object v must be added to the VM only once
	if(vm && v.IsObject()) 
		gravity_vm_transfer(vm, VALUE_AS_OBJECT(v));
	f->U.Nf.cpool.insert(v);
	return (uint16)f->U.Nf.cpool.getCount()-1;
}

GravityValue gravity_function_cpool_get(gravity_function_t * f, uint16 i) 
{
	assert(f->tag == EXEC_TYPE_NATIVE);
	return f->U.Nf.cpool.at(i);
}

gravity_list_t * gravity_function_params_get(gravity_vm * vm, gravity_function_t * f) 
{
	gravity_list_t * list = NULL;
	if(f->tag == EXEC_TYPE_NATIVE) {
		// written by user in Gravity
	}
	else if(f->tag == EXEC_TYPE_BRIDGED && f->xdata) {
		// ask bridge
	}
	else if(f->tag == EXEC_TYPE_INTERNAL) {
		// native C function
	}
	return list;
}

void gravity_function_setxdata(gravity_function_t * f, void * xdata) { f->xdata = xdata; }

static void gravity_function_array_serialize(gravity_function_t * f, GravityJson * json, gravity_value_r r) 
{
	assert(f->tag == EXEC_TYPE_NATIVE);
	const size_t n = r.getCount();
	for(size_t i = 0; i < n; i++) {
		GravityValue v = r.at(i);
		gravity_value_serialize(NULL, v, json);
	}
}

static void gravity_function_array_dump(gravity_function_t * f, gravity_value_r r) 
{
	assert(f->tag == EXEC_TYPE_NATIVE);
	size_t n = r.getCount();
	for(size_t i = 0; i<n; i++) {
		GravityValue v = r.at(i);
		if(v.IsNull()) {
			printf("%05zu\tNULL\n", i);
			continue;
		}
		if(v.IsUndefined()) {
			printf("%05zu\tUNDEFINED\n", i);
			continue;
		}
		if(v.IsBool()) {
			printf("%05zu\tBOOL: %d\n", i, (v.n == 0) ? 0 : 1);
			continue;
		}
		if(v.IsInt()) {
			printf("%05zu\tINT: %" PRId64 "\n", i, (int64_t)v.n);
			continue;
		}
		if(v.IsFloat()) {
			printf("%05zu\tFLOAT: %g\n", i, (double)v.f);
			continue;
		}
		if(v.IsFunction()) {
			gravity_function_t * vf = VALUE_AS_FUNCTION(v);
			printf("%05zu\tFUNC: %s\n", i, (vf->identifier) ? vf->identifier : "$anon");
			continue;
		}
		if(v.IsClass()) {
			gravity_class_t * c = static_cast<gravity_class_t *>(v);
			printf("%05zu\tCLASS: %s\n", i, (c->identifier) ? c->identifier : "$anon");
			continue;
		}
		if(v.IsString()) {
			printf("%05zu\tSTRING: %s\n", i, v.GetZString());
			continue;
		}
		if(v.IsList()) {
			gravity_list_t * value = static_cast<gravity_list_t *>(v);
			size_t count = value->array.getCount();
			printf("%05zu\tLIST: %zu items\n", i, count);
			continue;
		}
		if(v.IsMap()) {
			gravity_map_t * map = VALUE_AS_MAP(v);
			printf("%05zu\tMAP: %u items\n", i, gravity_hash_count(map->hash));
			continue;
		}
		assert(0); // should never reach this point
	}
}

static void gravity_function_bytecode_serialize(gravity_function_t * f, GravityJson * json) 
{
	if(!f->U.Nf.bytecode || !f->U.Nf.ninsts) {
		json_add_null(json, GRAVITY_JSON_LABELBYTECODE);
		return;
	}
	// bytecode
	uint32 ninst = f->U.Nf.ninsts;
	uint32 length = ninst * 2 * sizeof(uint32);
	uint8_t * hexchar = (uint8_t*)mem_alloc(NULL, sizeof(uint8_t) * length);
	for(uint32 k = 0, i = 0; i < ninst; ++i) {
		uint32 value = f->U.Nf.bytecode[i];
		for(int32 j = 2*sizeof(value)-1; j>=0; --j) {
			uint8_t c = "0123456789ABCDEF"[((value >> (j*4)) & 0xF)];
			hexchar[k++] = c;
		}
	}
	json_add_string(json, GRAVITY_JSON_LABELBYTECODE, PTRCHRC_(hexchar), length);
	mem_free(hexchar);
	// debug lineno
	if(!f->U.Nf.lineno) 
		return;
	ninst = f->U.Nf.ninsts;
	length = ninst * 2 * sizeof(uint32);
	hexchar = (uint8_t*)mem_alloc(NULL, sizeof(uint8_t) * length);
	for(uint32 k = 0, i = 0; i < ninst; ++i) {
		uint32 value = f->U.Nf.lineno[i];
		for(int32 j = 2*sizeof(value)-1; j>=0; --j) {
			uint8_t c = "0123456789ABCDEF"[((value >> (j*4)) & 0xF)];
			hexchar[k++] = c;
		}
	}
	json_add_string(json, GRAVITY_JSON_LABELLINENO, (const char*)hexchar, length);
	mem_free(hexchar);
}

uint32 * gravity_bytecode_deserialize(const char * buffer, size_t len, uint32 * n) 
{
	uint32 ninst = (uint32)len / 8;
	uint32 * bytecode = (uint32 *)mem_alloc(NULL, sizeof(uint32) * (ninst + 1)); // +1 to get a 0 terminated bytecode (0 is opcode RET0)
	for(uint32 j = 0; j<ninst; ++j) {
		uint32 v = 0;
		for(uint32 i = (j*8); i<=(j*8)+7; ++i) {
			// I was using a conversion code from
			// https://code.google.com/p/yara-project/source/browse/trunk/libyara/xtoi.c?r=150
			// but it caused issues under ARM processor so I decided to switch to an easier to read/maintain
			// code
			// http://codereview.stackexchange.com/questions/42976/hexadecimal-to-integer-conversion-function

			// no needs to have also the case:
			// if (c >= 'a' && c <= 'f') {
			//        c = c - 'a' + 10;
			// }
			// because bytecode is always uppercase
			uint32 c = buffer[i];
			if(c >= 'A' && c <= 'F') {
				c = c - 'A' + 10;
			}
			else if(c >= '0' && c <= '9') {
				c -= '0';
			}
			else 
				goto abort_conversion;
			v = v << 4 | c;
		}
		bytecode[j] = v;
	}
	*n = ninst;
	return bytecode;
abort_conversion:
	*n = 0;
	mem_free(bytecode);
	return NULL;
}

void gravity_function_dump(gravity_function_t * f, code_dump_function codef) 
{
	printf("Function: %s\n", (f->identifier) ? f->identifier : "$anon");
	printf("Params:%d Locals:%d Temp:%d Upvalues:%d Tag:%d xdata:%p\n", f->nparams, f->nlocals, f->ntemps, f->nupvalues, f->tag, f->xdata);
	if(f->tag == EXEC_TYPE_NATIVE) {
		if(f->U.Nf.cpool.getCount()) 
			printf("======= CONST POOL =======\n");
		gravity_function_array_dump(f, f->U.Nf.cpool);
		if(f->U.Nf.pname.getCount()) 
			printf("======= PARAM NAMES =======\n");
		gravity_function_array_dump(f, f->U.Nf.pname);
		if(f->U.Nf.pvalue.getCount()) 
			printf("======= PARAM VALUES =======\n");
		gravity_function_array_dump(f, f->U.Nf.pvalue);
		printf("======= BYTECODE =======\n");
		if((f->U.Nf.bytecode) && (codef)) 
			codef(f->U.Nf.bytecode);
	}
	printf("\n");
}

void gravity_function_special_serialize(gravity_function_t * f, const char * key, GravityJson * json) 
{
	const char * label = json_get_label(json, key);
	json_begin_object(json, label);
	json_add_cstring(json, GRAVITY_JSON_LABELTYPE, GRAVITY_JSON_FUNCTION); // MANDATORY 1st FIELD
	json_add_cstring(json, GRAVITY_JSON_LABELIDENTIFIER, key);            // MANDATORY 2nd FIELD
	json_add_int(json, GRAVITY_JSON_LABELTAG, f->tag);

	// common fields
	json_add_int(json, GRAVITY_JSON_LABELNPARAM, f->nparams);
	json_add_bool(json, GRAVITY_JSON_LABELARGS, f->U.Nf.useargs);
	json_add_int(json, GRAVITY_JSON_LABELINDEX, f->U.Sf.index);
	if(f->U.Sf.special[0]) {
		gravity_function_t * f2 = (gravity_function_t *)f->U.Sf.special[0];
		f2->identifier = GRAVITY_JSON_GETTER;
		gravity_function_serialize(f2, json);
		f2->identifier = NULL;
	}
	if(f->U.Sf.special[1]) {
		gravity_function_t * f2 = (gravity_function_t *)f->U.Sf.special[1];
		f2->identifier = GRAVITY_JSON_SETTER;
		gravity_function_serialize(f2, json);
		f2->identifier = NULL;
	}
	json_end_object(json);
}

void gravity_function_serialize(gravity_function_t * f, GravityJson * json) 
{
	// special functions need a special serialization
	if(f->tag == EXEC_TYPE_SPECIAL) {
		gravity_function_special_serialize(f, f->identifier, json);
	}
	else {
		// compute identifier (cannot be NULL)
		const char * identifier = f->identifier;
		char temp[256];
		if(!identifier) {
			snprintf(temp, sizeof(temp), "$anon_%p", f); 
			identifier = temp;
		}
		const char * label = json_get_label(json, identifier);
		json_begin_object(json, label);
		json_add_cstring(json, GRAVITY_JSON_LABELTYPE, GRAVITY_JSON_FUNCTION); // MANDATORY 1st FIELD
		json_add_cstring(json, GRAVITY_JSON_LABELIDENTIFIER, identifier);   // MANDATORY 2nd FIELD (not for getter/setter)
		json_add_int(json, GRAVITY_JSON_LABELTAG, f->tag);
		// common fields
		json_add_int(json, GRAVITY_JSON_LABELNPARAM, f->nparams);
		json_add_bool(json, GRAVITY_JSON_LABELARGS, f->U.Nf.useargs);
		if(f->tag == EXEC_TYPE_NATIVE) {
			// native only fields
			json_add_int(json, GRAVITY_JSON_LABELNLOCAL, f->nlocals);
			json_add_int(json, GRAVITY_JSON_LABELNTEMP, f->ntemps);
			json_add_int(json, GRAVITY_JSON_LABELNUPV, f->nupvalues);
			json_add_double(json, GRAVITY_JSON_LABELPURITY, f->U.Nf.purity);
			// bytecode
			gravity_function_bytecode_serialize(f, json);
			// constant pool
			json_begin_array(json, GRAVITY_JSON_LABELPOOL);
			gravity_function_array_serialize(f, json, f->U.Nf.cpool);
			json_end_array(json);
			// default values (if any)
			if(f->U.Nf.pvalue.getCount()) {
				json_begin_array(json, GRAVITY_JSON_LABELPVALUES);
				gravity_function_array_serialize(f, json, f->U.Nf.pvalue);
				json_end_array(json);
			}
			// arg names (if any)
			if(f->U.Nf.pname.getCount()) {
				json_begin_array(json, GRAVITY_JSON_LABELPNAMES);
				gravity_function_array_serialize(f, json, f->U.Nf.pname);
				json_end_array(json);
			}
		}
		json_end_object(json);
	}
}

gravity_function_t * gravity_function_deserialize(gravity_vm * vm, json_value * json) 
{
	gravity_function_t * f = gravity_function_new(vm, NULL, 0, 0, 0, NULL);
	DEBUG_DESERIALIZE("DESERIALIZE FUNCTION: %p\n", f);
	bool identifier_parsed = false;
	bool getter_parsed = false;
	bool setter_parsed = false;
	bool index_parsed = false;
	bool bytecode_parsed = false;
	bool cpool_parsed = false;
	bool nparams_parsed = false;
	bool nlocals_parsed = false;
	bool ntemp_parsed = false;
	bool nupvalues_parsed = false;
	bool nargs_parsed = false;
	bool tag_parsed = false;
	uint32 n = json->u.object.length;
	for(uint32 i = 1; i<n; ++i) { // from 1 to skip type
		const char * label = json->u.object.values[i].name;
		json_value * value = json->u.object.values[i].value;
		size_t label_size = strlen(label);
		// identifier
		if(string_casencmp(label, GRAVITY_JSON_LABELIDENTIFIER, label_size) == 0) {
			if(value->type != json_string) goto abort_load;
			if(identifier_parsed) goto abort_load;
			if(strncmp(value->u.string.ptr, "$anon", 5) != 0) {
				f->identifier = sstrdup(value->u.string.ptr);
				DEBUG_DESERIALIZE("IDENTIFIER: %s\n", value->u.string.ptr);
			}
			identifier_parsed = true;
			continue;
		}

		// tag
		if(string_casencmp(label, GRAVITY_JSON_LABELTAG, label_size) == 0) {
			if(value->type != json_integer) 
				goto abort_load;
			if(tag_parsed) 
				goto abort_load;
			f->tag = /*(uint16)*/static_cast<gravity_exec_type>(value->u.integer);
			tag_parsed = true;
			continue;
		}
		// index (only in special functions)
		if(string_casencmp(label, GRAVITY_JSON_LABELINDEX, label_size) == 0) {
			if(value->type != json_integer) 
				goto abort_load;
			if(f->tag != EXEC_TYPE_SPECIAL) 
				goto abort_load;
			if(index_parsed) 
				goto abort_load;
			f->U.Sf.index = (uint16)value->u.integer;
			index_parsed = true;
			continue;
		}
		// getter (only in special functions)
		if(string_casencmp(label, GRAVITY_JSON_GETTER, strlen(GRAVITY_JSON_GETTER)) == 0) {
			if(f->tag != EXEC_TYPE_SPECIAL) goto abort_load;
			if(getter_parsed) goto abort_load;
			gravity_function_t * getter = gravity_function_deserialize(vm, value);
			if(!getter) 
				goto abort_load;
			f->U.Sf.special[0] = gravity_closure_new(vm, getter);
			getter_parsed = true;
			continue;
		}

		// setter (only in special functions)
		if(string_casencmp(label, GRAVITY_JSON_SETTER, strlen(GRAVITY_JSON_SETTER)) == 0) {
			if(f->tag != EXEC_TYPE_SPECIAL) 
				goto abort_load;
			if(setter_parsed) 
				goto abort_load;
			gravity_function_t * setter = gravity_function_deserialize(vm, value);
			if(!setter) goto abort_load;
			f->U.Sf.special[1] = gravity_closure_new(vm, setter);
			setter_parsed = true;
			continue;
		}

		// nparams
		if(string_casencmp(label, GRAVITY_JSON_LABELNPARAM, label_size) == 0) {
			if(value->type != json_integer) goto abort_load;
			if(nparams_parsed) goto abort_load;
			f->nparams = (uint16)value->u.integer;
			nparams_parsed = true;
			continue;
		}

		// nlocals
		if(string_casencmp(label, GRAVITY_JSON_LABELNLOCAL, label_size) == 0) {
			if(value->type != json_integer) goto abort_load;
			if(nlocals_parsed) goto abort_load;
			f->nlocals = (uint16)value->u.integer;
			nlocals_parsed = true;
			continue;
		}

		// ntemps
		if(string_casencmp(label, GRAVITY_JSON_LABELNTEMP, label_size) == 0) {
			if(value->type != json_integer) goto abort_load;
			if(ntemp_parsed) goto abort_load;
			f->ntemps = (uint16)value->u.integer;
			ntemp_parsed = true;
			continue;
		}

		// nupvalues
		if(string_casencmp(label, GRAVITY_JSON_LABELNUPV, label_size) == 0) {
			if(value->type != json_integer) goto abort_load;
			if(nupvalues_parsed) goto abort_load;
			f->nupvalues = (uint16)value->u.integer;
			nupvalues_parsed = true;
			continue;
		}

		// args
		if(string_casencmp(label, GRAVITY_JSON_LABELARGS, label_size) == 0) {
			if(value->type != json_boolean) 
				goto abort_load;
			if(nargs_parsed) 
				goto abort_load;
			f->U.Nf.useargs = LOGIC(value->u.boolean);
			nargs_parsed = true;
			continue;
		}

		// bytecode
		if(string_casencmp(label, GRAVITY_JSON_LABELBYTECODE, label_size) == 0) {
			if(bytecode_parsed) goto abort_load;
			if(value->type == json_null) {
				// if function is empty then just one RET0 implicit bytecode instruction
				f->U.Nf.ninsts = 0;
				f->U.Nf.bytecode = (uint32 *)mem_alloc(NULL, sizeof(uint32) * (f->U.Nf.ninsts + 1));
			}
			else {
				if(value->type != json_string) 
					goto abort_load;
				if(f->tag != EXEC_TYPE_NATIVE) 
					goto abort_load;
				f->U.Nf.bytecode = gravity_bytecode_deserialize(value->u.string.ptr, value->u.string.length, &f->U.Nf.ninsts);
			}
			bytecode_parsed = true;
			continue;
		}

		// lineno debug info
		if(string_casencmp(label, GRAVITY_JSON_LABELLINENO, label_size) == 0) {
			if(value->type == json_string) 
				f->U.Nf.lineno = gravity_bytecode_deserialize(value->u.string.ptr, value->u.string.length, &f->U.Nf.ninsts);
		}

		// arguments names
		if(string_casencmp(label, GRAVITY_JSON_LABELPNAMES, label_size) == 0) {
			if(value->type != json_array) 
				goto abort_load;
			if(f->tag != EXEC_TYPE_NATIVE) 
				goto abort_load;
			uint32 m = value->u.array.length;
			for(uint32 j = 0; j<m; ++j) {
				json_value * r = value->u.array.values[j];
				if(r->type != json_string) 
					goto abort_load;
				f->U.Nf.pname.insert(VALUE_FROM_STRING(NULL, r->u.string.ptr, r->u.string.length));
			}
		}

		// arguments default values
		if(string_casencmp(label, GRAVITY_JSON_LABELPVALUES, label_size) == 0) {
			if(value->type != json_array) 
				goto abort_load;
			if(f->tag != EXEC_TYPE_NATIVE) 
				goto abort_load;
			uint32 m = value->u.array.length;
			for(uint32 j = 0; j<m; ++j) {
				json_value * r = value->u.array.values[j];
				switch(r->type) {
					case json_integer:
					    f->U.Nf.pvalue.insert(GravityValue::from_int((gravity_int_t)r->u.integer));
					    break;
					case json_double:
					    f->U.Nf.pvalue.insert(GravityValue::from_float((gravity_float_t)r->u.dbl));
					    break;
					case json_boolean:
					    f->U.Nf.pvalue.insert(GravityValue::from_bool(r->u.boolean));
					    break;
					case json_string:
					    f->U.Nf.pvalue.insert(VALUE_FROM_STRING(NULL, r->u.string.ptr, r->u.string.length));
					    break;
					case json_object:
					    f->U.Nf.pvalue.insert(GravityValue::from_undefined());
					    break;
					case json_null:
					    f->U.Nf.pvalue.insert(GravityValue::from_null());
					    break;
					case json_none:
					case json_array:
					    f->U.Nf.pvalue.insert(GravityValue::from_null());
					    break;
				}
			}
		}
		// cpool
		if(string_casencmp(label, GRAVITY_JSON_LABELPOOL, label_size) == 0) {
			if(value->type != json_array) 
				goto abort_load;
			if(f->tag != EXEC_TYPE_NATIVE) 
				goto abort_load;
			if(cpool_parsed) 
				goto abort_load;
			cpool_parsed = true;
			uint32 m = value->u.array.length;
			for(uint32 j = 0; j<m; ++j) {
				json_value * r = value->u.array.values[j];
				switch(r->type) {
					case json_integer:
					    gravity_function_cpool_add(NULL, f, GravityValue::from_int((gravity_int_t)r->u.integer));
					    break;
					case json_double:
					    gravity_function_cpool_add(NULL, f, GravityValue::from_float((gravity_float_t)r->u.dbl));
					    break;
					case json_boolean:
					    gravity_function_cpool_add(NULL, f, GravityValue::from_bool(r->u.boolean));
					    break;
					case json_string:
					    gravity_function_cpool_add(vm, f, VALUE_FROM_STRING(NULL, r->u.string.ptr, r->u.string.length));
					    break;
					case json_object: {
					    gravity_class_t * obj = gravity_object_deserialize(vm, r);
					    if(!obj) 
							goto abort_load;
					    gravity_function_cpool_add(NULL, f, GravityValue::from_object(obj));
					    break;
				    }

					case json_array: {
					    uint32 count = r->u.array.length;
					    gravity_list_t * list = gravity_list_new(NULL, count);
					    if(!list) 
							continue;
					    for(uint32 k = 0; k<count; ++k) {
						    json_value * jsonv = r->u.array.values[k];
						    GravityValue v;
						    // only literals allowed here
						    switch(jsonv->type) {
							    case json_integer: v = GravityValue::from_int((gravity_int_t)jsonv->u.integer); break;
							    case json_double: v = GravityValue::from_float((gravity_float_t)jsonv->u.dbl); break;
							    case json_boolean: v = GravityValue::from_bool(jsonv->u.boolean); break;
							    case json_string: v = VALUE_FROM_STRING(vm, jsonv->u.string.ptr, jsonv->u.string.length); break;
							    default: goto abort_load;
						    }
						    list->array.insert(v);
					    }
					    gravity_function_cpool_add(vm, f, GravityValue::from_object(reinterpret_cast<gravity_class_t *>(list)));
					    break;
				    }
					case json_none:
					case json_null:
					    gravity_function_cpool_add(NULL, f, GravityValue::from_null());
					    break;
				}
			}
		}
	}
	return f;
abort_load:
	// do not free f here because it is already garbage collected
	return NULL;
}

void gravity_function_free(gravity_vm * vm, gravity_function_t * f) 
{
	if(f) {
		DEBUG_FREE("FREE %s", gravity_object_debug((gravity_class_t *)f, true));
		// check if bridged data needs to be freed too
		if(f->xdata && vm) {
			gravity_delegate_t * p_delegate = gravity_vm_delegate(vm);
			if(p_delegate->bridge_free) 
				p_delegate->bridge_free(vm, (gravity_class_t *)f);
		}
		mem_free((void*)f->identifier);
		if(f->tag == EXEC_TYPE_NATIVE) {
			mem_free((void*)f->U.Nf.bytecode);
			mem_free((void*)f->U.Nf.lineno);
			// FREE EACH DEFAULT value
			size_t n = f->U.Nf.pvalue.getCount();
			for(size_t i = 0; i<n; i++) {
				//GravityValue v = f->U.Nf.pvalue.at(i);
				gravity_value_free(NULL, /*v*/f->U.Nf.pvalue.at(i));
			}
			f->U.Nf.pvalue.Z();
			// FREE EACH PARAM name
			n = f->U.Nf.pname.getCount();
			for(size_t i = 0; i<n; i++) {
				//GravityValue v = f->U.Nf.pname.at(i);
				gravity_value_free(NULL, /*v*/f->U.Nf.pname.at(i));
			}
			f->U.Nf.pname.Z();
			// DO NOT FREE EACH INDIVIDUAL CPOOL ITEM HERE
			f->U.Nf.cpool.Z();
		}
		delete f;//mem_free((void*)f);
	}
}

uint32 gravity_function_size(gravity_vm * vm, gravity_function_t * f)
{
	SET_OBJECT_VISITED_FLAG(f, true);
	uint32 func_size = sizeof(gravity_function_t) + sstrlen(f->identifier);
	if(f->tag == EXEC_TYPE_NATIVE) {
		if(f->U.Nf.bytecode) 
			func_size += f->U.Nf.ninsts * sizeof(uint32);
		// cpool size
		size_t n = f->U.Nf.cpool.getCount();
		for(size_t i = 0; i<n; i++) {
			GravityValue v = f->U.Nf.cpool.at(i);
			func_size += gravity_value_size(vm, v);
		}
	}
	else if(f->tag == EXEC_TYPE_SPECIAL) {
		if(f->U.Sf.special[0]) 
			func_size += gravity_closure_size(vm, (gravity_closure_t *)f->U.Sf.special[0]);
		if((f->U.Sf.special[1]) && (f->U.Sf.special[0] != f->U.Sf.special[1])) 
			func_size += gravity_closure_size(vm, (gravity_closure_t *)f->U.Sf.special[1]);
	}
	else if(f->tag == EXEC_TYPE_BRIDGED) {
		gravity_delegate_t * delegate = gravity_vm_delegate(vm);
		if(f->xdata && delegate->bridge_size)
			func_size += delegate->bridge_size(vm, reinterpret_cast<gravity_class_t *>(f->xdata));
	}
	SET_OBJECT_VISITED_FLAG(f, false);
	return func_size;
}

void gravity_function_blacken(gravity_vm * vm, gravity_function_t * f) 
{
	gravity_vm_memupdate(vm, gravity_function_size(vm, f));
	if(f->tag == EXEC_TYPE_SPECIAL) {
		if(f->U.Sf.special[0]) 
			gravity_gray_object(vm, (gravity_class_t *)f->U.Sf.special[0]);
		if(f->U.Sf.special[1]) 
			gravity_gray_object(vm, (gravity_class_t *)f->U.Sf.special[1]);
	}
	if(f->tag == EXEC_TYPE_NATIVE) {
		// constant pool
		uint n = f->U.Nf.cpool.getCount();
		for(uint i = 0; i < n; i++) {
			GravityValue v = f->U.Nf.cpool.at(i);
			gravity_gray_value(vm, v);
		}
	}
}

// MARK: -
gravity_closure_t * FASTCALL gravity_closure_new(gravity_vm * vm, gravity_function_t * f) 
{
	gravity_closure_t * closure = new gravity_closure_t; // (gravity_closure_t *)mem_alloc(NULL, sizeof(gravity_closure_t));
	assert(closure);
	//closure->isa = GravityEnv.P_ClsClosure;
	closure->f = f;
	// allocate upvalue array (+1 so I can simplify the iterator without the needs to access closure->f->nupvalues)
	uint16 nupvalues = (f) ? f->nupvalues : 0;
	closure->upvalue = (nupvalues) ? (gravity_upvalue_t**)mem_alloc(NULL, sizeof(gravity_upvalue_t*) * (f->nupvalues + 1)) : NULL;
	gravity_vm_transfer(vm, (gravity_class_t *)closure);
	return closure;
}

void FASTCALL gravity_closure_free(gravity_vm * vm, gravity_closure_t * closure) 
{
	if(closure && closure->refcount <= 0) {
		DEBUG_FREE("FREE %s", gravity_object_debug((gravity_class_t *)closure, true));
		mem_free(closure->upvalue);
		delete closure; // mem_free(closure);
	}
}

uint32 gravity_closure_size(gravity_vm * vm, gravity_closure_t * closure) 
{
	SET_OBJECT_VISITED_FLAG(closure, true);
	uint32 closure_size = sizeof(gravity_closure_t);
	gravity_upvalue_t ** upvalue = closure->upvalue;
	while(upvalue && upvalue[0]) {
		closure_size += sizeof(gravity_upvalue_t*);
		++upvalue;
	}
	SET_OBJECT_VISITED_FLAG(closure, false);
	return closure_size;
}

void gravity_closure_inc_refcount(gravity_vm * vm, gravity_closure_t * closure) 
{
	if(closure->refcount == 0) 
		gravity_gc_temppush(vm, (gravity_class_t *)closure);
	++closure->refcount;
}

void gravity_closure_dec_refcount(gravity_vm * vm, gravity_closure_t * closure) 
{
	if(closure->refcount == 1) 
		gravity_gc_tempnull(vm, (gravity_class_t *)closure);
	if(closure->refcount >= 1) 
		--closure->refcount;
}

void gravity_closure_blacken(gravity_vm * vm, gravity_closure_t * closure) 
{
	gravity_vm_memupdate(vm, gravity_closure_size(vm, closure));
	// mark function
	gravity_gray_object(vm, (gravity_class_t *)closure->f);
	// mark each upvalue
	gravity_upvalue_t ** upvalue = closure->upvalue;
	while(upvalue && upvalue[0]) {
		gravity_gray_object(vm, (gravity_class_t *)upvalue[0]);
		++upvalue;
	}
	// mark context (if any)
	if(closure->context) 
		gravity_gray_object(vm, closure->context);
}

// MARK: -

gravity_upvalue_t * gravity_upvalue_new(gravity_vm * vm, GravityValue * value) 
{
	gravity_upvalue_t * upvalue = new gravity_upvalue_t(value);
	//upvalue->isa = GravityEnv.P_ClsUpValue;
	//upvalue->value = value;
	//upvalue->closed = GravityValue::from_null();
	//upvalue->next = NULL;
	gravity_vm_transfer(vm, (gravity_class_t *)upvalue);
	return upvalue;
}

void gravity_upvalue_free(gravity_vm * vm, gravity_upvalue_t * upvalue) 
{
	if(upvalue) {
		DEBUG_FREE("FREE %s", gravity_object_debug((gravity_class_t *)upvalue, true));
		delete upvalue; //mem_free(upvalue);
	}
}

uint32 gravity_upvalue_size(gravity_vm * vm, gravity_upvalue_t * upvalue) 
{
	SET_OBJECT_VISITED_FLAG(upvalue, true);
	uint32 upvalue_size = sizeof(gravity_upvalue_t);
	SET_OBJECT_VISITED_FLAG(upvalue, false);
	return upvalue_size;
}

void gravity_upvalue_blacken(gravity_vm * vm, gravity_upvalue_t * upvalue) 
{
	gravity_vm_memupdate(vm, gravity_upvalue_size(vm, upvalue));
	// gray both closed and still opened values
	gravity_gray_value(vm, *upvalue->value);
	gravity_gray_value(vm, upvalue->closed);
}

// MARK: -

gravity_fiber_t * gravity_fiber_new(gravity_vm * vm, gravity_closure_t * closure, uint32 nstack, uint32 nframes) 
{
	//gravity_fiber_t * fiber = (gravity_fiber_t *)mem_alloc(NULL, sizeof(gravity_fiber_t));
	gravity_fiber_t * fiber = new gravity_fiber_t;
	assert(fiber);
	//fiber->isa = GravityEnv.P_ClsFiber;
	//fiber->caller = NULL;
	//fiber->result = GravityValue::from_null();
	if(nstack < DEFAULT_MINSTACK_SIZE) 
		nstack = DEFAULT_MINSTACK_SIZE;
	fiber->stack = static_cast<GravityValue *>(mem_alloc(NULL, sizeof(GravityValue) * nstack));
	fiber->stacktop = fiber->stack;
	fiber->stackalloc = nstack;
	if(nframes < DEFAULT_MINCFRAME_SIZE) 
		nframes = DEFAULT_MINCFRAME_SIZE;
	fiber->frames = (gravity_callframe_t*)mem_alloc(NULL, sizeof(gravity_callframe_t) * nframes);
	fiber->framesalloc = nframes;
	fiber->nframes = 1;
	//fiber->upvalues = NULL;
	gravity_callframe_t * frame = &fiber->frames[0];
	if(closure) {
		frame->closure = closure;
		frame->ip = (closure->f->tag == EXEC_TYPE_NATIVE) ? closure->f->U.Nf.bytecode : NULL;
	}
	frame->dest = 0;
	frame->stackstart = fiber->stack;
	// replace self with fiber instance
	frame->stackstart[0] = GravityValue::from_object(reinterpret_cast<gravity_class_t *>(fiber));
	gravity_vm_transfer(vm, (gravity_class_t *)fiber);
	return fiber;
}

void gravity_fiber_free(gravity_vm * vm, gravity_fiber_t * fiber) 
{
	if(fiber) {
		DEBUG_FREE("FREE %s", gravity_object_debug((gravity_class_t *)fiber, true));
		delete fiber;
		//mem_free(fiber->error);
		//mem_free(fiber->stack);
		//mem_free(fiber->frames);
		//mem_free(fiber);
	}
}

void gravity_fiber_reassign(gravity_fiber_t * fiber, gravity_closure_t * closure, uint16 nargs) 
{
	gravity_callframe_t * frame = &fiber->frames[0];
	frame->closure = closure;
	frame->ip = (closure->f->tag == EXEC_TYPE_NATIVE) ? closure->f->U.Nf.bytecode : NULL;
	frame->dest = 0;
	frame->stackstart = fiber->stack;
	fiber->nframes = 1;
	fiber->upvalues = NULL;
	// update stacktop in order to be GC friendly
	fiber->stacktop += FN_COUNTREG(closure->f, nargs);
}

/*void gravity_fiber_reset(gravity_fiber_t * fiber) 
{
	fiber->caller = NULL;
	fiber->result = GravityValue::from_null();
	fiber->nframes = 0;
	fiber->upvalues = NULL;
	fiber->stacktop = fiber->stack;
}*/

void FASTCALL gravity_fiber_seterror(gravity_fiber_t * fiber, const char * error) 
{
	mem_free(fiber->error);
	fiber->error = (char *)sstrdup(error);
}

uint32 gravity_fiber_size(gravity_vm * vm, gravity_fiber_t * fiber) 
{
	SET_OBJECT_VISITED_FLAG(fiber, true);
	// internal size
	uint32 fiber_size = sizeof(gravity_fiber_t);
	fiber_size += fiber->stackalloc * sizeof(GravityValue);
	fiber_size += fiber->framesalloc * sizeof(gravity_callframe_t);
	// stack size
	for(GravityValue* slot = fiber->stack; slot < fiber->stacktop; ++slot) {
		fiber_size += gravity_value_size(vm, *slot);
	}
	fiber_size += sstrlen(fiber->error);
	fiber_size += gravity_object_size(vm, (gravity_class_t *)fiber->caller);
	SET_OBJECT_VISITED_FLAG(fiber, false);
	return fiber_size;
}

void gravity_fiber_blacken(gravity_vm * vm, gravity_fiber_t * fiber) 
{
	gravity_vm_memupdate(vm, gravity_fiber_size(vm, fiber));
	// gray call frame functions
	for(uint32 i = 0; i < fiber->nframes; ++i) {
		gravity_gray_object(vm, (gravity_class_t *)fiber->frames[i].closure);
		gravity_gray_object(vm, (gravity_class_t *)fiber->frames[i].args);
	}
	// gray stack variables
	for(GravityValue * slot = fiber->stack; slot < fiber->stacktop; ++slot) {
		gravity_gray_value(vm, *slot);
	}
	// gray upvalues
	gravity_upvalue_t * upvalue = fiber->upvalues;
	while(upvalue) {
		gravity_gray_object(vm, (gravity_class_t *)upvalue);
		upvalue = upvalue->next;
	}
	gravity_gray_object(vm, (gravity_class_t *)fiber->caller);
}

// MARK: -

void gravity_object_serialize(gravity_class_t * obj, GravityJson * json) 
{
	if(obj->isa == GravityEnv.P_ClsFunc)
		gravity_function_serialize((gravity_function_t *)obj, json);
	else if(obj->isa == GravityEnv.P_ClsClass)
		gravity_class_serialize(obj, json);
	else assert(0);
}

gravity_class_t * gravity_object_deserialize(gravity_vm * vm, json_value * entry) 
{
	// this function is able to deserialize ONLY objects with a type label
	// sanity check
	if(entry->type != json_object) return NULL;
	if(entry->u.object.length == 0) return NULL;
	// the first entry value must specify gravity object type
	const char * label = entry->u.object.values[0].name;
	json_value * value = entry->u.object.values[0].value;
	if(string_casencmp(label, GRAVITY_JSON_LABELTYPE, 4) != 0) {
		// if no label type found then assume it is a map object
		gravity_map_t * m = gravity_map_deserialize(vm, entry);
		return (gravity_class_t *)m;
	}
	// sanity check
	if(value->type != json_string) 
		return NULL;
	// FUNCTION case
	if(string_casencmp(value->u.string.ptr, GRAVITY_JSON_FUNCTION, value->u.string.length) == 0) {
		gravity_function_t * f = gravity_function_deserialize(vm, entry);
		return (gravity_class_t *)f;
	}
	// CLASS case
	if(string_casencmp(value->u.string.ptr, GRAVITY_JSON_CLASS, value->u.string.length) == 0) {
		gravity_class_t * c = gravity_class_deserialize(vm, entry);
		return c;
	}
	// MAP/ENUM case
	if((string_casencmp(value->u.string.ptr, GRAVITY_JSON_MAP, value->u.string.length) == 0) ||
	    (string_casencmp(value->u.string.ptr, GRAVITY_JSON_ENUM, value->u.string.length) == 0)) {
		gravity_map_t * m = gravity_map_deserialize(vm, entry);
		return (gravity_class_t *)m;
	}
	// RANGE case
	if(string_casencmp(value->u.string.ptr, GRAVITY_JSON_RANGE, value->u.string.length) == 0) {
		gravity_range_t * range = gravity_range_deserialize(vm, entry);
		return (gravity_class_t *)range;
	}
	// unhandled case
	DEBUG_DESERIALIZE("gravity_object_deserialize unknown type");
	return NULL;
}

#undef REPORT_JSON_ERROR

const char * gravity_object_debug(gravity_class_t * obj, bool is_free) 
{
	if(!obj || !*obj) 
		return "";
	if(obj->IsInt()) 
		return "INT";
	if(obj->IsFloat()) 
		return "FLOAT";
	if(obj->IsBool()) 
		return "BOOL";
	if(obj->IsNullClass()) 
		return "NULL";
	static char buffer[512];
	if(obj->IsFunction()) {
		const char * name = ((gravity_function_t *)obj)->identifier;
		SETIFZ(name, "ANONYMOUS");
		snprintf(buffer, sizeof(buffer), "FUNCTION %p %s", obj, name);
		return buffer;
	}
	if(obj->IsClosure()) {
		// cannot guarantee ptr validity during a free
		const char * name = (is_free) ? NULL : ((gravity_closure_t *)obj)->f->identifier;
		SETIFZ(name, "ANONYMOUS");
		snprintf(buffer, sizeof(buffer), "CLOSURE %p %s", obj, name);
		return buffer;
	}
	if(obj->IsClass()) {
		const char * name = obj->identifier;
		SETIFZ(name, "ANONYMOUS");
		snprintf(buffer, sizeof(buffer), "CLASS %p %s", obj, name);
		return buffer;
	}
	if(obj->IsString()) {
		snprintf(buffer, sizeof(buffer), "STRING %p %s", obj, reinterpret_cast<const gravity_string_t *>(obj)->cptr());
		return buffer;
	}
	if(obj->IsInstance()) {
		// cannot guarantee ptr validity during a free
		gravity_class_t * c = (is_free) ? NULL : ((gravity_instance_t*)obj)->objclass;
		const char * name = (c && c->identifier) ? c->identifier : "ANONYMOUS";
		snprintf(buffer, sizeof(buffer), "INSTANCE %p OF %s", obj, name);
		return buffer;
	}
	if(obj->IsRange()) {
		snprintf(buffer, sizeof(buffer), "RANGE %p %ld %ld", obj, (long)((gravity_range_t*)obj)->from, (long)((gravity_range_t*)obj)->to);
		return buffer;
	}
	if(obj->IsList()) {
		snprintf(buffer, sizeof(buffer), "LIST %p (%ld items)", obj, (long)((gravity_list_t *)obj)->array.getCount());
		return buffer;
	}
	if(obj->IsMap()) {
		snprintf(buffer, sizeof(buffer), "MAP %p (%ld items)", obj, (long)gravity_hash_count(((gravity_map_t*)obj)->hash));
		return buffer;
	}
	if(obj->IsFiber()) {
		snprintf(buffer, sizeof(buffer), "FIBER %p", obj);
		return buffer;
	}
	if(obj->IsUpValue()) {
		snprintf(buffer, sizeof(buffer), "UPVALUE %p", obj);
		return buffer;
	}
	return "N/A";
}

void gravity_object_free(gravity_vm * vm, gravity_class_t * obj) 
{
	if(obj && !!(*obj)) {
		if(obj->IsClass()) 
			gravity_class_free(vm, obj);
		else if(obj->IsFunction()) 
			gravity_function_free(vm, (gravity_function_t *)obj);
		else if(obj->IsClosure()) 
			gravity_closure_free(vm, (gravity_closure_t *)obj);
		else if(obj->IsInstance()) 
			gravity_instance_free(vm, (gravity_instance_t*)obj);
		else if(obj->IsList()) 
			gravity_list_free(vm, (gravity_list_t *)obj);
		else if(obj->IsMap()) 
			gravity_map_free(vm, (gravity_map_t *)obj);
		else if(obj->IsFiber()) 
			gravity_fiber_free(vm, (gravity_fiber_t *)obj);
		else if(obj->IsRange()) 
			gravity_range_free(vm, (gravity_range_t *)obj);
		else if(obj->IsModule()) 
			gravity_module_free(vm, (gravity_module_t *)obj);
		else if(obj->IsString()) 
			gravity_string_free(vm, (gravity_string_t *)obj);
		else if(obj->IsUpValue()) 
			gravity_upvalue_free(vm, (gravity_upvalue_t *)obj);
		else 
			assert(0); // should never reach this point
	}
}

uint32 gravity_object_size(gravity_vm * vm, gravity_class_t * obj) 
{
	if(obj && !!(*obj)) {
		if(!(obj->Flags & GravityObjectBase::fGcVisited)/*!obj->GcVisited*/) { // check if object has already been visited (to avoid infinite loop)
			if(obj->IsClass()) 
				return gravity_class_size(vm, obj);
			else if(obj->IsFunction()) 
				return gravity_function_size(vm, (gravity_function_t *)obj);
			else if(obj->IsClosure()) 
				return gravity_closure_size(vm, (gravity_closure_t *)obj);
			else if(obj->IsInstance()) 
				return gravity_instance_size(vm, (gravity_instance_t *)obj);
			else if(obj->IsList()) 
				return gravity_list_size(vm, (gravity_list_t *)obj);
			else if(obj->IsMap()) 
				return gravity_map_size(vm, (gravity_map_t *)obj);
			else if(obj->IsFiber()) 
				return gravity_fiber_size(vm, (gravity_fiber_t *)obj);
			else if(obj->IsRange()) 
				return gravity_range_size(vm, (gravity_range_t *)obj);
			else if(obj->IsModule()) 
				return gravity_module_size(vm, (gravity_module_t *)obj);
			else if(obj->IsString()) 
				return gravity_string_size(vm, (gravity_string_t *)obj);
			else if(obj->IsUpValue()) 
				return gravity_upvalue_size(vm, (gravity_upvalue_t *)obj);
		}
	}
	return 0;
}

void gravity_object_blacken(gravity_vm * vm, gravity_class_t * obj) 
{
	if(obj && !!*obj) {
		if(obj->IsClass()) 
			gravity_class_blacken(vm, obj);
		else if(obj->IsFunction()) 
			gravity_function_blacken(vm, (gravity_function_t *)obj);
		else if(obj->IsClosure()) 
			gravity_closure_blacken(vm, (gravity_closure_t *)obj);
		else if(obj->IsInstance()) 
			gravity_instance_blacken(vm, (gravity_instance_t *)obj);
		else if(obj->IsList()) 
			gravity_list_blacken(vm, reinterpret_cast<gravity_list_t *>(obj));
		else if(obj->IsMap()) 
			gravity_map_blacken(vm, (gravity_map_t *)obj);
		else if(obj->IsFiber()) 
			gravity_fiber_blacken(vm, (gravity_fiber_t *)obj);
		else if(obj->IsRange()) 
			gravity_range_blacken(vm, (gravity_range_t *)obj);
		else if(obj->IsModule()) 
			gravity_module_blacken(vm, (gravity_module_t *)obj);
		else if(obj->IsString()) 
			gravity_string_blacken(vm, (gravity_string_t *)obj);
		else if(obj->IsUpValue()) 
			gravity_upvalue_blacken(vm, (gravity_upvalue_t *)obj);
		//else assert(0); // should never reach this point
	}
}

// MARK: -

gravity_instance_t * gravity_instance_new(gravity_vm * vm, gravity_class_t * c) 
{
	gravity_instance_t * instance = (gravity_instance_t*)mem_alloc(NULL, sizeof(gravity_instance_t));
	instance->isa = GravityEnv.P_ClsInstance;
	instance->objclass = c;
	if(c->nivars) 
		instance->ivars = static_cast<GravityValue *>(mem_alloc(NULL, c->nivars * sizeof(GravityValue)));
	for(uint32 i = 0; i<c->nivars; ++i) 
		instance->ivars[i] = GravityValue::from_null();
	gravity_vm_transfer(vm, (gravity_class_t *)instance);
	return instance;
}

gravity_instance_t * gravity_instance_clone(gravity_vm * vm, gravity_instance_t * src_instance) 
{
	gravity_class_t * c = src_instance->objclass;
	gravity_instance_t * instance = (gravity_instance_t*)mem_alloc(NULL, sizeof(gravity_instance_t));
	instance->isa = GravityEnv.P_ClsInstance;
	instance->objclass = c;
	// if c is an anonymous class then it must be deeply copied
	if(gravity_class_is_anon(c)) {
		// clone class c and all its closures
	}
	gravity_delegate_t * delegate = gravity_vm_delegate(vm);
	instance->xdata = (src_instance->xdata && delegate->bridge_clone) ? delegate->bridge_clone(vm, src_instance->xdata) : NULL;
	if(c->nivars) 
		instance->ivars = static_cast<GravityValue *>(mem_alloc(NULL, c->nivars * sizeof(GravityValue)));
	for(uint32 i = 0; i<c->nivars; ++i) 
		instance->ivars[i] = src_instance->ivars[i];
	gravity_vm_transfer(vm, (gravity_class_t *)instance);
	return instance;
}

void gravity_instance_setivar(gravity_instance_t * instance, uint32 idx, GravityValue value) 
{
	if(idx < instance->objclass->nivars) 
		instance->ivars[idx] = value;
}

void gravity_instance_setxdata(gravity_instance_t * i, void * xdata) 
{
	i->xdata = xdata;
}

void gravity_instance_free(gravity_vm * vm, gravity_instance_t * i) 
{
	DEBUG_FREE("FREE %s", gravity_object_debug((gravity_class_t *)i, true));
	// check if bridged data needs to be freed too
	if(i->xdata && vm) {
		gravity_delegate_t * delegate = gravity_vm_delegate(vm);
		if(delegate->bridge_free) 
			delegate->bridge_free(vm, (gravity_class_t *)i);
	}
	mem_free(i->ivars);
	mem_free((void*)i);
}

gravity_closure_t * gravity_instance_lookup_event(gravity_instance_t * i, const char * name) 
{
	// TODO: implemented as gravity_class_lookup but should be the exact opposite
	STATICVALUE_FROM_STRING(key, name, strlen(name));
	gravity_class_t * c = i->objclass;
	while(c) {
		GravityValue * v = gravity_hash_lookup(c->htable, key);
		// NOTE: there could be events (like InitContainer) which are empty (bytecode NULL) should I handle them
		// here?
		if(v && v->Ptr->IsClosure()) 
			return reinterpret_cast<gravity_closure_t *>(v->Ptr);
		c = c->superclass;
	}
	return NULL;
}

uint32 gravity_instance_size(gravity_vm * vm, gravity_instance_t * i) 
{
	SET_OBJECT_VISITED_FLAG(i, true);
	uint32 instance_size = sizeof(gravity_instance_t) + (i->objclass->nivars * sizeof(GravityValue));
	gravity_delegate_t * delegate = gravity_vm_delegate(vm);
	if(i->xdata && delegate->bridge_size)
		instance_size += delegate->bridge_size(vm, reinterpret_cast<gravity_class_t *>(i->xdata));
	SET_OBJECT_VISITED_FLAG(i, false);
	return instance_size;
}

void gravity_instance_blacken(gravity_vm * vm, gravity_instance_t * i) 
{
	gravity_vm_memupdate(vm, gravity_instance_size(vm, i));
	// instance class
	gravity_gray_object(vm, i->objclass);
	// ivars
	for(uint32 j = 0; j<i->objclass->nivars; ++j) {
		gravity_gray_value(vm, i->ivars[j]);
	}
	// xdata
	if(i->xdata) {
		gravity_delegate_t * delegate = gravity_vm_delegate(vm);
		if(delegate->bridge_blacken) 
			delegate->bridge_blacken(vm, i->xdata);
	}
}

void gravity_instance_serialize(gravity_instance_t * instance, GravityJson * json) 
{
	gravity_class_t * c = instance->objclass;
	const char * label = json_get_label(json, NULL);
	json_begin_object(json, label);
	json_add_cstring(json, GRAVITY_JSON_LABELTYPE, GRAVITY_JSON_INSTANCE);
	json_add_cstring(json, GRAVITY_JSON_CLASS, c->identifier);
	if(c->nivars) {
		json_begin_array(json, GRAVITY_JSON_LABELIVAR);
		for(uint32 i = 0; i<c->nivars; ++i) {
			gravity_value_serialize(NULL, instance->ivars[i], json);
		}
		json_end_array(json);
	}

	json_end_object(json);
}

bool FASTCALL gravity_instance_isstruct(const gravity_instance_t * i) { return LOGIC(i->objclass->Flags & GravityObjectBase::fIsStruct)/*i->objclass->is_struct*/; }

// MARK: -
static bool hash_value_compare_cb(GravityValue v1, GravityValue v2, void * data) 
{
	return gravity_value_equals(v1, v2);
}

bool gravity_value_vm_equals(gravity_vm * vm, GravityValue v1, GravityValue v2) 
{
	bool result = gravity_value_equals(v1, v2);
	if(result || !vm) 
		return result;
	// sanity check
	if(!(v1.IsInstance() && v2.IsInstance())) 
		return false;
	// if here means that they are two heap allocated objects
	gravity_instance_t * obj1 = (gravity_instance_t*)VALUE_AS_OBJECT(v1);
	gravity_instance_t * obj2 = (gravity_instance_t*)VALUE_AS_OBJECT(v2);
	gravity_delegate_t * delegate = gravity_vm_delegate(vm);
	if(obj1->xdata && obj2->xdata && delegate->bridge_equals) {
		return delegate->bridge_equals(vm, obj1->xdata, obj2->xdata);
	}
	return false;
}

bool gravity_value_equals(GravityValue v1, GravityValue v2) 
{
	// check same class
	if(v1.isa != v2.isa) 
		return false;
	// check same value for value types
	if((v1.isa == GravityEnv.P_ClsInt) || (v1.isa == GravityEnv.P_ClsBool) || (v1.isa == GravityEnv.P_ClsNull)) {
		return (v1.n == v2.n);
	}
	else if(v1.isa == GravityEnv.P_ClsFloat) {
	#if GRAVITY_ENABLE_DOUBLE
		return (fabs(v1.f - v2.f) < EPSILON);
	#else
		return (fabsf(v1.f - v2.f) < EPSILON);
	#endif
	}
	else if(v1.isa == GravityEnv.P_ClsString) {
		const gravity_string_t * s1 = static_cast<gravity_string_t *>(v1);
		const gravity_string_t * s2 = static_cast<gravity_string_t *>(v2);
		if(s1->hash != s2->hash) 
			return false;
		else if(s1->len != s2->len) 
			return false;
		else {
			// same hash and same len so let's compare bytes
			return (memcmp(s1->cptr(), s2->cptr(), s1->len) == 0);
		}
	}
	else if(v1.isa == GravityEnv.P_ClsRange) {
		const gravity_range_t * r1 = VALUE_AS_RANGE(v1);
		const gravity_range_t * r2 = VALUE_AS_RANGE(v2);
		return ((r1->from == r2->from) && (r1->to == r2->to));
	}
	else if(v1.isa == GravityEnv.P_ClsList) {
		const gravity_list_t * list1 = static_cast<gravity_list_t *>(v1);
		const gravity_list_t * list2 = static_cast<gravity_list_t *>(v2);
		if(list1->array.getCount() != list2->array.getCount()) 
			return false;
		size_t count = list1->array.getCount();
		for(size_t i = 0; i<count; ++i) {
			GravityValue value1 = list1->array.at(i);
			GravityValue value2 = list2->array.at(i);
			if(!gravity_value_equals(value1, value2)) 
				return false;
		}
		return true;
	}
	else if(v1.isa == GravityEnv.P_ClsMap) {
		gravity_map_t * map1 = VALUE_AS_MAP(v1);
		gravity_map_t * map2 = VALUE_AS_MAP(v2);
		return gravity_hash_compare(map1->hash, map2->hash, hash_value_compare_cb, NULL);
	}
	// if here means that they are two heap allocated objects
	const gravity_class_t * obj1 = VALUE_AS_OBJECT(v1);
	const gravity_class_t * obj2 = VALUE_AS_OBJECT(v2);
	return (obj1->isa != obj2->isa) ? false : (obj1 == obj2);
}

uint32 gravity_value_hash(GravityValue value) 
{
	if(value.isa == GravityEnv.P_ClsString)
		return static_cast<gravity_string_t *>(value)->hash;
	else if(oneof3(value.isa, GravityEnv.P_ClsInt, GravityEnv.P_ClsBool, GravityEnv.P_ClsNull))
		return gravity_hash_compute_int(value.n);
	else if(value.isa == GravityEnv.P_ClsFloat)
		return gravity_hash_compute_float(value.f);
	else
		return gravity_hash_compute_buffer(reinterpret_cast<const char *>(value.Ptr), sizeof(gravity_class_t *));
}

/* @sobolev replaced with GetClass()
//inline
gravity_class_t * gravity_value_getclass(GravityValue v) 
{
	if((v.isa == GravityEnv.P_ClsClass) && (v.p->objclass == GravityEnv.P_ClsObj)) return (gravity_class_t *)v.p;
	if((v.isa == GravityEnv.P_ClsInstance) || (v.isa == GravityEnv.P_ClsClass)) return (v.p) ? v.p->objclass : NULL;
	return v.isa;
}*/

/*inline*/gravity_class_t * gravity_value_getsuper(GravityValue v) 
{
	gravity_class_t * c = v.GetClass();
	return (c && c->superclass) ? c->superclass : NULL;
}

void FASTCALL gravity_value_free(gravity_vm * vm, GravityValue & rV)
{
	if(rV.IsObject())
		gravity_object_free(vm, VALUE_AS_OBJECT(rV));
}

static void gravity_map_serialize_iterator(gravity_hash_t * hashtable, GravityValue key, GravityValue v, void * data) 
{
	assert(key.isa == GravityEnv.P_ClsString);
	GravityJson * json = static_cast<GravityJson *>(data);
	const char * key_value = static_cast<gravity_string_t *>(key)->cptr();
	gravity_value_serialize(key_value, v, json);
}

void gravity_value_serialize(const char * key, GravityValue v, GravityJson * json) 
{
	if(v.IsNull()) { // NULL
		json_add_null(json, key);
		return;
	}
	if(v.IsUndefined()) { // UNDEFINED (convention used to represent an UNDEFINED value)
		if(json_option_isset(json, json_opt_no_undef)) {
			json_add_null(json, key);
		}
		else {
			json_begin_object(json, key);
			json_end_object(json);
		}
		return;
	}
	if(v.IsBool()) { // BOOL
		json_add_bool(json, key, (v.n == 0) ? false : true);
		return;
	}
	if(v.IsInt()) { // INT
		json_add_int(json, key, (int64_t)v.n);
		return;
	}
	if(v.IsFloat()) { // FLOAT
		json_add_double(json, key, (double)v.f);
		return;
	}
	if(v.IsFunction()) { // FUNCTION
		json_set_label(json, key);
		gravity_function_serialize(VALUE_AS_FUNCTION(v), json);
		return;
	}
	if(v.IsClosure()) { // CLOSURE
		json_set_label(json, key);
		gravity_function_serialize(VALUE_AS_CLOSURE(v)->f, json);
		return;
	}
	if(v.IsClass()) { // CLASS
		json_set_label(json, key);
		gravity_class_serialize(static_cast<gravity_class_t *>(v), json);
		return;
	}
	if(v.IsString()) { // STRING
		gravity_string_t * value = static_cast<gravity_string_t *>(v);
		json_add_string(json, key, value->cptr(), value->len);
		return;
	}
	if(v.IsList()) { // LIST (ARRAY)
		gravity_list_t * value = static_cast<gravity_list_t *>(v);
		json_begin_array(json, key);
		size_t count = value->array.getCount();
		for(size_t j = 0; j<count; j++) {
			GravityValue item = value->array.at(j);
			// here I am sure that value is a literal value
			gravity_value_serialize(NULL, item, json);
		}
		json_end_array(json);
		return;
	}
	if(v.IsMap()) { // MAP (HASH) a map is serialized only if it contains only literals, otherwise it is computed at runtime
		gravity_map_t * value = VALUE_AS_MAP(v);
		json_begin_object(json, key);
		if(!json_option_isset(json, json_opt_no_maptype)) json_add_cstring(json, GRAVITY_JSON_LABELTYPE, GRAVITY_JSON_MAP);
		gravity_hash_iterate(value->hash, gravity_map_serialize_iterator, json);
		json_end_object(json);
		return;
	}
	if(v.IsRange()) { // RANGE
		json_set_label(json, key);
		gravity_range_serialize(VALUE_AS_RANGE(v), json);
		return;
	}
	if(v.IsInstance()) { // INSTANCE
		json_set_label(json, key);
		gravity_instance_serialize(VALUE_AS_INSTANCE(v), json);
		return;
	}
	if(v.IsFiber()) { // FIBER
		return;
	}
	assert(0); // should never reach this point
}

/*bool FASTCALL gravity_value_isobject(const GravityValue v) 
{
	// was:
	// if (!v) return false;
	// if (v.IsInt()) return false;
	// if (v.IsFloat()) return false;
	// if (v.IsBool()) return false;
	// if (VALUE_ISA_NULL(v)) return false;
	// if (VALUE_ISA_UNDEFINED(v)) return false;
	// return true;
	if(!v.isa || oneof4(v.isa, GravityEnv.P_ClsInt, GravityEnv.P_ClsFloat, GravityEnv.P_ClsBool, GravityEnv.P_ClsNull) || !v.p) 
		return false;
	// extra check to allow ONLY known objects
	if(oneof12(v.isa, GravityEnv.P_ClsString, GravityEnv.P_ClsObj, GravityEnv.P_ClsFunc, GravityEnv.P_ClsClosure,
		GravityEnv.P_ClsFiber, GravityEnv.P_ClsClass, GravityEnv.P_ClsInstance, GravityEnv.P_ClsModule, GravityEnv.P_ClsList, 
		GravityEnv.P_ClsMap, GravityEnv.P_ClsRange, GravityEnv.P_ClsUpValue)) 
		return true;
	return false;
}*/

uint32 gravity_value_size(gravity_vm * vm, GravityValue v) 
{
	return v.IsObject() ? gravity_object_size(vm, v.Ptr) : 0;
}

void * gravity_value_xdata(GravityValue value) 
{
	if(value.IsInstance()) {
		gravity_instance_t * i = VALUE_AS_INSTANCE(value);
		return i->xdata;
	}
	else if(value.IsClass()) {
		gravity_class_t * c = static_cast<gravity_class_t *>(value);
		return c->xdata;
	}
	return NULL;
}

const char * gravity_value_name(GravityValue value) 
{
	if(value.IsInstance()) {
		gravity_instance_t * instance = VALUE_AS_INSTANCE(value);
		return instance->objclass->identifier;
	}
	else if(value.IsClass()) {
		gravity_class_t * c = static_cast<gravity_class_t *>(value);
		return c->identifier;
	}
	return NULL;
}

void gravity_value_dump(gravity_vm * vm, GravityValue v, char * buffer, uint16 len) 
{
	const char * type = NULL;
	const char * value = NULL;
	char sbuffer[1024];
	SETIFZ(buffer, sbuffer);
	SETIFZ(len, sizeof(sbuffer));
	if(v.isa == NULL) {
		type = "INVALID!";
		snprintf(buffer, len, "%s", type);
		value = buffer;
	}
	else if(v.isa == GravityEnv.P_ClsBool) {
		type = "BOOL";
		value = (v.n == 0) ? "false" : "true";
		snprintf(buffer, len, "(%s) %s", type, value);
		value = buffer;
	}
	else if(v.isa == GravityEnv.P_ClsNull) {
		type = (v.n == 0) ? "NULL" : "UNDEFINED";
		snprintf(buffer, len, "%s", type);
		value = buffer;
	}
	else if(v.isa == GravityEnv.P_ClsInt) {
		type = "INT";
		snprintf(buffer, len, "(%s) %" PRId64, type, v.n);
		value = buffer;
	}
	else if(v.isa == GravityEnv.P_ClsFloat) {
		type = "FLOAT";
		snprintf(buffer, len, "(%s) %g", type, v.f);
		value = buffer;
	}
	else if(v.isa == GravityEnv.P_ClsFunc) {
		type = "FUNCTION";
		value = VALUE_AS_FUNCTION(v)->identifier;
		snprintf(buffer, len, "(%s) %s (%p)", type, value, VALUE_AS_FUNCTION(v));
		value = buffer;
	}
	else if(v.isa == GravityEnv.P_ClsClosure) {
		type = "CLOSURE";
		gravity_function_t * f = VALUE_AS_CLOSURE(v)->f;
		value = (f->identifier) ? (f->identifier) : "anon";
		snprintf(buffer, len, "(%s) %s (%p)", type, value, VALUE_AS_CLOSURE(v));
		value = buffer;
	}
	else if(v.isa == GravityEnv.P_ClsClass) {
		type = "CLASS";
		value = static_cast<gravity_class_t *>(v)->identifier;
		snprintf(buffer, len, "(%s) %s (%p)", type, value, static_cast<gravity_class_t *>(v));
		value = buffer;
	}
	else if(v.isa == GravityEnv.P_ClsString) {
		type = "STRING";
		gravity_string_t * s = static_cast<gravity_string_t *>(v);
		snprintf(buffer, len, "(%s) %.*s (%p)", type, s->len, s->cptr(), s);
		value = buffer;
	}
	else if(v.isa == GravityEnv.P_ClsInstance) {
		type = "INSTANCE";
		gravity_instance_t * i = VALUE_AS_INSTANCE(v);
		gravity_class_t * c = i->objclass;
		value = c->identifier;
		snprintf(buffer, len, "(%s) %s (%p)", type, value, i);
		value = buffer;
	}
	else if(v.isa == GravityEnv.P_ClsList) {
		type = "LIST";
		GravityValue sval = convert_value2string(vm, v);
		gravity_string_t * s = static_cast<gravity_string_t *>(sval);
		snprintf(buffer, len, "(%s) %.*s (%p)", type, s->len, s->cptr(), s);
		value = buffer;
	}
	else if(v.isa == GravityEnv.P_ClsMap) {
		type = "MAP";
		GravityValue sval = convert_value2string(vm, v);
		gravity_string_t * s = static_cast<gravity_string_t *>(sval);
		snprintf(buffer, len, "(%s) %.*s (%p)", type, s->len, s->cptr(), s);
		value = buffer;
	}
	else if(v.isa == GravityEnv.P_ClsRange) {
		type = "RANGE";
		gravity_range_t * r = VALUE_AS_RANGE(v);
		snprintf(buffer, len, "(%s) from %" PRId64 " to %" PRId64, type, r->from, r->to);
		value = buffer;
	}
	else if(v.isa == GravityEnv.P_ClsObj) {
		type = "OBJECT";
		value = "N/A";
		snprintf(buffer, len, "(%s) %s", type, value);
		value = buffer;
	}
	else if(v.isa == GravityEnv.P_ClsFiber) {
		type = "FIBER";
		snprintf(buffer, len, "(%s) %p", type, v.Ptr);
		value = buffer;
	}
	else {
		type = "N/A";
		value = "N/A";
		snprintf(buffer, len, "(%s) %s", type, value);
		value = buffer;
	}
	if(buffer == sbuffer) printf("%s\n", value);
}

// MARK: -
gravity_list_t * FASTCALL gravity_list_new(gravity_vm * vm, uint32 n) 
{
	gravity_list_t * list = 0;
	if(n <= MAX_ALLOCATION) {
		list = new gravity_list_t; //static_cast<gravity_list_t *>(mem_alloc(NULL, sizeof(gravity_list_t)));
		// @ctr list->isa = GravityEnv.P_ClsList;
		// @ctr marray_init(list->array);
		list->array.resize(n + MARRAY_DEFAULT_SIZE);
		gravity_vm_transfer(vm, (gravity_class_t *)list);
	}
	return list;
}

gravity_list_t * gravity_list_from_array(gravity_vm * vm, uint32 n, GravityValue * p) 
{
	gravity_list_t * list = new gravity_list_t;//static_cast<gravity_list_t *>(mem_alloc(NULL, sizeof(gravity_list_t)));
	// @ctr list->isa = GravityEnv.P_ClsList;
	// @ctr marray_init(list->array);
	// elements must be copied because for the compiler their registers are TEMP
	// and could be reused by other successive operations
	for(size_t i = 0; i < n; ++i) 
		list->array.insert(p[i]);
	gravity_vm_transfer(vm, (gravity_class_t *)list);
	return list;
}

void gravity_list_free(gravity_vm * vm, gravity_list_t * list) 
{
	if(list) {
		DEBUG_FREE("FREE %s", gravity_object_debug((gravity_class_t *)list, true));
		list->array.Z();
		mem_free((void*)list);
	}
}

void gravity_list_append_list(gravity_vm * vm, gravity_list_t * list1, gravity_list_t * list2) 
{
	// append list2 to list1
	size_t count = list2->array.getCount();
	for(size_t i = 0; i<count; ++i) {
		list1->array.insert(list2->array.at(i));
	}
}

uint32 gravity_list_size(gravity_vm * vm, gravity_list_t * list) 
{
	SET_OBJECT_VISITED_FLAG(list, true);
	uint32 internal_size = 0;
	size_t count = list->array.getCount();
	for(size_t i = 0; i<count; ++i) {
		internal_size += gravity_value_size(vm, list->array.at(i));
	}
	internal_size += sizeof(gravity_list_t);
	SET_OBJECT_VISITED_FLAG(list, false);
	return internal_size;
}

void gravity_list_blacken(gravity_vm * vm, gravity_list_t * list) 
{
	gravity_vm_memupdate(vm, gravity_list_size(vm, list));
	size_t count = list->array.getCount();
	for(size_t i = 0; i<count; ++i) {
		gravity_gray_value(vm, list->array.at(i));
	}
}

// MARK: -
gravity_map_t * gravity_map_new(gravity_vm * vm, uint32 n) 
{
	gravity_map_t * map = (gravity_map_t*)mem_alloc(NULL, sizeof(gravity_map_t));
	map->isa = GravityEnv.P_ClsMap;
	map->hash = gravity_hash_create(n, gravity_value_hash, gravity_value_equals, NULL, NULL);
	gravity_vm_transfer(vm, (gravity_class_t *)map);
	return map;
}

void gravity_map_free(gravity_vm * vm, gravity_map_t * map) 
{
	DEBUG_FREE("FREE %s", gravity_object_debug((gravity_class_t *)map, true));
	gravity_hash_free(map->hash);
	mem_free((void*)map);
}

void gravity_map_append_map(gravity_vm * vm, gravity_map_t * map1, gravity_map_t * map2) 
{
	// append map2 to map1
	gravity_hash_append(map1->hash, map2->hash);
}

void gravity_map_insert(gravity_vm * vm, gravity_map_t * map, GravityValue key, GravityValue value) 
{
	gravity_hash_insert(map->hash, key, value);
}

static gravity_map_t * gravity_map_deserialize(gravity_vm * vm, json_value * json) 
{
	uint32 n = json->u.object.length;
	gravity_map_t * map = gravity_map_new(vm, n);
	DEBUG_DESERIALIZE("DESERIALIZE MAP: %p\n", map);
	for(uint32 i = 0; i<n; ++i) { // from 1 to skip type
		const char * label = json->u.object.values[i].name;
		json_value * jsonv = json->u.object.values[i].value;
		GravityValue key = gravity_zstring_to_value(vm, label);
		GravityValue value;
		switch(jsonv->type) {
			case json_integer: value = GravityValue::from_int((gravity_int_t)jsonv->u.integer); break;
			case json_double: value = GravityValue::from_float((gravity_float_t)jsonv->u.dbl); break;
			case json_boolean: value = GravityValue::from_bool(jsonv->u.boolean); break;
			case json_string: value = VALUE_FROM_STRING(vm, jsonv->u.string.ptr, jsonv->u.string.length); break;
			case json_null: value = GravityValue::from_null(); break;
			case json_object: 
				{
					gravity_class_t * obj = gravity_object_deserialize(vm, jsonv);
					value = (obj) ? GravityValue::from_object(obj) : GravityValue::from_null();
				}
			    break;
			case json_array: 
				{
				}
			default:
			    goto abort_load;
		}
		gravity_map_insert(NULL, map, key, value);
	}
	return map;
abort_load:
	// do not free map here because it is already garbage collected
	return NULL;
}

uint32 gravity_map_size(gravity_vm * vm, gravity_map_t * map) 
{
	SET_OBJECT_VISITED_FLAG(map, true);
	uint32 hash_size = 0;
	gravity_hash_iterate2(map->hash, gravity_hash_internalsize, (void*)&hash_size, (void*)vm);
	hash_size += gravity_hash_memsize(map->hash);
	hash_size += sizeof(gravity_map_t);
	SET_OBJECT_VISITED_FLAG(map, false);
	return hash_size;
}

void gravity_map_blacken(gravity_vm * vm, gravity_map_t * map) 
{
	gravity_vm_memupdate(vm, gravity_map_size(vm, map));
	gravity_hash_iterate(map->hash, gravity_hash_gray, (void*)vm);
}

// MARK: -

gravity_range_t * gravity_range_new(gravity_vm * vm, gravity_int_t from_range, gravity_int_t to_range, bool inclusive) 
{
	gravity_range_t * range = static_cast<gravity_range_t *>(mem_alloc(NULL, sizeof(gravity_range_t)));
	range->isa = GravityEnv.P_ClsRange;
	range->from = from_range;
	range->to = (inclusive) ? to_range : --to_range;
	gravity_vm_transfer(vm, (gravity_class_t *)range);
	return range;
}

void gravity_range_free(gravity_vm * vm, gravity_range_t * range) 
{
	DEBUG_FREE("FREE %s", gravity_object_debug((gravity_class_t *)range, true));
	mem_free((void*)range);
}

uint32 gravity_range_size(gravity_vm * vm, gravity_range_t * range) 
{
	SET_OBJECT_VISITED_FLAG(range, true);
	uint32 range_size = sizeof(gravity_range_t);
	SET_OBJECT_VISITED_FLAG(range, false);
	return range_size;
}

void gravity_range_serialize(gravity_range_t * r, GravityJson * json) 
{
	const char * label = json_get_label(json, NULL);
	json_begin_object(json, label);
	json_add_cstring(json, GRAVITY_JSON_LABELTYPE, GRAVITY_JSON_RANGE); // MANDATORY 1st FIELD
	json_add_int(json, GRAVITY_JSON_LABELFROM, r->from);
	json_add_int(json, GRAVITY_JSON_LABELTO, r->to);
	json_end_object(json);
}

gravity_range_t * gravity_range_deserialize(gravity_vm * vm, json_value * json) 
{
	json_int_t from = 0;
	json_int_t to = 0;
	uint32 n = json->u.object.length;
	for(uint32 i = 1; i<n; ++i) { // from 1 to skip type
		const char * label = json->u.object.values[i].name;
		json_value * value = json->u.object.values[i].value;
		size_t label_size = strlen(label);
		// from
		if(string_casencmp(label, GRAVITY_JSON_LABELFROM, label_size) == 0) {
			if(value->type != json_integer) goto abort_load;
			from = value->u.integer;
			continue;
		}
		// to
		if(string_casencmp(label, GRAVITY_JSON_LABELTO, label_size) == 0) {
			if(value->type != json_integer) goto abort_load;
			to = value->u.integer;
			continue;
		}
	}
	return gravity_range_new(vm, (gravity_int_t)from, (gravity_int_t)to, true);
abort_load:
	return NULL;
}

void gravity_range_blacken(gravity_vm * vm, gravity_range_t * range) 
{
	gravity_vm_memupdate(vm, gravity_range_size(vm, range));
}

// MARK: -


GravityValue FASTCALL gravity_zstring_to_value(gravity_vm * vm, const char * s)
{
	gravity_string_t * obj = new gravity_string_t;
	uint32 len = (uint32)sstrlen(s);
	uint32 alloc = MAX(len+1, DEFAULT_MINSTRING_SIZE);
	char * ptr = static_cast<char *>(mem_alloc(NULL, alloc));
	memcpy(ptr, s, len);
	//obj->isa = GravityEnv.P_ClsString;
	obj->P_StrBuf = ptr;
	obj->len = len;
	obj->alloc = alloc;
	obj->hash = gravity_hash_compute_buffer((const char*)ptr, len);
	GravityValue value;
	value.isa = GravityEnv.P_ClsString;
	value.Ptr = reinterpret_cast<gravity_class_t *>(obj);
	gravity_vm_transfer(vm, reinterpret_cast<gravity_class_t *>(obj));
	return value;
}

/*inline*/GravityValue gravity_string_to_value(gravity_vm * vm, const char * s, uint32 len) 
{
	gravity_string_t * obj = new gravity_string_t;
	if(len == AUTOLENGTH) 
		len = (uint32)sstrlen(s);
	uint32 alloc = MAX(len+1, DEFAULT_MINSTRING_SIZE);
	char * ptr = static_cast<char *>(mem_alloc(NULL, alloc));
	memcpy(ptr, s, len);
	//obj->isa = GravityEnv.P_ClsString;
	obj->P_StrBuf = ptr;
	obj->len = len;
	obj->alloc = alloc;
	obj->hash = gravity_hash_compute_buffer((const char*)ptr, len);
	GravityValue value;
	value.isa = GravityEnv.P_ClsString;
	value.Ptr = reinterpret_cast<gravity_class_t *>(obj);
	gravity_vm_transfer(vm, reinterpret_cast<gravity_class_t *>(obj));
	return value;
}

/*inline*/gravity_string_t * gravity_string_new(gravity_vm * vm, char * pS, uint32 len, uint32 alloc) 
{
	gravity_string_t * obj = new gravity_string_t;
	if(len == AUTOLENGTH) 
		len = (uint32)sstrlen(pS);
	//obj->isa = GravityEnv.P_ClsString;
	obj->P_StrBuf = (char *)pS;
	obj->len = len;
	obj->alloc = NZOR(alloc, len);
	if(pS && len) 
		obj->hash = gravity_hash_compute_buffer((const char *)pS, len);
	gravity_vm_transfer(vm, reinterpret_cast<gravity_class_t *>(obj));
	return obj;
}

/*inline*//*void gravity_string_set(gravity_string_t * obj, char * s, uint32 len) 
{
	obj->P_StrBuf = (char *)s;
	obj->len = len;
	obj->hash = gravity_hash_compute_buffer((const char*)s, len);
}*/

/*inline*/void gravity_string_free(gravity_vm * vm, gravity_string_t * value) 
{
	DEBUG_FREE("FREE %s", gravity_object_debug((gravity_class_t *)value, true));
	if(value) {
		if(value->alloc) 
			mem_free(value->P_StrBuf);
		delete value;
	}
}

uint32 gravity_string_size(gravity_vm * vm, gravity_string_t * string) 
{
	SET_OBJECT_VISITED_FLAG(string, true);
	uint32 string_size = (sizeof(gravity_string_t)) + string->alloc;
	SET_OBJECT_VISITED_FLAG(string, false);
	return string_size;
}

void gravity_string_blacken(gravity_vm * vm, gravity_string_t * string) 
{
	gravity_vm_memupdate(vm, gravity_string_size(vm, string));
}
