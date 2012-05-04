/**
 * @file plustache_types.hpp
 * @brief header file for plustache types
 * @author Daniel Schauenberg <d@unwiredcouch.com>
 */
#ifndef PLUSTACHE_TYPES_H_
#define PLUSTACHE_TYPES_H_

// Without warning: "Name truncated"
#pragma warning( disable : 4503 )


#include <string>
#include <map>
#include <vector>


namespace PlustacheTypes {
    /* typedefs */
    typedef std::map<std::string, std::string> ObjectType;
    typedef std::vector<ObjectType> CollectionType;
}

#endif
