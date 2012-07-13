#include "../include/Mode.h"
#include "../include/Database.h"
#include "../include/Exception.h"
#include <typelib/typelib.h>


using namespace CouchFine;


/**
* (!) Внутри json-данных должны быть только одинарные кавычки. Иначе CouchDB
* откажется принять документ.
*/
static std::string createJSON( const Variant &data ) {
   std::ostringstream ostr;
   ostr << data;
   return ostr.str();
}



Database::Database(Communication &_comm, const std::string& _name)
   : comm(_comm)
   , name(_name)
{
}



Database::Database(const Database &db)
   : comm(db.comm)
   , name(db.name)
{
}



Database::~Database() {
}




Object Database::about() const {
   const Variant var = comm.getData( "/" + name );
   const Object obj = boost::any_cast< Object >( *var );
   return obj;
}




size_t Database::countDoc() const {
    Object a = about();
    //const size_t n = boost::any_cast< size_t >( *a["doc_count"] );
    auto f = a.find( "doc_count" );
    if (f != a.cend()) {
        const int n = boost::any_cast< int >( *f->second );
        return (size_t)n;
    }
    throw "Info not loaded.";
}



size_t Database::countDesign() const {
    const Variant var = comm.getData( "/" + name + "/_all_docs?startkey=\"_design/a\"&endkey=\"_design/z\"" );
    const Object obj = boost::any_cast< Object >( *var );
    const Array rows = boost::any_cast< Array >( *obj.at( "rows" ) );
    return rows.size();
}



std::vector< Document >  Database::listDocuments() {
   const Variant var = comm.getData( "/" + name + "/_all_docs" );
   const Object obj = boost::any_cast< Object >( *var );

   const int numRows = boost::any_cast< int >( *obj.at( "total_rows" ) );

   std::vector< Document >  docs;
   if (numRows > 0) {
      Array rows = boost::any_cast< Array >( *obj.at( "rows" ) );
      for (auto itr = rows.cbegin(); itr != rows.cend(); ++itr) {
         const Object docObj = boost::any_cast< Object >( **itr );
         const Object values = boost::any_cast< Object >( *docObj.at( "value" ) );
         const Document doc(
             comm, name,
             CouchFine::uid( docObj ),
             boost::any_cast< std::string >( *docObj.at( "key" ) ),
             CouchFine::revision( values )
         );
         docs.push_back( doc );
      }
   }

   return docs;
}



Document Database::getDocument( const uid_t& id, const rev_t& rev ) {

    const std::string url = "/" + name + "/" + id + ( rev.empty() ? "" : ("?rev=" + rev) );

    // (!) Целые числа хранятся как знаковый тип, при превышении лимита - исключение
    const Variant var = comm.getData( url );
    const Object obj = boost::any_cast< Object >( *var );
    if ( hasError( obj ) ) {
        throw Exception("Document " + id + " (v" + rev + ") not found: " + error( obj ) );
    }

    return Document(
        comm,
        name,
        CouchFine::uid( obj ),
        "", // no key returned here
        CouchFine::revision( obj )
    );
}






CouchFine::Object Database::getView(
    const std::string& viewName,
    const std::string& designName,
    const std::string& key
) {
    const std::string designUID = getDesignUID( designName );
    std::string url = "/" + name + "/" + designUID + "/_view/" + viewName;
    if ( !key.empty() ) {
        /* - Не обязательно. Опущено.
        // перекодируем ключ
        std::string k = key;
        boost::replace_all( k, "\"", "%22" );
        boost::replace_all( k, "[", "%5B" );
        boost::replace_all( k, "]", "%5D" );
        url += "?" + k;
        */
        url += "?" + key;
    }


#if 0
    // @test 1 Мучения с кодировками, ох...
    url = "/" + name + "/" + designUID + "/_view/" + viewName
        //+ "?startkey=[\"rus\"]&endkey=[\"rusабвгдеёжзийклмнопрстуфхцчшщъыьэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ\",{}]";
        + "?startkey=[\"rus\"]&endkey=[\"rus\",{}]";
    url = typelib::convert::codepage::cyrillicEncode( url );
    boost::replace_all( url, "\"", "%22" );
    boost::replace_all( url, "[", "%5B" );
    boost::replace_all( url, "]", "%5D" );
#endif

#if 0
    // @test 2 Мучения с кодировками, ух...
    const std::string name = "абвгдеёжзийклмнопрстуфхцчшщъыьэюя АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ";
    //const std::string name = "abcd ABCD Ф";
    const std::string nameCyrEnc = typelib::convert::codepage::cyrillicEncode( name );
    const std::string hashName = typelib::hash::sha1( name );
    const std::string hashNameCyrEnc = typelib::hash::sha1( nameCyrEnc );

    //@result С кириллицей подружить не удалось. Сравниваем через SHA1 и
    //        cyrillicEncode на стороне CouchDB.
    //        Для С++ - см. методы hash::searchHash() или Name::searchHash().
#endif


    const Variant var = comm.getData( url );
    const Object obj = boost::any_cast< Object >( *var );
    if ( hasError( obj ) ) {
        throw Exception( "View '" + viewName + "': " + error( obj ) );
    }

        return obj;
}






std::vector< std::string >  Database::getUUIDs( size_t n ) const {

    const Variant var = comm.getData( "/_uuids?count=100" );
    const Object obj = boost::any_cast< Object >( *var );
    if ( hasError( obj ) ) {
        throw Exception( "Set of ID's is not created: " + error( obj ) );
    }

    // Переводим в std::vector для более быстрой работы со списком
    std::vector< std::string >  uuids;
    const auto a = boost::any_cast< Array >( *obj.at( "uuids" ) );
    for (auto itr = a.cbegin(); itr != a.cend(); ++itr) {
        const Variant v = *itr;
        const auto id = boost::any_cast< std::string >( *v );
        uuids.push_back( id );
    }

    return uuids;
}





Document Database::createDocument( const Object& obj, const std::string& id ) const {
   return createDocument( typelib::json::cjv( obj ),  std::vector< Attachment >(),  id );
}




Document Database::createDocument( const Variant& data, const std::string& id ) const {
   return createDocument( data,  std::vector< Attachment >(),  id );
}




Document Database::createDocument(
    Variant data,
    const std::vector< Attachment >& attachments,
    const std::string& id
) const {
    if ( !attachments.empty() ) {
        Object attachmentObj;
        for (auto attachment = attachments.cbegin(); attachment != attachments.cend(); ++attachment) {
           Object attachmentData;
           attachmentData["content_type"] = typelib::json::cjv( attachment->getContentType() );
           attachmentData["data"        ] = typelib::json::cjv( attachment->getData() );
           attachmentObj[ attachment->getID() ] = typelib::json::cjv( attachmentData );
        }

        Object obj = boost::any_cast< Object >( *data );
        obj["_attachments"] = typelib::json::cjv( attachmentObj );
        data = typelib::json::cjv( obj );
    }

    const std::string json = createJSON( data );
    const auto doc = createDocument( json, id );

    return doc;
}





Document Database::createDocument( const std::string& json, const std::string& id ) const {

    const Variant var = id.empty()
        ? comm.getData( "/" + name + "/",      "POST", json )
        : comm.getData( "/" + name + "/" + id, "PUT",  json );
        
    const Object obj = boost::any_cast< Object >( *var );
    if ( hasError( obj ) ) {
       throw Exception( "Document could not be created: " + error( obj ) );
    }

    return Document(
        comm, name,
        CouchFine::uid( obj ),
        "", // no key returned here
        CouchFine::revision( obj )
    );
}





CouchFine::Array Database::createBulk(
    const CouchFine::Array& docs,
    CouchFine::fnCreateJSON_t fnCreateJSON
) {
    // I. Сохраним документы.

    // Исключим из списка документов поля, начинающиеся с Mode::File::PREFIX
    // @todo optimize Делается копия 'docs'. Просадка производительности.
    Array preparedDocs;
    for (auto itr = docs.cbegin(); itr != docs.cend(); ++itr) {
#ifdef _DEBUG
        const std::string& typeName = (*itr)->type().name();
#endif
        const Object* d = boost::any_cast< Object* >( **itr );
        // просматриваем поля, ставляем те, что без Mode::File::PREFIX
        Object obj;
        for (auto dtr = d->cbegin(); dtr != d->cend(); ++dtr) {
            const std::string& field = dtr->first;
            if ( !boost::starts_with( field, Mode::File::PREFIX() ) ) {
                // @todo optimize Собирать только ссылки?
                obj[ field ] = dtr->second;
            }
        }

        preparedDocs.push_back( typelib::json::cjv( obj ) );

    } // for (auto itr = docs.cbegin(); itr != docs.cend(); ++itr)


    // @see http://wiki.apache.org/couchdb/HTTP_Bulk_Document_API#Modify_Multiple_Documents_With_a_Single_Request
    Object o;
    //o["docs"] = typelib::json::cjv( docs );
    o["docs"] = typelib::json::cjv( preparedDocs );
    const std::string json = fnCreateJSON
        ? ( fnCreateJSON )( typelib::json::cjv( o ) )
        : createJSON( typelib::json::cjv( o ) );
    //std::cout << std::endl << typelib::json::cjv( o ) << std::endl << std::endl;

    const Variant var = comm.getData( "/" + name + "/_bulk_docs",  "POST",  json );
    if ( hasError( var ) ) {
        std::cerr << "JSON: " << json << std::endl;
        std::cerr << "CouchFine::operator<<( Database, NewUpdate ) " << error( var ) << std::endl;
        throw CouchFine::Exception( "Unrecognized exception: " + error( var ) );
    }

    //std::cout << std::endl << var << std::endl << std::endl;
    /* - Быстрее. Проще. Обходимся.
    transformObject( &var );
    */

    // @todo fine Если сбой на сервере, вместо списка с ошибками может
    //       быть возвращён Object?
    /* - Лучше вернём результат для анализа: позволит вызвавшему методу
         самому решить, что делать с ошибками (часто - несовпадение ревизий).
    // При удачном завершении (см. catch ниже) получим список ключей CouchFine::Array
    const CouchFine::Array a = boost::any_cast< CouchFine::Array >( *var );
    // При неудачном - вместо ключей получим объекты с информ. об ошибках
    for (auto itr = a.cbegin(); itr != a.cend(); ++itr) {
        const auto& type = itr->get()->type();
        if (type == typeid( Object )) {
            // ошибка, однозначно
            std::cerr << typelib::json::cjv( a );
            const CouchFine::Object obj = boost::any_cast< CouchFine::Object >( *itr );
            throw Exception( "Part of set of documents could not be created. First error: " + obj.error() );
        }
    }
    */

    const Array ra = boost::any_cast< Array >( *var );


    // II. Сохраняем файлы-вложения.
    // @todo Позволить сохранять разные типы данных, не только plain/text.
    // @todo optimize Сохранять поля документов и файлы в один запрос.
    for (auto itr = docs.cbegin(); itr != docs.cend(); ++itr) {
        const Object* d = boost::any_cast< Object* >( **itr );
        for (auto dtr = d->cbegin(); dtr != d->cend(); ++dtr) {
            const std::string& field = dtr->first;
            if ( boost::starts_with( field, Mode::File::PREFIX() ) ) {
                // порядок следования - совпадает
                // инициируем здесь в расчёте на то, что кол-во документов с
                // файлами-вложения много меньше общего кол-ва в 'docs'
                const std::size_t i = std::distance( docs.cbegin(), itr );
                const Object& ora = boost::any_cast< Object& >( *ra.at( i ) );
                const std::string nameFile =
                    boost::erase_first_copy( field, Mode::File::PREFIX() );
                const std::string& dataFile = boost::any_cast< std::string& >( *dtr->second );
                const uid_t& uidDoc = uid( ora );
                assert( !uidDoc.empty() && "Обнаружен пустой UID. Неожиданно..." );
                const rev_t& revisionDoc = revision( ora );
                assert( !revisionDoc.empty() && "Обнаружена пустая ревизия. Неожиданно..." );
                /* - Заменено. См. ниже.
                *this << Mode::File( name, data, uidDoc, revisionDoc );
                */
                /* - Создать документ дешевле, чем тащить его из хранилища. См. ниже.
                Document doc = getDocument( uidDoc );
                */
                Document doc(
                    Database::comm, Database::name,
                    uidDoc, "", revisionDoc
                );
                const bool result = doc.addAttachment( nameFile, "text/plain", dataFile );
                if ( !result ) {
                    throw CouchFine::Exception( "Could not create attachment '" + nameFile + "' with data '" + dataFile + "'." );
                }

            } // if ( boost::starts_with( field, Mode::File::PREFIX() ) )

        } // for (auto dtr = d.cbegin(); dtr != d.cend(); ++dtr)

    } // for (auto itr = docs.cbegin(); itr != docs.cend(); ++itr)


    return ra;
}





std::string Database::createBulk(
    const CouchFine::Object& doc,
    CouchFine::fnCreateJSON_t fnCreateJSON
) {

    // Проверяем, что в запасе ещё есть UID для докментов
    if (accUID.size() == 0) {
        accUID = getUUIDs( 100 );
    }

    // Назначаем документу ID в хранилище
    CouchFine::Object o = doc;
    const std::string id = accUID.back();
    accUID.pop_back();
    o["_id"] = typelib::json::cjv( id );

    // Добавляем документ в аккумулятор и проверяем, не пора ли сохранить
    // накопленные документы
    acc.push_back( typelib::json::cjv( doc ) );
    if (acc.size() >= ACC_SIZE) {
        createBulk( acc, fnCreateJSON );
        acc.clear();
    }

    return id;
}





void Database::deleteDocument( const std::string& id, const std::string& rev ) {

    const std::string url = "/" + name + "/" + id;

    if ( rev.empty() ) {
        // Получаем значение ревизии
        // @todo optimize?
        const Variant var = comm.getData( url );
        const Object obj = boost::any_cast< Object >( *var );
        if ( hasError( obj ) ) {
            throw error( obj );
        }
    }

    // Удаляем документ
    const Variant var = comm.getData( url + "?rev=" + rev, "DELETE" );
    const Object obj = boost::any_cast< Object >( *var );
    if ( hasError( obj ) ) {
        throw error( obj );
    }
}







void Database::addView(
    const std::string& design,
    const std::string& name,
    const std::string& map,
    const std::string& reduce
) {
    const std::string designUID = getDesignUID( design );
    try {
        getDocument( designUID );
        // Уже есть документ 'design'
    } catch ( ... ) {
        // Документа 'design' не существует, создаём
        const std::string designJSON = "{ \
            \"language\": \"javascript\" \
          , \"views\": { } \
        }";
        createDocument( designJSON, designUID );
    }

    // Добавляем представление
    const auto doc = getDocument( designUID );
    const CouchFine::Variant jv = doc.getData();
    CouchFine::Object jo = boost::any_cast< CouchFine::Object >( *jv );
    auto ds = boost::any_cast< CouchFine::Object >( *jo["views"] );

    CouchFine::Object content;
    content["map"] = typelib::json::cjv( map );
    if ( !reduce.empty() ) {
        content["reduce"] = typelib::json::cjv( reduce );
    }
    ds[name] = typelib::json::cjv( content );

    jo["views"] = typelib::json::cjv( ds );
    createDocument( typelib::json::cjv( jo ), designUID );
}
