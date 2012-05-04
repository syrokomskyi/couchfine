#include "Database.h"
#include "Exception.h"


using namespace CouchDB;


/**
* (!) Внутри json-данных должны быть только одинарные кавычки. Иначе CouchDB
* откажется принять документ.
*/
static std::string createJSON( const Variant &data ) {
   std::ostringstream ostr;
   ostr << data;
   return ostr.str();
}



Database::Database(Communication &_comm, const std::string &_name)
   : comm(_comm)
   , name(_name)
{
}



Database::Database(const Database &db)
   : comm(db.comm)
   , name(db.name)
{
}



Database::~Database(){
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
    Variant var = comm.getData( "/" + name + "/_all_docs?startkey=\"_design/a\"&endkey=\"_design/z\"" );
    Object  obj = boost::any_cast< Object >( *var );
    const Array rows = boost::any_cast< Array >( *obj["rows"] );
    return rows.size();
}



std::vector<Document> Database::listDocuments(){
   Variant var = comm.getData("/" + name + "/_all_docs");
   Object  obj = boost::any_cast<Object>(*var);

   int numRows = boost::any_cast<int>(*obj["total_rows"]);

   std::vector<Document> docs;

   if(numRows > 0){
      Array rows = boost::any_cast<Array>(*obj["rows"]);

      Array::iterator        row     = rows.begin();
      const Array::iterator &row_end = rows.end();
      for(; row != row_end; ++row){
         Object docObj = boost::any_cast<Object>(**row);
         Object values = boost::any_cast<Object>(*docObj["value"]);

         Document doc(comm, name,
                      boost::any_cast<std::string>(*docObj["id"]),
                      boost::any_cast<std::string>(*docObj["key"]),
                      boost::any_cast<std::string>(*values["rev"]));

         docs.push_back(doc);
      }
   }

   return docs;
}



Document Database::getDocument( const std::string &id, const std::string &rev ) {

    std::string url = "/" + name + "/" + id + ( rev.empty() ? "" : ("?rev=" + rev) );

    // (!) Целые числа хранятся как знаковый тип, при превышении лимита - исключение
    const Variant var = comm.getData( url );
    Object obj = boost::any_cast< Object >( *var );
    if (obj.find( "error" ) != obj.end()) {
        throw Exception("Document " + id + " (v" + rev + ") not found: " + error( obj ) );
    }

    return Document(
        comm,
        name,
        //boost::any_cast< std::string >( *obj["_id"] ),
        CouchDB::uid( obj ),
        "", // no key returned here
        //boost::any_cast< std::string >( *obj["_rev"] )
        CouchDB::rev( obj )
    );
}



/* - inline
bool Database::hasDocument( const std::string &id ) {
    // @todo optimize?
    const std::string url = "/" + name + "/" + id;
    const Variant var = comm.getData( url );
    Object obj = boost::any_cast< Object >( *var );

    return (obj.find( "error" ) == obj.end());
}
*/




/* - inline
Object Database::getView(
    const std::string& viewName,
    const std::string& designName,
    const std::string& key
) {
    // Для корректного запроса ключ требует преобразования
    //std::string preparedKey = Communication::escapeKey( key );
    std::string preparedKey = key;

    const bool a = !preparedKey.empty() && (preparedKey[0] == '[');
    const std::string k = a ? preparedKey : ("\"" + preparedKey + "\"");
    const std::string designUID = getDesignUID( designName );
    const std::string url = "/" + name + "/" + designUID + "/_view/" + viewName +
        ( preparedKey.empty() ? "" : ("?key=" + k) );

    const Variant var = comm.getData( url );
    //const auto t = var->type().name();
    const Object obj = boost::any_cast< Object >( *var );
    auto ftr = obj.find( "error" );
    if (ftr != obj.end()) {
        throw Exception( "View '" + viewName + "': " + boost::any_cast< std::string >( *ftr->second ) );
    }

    return obj;
}
*/




/* - inline
bool Database::hasView( const std::string& viewName, const std::string& designName ) {
    // @todo optimize?
    const std::string designUID = getDesignUID( designName );
    const std::string url = "/" + name + "/" + designUID + "/_view/" + viewName + "?limit=1";
    const Variant var = comm.getData( url );
    Object obj = boost::any_cast< Object >( *var );

    return (obj.find( "error" ) == obj.end());
}
*/




std::vector< std::string >  Database::getUUIDs( size_t n ) const {

    const Variant var = comm.getData( "/_uuids?count=100" );
    Object obj = boost::any_cast< Object >( *var );
    if (obj.find( "error" ) != obj.end()) {
        throw Exception( "Set of ID's is not created: " + error( obj ) );
    }

    // Переводим в std::vector для более быстрой работы со списком
    std::vector< std::string >  uuids;
    const auto a = boost::any_cast< Array >( *obj["uuids"] );
    for (auto itr = a.cbegin(); itr != a.cend(); ++itr) {
        const Variant v = *itr;
        const auto id = boost::any_cast< std::string >( *v );
        uuids.push_back( id );
    }

    return uuids;
}





Document Database::createDocument( const Object& obj, const std::string& id ) {
   return createDocument( cjv( obj ) );
}




Document Database::createDocument( const Variant& data, const std::string& id ) {
   return createDocument( data,  std::vector< Attachment >(),  id );
}




Document Database::createDocument(
    Variant data,
    std::vector< Attachment > attachments,
    const std::string &id
) {
    if (attachments.size() > 0) {
        Object attachmentObj;
        std::vector< Attachment >::iterator attachment = attachments.begin();
        const std::vector< Attachment >::iterator &attachmentEnd = attachments.end();
        for ( ; attachment != attachmentEnd; ++attachment) {
           Object attachmentData;
           attachmentData["content_type"] = createVariant( attachment->getContentType() );
           attachmentData["data"        ] = createVariant( attachment->getData() );
           attachmentObj[ attachment->getID() ] = createVariant( attachmentData );
        }

        Object obj = boost::any_cast< Object >( *data );
        obj["_attachments"] = createVariant( attachmentObj );
        data = createVariant( obj );
    }

    const std::string json = createJSON( data );
    const auto doc = createDocument( json, id );

    return doc;
}





Document Database::createDocument( const std::string& json, const std::string &id ) {

    const Variant var = (id.size() > 0) ?
        comm.getData( "/" + name + "/" + id, "PUT",  json ) :
        comm.getData( "/" + name + "/",      "POST", json );

    const Object obj = boost::any_cast< Object >( *var );
    if (obj.find( "error" ) != obj.cend()) {
       throw Exception( "Document could not be created: " + error( obj ) );
    }

    return Document(
        comm, name,
        //boost::any_cast< std::string >( *obj["id"] ),
        CouchDB::uid( obj ),
        "", // no key returned here
        //boost::any_cast< std::string >( *obj["rev"] )
        CouchDB::rev( obj )
    );
}





CouchDB::Array Database::createBulk( const CouchDB::Array&  docs ) {

    // @see http://wiki.apache.org/couchdb/HTTP_Bulk_Document_API#Modify_Multiple_Documents_With_a_Single_Request
    CouchDB::Object o;
    o["docs"] = createVariant( docs );
    const std::string json = createJSON( createVariant( o ) );
    //std::cout << std::endl << createVariant( o ) << std::endl << std::endl;

    Variant var = comm.getData( "/" + name + "/_bulk_docs",  "POST",  json );
    //std::cout << std::endl << var << std::endl << std::endl;
    /* - Быстрее. Проще. Обходимся.
    transformObject( &var );
    */

    // @todo fine Если сбой на сервере, вместо списка с ошибками может
    //       быть возвращён Object?
    /* - Лучше вернём результат для анализа: позволит вызвавшему методу
         самому решить, что делать с ошибками (часто - несовпадение ревизий).
    // При удачном завершении (см. catch ниже) получим список ключей CouchDB::Array
    const CouchDB::Array a = boost::any_cast< CouchDB::Array >( *var );
    // При неудачном - вместо ключей получим объекты с информ. об ошибках
    for (auto itr = a.cbegin(); itr != a.cend(); ++itr) {
        const auto& type = itr->get()->type();
        if (type == typeid( Object )) {
            // ошибка, однозначно
            std::cerr << cjv( a );
            const CouchDB::Object obj = boost::any_cast< CouchDB::Object >( *itr );
            throw Exception( "Part of set of documents could not be created. First error: " + obj.error() );
        }
    }
    */

    const CouchDB::Array a = boost::any_cast< CouchDB::Array >( *var );

    return a;
}





std::string Database::createBulk( const CouchDB::Object& doc ) {

    // Проверяем, что в запасе ещё есть UID для докментов
    if (accUID.size() == 0) {
        accUID = getUUIDs( 100 );
    }

    // Назначаем документу ID в хранилище
    CouchDB::Object o = doc;
    const std::string id = accUID.back();
    accUID.pop_back();
    o["_id"] = cjv( id );

    // Добавляем документ в аккумулятор и проверяем, не пора ли сохранить
    // накопленные документы
    acc.push_back( cjv( doc ) );
    if (acc.size() >= ACC_SIZE) {
        createBulk( acc );
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
		Object obj = boost::any_cast< Object >( *var );
        if (obj.find( "error" ) != obj.cend()) {
			throw error( obj );
		}
	}

	// Удаляем документ
	const Variant var = comm.getData( url + "?rev=" + rev, "DELETE" );
	Object obj = boost::any_cast< Object >( *var );
	if (obj.find( "error" ) != obj.cend()) {
		throw error( obj );
	}
}







void Database::addView(
    const std::string& name,
    const std::string& design,
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
    const CouchDB::Variant jv = doc.getData();
    CouchDB::Object jo = boost::any_cast< CouchDB::Object >( *jv );
    auto ds = boost::any_cast< CouchDB::Object >( *jo["views"] );

    CouchDB::Object content;
    content["map"] = createVariant( map );
	if ( !reduce.empty() ) {
        content["reduce"] = createVariant( reduce );
	}
    ds[name] = createVariant( content );

	jo["views"] = createVariant( ds );
    createDocument( createVariant( jo ), designUID );
}
