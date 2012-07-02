#pragma once

// @todo fine Убрать using namespace из global.
// @todo fine Убрать лишние указания CouchFine:: в коде.

// @todo fine optimize Создать библиотеку, которая могла бы становиться
//       как чистая inline в зависимости от директивы препроцессора.

#include "configure.h"
#include "Mode.h"
#include "Communication.h"
#include "Connection.h"
#include "View.h"
#include "Database.h"
#include "Pool.h"


namespace CouchFine {


/**
* (!) Внутри design-методов должны использоваться только одинарные кавычки.
*/


// Методы работы с потоками вынесены сюда, чтобы:
// 1. Было нагляднее.
// 2. Их можно было определить как inline.


/**
* Чтение документов из хранилища.
*/


/**
* Читается один документ.
*/
CouchFine::Database& operator>>(
    CouchFine::Database& store,
    CouchFine::Object& doc
);




/**
* Читается множество документов.
* 'pool' должен содержать UID документов.
*
* @todo extend Возможность задавать в пуле нужную ревизию документа.
*/
CouchFine::Database& operator>>(
    CouchFine::Database& store,
    CouchFine::Mode::Doc& doc
);






CouchFine::Database& operator>>(
    CouchFine::Database& store,
    CouchFine::Mode::View& view
);








/**
* Сохранение документов в хранилище.
*/


/**
* Сохраняет документ в CouchDB.
*//* - Не используется.
inline CouchFine::Database& operator<<(
    CouchFine::Database& store,
    const CouchFine::Variant& doc
) {
    store.createDocument( doc );
    return store;
}
*/


// Декларируем этот метод, чтобы запретить неявное преобразование типов.
template< typename T >
inline CouchFine::Database& operator<<(
    CouchFine::Database& store,
    T& data
) {
    static_assert( (typename( T ) == typename( CouchFine::Object )),
        "Оборачиваем в CouchFine::Mode::*() для гибкой работы с документами в хранилище." );
    static_assert( false, "Тип не поддерживается." );
    return store;
}




CouchFine::Database& operator<<(
    CouchFine::Database& store,
    CouchFine::Mode::NewOnly& doc
);






CouchFine::Database& operator<<(
    CouchFine::Database& store,
    CouchFine::Mode::NewSkip& doc
);






CouchFine::Database& operator<<(
    CouchFine::Database& store,
    CouchFine::Mode::File& file
);






CouchFine::Database& operator<<(
    CouchFine::Database& store,
    CouchFine::Mode::NewUpdate& doc
);





#if 0
// - @todo ...
inline CouchFine::Database& operator<<(
    CouchFine::Database& store,
    const CouchFine::Mode::AppendNewOnly& doc
) {
    throw "Not implemented.";

#if 0
// - @todo Обход на всю глубину map.

    const std::string& uid = doc.o.uid();
    if ( uid.empty() ) {
        // В д. не указан UID, будет создан новый д.
        store.createDocument( doc.o );
        return store;
    }

    CouchFine::Object sDoc;
    store >> sDoc.uid( doc.o.uid() );
    if ( sDoc.empty() ) {
        // Такого д. в хранилище ещё нет, будет создан новый д.
        store.createDocument( doc.o );
        return store;
    }

    // Д. с заданным в 'doc' UID существует в хранилище. Документы
    // необходимо объединить, добавив только *новые* поля.
    // Работать будем по всей глубине документа.
    /* - Заменено.
    const auto merge = [ &sDoc ] ( const std::pair< std::string, CouchFine::Variant >&  d ) {
        const auto ftr = sDoc.find( d.first );
        if (ftr == sDoc.cend()) {
            // новое поле, добавляем
            // не позволяем заменить ревизию
            if (d.first != "_rev") {
                sDoc[ d.first ] = d.second;
            }
        } else {
            // поле уже есть, но, возможно, его содержание является объектом
            const auto& type = ftr->second->type();
            if (type == typeid( CouchFine::Object )) {
                CouchFine::Object& obj =
                    boost::any_cast< CouchFine::Object >( *ftr->second );
                // ...
            }
        }
    };
    std::for_each( doc.o.cbegin(), doc.o.cend(), merge );
    */

    Обход на всю глубину map.

    const bool updated = (sDoc.size() != doc.o.size());
    if ( updated ) {
        store.createDocument( sDoc );
    }
#endif

    return store;
}

#endif







/**
* Запись данных в CouchFine::Array.
*/
template< typename T >
inline CouchFine::Array& operator<<(
    CouchFine::Array& a,
    const T& v
) {
    /* - @todo fine Добавить проверку типов.
    static_assert( (
        (typename( T ) == typename( double ))
     || (typename( T ) == typename( float ))
     || (typename( T ) == typename( int ))
     || (typename( T ) == typename( long ))
     || (typename( T ) == typename( size_t ))
     || (typename( T ) == typename( std::string ))
     || (typename( T ) == typename( CouchFine::Array ))
     || (typename( T ) == typename( CouchFine::Object ))
     ), "Тип не поддерживается." );
    */

    a.push_back( typelib::json::cjv( v ) );
    return a;
}


} // CouchFine
