#pragma once

#include "configure.h"
#include "Communication.h"


namespace CouchDB {

/**
* ѕул объектов. ƒинамический.
* »спользуетс€ дл€ накоплени€ объектов, например, с целью сохранить
* их одним махом.
*
* @info »з-за т€жЄлого преобразовани€ русских букв (от которого надо
*       избавитьс€), лучше не смешивать д. с русск. символами и без
*       в одном пуле, сохран€€ эти пулы по отдельности.
*/
class Pool :
    public CouchDB::Array
{
public:
    inline Pool() :
        CouchDB::Array()
    {
    }


    virtual inline ~Pool() {
    }

};


} // namespace CouchDB




inline CouchDB::Pool& operator<<(
    CouchDB::Pool& pool,
    const CouchDB::Object& doc
) {
    pool.push_back( CouchDB::cjv( doc ) );
    return pool;
}
