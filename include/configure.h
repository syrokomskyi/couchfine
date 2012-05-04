#pragma once

// Without warning: "Name truncated"
#pragma warning( disable : 4503 )


/**
* How max elements prepare to bulkSave defore save.
* @see createBulk( CouchDB::Object )
*/
#define ACC_SIZE 1000


/**
* Save order of results (use map instead of unordered_map -> slower).
*/
#define SAVE_ORDER_FIELDS



/**
* Switch off the macroses min/max in WinDef.h if your project
* use std::min() / std::max().
*/
#define NOMINMAX


/**
* Print debug information.
*/
//#define COUCH_DB_DEBUG


#include <assert.h>
#include <deque>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stack>
#include <string>
#include <streambuf>
#include <utility>
#include <vector>

#ifdef SAVE_ORDER_FIELDS
#include <map>
#else
#include <unordered_map>
#endif

#include <boost/algorithm/string/trim.hpp>
#include <boost/any.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>

#include <curl/curl.h>
