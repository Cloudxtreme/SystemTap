// interned string table
// Copyright (C) 2015 Red Hat Inc.
//
// This file is part of systemtap, and is free software.  You can
// redistribute it and/or modify it under the terms of the GNU General
// Public License (GPL); either version 2, or (at your option) any
// later version.

#include "config.h"
#include "stringtable.h"
#include "unordered.h"

#include <string>
#include <cstring>

typedef unordered_set<std::string> stringtable_t;

using namespace std;
using namespace boost;

stringtable_t stringtable;


// Generate a long-lived string_ref for the given input string.  In
// the absence of proper refcounting, memory is kept for the whole
// duration of the systemtap run.  Try to reuse the same string
// object for multiple invocations.  Old string_refs remain valid 
// because std::set<> guarantees iterator validity across inserts,
// which means that our value strings stay put.

static interned_string intern(const string& value)
{
  // check the string table for exact match
  stringtable_t::iterator it = stringtable.find(value);
  if (it != stringtable.end())
    return interned_string (string_ref (it->data(), it->length()));

  // alas ... no joy ... insert into set
  it = (stringtable.insert(value)).first; // persistent iterator!
  return interned_string (string_ref (it->data(), it->length()));

  // XXX: for future consideration, consider searching the stringtable
  // for instances where 'value' is a substring.  We could string_ref
  // to substrings just fine.  The trouble is that searching the
  // stringtable naively is very timetaking; it saves memory but costs
  // mucho CPU.
}


interned_string::interned_string(): string_ref(), _c_str(0)
{
}


interned_string::interned_string(const char* value): string_ref(intern(value)), _c_str(0)
{
}
                                                                
interned_string::interned_string(const string& value): string_ref(intern(value)), _c_str(0)
{
}

interned_string::interned_string(const interned_string& value): string_ref(value), _c_str(0)
{
}

interned_string::interned_string(const boost::string_ref& value): string_ref(value), _c_str(0)
{
}

interned_string& interned_string::operator = (const std::string& value)
{
  *this = intern(value);
  return *this;
}

interned_string& interned_string::operator = (const char* value)
{
  *this = intern(value);
  return *this;
}

interned_string::~interned_string()
{
  free (_c_str);
}

// easy out-conversion operators
interned_string::operator std::string () const
{
  return this->to_string();
}

const char* interned_string::c_str() const
{
  free (_c_str);
  _c_str = (char*) malloc(this->length()+1);
  strncpy(_c_str, this->data(), this->length());
  _c_str[this->length()]='\0';
  return _c_str;
}

/* vim: set sw=2 ts=8 cino=>4,n-2,{2,^-2,t0,(0,u0,w1,M1 : */
