#include "../include/CouchFine.h"


namespace CouchFine {


Database& operator>>(
    Database& store,
    Object& doc
) {
    const std::string& uid = boost::any_cast< std::string >( *doc[ "_id" ] );
    if ( uid.empty() ) {
        std::cerr << "ID required." << std::endl;
        throw Exception( "ID required." );
    }

    // Не падаем
    try {
        // @todo optimize Жирный по времени блок операций: использует parseData() 2 раза.
        const Document d = store.getDocument( uid );
        const auto& t = d.getData();
        doc = boost::any_cast< Object >( *t );

    } catch ( ... ) {
        // Ничего не нашли, пустой документ
        doc = Object();
    }

    return store;
}




Database& operator>>(
    Database& store,
    Mode::Doc& doc
) {
    // Используем умение CouchDB вернуть все документа с заданными UID
    // @see http://wiki.apache.org/couchdb/HTTP_view_API#Querying_Options / keys

    // Получаем документы из хранилища
    // Формируем POST-запрос в формате JSON
    // @source http://wiki.apache.org/couchdb/HTTP_Bulk_Document_API
    const std::string url = "/" + store.getName() + "/_all_docs?include_docs=true";
    std::ostringstream ss;
    ss << "{\"keys\":";
        typelib::print( ss, doc.uid, "[", "]", "\"", "," );
    ss << "}";

    // Не падаем
    doc.ok = true;
    try {
        typelib::json::Variant var =
            store.getCommunication().getData( url, "POST", ss.str() );
        typelib::json::Object o = static_cast< typelib::json::Object >( var );
        // здесь будет кол-во всех документов в хранилище
        doc.totalRows = static_cast< size_t >( o["total_rows"] );
        // здесь - те, которые мы запросили в по ключу 'keys'
        doc.result = static_cast< Array >( o["rows"] );

    } catch ( const Exception& ex ) {
        // Пустой список и отметка об ошибке
        doc.totalRows = 0;
        doc.result = Array();
        doc.ok = false;
        doc.exception = std::shared_ptr< Exception >( new Exception( ex ) );
    }

    return store;
}






Database& operator>>(
    Database& store,
    Mode::View& view
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

    std::string key = view.key;
    if ( view.withDoc ) {
        key += "&include_docs=true";
    }
    if (view.limit > 0) {
        key += "&limit=" + boost::lexical_cast< std::string >( view.limit );
    }
    if (key[0] == '&') {
        key = key.substr( 1 );
    }

    // Не падаем
    view.ok = true;
    try {
        Object o = store.getView( view.view, view.design, key );
        // (!) При использовании ключей limit / offset, значение 'totalRows'
        // не совпадает с 'result.count()'
        view.totalRows = static_cast< size_t >( o["total_rows"] );
        view.result = static_cast< Array >( o["rows"] );

    } catch ( const Exception& ex ) {
        // Пустой список и отметка об ошибке
        view.totalRows = 0;
        view.result = Array();
        view.ok = false;
        view.exception = std::shared_ptr< Exception >( new Exception( ex ) );
    }

    return store;
}






Database& operator<<(
    Database& store,
    Mode::NewOnly& doc
) {
    // 'doc' может быть представлен как Object или как Pool
    assert( doc.p || doc.o );

    if ( doc.p ) {
        const Array a = (Array)( *doc.p );
        store.createBulk( a, doc.fnCreateJSON );

    } else {
        store.createDocument( typelib::json::cjv( *doc.o ) );
    }

    return store;
}






Database& operator<<(
    Database& store,
    Mode::NewSkip& doc
) {
    // @info Аналогично NewUpdate, но проще и работает быстрее

    // 'doc' может быть представлен как Object или как Pool
    assert( doc.p || doc.o );

    if ( doc.p ) {
        // Сохраним скопом, д. с ошибками ревизий - пропустим
        const Array a = (Array)( *doc.p );
        const Array r = store.createBulk( a, doc.fnCreateJSON );
        // Ошибки игнорируем: практически всегда это "несовпадение ревизий"
        // с существующим документом
        // @todo fine Действительно игнорировать все ошибки?

    } else {
        // Переадресовываем запрос себе же - так проще
        Pool singleRepush;
        singleRepush.push_back( typelib::json::cjv( *doc.o ) );
        store << Mode::NewSkip( singleRepush, doc.fnCreateJSON );

    } // else if ( doc.p )

    return store;
}






Database& operator<<(
    Database& store,
    Mode::File& file
) {
    assert( !file.uidDoc.empty()
        && "Отсутствует UID документа, к которому следует прикрепить файл." );

    Document doc = store.createDocument( Object(), file.uidDoc );
    if ( !file.revisionDoc.empty() ) {
        doc.setRevision( file.revisionDoc );
    }

    const bool result =
        doc.addAttachment( file.name, "text/plain", file.data );
    if ( !result ) {
        throw Exception( "Could not create attachment '" + file.name + "' with data '" + file.data + "'." );
    }

    return store;
}






Database& operator<<(
    Database& store,
    Mode::NewUpdate& doc
) {
    assert( (doc.p || doc.o)
        && "'doc' должен быть представлен или как Object, или как Pool." );

    if ( doc.p ) {
        // *Pool*
        // Сохраняем документы и файлы-вложения
        // Сохраним скопом; д. с ошибками ревизий - обновим
        Array& a = static_cast< Array& >( *doc.p );
        // #! @todo Здесь выбрасывается ошибка?
        //    1. doc не должен быть const.
        //    2. doc должен состоять из typelib::json::variant.
        const Array result = store.createBulk( a, doc.fnCreateJSON );
        // Смотрим ошибки, обновляем UID и REV, собираем документы
        // для повторного сброса в хранилище
        Pool repush;
        for (auto rtr = result.cbegin(); rtr != result.cend(); ++rtr) {
            //const auto& type = itr->get()->type();

            // результат createBulk() в точности соотв. порядку отправляемых элементов
            const auto i = std::distance( result.cbegin(), rtr );
            auto& objAny = *a.at( i );
#ifdef _DEBUG
            const std::string& typeName = objAny.type().name();
#endif
            /* - Содержимое Pool не может идти в обёртке std::shared_ptr.
            Object* obj =
                (objAny.type() == typeid( std::shared_ptr< Object > ))
                ? boost::any_cast< std::shared_ptr< Object > >( objAny ).get()
                : boost::any_cast< Object* >( objAny );
            */
            assert( (objAny.type() == typeid( Object* ))
                && "Значение Object передано не по ссылке. Чревато трудно диагностируемыми ошибками. Используйте обёртку std::shared_ptr для передачи Object в Pool." );
            Object* obj = boost::any_cast< Object* >( objAny );

            const Object& ro = boost::any_cast< Object >( **rtr );
            if ( !hasError( ro ) ) {
                // Обновлённый документ получает UID и ревизию
                const uid_t& uid = CouchFine::uid( ro );
                const rev_t& rev = revision( ro );
                CouchFine::uid( *obj, uid, rev );
                // 'obj', увы, не ссылается на документ из 'doc.p'
                //a.at( i ) = typelib::json::cjv( obj );
                // @todo fine А если документ всегда будет возвращать ошибку? Добавить
                //       в сам документ ненавязчивый счётчик попыток записи.
                continue;
            }

            // Ошибочный документ (без ревизии) надо будет отправить повторно
#ifdef _DEBUG
            std::cerr << error( ro ) << std::endl;
#endif

            // Получим ревизию существующего документа
            const auto uid = CouchFine::uid( *obj );
            assert( !uid.empty()
                && "Для документа без UID ревизия не может быть получена." );
            Object sObj;
            store >> CouchFine::uid( sObj, uid );
            if ( hasError( sObj ) ) {
                // Такого д. в хранилище ещё нет ?!
                std::cerr << "operator<<( Database, NewUpdate ) " << error( sObj ) << std::endl;
                throw Exception( "Unrecognized exception: " + error( sObj ) );
            }

            // Д. с заданным в 'doc' UID существует в хранилище, обновим его
            const rev_t& rev = revision( sObj );
            revision( *obj, rev );
            repush << obj;

        } // for (auto itr = r.cbegin(); itr != r.cend(); ++itr)

        // Сохраняем повторно (обновляем)
        if ( !repush.empty() ) {
            store << Mode::NewUpdate( repush, doc.fnCreateJSON );
            // @todo fine Отлавливать бесконечные циклы.
        }

    } else {
        // *Object*
        // Переадресовываем запрос себе же - так проще...
        Pool singleRepush;
        singleRepush.push_back( typelib::json::cjv( *doc.o ) );
        store << Mode::NewUpdate( singleRepush, doc.fnCreateJSON );

        // ...Но помним: ждут обновления объекта
            const auto& objAny = *singleRepush.front();
            const Object& ro = boost::any_cast< Object >( objAny );
            if ( hasError( ro ) ) {
                std::cerr << "operator<<( Database, NewUpdate ) " << error( ro ) << std::endl;
                throw Exception( "Unrecognized exception: " + error( ro ) );
            }
            // Обновлённый документ получает UID и ревизию
            const uid_t& uid = CouchFine::uid( ro );
            const rev_t& rev = revision( ro );
            CouchFine::uid( *doc.o, uid, rev );
            // @todo fine А если документ всегда будет возвращать ошибку? Добавить
            //       в сам документ ненавязчивый счётчик попыток записи.

    } // else if ( doc.p )

    return store;
}



} // CouchFine
