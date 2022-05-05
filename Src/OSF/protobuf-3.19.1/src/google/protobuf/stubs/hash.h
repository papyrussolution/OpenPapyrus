// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// Author: kenton@google.com (Kenton Varda)

#ifndef GOOGLE_PROTOBUF_STUBS_HASH_H__
#define GOOGLE_PROTOBUF_STUBS_HASH_H__

//#include <cstring>
//#include <string>
//#include <unordered_map>
//#include <unordered_set>

#define GOOGLE_PROTOBUF_HASH_NAMESPACE_DECLARATION_START namespace google { namespace protobuf {
#define GOOGLE_PROTOBUF_HASH_NAMESPACE_DECLARATION_END }}

namespace google {
namespace protobuf {

template <typename Key> struct hash : public std::hash<Key> {};

template <typename Key> struct hash<const Key*> {
  inline size_t operator()(const Key* key) const {
    return reinterpret_cast<size_t>(key);
  }
};

// Unlike the old SGI version, the TR1 "hash" does not special-case char*.  So,
// we go ahead and provide our own implementation.
template <> struct hash<const char*> {
  inline size_t operator()(const char* str) const 
  {
    size_t result = 0;
    for (; *str != '\0'; str++) {
      result = 5 * result + static_cast<size_t>(*str);
    }
    return result;
  }
};

template<> struct hash<bool> 
{
  size_t operator()(bool x) const { return static_cast<size_t>(x); }
};

template <> struct hash<std::string> {
  inline size_t operator()(const std::string& key) const { return hash<const char*>()(key.c_str()); }
  static const size_t bucket_size = 4;
  static const size_t min_buckets = 8;
  inline bool operator()(const std::string& a, const std::string& b) const { return a < b; }
};

template <typename First, typename Second> struct hash<std::pair<First, Second> > {
  inline size_t operator()(const std::pair<First, Second>& key) const 
  {
    size_t first_hash = hash<First>()(key.first);
    size_t second_hash = hash<Second>()(key.second);
    // FIXME(kenton):  What is the best way to compute this hash?  I have
    // no idea!  This seems a bit better than an XOR.
    return first_hash * ((1 << 16) - 1) + second_hash;
  }

  static const size_t bucket_size = 4;
  static const size_t min_buckets = 8;
  inline bool operator()(const std::pair<First, Second>& a, const std::pair<First, Second>& b) const { return a < b; }
};

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_STUBS_HASH_H__
