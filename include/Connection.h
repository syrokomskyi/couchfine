#pragma once

#include "configure.h"
#include "Exception.h"
#include "Database.h"
#include "Communication.h"


namespace CouchFine {

/**
* (!) Внутри design-методов должны использоваться только одинарные кавычки.
*/
class Connection{
   public:
      Connection();
      Connection(const std::string&);
      ~Connection();

      std::string getCouchDBVersion() const;

      std::vector< std::string >  listDatabases();
      Database getDatabase( const std::string& );

      bool existsDatabase( const std::string& );
      bool createDatabase( const std::string& );
      bool deleteDatabase( const std::string& );


      /**
      * Очистка хранилища.
      *
      * @param includeDesign true, если необходимо очистить и design-документы.
      * 
      * (!) Учитываются только design-д., название которых начинается
      * на англ. букву в нижнем регистре.
      */
      void clearDatabase( const std::string& name, bool includeDesign );



   private:
      void init( const std::string& );
      void getInfo();

      Communication comm;
      std::string   couchDBVersion;
};

}
