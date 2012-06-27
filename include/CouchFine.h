#pragma once

// @todo fine Убрать using namespace из global.

#include "configure.h"
#include "Mode.h"
#include "Communication.h"
#include "Connection.h"
#include "View.h"
#include "Database.h"
#include "Pool.h"


/**
* (!) Внутри design-методов должны использоваться только одинарные кавычки.
*/


// Методы работы с потоками вынесены сюда, чтобы:
// 1. Было нагляднее.
// 2. Их можно было определить как inline.


/**
* Чтение документов из хранилища.
*/

inline CouchFine::Database& operator>>(
    CouchFine::Database& store,
    CouchFine::Object& doc
) {
    const std::string& uid = boost::any_cast< std::string >( *doc[ "_id" ] );
    if ( uid.empty() ) {
        std::cerr << "ID required." << std::endl;
        throw CouchFine::Exception( "ID required." );
    }

    // Не падаем
    try {
        // @todo optimize Жирный по времени блок операций: использует parseData() 2 раза.
        const CouchFine::Document d = store.getDocument( uid );
        const auto& t = d.getData();
        doc = boost::any_cast< CouchFine::Object >( *t );

    } catch ( ... ) {
        // Ничего не нашли, пустой документ
        doc = CouchFine::Object();
    }

    return store;
}




inline CouchFine::Database& operator>>(
    CouchFine::Database& store,
    CouchFine::Mode::View& view
) {
    /* - @todo Уметь работать со всеми ключами из http://wiki.apache.org/couchdb/HTTP_view_API?action=show&redirect=HttpViewApi#Querying_Options
    // Ключи всегда передаются парой строк
    std::string key = "";
    for (auto itr = view.key.cbegin(); itr != view.key.cend(); ++itr) {
        const std::string delimiter =
            (std::distance( view.key.cbegin(), itr) == 0) ? "?" : "&";
        key += delimiter + itr->first + "=" + itr->second;
    }
    */

    const std::string key = view.key;
    std::string otherKey = view.withDoc ? "include_docs=true" : "";
    otherKey += (view.limit > 0)
        ? ( (otherKey.empty() ? "" : "&") + ("limit=" + boost::lexical_cast< std::string >( view.limit ))  )
        : "";

    // Не падаем
    view.ok = true;
    try {
        CouchFine::Object o = store.getView( view.view, view.design, key, otherKey );
        view.result = boost::any_cast< CouchFine::Array >( *o["rows"] );

    } catch ( const CouchFine::Exception& ex ) {
        // Пустой список и отметка об ошибке
        view.result = CouchFine::Array();
        view.ok = false;
        view.exception = std::shared_ptr< CouchFine::Exception >( new CouchFine::Exception( ex ) );
    }

    return store;
}








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




inline CouchFine::Database& operator<<(
    CouchFine::Database& store,
    CouchFine::Mode::NewOnly& doc
) {
    // 'doc' может быть представлен как Object или как Pool
    assert( doc.p || doc.o );

    if ( doc.p ) {
        const CouchFine::Array a = (CouchFine::Array)( *doc.p );
        store.createBulk( a, doc.fnCreateJSON );

    } else {
        store.createDocument( typelib::json::cjv( *doc.o ) );
    }

    return store;
}






inline CouchFine::Database& operator<<(
    CouchFine::Database& store,
    CouchFine::Mode::NewSkip& doc
) {
    // @info Аналогично NewUpdate, но проще и работает быстрее

    // 'doc' может быть представлен как Object или как Pool
    assert( doc.p || doc.o );

    if ( doc.p ) {
        // Сохраним скопом, д. с ошибками ревизий - пропустим
        const CouchFine::Array a = (CouchFine::Array)( *doc.p );
        const CouchFine::Array r = store.createBulk( a, doc.fnCreateJSON );
        // Ошибки игнорируем: практически всегда это "несовпадение ревизий"
        // с существующим документом
        // @todo fine Действительно игнорировать все ошибки?

    } else {
        // Переадресовываем запрос себе же - так проще
        CouchFine::Pool singleRepush;
        singleRepush.push_back( typelib::json::cjv( *doc.o ) );
        store << CouchFine::Mode::NewSkip( singleRepush, doc.fnCreateJSON );

    } // else if ( doc.p )

    return store;
}






inline CouchFine::Database& operator<<(
    CouchFine::Database& store,
    CouchFine::Mode::File& file
) {
    assert( !file.uidDoc.empty()
        && "Отсутствует UID документа, к которому следует прикрепить файл." );

    CouchFine::Document doc = store.createDocument( CouchFine::Object(), file.uidDoc );
    if ( !file.revisionDoc.empty() ) {
        doc.setRevision( file.revisionDoc );
    }

    const bool result =
        doc.addAttachment( file.name, "text/plain", file.data );
    if ( !result ) {
        throw CouchFine::Exception( "Could not create attachment '" + file.name + "' with data '" + file.data + "'." );
    }
}






inline CouchFine::Database& operator<<(
    CouchFine::Database& store,
    CouchFine::Mode::NewUpdate& doc
) {
    assert( (doc.p || doc.o)
        && "'doc' должен быть представлен или как Object, или как Pool." );

    if ( doc.p ) {
        // *Pool*
        // Сохраняем документы и файлы-вложения
        // Сохраним скопом; д. с ошибками ревизий - обновим
        CouchFine::Array& a = static_cast< CouchFine::Array& >( *doc.p );
        const CouchFine::Array result = store.createBulk( a, doc.fnCreateJSON );
        // Смотрим ошибки, обновляем UID и REV, собираем документы
        // для повторного сброса в хранилище
        CouchFine::Pool repush;
        for (auto rtr = result.cbegin(); rtr != result.cend(); ++rtr) {
            //const auto& type = itr->get()->type();

            // результат createBulk() в точности соотв. порядку отправляемых элементов
            const auto i = std::distance( result.cbegin(), rtr );
            auto& objAny = *a.at( i );
#ifdef _DEBUG
            const std::string& typeName = objAny.type().name();
#endif
            /* - Содержимое Pool не может идти в обёртке std::shared_ptr.
            CouchFine::Object* obj =
                (objAny.type() == typeid( std::shared_ptr< CouchFine::Object > ))
                ? boost::any_cast< std::shared_ptr< CouchFine::Object > >( objAny ).get()
                : boost::any_cast< CouchFine::Object* >( objAny );
            */
            assert( (objAny.type() == typeid( CouchFine::Object* ))
                && "Значение Object передано не по ссылке. Чревато трудно диагностируемыми ошибками. Используйте обёртку std::shared_ptr для передачи Object в Pool." );
            CouchFine::Object* obj = boost::any_cast< CouchFine::Object* >( objAny );

            const CouchFine::Object& ro = boost::any_cast< CouchFine::Object >( **rtr );
            if ( !CouchFine::hasError( ro ) ) {
                // Обновлённый документ получает UID и ревизию
                const CouchFine::uid_t& uid = CouchFine::uid( ro );
                const CouchFine::rev_t& rev = CouchFine::revision( ro );
                CouchFine::uid( *obj, uid, rev );
                // 'obj', увы, не ссылается на документ из 'doc.p'
                //a.at( i ) = typelib::json::cjv( obj );
                // @todo fine А если документ всегда будет возвращать ошибку? Добавить
                //       в сам документ ненавязчивый счётчик попыток записи.
                continue;
            }

            // Ошибочный документ (без ревизии) надо будет отправить повторно
#ifdef _DEBUG
            std::cerr << CouchFine::error( ro ) << std::endl;
#endif

            // Получим ревизию существующего документа
            const auto uid = CouchFine::uid( *obj );
            assert( !uid.empty()
                && "Для документа без UID ревизия не может быть получена." );
            CouchFine::Object sObj;
            store >> CouchFine::uid( sObj, uid );
            if ( CouchFine::hasError( sObj ) ) {
                // Такого д. в хранилище ещё нет ?!
                std::cerr << "CouchFine::operator<<( Database, NewUpdate ) " << CouchFine::error( sObj ) << std::endl;
                throw CouchFine::Exception( "Unrecognized exception: " + CouchFine::error( sObj ) );
            }

            // Д. с заданным в 'doc' UID существует в хранилище, обновим его
            const CouchFine::rev_t& rev = CouchFine::revision( sObj );
            CouchFine::revision( *obj, rev );
            repush << obj;

        } // for (auto itr = r.cbegin(); itr != r.cend(); ++itr)

        // Сохраняем повторно (обновляем)
        if ( !repush.empty() ) {
            store << CouchFine::Mode::NewUpdate( repush, doc.fnCreateJSON );
            // @todo fine Отлавливать бесконечные циклы.
        }

    } else {
        // *Object*
        // Переадресовываем запрос себе же - так проще...
        CouchFine::Pool singleRepush;
        singleRepush.push_back( typelib::json::cjv( *doc.o ) );
        store << CouchFine::Mode::NewUpdate( singleRepush, doc.fnCreateJSON );

        // ...Но помним: ждут обновления объекта
            const auto& objAny = *singleRepush.front();
            const CouchFine::Object& ro = boost::any_cast< CouchFine::Object >( objAny );
            if ( CouchFine::hasError( ro ) ) {
                std::cerr << "CouchFine::operator<<( Database, NewUpdate ) " << CouchFine::error( ro ) << std::endl;
                throw CouchFine::Exception( "Unrecognized exception: " + CouchFine::error( ro ) );
            }
            // Обновлённый документ получает UID и ревизию
            const CouchFine::uid_t& uid = CouchFine::uid( ro );
            const CouchFine::rev_t& rev = CouchFine::revision( ro );
            CouchFine::uid( *doc.o, uid, rev );
            // @todo fine А если документ всегда будет возвращать ошибку? Добавить
            //       в сам документ ненавязчивый счётчик попыток записи.

    } // else if ( doc.p )

    return store;
}





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
