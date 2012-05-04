#pragma once

/**
* Include this header in your project.
*/

#include "configure.h"
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


namespace CouchDB {

/**
* Класс определяет разные режимы работы с записями CouchDb.
*
* (!) Не является контейнером для документов: содержит только ссылку на объект.
*/
struct Mode {
private:
    /**
    * Базовая структура для сохранения объектов в хранилище.
    * Может принимать объект или пул объектов.
    *
    * @see Pool
    */
    struct Save {
        const Object* const o;
        const Pool* const p;

        inline Save( const Object& o ) :
            o( &o ), p( nullptr )
        {
        };

        inline Save( const Pool& p ) :
            o( nullptr ), p( &p )
        {
        };

        /* - Определяем в operator<<()
        inline Save( const T& o ) : o( o ) {};
        inline void save( Database& store ) const {
            static_assert( (
                (type == typename( Pool ))
             || (type == typename( Object ))
            ) && "Type not recognized." );
            const auto& type = typename( T );
            if (type == typename( Pool )) {
                saveMany( store );
            } else if (type == typename( Object )) {
                saveSingle( store );
            }
            throw "Type not recognized.";
        }

    private:
        inline void saveMany( Database& store ) const {
            store->createBulk( (Array)o );
        }
        inline void saveSingle( Database& store ) const {
            store.createDocument( cjv( o ) );
        }
        */
    };


    /**
    * Базовая структура для получения объектов из хранилища.
    */
    struct Load {
        // Название design-документа
        const std::string design;
        // Название представления к которому следует сделать запрос
        const std::string view;
        // Строковой ключ для выборки из представления
        // @see http://wiki.apache.org/couchdb/HTTP_view_API?action=show&redirect=HttpViewApi
        // @see http://wiki.apache.org/couchdb/View_collation
        const std::string key;

        // Признак: вместе с результатом возвращать связанный с представлением документ
        const bool withDoc;

        // Максимальное количество возвращаемых результатов. Помогает
        // снизить нагрузку при большом кол-ве документов в выборке.
        const size_t limit;

        // Результат
        Array result;
        bool ok;
        std::shared_ptr< Exception >  exception;

        inline Load( const std::string& design, const std::string& view, const std::string& key, bool withDoc, size_t limit ) :
            design( design ), view( view ), key( key ),
            withDoc( withDoc ),
            limit( limit ),
            ok( false ), exception( nullptr )
        {
        };

    };


public:
    /**
    * Сохраняется только новый документ.
    * Если UID д. указан и оказывается, что д. с таким UID существует,
    * выбрасывается исключение.
    * Если UID д. не указан, создаётся новый документ.
    */
    struct NewOnly : public Save {
        inline NewOnly( const Object& o ) : Save( o ) {};
        inline NewOnly( const Pool&   p ) : Save( p ) {};
    };

    /**
    * Сохраняется новый документ, но не обновляется существующий.
    * Ревизия документа не учитывается, только UID.
    * Если UID д. не указан, создаётся новый документ.
    */
    struct NewSkip : public Save {
        inline NewSkip( const Object& o ) : Save( o ) {};
        inline NewSkip( const Pool&   p ) : Save( p ) {};
    };

    /** - @todo Собирать индекс отправленных серверу UID, отправлять только новые.
    * То же, что NewSkip, но работает быстрее, когда количество повторяющихся
    * документов сравнимо или превышает кол-во добавляемых.
    *
    struct NewSkipMany : public Save {
        inline NewSkipMany( const Object& o ) : Save( o ) {};
        inline NewSkipMany( const Pool&   p ) : Save( p ) {};
    };
    */

    /**
    * Сохраняется новый документ или обновляется существующий.
    * Ревизия документа не учитывается, только UID.
    * Если UID д. не указан, создаётся новый документ.
    */
    struct NewUpdate : public Save {
        inline NewUpdate( const Object& o ) : Save( o ) {};
        inline NewUpdate( const Pool&   p ) : Save( p ) {};
    };



    /**
    * Обращается к представлению для получения списка документов.
    *
    * @param withDoc Вместе с результатом возвращает связанные с
    *        представлением документы.
    */
    struct View : public Load {
        inline View(
            const std::string& design,
            const std::string& view,
            const std::string& key,
            bool withDoc,
            size_t limit = 0
        ) : Load( design, view, key, withDoc, limit ) {
            assert ( !view.empty() && "Название представления должно быть указано." );
        };
    };



#if 0
// - @todo ...
    /**
    * Пары "ключ-значение" добавляются к существующему документу.
    * Если ключи совпадают, данные по ним *не перезаписываются*.
    * Если самого документа ещё нет (проверяется по UID), создаётся новый д..
    * Если UID д. не указан, создаётся новый д..
    */
    struct AppendNewOnly {
        const CouchDB::Object& o;
        inline AppendNewOnly( const CouchDB::Object& o ) : o( o ) {};
    };

    /**
    * Пары "ключ-значение" добавляются к существующему документу.
    * Если ключи совпадают, данные по ним *обновляются*.
    * Если самого документа ещё нет (проверяется по UID), создаётся новый д..
    * Если UID д. не указан, создаётся новый д..
    */
    struct AppendUpdate {
        const CouchDB::Object& o;
        inline AppendUpdate( const CouchDB::Object& o ) : o( o ) {};
    };

    // @todo ...
#endif
};


} // namespace CouchDB










/**
* Чтение документов из хранилища.
*/

inline CouchDB::Database& operator>>(
    CouchDB::Database& store,
    CouchDB::Object& doc
) {
    const std::string& uid = boost::any_cast< std::string >( *doc[ "_id" ] );
    if ( uid.empty() ) {
        // @todo extend Искать по полям документа?
        throw CouchDB::Exception( "ID required." );
    }

    // Не падаем
    try {
        // @todo optimize Жирный по времени блок операций: использует parseData() 2 раза.
        const CouchDB::Document d = store.getDocument( uid );
        const auto& t = d.getData();
        doc = boost::any_cast< CouchDB::Object >( *t );

    } catch ( ... ) {
        // Ничего не нашли, пустой документ
        doc = CouchDB::Object();
    }

    return store;
}




inline CouchDB::Database& operator>>(
    CouchDB::Database& store,
    CouchDB::Mode::View& view
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
        CouchDB::Object o = store.getView( view.view, view.design, key, otherKey );
        view.result = boost::any_cast< CouchDB::Array >( *o["rows"] );

    } catch ( const CouchDB::Exception& ex ) {
        // Пустой список и отметка об ошибке
        view.result = CouchDB::Array();
        view.ok = false;
        view.exception = std::shared_ptr< CouchDB::Exception >( new CouchDB::Exception( ex ) );
    }

    return store;
}








/**
* Сохранение документов в хранилище.
*/


/**
* Сохраняет документ в CouchDB.
*//* - Не используется.
inline CouchDB::Database& operator<<(
    CouchDB::Database& store,
    const CouchDB::Variant& doc
) {
    store.createDocument( doc );
    return store;
}
*/


// Декларируем этот метод, чтобы запретить неявное преобразование типов.
template< typename T >
inline CouchDB::Database& operator<<(
    CouchDB::Database& store,
    const T& data
) {
    static_assert( (typename( T ) == typename( CouchDB::Object )),
        "Оборачиваем в CouchDB::Mode::*() для гибкой работы с документами в хранилище." );
    static_assert( false, "Тип не поддерживается." );
    return store;
}




inline CouchDB::Database& operator<<(
    CouchDB::Database& store,
    const CouchDB::Mode::NewOnly& doc
) {
    // 'doc' может быть представлен как Object или как Pool( Object )
    assert( doc.p || doc.o );

    if ( doc.p ) {
        const CouchDB::Array a = (CouchDB::Array)( *doc.p );
        store.createBulk( a );

    } else {
        store.createDocument( CouchDB::cjv( *doc.o ) );
    }

    return store;
}






inline CouchDB::Database& operator<<(
    CouchDB::Database& store,
    const CouchDB::Mode::NewSkip& doc
) {
    // @info Аналогично NewUpdate, но проще (и работает быстрее)

    // 'doc' может быть представлен как Object или как Pool( Object )
    assert( doc.p || doc.o );

    if ( doc.p ) {
        // Сохраним скопом, д. с ошибками ревизий - пропустим
        const CouchDB::Array a = (CouchDB::Array)( *doc.p );
        const CouchDB::Array r = store.createBulk( a );
        // Ошибки игнорируем: практически всегда это "несовпадение ревизий"
        // с существующим документом
        // @todo fine Действительно игнорировать все ошибки?

    } else {
        // Переадресовываем запрос себе же - так проще
        CouchDB::Pool singleRepush;
        singleRepush.push_back( CouchDB::cjv( *doc.o ) );
        store << CouchDB::Mode::NewSkip( singleRepush );

    } // else if ( doc.p )

    return store;
}




inline CouchDB::Database& operator<<(
    CouchDB::Database& store,
    const CouchDB::Mode::NewUpdate& doc
) {
    // 'doc' может быть представлен как Object или как Pool( Object )
    assert( doc.p || doc.o );

    if ( doc.p ) {
        // Сохраним скопом, д. с ошибками ревизий - обновим
        const CouchDB::Array a = (CouchDB::Array)( *doc.p );
        const CouchDB::Array r = store.createBulk( a );
        // Смотрим ошибки, собираем документы для повторного сброса
        // в хранилище
        CouchDB::Pool repush;
        for (auto itr = r.cbegin(); itr != r.cend(); ++itr) {
            const auto& type = itr->get()->type();
            /* - Работаем быстро, без обёртки.
            CouchDB::Object convertObj;
            if (type == typeid( CouchDB::Object )) {
                // @todo optimize Сразу конвертировать в Object. Почему не получается?
                convertObj = CouchDB::Object( boost::any_cast< CouchDB::Object >( **itr ) );

            } else if (type == typeid( CouchDB::Object )) {
                convertObj = boost::any_cast< CouchDB::Object >( **itr );

            } else {
                const std::string tn = type.name();
                throw CouchDB::Exception( "Unrecognized type: " + tn );
            }

            if ( !convertObj.hasError() ) {
                // документ успешно сохранён (возвращено UID д.)
                continue;
            }
            */
            const CouchDB::Object& o = boost::any_cast< CouchDB::Object >( **itr );

            // ошибка
            if ( CouchDB::hasError( o ) ) {
                std::cerr << CouchDB::error( o ) << std::endl;
            }

            // результат createBulk() в точности соотв. порядку отправляемых элементов
            const auto i = std::distance( r.cbegin(), itr );

            // Обновим существующий документ
            const auto& objAny = *a.at( i );
            const CouchDB::Object& obj = boost::any_cast< CouchDB::Object >( obj );
            const std::string uid = CouchDB::uid( obj );
            CouchDB::Object sObj;
            store >> CouchDB::uid( sObj, uid );
            if ( sObj.empty() ) {
                // Такого д. в хранилище ещё нет: вероятно, раньше
                // случилась неизвестная ошибка
                throw CouchDB::Exception( "Unrecognized exception: " + CouchDB::error( obj ) );
            }

            // Д. с заданным в 'doc' UID существует в хранилище, обновим его
            assert( !CouchDB::rev( sObj ).empty() && "Ревизия не должна быть пустой." );
            repush << sObj;

        } // for (auto itr = r.cbegin(); itr != r.cend(); ++itr)

        // Сохраняем документы повторно (обновляем)
        if ( !repush.empty() ) {
            store << CouchDB::Mode::NewUpdate( repush );
            // @todo fine Отлавливать бесконечные циклы.
        }

    } else {
        // Переадресовываем запрос себе же - так проще
        CouchDB::Pool singleRepush;
        singleRepush.push_back( CouchDB::cjv( *doc.o ) );
        store << CouchDB::Mode::NewUpdate( singleRepush );

    } // else if ( doc.p )

    return store;
}





#if 0
// - @todo ...
inline CouchDB::Database& operator<<(
    CouchDB::Database& store,
    const CouchDB::Mode::AppendNewOnly& doc
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

    CouchDB::Object sDoc;
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
    const auto merge = [ &sDoc ] ( const std::pair< std::string, CouchDB::Variant >&  d ) {
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
            if (type == typeid( CouchDB::Object )) {
                CouchDB::Object& obj =
                    boost::any_cast< CouchDB::Object >( *ftr->second );
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
* Запись данных в CouchDB::Array.
*/
template< typename T >
inline CouchDB::Array& operator<<(
    CouchDB::Array& a,
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
     || (typename( T ) == typename( CouchDB::Array ))
     || (typename( T ) == typename( CouchDB::Object ))
     ), "Тип не поддерживается." );
    */

    a.push_back( CouchDB::cjv( v ) );
    return a;
}
