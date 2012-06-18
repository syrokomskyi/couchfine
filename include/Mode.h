#pragma once

#include "configure.h"
#include "type.h"
#include "Pool.h"
#include "Exception.h"


namespace CouchFine {

/**
* Класс определяет режимы работы с записями CouchDB.
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
        Object* const o;
        Pool* const p;
        const fnCreateJSON_t fnCreateJSON;

        inline Save( Object& o, fnCreateJSON_t fnCreateJSON ) :
            o( &o ), p( nullptr ), fnCreateJSON( fnCreateJSON )
        {
        };

        inline Save( Pool& p, fnCreateJSON_t fnCreateJSON ) :
            o( nullptr ), p( &p ), fnCreateJSON( fnCreateJSON )
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
        inline NewOnly( Object& o, fnCreateJSON_t fnCreateJSON ) : Save( o, fnCreateJSON ) {};
        inline NewOnly( Pool& p, fnCreateJSON_t fnCreateJSON ) : Save( p, fnCreateJSON ) {};
    };

    /**
    * Сохраняется новый документ, но не обновляется существующий.
    * Ревизия документа не учитывается, только UID.
    * Если UID д. не указан, создаётся новый документ.
    */
    struct NewSkip : public Save {
        inline NewSkip( Object& o, fnCreateJSON_t fnCreateJSON ) : Save( o, fnCreateJSON ) {};
        inline NewSkip( Pool& p, fnCreateJSON_t fnCreateJSON ) : Save( p, fnCreateJSON ) {};
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
    * Сохранённый документ получает UID и REV для последующего быстрого
    * доступа к нему.
    */
    struct NewUpdate : public Save {
        inline NewUpdate( Object& o, fnCreateJSON_t fnCreateJSON ) : Save( o, fnCreateJSON ) {};
        inline NewUpdate( Pool& p, fnCreateJSON_t fnCreateJSON ) : Save( p, fnCreateJSON ) {};
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
        const CouchFine::Object& o;
        inline AppendNewOnly( const CouchFine::Object& o ) : o( o ) {};
    };

    /**
    * Пары "ключ-значение" добавляются к существующему документу.
    * Если ключи совпадают, данные по ним *обновляются*.
    * Если самого документа ещё нет (проверяется по UID), создаётся новый д..
    * Если UID д. не указан, создаётся новый д..
    */
    struct AppendUpdate {
        const CouchFine::Object& o;
        inline AppendUpdate( const CouchFine::Object& o ) : o( o ) {};
    };

    // @todo ...
#endif




    /**
    * Структура для добавления к документу приложения в виде файла.
    */
    struct File {
        /**
        * Название и содержание файла.
        */
        const std::string name;
        const std::string data;

        /**
        * UID и ревизия документа, к которому добавляется файл.
        */
        const uid_t uidDoc;
        const rev_t revisionDoc;

        /**
        * База данных
        */

        inline File(
            const std::string& name, const std::string& data,
            const uid_t& uidDoc, const rev_t& revisionDoc
        ) :
            name( name ), data( data ),
            uidDoc( uidDoc ), revisionDoc( revisionDoc )
        {
        };

        /**
        * Префикс, по которому определяется необходимость прикреплять файл
        * вместо поля документа.
        */
        static std::string const& PREFIX() {
            const static std::string r = "file://";
            return r;
        }

    };



};


} // namespace CouchFine
