/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop

void * ngx_hash_find(ngx_hash_t * hash, ngx_uint_t key, const uchar * name, size_t len)
{
#if 0
	ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, "hf:\"%*s\"", len, name);
#endif
	ngx_hash_elt_t * elt = hash->buckets[key % hash->size];
	if(elt) {
		// modified by @sobolev (��-������� ��������� �������� �� memcmp)
		while(elt->value) {
			if(len == (size_t)elt->len && memcmp(name, elt->name, len) == 0)
				return elt->value;
			else
				elt = (ngx_hash_elt_t*)ngx_align_ptr(&elt->name[0] + elt->len, sizeof(void *));
		}
	}
	return NULL;
}

void * ngx_hash_find_wc_head(ngx_hash_wildcard_t * hwc, const uchar * name, size_t len)
{
	void * value;
	ngx_uint_t i, n, key;
#if 0
	ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, "wch:\"%*s\"", len, name);
#endif
	n = len;
	while(n) {
		if(name[n-1] == '.') {
			break;
		}
		n--;
	}
	key = 0;
	for(i = n; i < len; i++) {
		key = ngx_hash(key, name[i]);
	}
#if 0
	ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, "key:\"%ui\"", key);
#endif
	value = ngx_hash_find(&hwc->hash, key, &name[n], len - n);
#if 0
	ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, "value:\"%p\"", value);
#endif
	if(value) {
		/*
		 * the 2 low bits of value have the special meaning:
		 *   00 - value is data pointer for both "example.com" and "*.example.com";
		 *   01 - value is data pointer for "*.example.com" only;
		 *   10 - value is pointer to wildcard hash allowing both "example.com" and "*.example.com";
		 *   11 - value is pointer to wildcard hash allowing "*.example.com" only.
		 */
		if((uintptr_t)value & 2) {
			if(n == 0) {
				/* "example.com" */
				if((uintptr_t)value & 1) {
					return NULL;
				}
				hwc = (ngx_hash_wildcard_t*)((uintptr_t)value & (uintptr_t) ~3);
				return hwc->value;
			}
			hwc = (ngx_hash_wildcard_t*)((uintptr_t)value & (uintptr_t) ~3);
			value = ngx_hash_find_wc_head(hwc, name, n - 1);
			if(value) {
				return value;
			}
			return hwc->value;
		}
		if((uintptr_t)value & 1) {
			if(n == 0) {
				/* "example.com" */
				return NULL;
			}
			return (void *)((uintptr_t)value & (uintptr_t) ~3);
		}
		return value;
	}
	return hwc->value;
}

void * ngx_hash_find_wc_tail(ngx_hash_wildcard_t * hwc, const uchar * name, size_t len)
{
	void * value;
	ngx_uint_t i, key;
#if 0
	ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, "wct:\"%*s\"", len, name);
#endif
	key = 0;
	for(i = 0; i < len; i++) {
		if(name[i] == '.') {
			break;
		}
		key = ngx_hash(key, name[i]);
	}
	if(i == len) {
		return NULL;
	}
#if 0
	ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, "key:\"%ui\"", key);
#endif
	value = ngx_hash_find(&hwc->hash, key, name, i);
#if 0
	ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, "value:\"%p\"", value);
#endif
	if(value) {
		/*
		 * the 2 low bits of value have the special meaning:
		 *   00 - value is data pointer;
		 *   11 - value is pointer to wildcard hash allowing "example.*".
		 */
		if((uintptr_t)value & 2) {
			i++;
			hwc = (ngx_hash_wildcard_t*)((uintptr_t)value & (uintptr_t) ~3);
			value = ngx_hash_find_wc_tail(hwc, &name[i], len - i);
			if(value) {
				return value;
			}
			return hwc->value;
		}
		return value;
	}
	return hwc->value;
}

void * ngx_hash_find_combined(ngx_hash_combined_t * hash, ngx_uint_t key, const uchar * name, size_t len)
{
	if(hash->hash.buckets) {
		void * value = ngx_hash_find(&hash->hash, key, name, len);
		if(value)
			return value;
	}
	if(len) {
		if(hash->wc_head && hash->wc_head->hash.buckets) {
			void * value = ngx_hash_find_wc_head(hash->wc_head, name, len);
			if(value)
				return value;
		}
		if(hash->wc_tail && hash->wc_tail->hash.buckets) {
			void * value = ngx_hash_find_wc_tail(hash->wc_tail, name, len);
			if(value)
				return value;
		}
	}
	return NULL;
}

#define NGX_HASH_ELT_SIZE(name) (sizeof(void *) + ngx_align((name)->key.len + 2, sizeof(void *)))

ngx_int_t ngx_hash_init(ngx_hash_init_t * hinit, ngx_hash_key_t * names, ngx_uint_t nelts)
{
	uchar * elts;
	size_t len;
	ushort  * test;
	ngx_uint_t i, n, key, size, start, bucket_size;
	ngx_hash_elt_t  * elt, ** buckets;
	if(hinit->max_size == 0) {
		ngx_log_error(NGX_LOG_EMERG, hinit->pool->log, 0, "could not build %s, you should increase %s_max_size: %i",
		    hinit->name, hinit->name, hinit->max_size);
		return NGX_ERROR;
	}
	for(n = 0; n < nelts; n++) {
		if(hinit->bucket_size < NGX_HASH_ELT_SIZE(&names[n]) + sizeof(void *)) {
			ngx_log_error(NGX_LOG_EMERG, hinit->pool->log, 0, "could not build %s, you should increase %s_bucket_size: %i",
			    hinit->name, hinit->name, hinit->bucket_size);
			return NGX_ERROR;
		}
	}
	test = (ushort *)ngx_alloc(hinit->max_size * sizeof(ushort), hinit->pool->log);
	if(test == NULL) {
		return NGX_ERROR;
	}
	bucket_size = hinit->bucket_size - sizeof(void *);
	start = nelts / (bucket_size / (2 * sizeof(void *)));
	start = start ? start : 1;
	if(hinit->max_size > 10000 && nelts && hinit->max_size / nelts < 100) {
		start = hinit->max_size - 1000;
	}
	for(size = start; size <= hinit->max_size; size++) {
		memzero(test, size * sizeof(ushort));
		for(n = 0; n < nelts; n++) {
			if(names[n].key.data == NULL) {
				continue;
			}
			key = names[n].key_hash % size;
			test[key] = (ushort)(test[key] + NGX_HASH_ELT_SIZE(&names[n]));
#if 0
			ngx_log_error(NGX_LOG_ALERT, hinit->pool->log, 0, "%ui: %ui %ui \"%V\"", size, key, test[key], &names[n].key);
#endif
			if(test[key] > (ushort)bucket_size) {
				goto next;
			}
		}
		goto found;
next:
		continue;
	}
	size = hinit->max_size;
	ngx_log_error(NGX_LOG_WARN, hinit->pool->log, 0, "could not build optimal %s, you should increase either %s_max_size: %i or %s_bucket_size: %i; ignoring %s_bucket_size",
	    hinit->name, hinit->name, hinit->max_size, hinit->name, hinit->bucket_size, hinit->name);
found:
	for(i = 0; i < size; i++) {
		test[i] = sizeof(void *);
	}
	for(n = 0; n < nelts; n++) {
		if(names[n].key.data == NULL) {
			continue;
		}
		key = names[n].key_hash % size;
		test[key] = (ushort)(test[key] + NGX_HASH_ELT_SIZE(&names[n]));
	}
	len = 0;
	for(i = 0; i < size; i++) {
		if(test[i] == sizeof(void *)) {
			continue;
		}
		test[i] = (ushort)(ngx_align(test[i], ngx_cacheline_size));
		len += test[i];
	}
	if(hinit->hash == NULL) {
		hinit->hash = (ngx_hash_t *)ngx_pcalloc(hinit->pool, sizeof(ngx_hash_wildcard_t) + size * sizeof(ngx_hash_elt_t *));
		if(hinit->hash == NULL) {
			SAlloc::F(test);
			return NGX_ERROR;
		}
		buckets = (ngx_hash_elt_t**)((uchar *)hinit->hash + sizeof(ngx_hash_wildcard_t));
	}
	else {
		buckets = (ngx_hash_elt_t **)ngx_pcalloc(hinit->pool, size * sizeof(ngx_hash_elt_t *));
		if(buckets == NULL) {
			SAlloc::F(test);
			return NGX_ERROR;
		}
	}
	elts = (uchar *)ngx_palloc(hinit->pool, len + ngx_cacheline_size);
	if(elts == NULL) {
		SAlloc::F(test);
		return NGX_ERROR;
	}
	elts = ngx_align_ptr(elts, ngx_cacheline_size);
	for(i = 0; i < size; i++) {
		if(test[i] == sizeof(void *)) {
			continue;
		}
		buckets[i] = (ngx_hash_elt_t*)elts;
		elts += test[i];
	}
	for(i = 0; i < size; i++) {
		test[i] = 0;
	}
	for(n = 0; n < nelts; n++) {
		if(names[n].key.data == NULL) {
			continue;
		}
		key = names[n].key_hash % size;
		elt = (ngx_hash_elt_t*)((uchar *)buckets[key] + test[key]);
		elt->value = names[n].value;
		elt->len = (ushort)names[n].key.len;
		ngx_strlow(elt->name, names[n].key.data, names[n].key.len);
		test[key] = (ushort)(test[key] + NGX_HASH_ELT_SIZE(&names[n]));
	}
	for(i = 0; i < size; i++) {
		if(buckets[i] == NULL) {
			continue;
		}
		elt = (ngx_hash_elt_t*)((uchar *)buckets[i] + test[i]);
		elt->value = NULL;
	}
	SAlloc::F(test);
	hinit->hash->buckets = buckets;
	hinit->hash->size = size;
#if 0
	for(i = 0; i < size; i++) {
		ngx_str_t val;
		ngx_uint_t key;
		elt = buckets[i];
		if(elt == NULL) {
			ngx_log_error(NGX_LOG_ALERT, hinit->pool->log, 0, "%ui: NULL", i);
			continue;
		}
		while(elt->value) {
			val.len = elt->len;
			val.data = &elt->name[0];
			key = hinit->key(val.data, val.len);
			ngx_log_error(NGX_LOG_ALERT, hinit->pool->log, 0, "%ui: %p \"%V\" %ui", i, elt, &val, key);
			elt = (ngx_hash_elt_t*)ngx_align_ptr(&elt->name[0] + elt->len, sizeof(void *));
		}
	}
#endif
	return NGX_OK;
}

ngx_int_t ngx_hash_wildcard_init(ngx_hash_init_t * hinit, ngx_hash_key_t * names, ngx_uint_t nelts)
{
	size_t len, dot_len;
	ngx_uint_t i, n, dot;
	ngx_array_t curr_names, next_names;
	ngx_hash_key_t  * name, * next_name;
	ngx_hash_init_t h;
	ngx_hash_wildcard_t  * wdc;
	if(ngx_array_init(&curr_names, hinit->temp_pool, nelts, sizeof(ngx_hash_key_t)) != NGX_OK) {
		return NGX_ERROR;
	}
	if(ngx_array_init(&next_names, hinit->temp_pool, nelts, sizeof(ngx_hash_key_t)) != NGX_OK) {
		return NGX_ERROR;
	}
	for(n = 0; n < nelts; n = i) {
#if 0
		ngx_log_error(NGX_LOG_ALERT, hinit->pool->log, 0, "wc0: \"%V\"", &names[n].key);
#endif
		dot = 0;
		for(len = 0; len < names[n].key.len; len++) {
			if(names[n].key.data[len] == '.') {
				dot = 1;
				break;
			}
		}
		name = (ngx_hash_key_t *)ngx_array_push(&curr_names);
		if(!name) {
			return NGX_ERROR;
		}
		name->key.len = len;
		name->key.data = names[n].key.data;
		name->key_hash = hinit->key(name->key.data, name->key.len);
		name->value = names[n].value;
#if 0
		ngx_log_error(NGX_LOG_ALERT, hinit->pool->log, 0, "wc1: \"%V\" %ui", &name->key, dot);
#endif
		dot_len = len + 1;
		if(dot) {
			len++;
		}
		next_names.nelts = 0;
		if(names[n].key.len != len) {
			next_name = (ngx_hash_key_t *)ngx_array_push(&next_names);
			if(next_name == NULL) {
				return NGX_ERROR;
			}
			next_name->key.len = names[n].key.len - len;
			next_name->key.data = names[n].key.data + len;
			next_name->key_hash = 0;
			next_name->value = names[n].value;
#if 0
			ngx_log_error(NGX_LOG_ALERT, hinit->pool->log, 0, "wc2: \"%V\"", &next_name->key);
#endif
		}
		for(i = n + 1; i < nelts; i++) {
			if(ngx_strncmp(names[n].key.data, names[i].key.data, len) != 0) {
				break;
			}
			if(!dot && names[i].key.len > len && names[i].key.data[len] != '.') {
				break;
			}
			next_name = (ngx_hash_key_t *)ngx_array_push(&next_names);
			if(next_name == NULL) {
				return NGX_ERROR;
			}
			next_name->key.len = names[i].key.len - dot_len;
			next_name->key.data = names[i].key.data + dot_len;
			next_name->key_hash = 0;
			next_name->value = names[i].value;
#if 0
			ngx_log_error(NGX_LOG_ALERT, hinit->pool->log, 0, "wc3: \"%V\"", &next_name->key);
#endif
		}
		if(next_names.nelts) {
			h = *hinit;
			h.hash = NULL;
			if(ngx_hash_wildcard_init(&h, (ngx_hash_key_t*)next_names.elts, next_names.nelts) != NGX_OK) {
				return NGX_ERROR;
			}
			wdc = (ngx_hash_wildcard_t*)h.hash;
			if(names[n].key.len == len) {
				wdc->value = names[n].value;
			}
			name->value = reinterpret_cast<void *>((uintptr_t)wdc | (dot ? 3 : 2));
		}
		else if(dot) {
			name->value = reinterpret_cast<void *>((uintptr_t)name->value | 1);
		}
	}
	if(ngx_hash_init(hinit, (ngx_hash_key_t*)curr_names.elts, curr_names.nelts) != NGX_OK) {
		return NGX_ERROR;
	}
	return NGX_OK;
}

ngx_uint_t ngx_hash_key(const uchar * data, size_t len)
{
	ngx_uint_t key = 0;
	for(ngx_uint_t i = 0; i < len; i++) {
		key = ngx_hash(key, data[i]);
	}
	return key;
}

ngx_uint_t ngx_hash_key_lc(const uchar * data, size_t len)
{
	ngx_uint_t key = 0;
	for(ngx_uint_t i = 0; i < len; i++) {
		key = ngx_hash(key, stolower_ascii(data[i]));
	}
	return key;
}

ngx_uint_t ngx_hash_strlow(uchar * dst, uchar * src, size_t n)
{
	ngx_uint_t key = 0;
	while(n--) {
		*dst = stolower_ascii(*src);
		key = ngx_hash(key, *dst);
		dst++;
		src++;
	}
	return key;
}

ngx_int_t FASTCALL ngx_hash_keys_array_init(ngx_hash_keys_arrays_t * ha, ngx_uint_t type)
{
	ngx_uint_t asize;
	if(type == NGX_HASH_SMALL) {
		asize = 4;
		ha->hsize = 107;
	}
	else {
		asize = NGX_HASH_LARGE_ASIZE;
		ha->hsize = NGX_HASH_LARGE_HSIZE;
	}
	if(ngx_array_init(&ha->keys, ha->temp_pool, asize, sizeof(ngx_hash_key_t)) != NGX_OK) {
		return NGX_ERROR;
	}
	if(ngx_array_init(&ha->dns_wc_head, ha->temp_pool, asize, sizeof(ngx_hash_key_t)) != NGX_OK) {
		return NGX_ERROR;
	}
	if(ngx_array_init(&ha->dns_wc_tail, ha->temp_pool, asize, sizeof(ngx_hash_key_t)) != NGX_OK) {
		return NGX_ERROR;
	}
	ha->keys_hash = (ngx_array_t *)ngx_pcalloc(ha->temp_pool, sizeof(ngx_array_t) * ha->hsize);
	if(ha->keys_hash == NULL) {
		return NGX_ERROR;
	}
	ha->dns_wc_head_hash = (ngx_array_t *)ngx_pcalloc(ha->temp_pool, sizeof(ngx_array_t) * ha->hsize);
	if(ha->dns_wc_head_hash == NULL) {
		return NGX_ERROR;
	}
	ha->dns_wc_tail_hash = (ngx_array_t *)ngx_pcalloc(ha->temp_pool, sizeof(ngx_array_t) * ha->hsize);
	if(ha->dns_wc_tail_hash == NULL) {
		return NGX_ERROR;
	}
	return NGX_OK;
}

ngx_int_t ngx_hash_add_key(ngx_hash_keys_arrays_t * ha, ngx_str_t * key, void * value, ngx_uint_t flags)
{
	size_t len;
	uchar   * p;
	ngx_str_t  * name;
	ngx_uint_t i, k, n, skip;
	ngx_array_t   * keys, * hwc;
	ngx_hash_key_t  * hk;
	ngx_uint_t last = key->len;
	if(flags & NGX_HASH_WILDCARD_KEY) {
		/*
		 * supported wildcards:
		 *   "*.example.com", ".example.com", and "www.example.*"
		 */
		n = 0;
		for(i = 0; i < key->len; i++) {
			if(key->data[i] == '*') {
				if(++n > 1) {
					return NGX_DECLINED;
				}
			}
			if(key->data[i] == '.' && key->data[i+1] == '.') {
				return NGX_DECLINED;
			}
			if(key->data[i] == '\0') {
				return NGX_DECLINED;
			}
		}
		if(key->len > 1 && key->data[0] == '.') {
			skip = 1;
			goto wildcard;
		}
		if(key->len > 2) {
			if(key->data[0] == '*' && key->data[1] == '.') {
				skip = 2;
				goto wildcard;
			}
			if(key->data[i - 2] == '.' && key->data[i - 1] == '*') {
				skip = 0;
				last -= 2;
				goto wildcard;
			}
		}
		if(n) {
			return NGX_DECLINED;
		}
	}
	/* exact hash */
	k = 0;
	for(i = 0; i < last; i++) {
		if(!(flags & NGX_HASH_READONLY_KEY)) {
			key->data[i] = stolower_ascii(key->data[i]);
		}
		k = ngx_hash(k, key->data[i]);
	}
	k %= ha->hsize;
	/* check conflicts in exact hash */
	name = (ngx_str_t *)ha->keys_hash[k].elts;
	if(name) {
		for(i = 0; i < ha->keys_hash[k].nelts; i++) {
			if(last != name[i].len) {
				continue;
			}
			if(ngx_strncmp(key->data, name[i].data, last) == 0) {
				return NGX_BUSY;
			}
		}
	}
	else {
		if(ngx_array_init(&ha->keys_hash[k], ha->temp_pool, 4, sizeof(ngx_str_t)) != NGX_OK) {
			return NGX_ERROR;
		}
	}
	name = (ngx_str_t *)ngx_array_push(&ha->keys_hash[k]);
	if(!name) {
		return NGX_ERROR;
	}
	*name = *key;
	hk = (ngx_hash_key_t *)ngx_array_push(&ha->keys);
	if(!hk) {
		return NGX_ERROR;
	}
	hk->key = *key;
	hk->key_hash = ngx_hash_key(key->data, last);
	hk->value = value;
	return NGX_OK;
wildcard:
	/* wildcard hash */
	k = ngx_hash_strlow(&key->data[skip], &key->data[skip], last - skip);
	k %= ha->hsize;
	if(skip == 1) {
		/* check conflicts in exact hash for ".example.com" */
		name = (ngx_str_t *)ha->keys_hash[k].elts;
		if(name) {
			len = last - skip;
			for(i = 0; i < ha->keys_hash[k].nelts; i++) {
				if(len != name[i].len) {
					continue;
				}
				if(ngx_strncmp(&key->data[1], name[i].data, len) == 0) {
					return NGX_BUSY;
				}
			}
		}
		else {
			if(ngx_array_init(&ha->keys_hash[k], ha->temp_pool, 4, sizeof(ngx_str_t)) != NGX_OK) {
				return NGX_ERROR;
			}
		}
		name = (ngx_str_t *)ngx_array_push(&ha->keys_hash[k]);
		if(!name) {
			return NGX_ERROR;
		}
		name->len = last - 1;
		name->data = (uchar *)ngx_pnalloc(ha->temp_pool, name->len);
		if(name->data == NULL) {
			return NGX_ERROR;
		}
		memcpy(name->data, &key->data[1], name->len);
	}
	if(skip) {
		/*
		 * convert "*.example.com" to "com.example.\0"
		 * and ".example.com" to "com.example\0"
		 */
		p = (uchar *)ngx_pnalloc(ha->temp_pool, last);
		if(!p) {
			return NGX_ERROR;
		}
		len = 0;
		n = 0;
		for(i = last - 1; i; i--) {
			if(key->data[i] == '.') {
				memcpy(&p[n], &key->data[i+1], len);
				n += len;
				p[n++] = '.';
				len = 0;
				continue;
			}
			len++;
		}
		if(len) {
			memcpy(&p[n], &key->data[1], len);
			n += len;
		}
		p[n] = '\0';
		hwc = &ha->dns_wc_head;
		keys = &ha->dns_wc_head_hash[k];
	}
	else {
		/* convert "www.example.*" to "www.example\0" */
		last++;
		p = (uchar *)ngx_pnalloc(ha->temp_pool, last);
		if(!p) {
			return NGX_ERROR;
		}
		ngx_cpystrn(p, key->data, last);
		hwc = &ha->dns_wc_tail;
		keys = &ha->dns_wc_tail_hash[k];
	}
	/* check conflicts in wildcard hash */
	name = (ngx_str_t *)keys->elts;
	if(name) {
		len = last - skip;
		for(i = 0; i < keys->nelts; i++) {
			if(len != name[i].len) {
				continue;
			}
			if(ngx_strncmp(key->data + skip, name[i].data, len) == 0) {
				return NGX_BUSY;
			}
		}
	}
	else {
		if(ngx_array_init(keys, ha->temp_pool, 4, sizeof(ngx_str_t)) != NGX_OK) {
			return NGX_ERROR;
		}
	}
	name = (ngx_str_t *)ngx_array_push(keys);
	if(!name) {
		return NGX_ERROR;
	}
	name->len = last - skip;
	name->data = (uchar *)ngx_pnalloc(ha->temp_pool, name->len);
	if(name->data == NULL) {
		return NGX_ERROR;
	}
	memcpy(name->data, key->data + skip, name->len);
	/* add to wildcard hash */
	hk = (ngx_hash_key_t *)ngx_array_push(hwc);
	if(!hk) {
		return NGX_ERROR;
	}
	hk->key.len = last - 1;
	hk->key.data = p;
	hk->key_hash = 0;
	hk->value = value;
	return NGX_OK;
}

