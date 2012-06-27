#pragma once

#include "type.h"


namespace CouchFine {

/**
* Пул объектов. Динамический.
* Используется для накопления объектов, например, с целью сохранить
* их одним махом.
*
* @info Из-за тяжёлого преобразования русских букв (от которого надо
*       избавиться @todo Свежая версия CouchDB?), лучше не смешивать
*       д. с русск. символами и без в одном пуле, сохраняя эти пулы
*       по отдельности.
*/
class Pool :
    public CouchFine::Array
{
public:
    inline Pool() :
        CouchFine::Array()
    {
    }


    virtual inline ~Pool() {
    }


};


} // CouchFine





// (!) Меняет Object.
inline CouchFine::Pool& operator<<(
    CouchFine::Pool& pool,
    // Передаётся ссылка, т.к. методы могут добавить к объекту свои данные.
    // Например, _id и _rev.
    CouchFine::Variant* var
) {
    pool.push_back( *var );
    return pool;
}



// (!) Меняет Object.
inline CouchFine::Pool& operator<<(
    CouchFine::Pool& pool,
    // Передаётся ссылка, т.к. методы могут добавить к объекту свои данные.
    // Например, _id и _rev.
    CouchFine::Object* doc
) {
    pool << &typelib::json::cjv( doc );
    return pool;
}



/* - Не усложняем. Чревато утечкой памяти.
// (!) Object не меняет.
inline CouchFine::Pool& operator<<(
    CouchFine::Pool& pool,
    const CouchFine::Object& doc
) {
    pool.push_back( typelib::json::cjv( doc ) );
    return pool;
}
*/
