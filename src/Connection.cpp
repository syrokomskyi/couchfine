#include "../include/Connection.h"


using namespace CouchFine;


Connection::Connection() {
   getInfo();
}



Connection::Connection( const std::string& url ) : comm( url ) {
   getInfo();
}



void Connection::getInfo() {
   const Variant var = comm.getData( "" );
   const Object obj = boost::any_cast< Object >( *var );
   couchDBVersion = boost::any_cast< std::string >( *obj.at( "version" ) );
}



Connection::~Connection() {
}



std::string Connection::getCouchDBVersion() const {
   return couchDBVersion;
}



std::vector<std::string> Connection::listDatabases() {
   const Variant var = comm.getData( "/_all_dbs" );
   const Array arr = boost::any_cast< Array >( *var );

   std::vector< std::string >  dbs;
   for (auto itr = arr.cbegin(); itr != arr.cend(); ++itr) {
      dbs.push_back( boost::any_cast< std::string >( **itr ) );
   }

   return dbs;
}



Database Connection::getDatabase(const std::string& db) {
   return Database( comm, db );
}





bool Connection::existsDatabase( const std::string& name ) {
   const Variant var = comm.getData( "/_all_dbs" );
   const Array arr = boost::any_cast< Array >( *var );
   for(auto db = arr.cbegin(); db != arr.cend(); ++db) {
      const std::string n = boost::any_cast< std::string >( **db );
      if (name == n) {
          return true;
      }
   }
   return false;
}



bool Connection::createDatabase( const std::string& db ) {
   const Variant var = comm.getData( "/" + db, "PUT" );
   const Object obj = boost::any_cast< Object >( *var );
   if ( hasError( obj ) ) {
      throw Exception( "Unable to create database '" + db + "': " + error( obj ) );
   }

   return ok( obj );
}




bool Connection::deleteDatabase(const std::string& db) {
   const Variant var = comm.getData( "/" + db, "DELETE" );
   const Object obj = boost::any_cast< Object >( *var );
   if ( hasError( obj ) ) {
      throw Exception( "Unable to create database '" + db + "': " + error( obj ) );
   }

   return ok( obj );
}




void Connection::clearDatabase( const std::string& name, bool includeDesign ) {

    if ( !existsDatabase( name ) ) {
        throw "Database '" + name + "' not found.";
    }

    /* - Слишком медленно. Заменено. См. ниже.
    // Проверяем наличие в базе design-документов
    bool hasDesign = false;
    if ( !includeDesign ) {
        Variant var = comm.getData( "/" + name + "/_all_docs?startkey=\"_design/\"" );
        Object obj = boost::any_cast<Object>(*var);
        const Array rows = boost::any_cast< Array >( *obj["rows"] );
        hasDesign = (rows.size() > 0);
    }

    // Быстрая очистка
    if ( !hasDesign || includeDesign ) {
        if ( existsDatabase( name ) ) {
            deleteDatabase( name );
        }
        createDatabase( name );
        return;
    }

    // Нужно сохранить design-документы
    Variant var = comm.getData( "/" + name + "/_all_docs" );
    Object obj = boost::any_cast<Object>( *var );
    const Array rows = boost::any_cast<Array>( *obj["rows"] );
    for (auto itr = rows.cbegin(); itr != rows.cend(); ++itr) {
        Object doc = boost::any_cast<Object>( **itr );
        Object value = boost::any_cast<Object>( *doc["value"] );
        const std::string id = boost::any_cast< std::string> ( *doc["id"] );
        const std::string rev = boost::any_cast< std::string> ( *value["rev"] );
        // Удаляем документ
        const Variant var = comm.getData( "/" + name + "/" + id + "?rev=" + rev, "DELETE" );
        Object obj = boost::any_cast< Object >( *var );
        if (obj.find( "error" ) != obj.end()) {
            throw boost::any_cast< std::string >( obj["error"] );
        }
    }
    */

    // Запоминаем design-документы
    Array designRows;
    if ( !includeDesign ) {
        const Variant var = comm.getData( "/" + name + "/_all_docs?startkey=\"_design/a\"&endkey=\"_design/z\"" );
        const Object obj = boost::any_cast< Object >( *var );
        const Array rows = boost::any_cast< Array >( *obj.at( "rows" ) );
        for (auto itr = rows.cbegin(); itr != rows.cend(); ++itr) {
            const Object doc = boost::any_cast< Object >( **itr );
            const Object value = boost::any_cast< Object >( *doc.at( "value" ) );
            const uid_t& id = CouchFine::uid( doc );
            const rev_t& rev = CouchFine::revision( value );
            // Запоминаем документ
            const Variant var = comm.getData( "/" + name + "/" + id + "?rev=" + rev );
            const Object obj = boost::any_cast< Object >( *var );
            if ( hasError( obj ) ) {
                throw error( obj );
            }
            designRows.push_back( var );
        }
    }

    // Быстрая очистка
    deleteDatabase( name );
    createDatabase( name );

    // Восстанавливаем design-документы
    // (!) Внутри методов должны использоваться только одинарные кавычки.
    for (auto itr = designRows.cbegin(); itr != designRows.cend(); ++itr) {
        Object doc = boost::any_cast< Object >( **itr );
        //const std::string id = boost::any_cast< std::string> ( *doc["_id"] );
        //doc.erase( "_id" );
        doc.erase( "_rev" );

        std::ostringstream os;
        os << cjv( doc );
        const std::string json = os.str();
        //std::cout << json << std::endl;

        const Variant var = comm.getData( "/" + name, "POST",  json );
        const Object obj = boost::any_cast< Object >( *var );
        if ( hasError( obj ) ) {
            throw Exception( "Document could not be created: " + error( obj ) );
        }
    }


    /* - @test
    const std::string id = "_design/t";
    const std::string json = "{ \
        \"language\": \"javascript\" \
        , \"views\": { \n } \
    }";
    const Variant var = comm.getData( "/" + name + "/" + id, "PUT",  json );
    Object obj = boost::any_cast< Object >( *var );
    if ( obj.find( "error" ) != obj.end() ) {
        throw Exception( "Document could not be created: " + boost::any_cast< std::string >( *obj["reason"] ) );
    }
    */

}
