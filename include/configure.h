#pragma once


/**
* Принятые соглашения для проекта:
*
*   # Клиент для работы с CouchDB использует методы operator<<() и operator>>,
*     представляющие собой запросы для изменения и чтения в / из хранилища.
*   # Методы, печатающие в поток - вида operator<<( std::ostream, * ) - делают
*     это в формате JSON.
*/




// Without warning: "Name truncated"
#pragma warning( disable : 4503 )


/**
* How max elements prepare to bulkSave before save.
* @see createBulk( CouchFine::Object )
*/
static const size_t ACC_SIZE = 1000;
static const size_t ACC_PLAIN_SIZE = 1024 * 1000;


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
//#define COUCHFINE_DEBUG


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

#include <boost/algorithm/string/trim.hpp>
#include <boost/any.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>

#include <curl/curl.h>
