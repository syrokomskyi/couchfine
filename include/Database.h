#pragma once

#include "configure.h"
#include "Document.h"


namespace CouchFine {

class Database {
   friend class Connection;

   protected:
      Database(Communication&, const std::string&);

   public:
      Database(const Database&);

      ~Database();

      
      inline Database& operator=(Database& db) {
          name = db.getName();
          return *this;
      }


      const std::string& getName() const {
          return name;
      }


      /**
      * @return Обзорная информация об этом хранилище.
      */
      Object about() const;



      /**
      * @return Количество документов в этом хранилище, включая
      *         design-документы.
      */
      size_t countDoc() const;



      /**
      * @return Количество design-документов в этом хранилище.
      *
      * (!) Учитываются только design-д., название которых начинается
      * на англ. букву в нижнем регистре.
      */
      size_t countDesign() const;



      /**
      * @return UID для design-документа.
      */
      inline std::string getDesignUID( const std::string& designName = "" ) const {
          return "_design/" + ( designName.empty() ? name : designName );
      }


      std::vector< Document >  listDocuments();


      Document getDocument( const uid_t&, const rev_t& rev = "" );


      inline bool hasDocument( const uid_t& id ) {
        // @todo optimize?
        const std::string url = "/" + name + "/" + id;
        const Variant var = comm.getData( url );
        const Object obj = boost::any_cast< Object >( *var );

        return !hasError( obj );
      }


      
      /**
      * @param key Ключ для выборки. Примеры:
      *            (1) startkey="_design/a"&endkey="_design/{"
      *            (2) key=1000
      *            (3) descending=true&stale=ok
      */
      Object getView(
          const std::string& viewName,
          const std::string& designName = "",
          const std::string& key = ""
      );



      inline bool hasView( const std::string& viewName, const std::string& designName = "" ) {
        // @todo optimize?
        const std::string designUID = getDesignUID( designName );
        const std::string url = "/" + name + "/" + designUID + "/_view/" + viewName + "?limit=1";
        const Variant var = comm.getData( url );
        const Object obj = boost::any_cast< Object >( *var );

        return !hasError( obj );
      }



      /**
      * @return UID, которые могут использоваться приложениями.
      *
      * @param n Желаемое количество UID.
      */
      std::vector< std::string >  getUUIDs( size_t n ) const;


      Document createDocument( const Object&, const std::string& id = "" ) const;
      Document createDocument( const Variant&, const std::string& id = "" ) const;
      Document createDocument( Variant, const std::vector< Attachment >&, const std::string& id = "" ) const;
      Document createDocument( const std::string& json, const std::string& id = "" ) const;

      /**
      * Записывает в хранилище набор документов. Работает значительно быстрее
      * записи документов по отдельности.
      * 
      * @return Результат выполнения. Позволяет вызвавшему методу самому решить,
      *         что делать с ошибками (чаще всего - несовпадение ревизий д.).
      */
      CouchFine::Array createBulk(
          const CouchFine::Array& docs,
          CouchFine::fnCreateJSON_t fnCreateJSON
      );


      /**
      * Накапливает документы и сбрасывает их в хранилище при вызове flush().
      * Работает значительно быстрее записи документов по отдельности.
      *
      * @return UID документа, под которым он *будет* сохранён.
      */
      std::string createBulk(
          const CouchFine::Object& doc,
          CouchFine::fnCreateJSON_t fnCreateJSON
      );


      /**
      * Сохраняет накопленные пулом документы в хранилище. Освобождает пул.
      *
      * @see createBulk()
      */
      inline void flush( CouchFine::fnCreateJSON_t fnCreateJSON ) {
          createBulk( acc, fnCreateJSON );
          acc.clear();
      }



      /**
      * Если rev.empty(), к хранилищу делается дополнительный запрос.
      */
      void deleteDocument( const std::string& id, const std::string& rev = "" );



      /**
      * Добавляет представление.
      */
      void addView(
          const std::string& design,
          const std::string& name,
          const std::string& map,
          const std::string& reduce = ""
      );




      inline Communication& getCommunication() {
          return comm;
      }





   private:
      Communication& comm;
      std::string name;


      /**
      * Аккумулятор для работы метода createBulk( const CouchFine::Object& ).
      */
      CouchFine::Array acc;
      std::vector< std::string >  accUID;

};

}




// @todo Печатать для всех в формате JSON.
inline std::ostream& operator<<(std::ostream& out, const CouchFine::Database& db) {
   return out << "Database: " << db.getName() << std::endl;
}
