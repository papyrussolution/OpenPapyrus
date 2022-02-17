// Copyright (C) 2011 The Libphonenumber Authors
// @license Apache License 2.0
// Author: Fredrik Roubert
//
#include <libphonenumber-internal.h>
#pragma hdrstop

using std::string;

namespace i18n {
namespace phonenumbers {
RegExpCache::RegExpCache(const AbstractRegExpFactory& regexp_factory, size_t min_items) : regexp_factory_(regexp_factory),
#ifdef I18N_PHONENUMBERS_USE_TR1_UNORDERED_MAP
	cache_impl_(new CacheImpl(min_items))
#else
	cache_impl_(new CacheImpl())
#endif
{
}

RegExpCache::~RegExpCache() 
{
	AutoLock l(lock_);
	for(CacheImpl::const_iterator
	    it = cache_impl_->begin(); it != cache_impl_->end(); ++it) {
		delete it->second;
	}
}

const RegExp& RegExpCache::GetRegExp(const string& pattern) 
{
	AutoLock l(lock_);
	CacheImpl::const_iterator it = cache_impl_->find(pattern);
	if(it != cache_impl_->end()) 
		return *it->second;
	const RegExp* regexp = regexp_factory_.CreateRegExp(pattern);
	cache_impl_->insert(std::make_pair(pattern, regexp));
	return *regexp;
}
}  // namespace phonenumbers
}  // namespace i18n
