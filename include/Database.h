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

      Document getDocument( const std::string&, const std::string& rev = "" );


      inline bool hasDocument( const std::string& id ) {
        // @todo optimize?
        const std::string url = "/" + name + "/" + id;
        const Variant var = comm.getData( url );
        const Object obj = boost::any_cast< Object >( *var );

        return !hasError( obj );
      }


      
      /**
      * @param otherKey Список ключей добавляется к основному ключу.
      */
      inline Object getView(
          const std::string& viewName,
          const std::string& designName = "",
          const std::string& key = "",
          const std::string& otherKey = ""
      )  {
        // Для корректного запроса ключ требует преобразования
        std::string preparedKey = key;
        boost::replace_all( preparedKey, "%", "%25" );
        boost::replace_all( preparedKey, "#", "%23" );
        /* - @todo Нельзя менять одним махом: могли передать составной ключ.
        boost::replace_all( preparedKey, "&", "%26" );
        */

        //const std::string k = a ? preparedKey : ("\"" + preparedKey + "\"");
        std::string k = "";
        try {
            const double t = boost::lexical_cast< double >( preparedKey );
            k = preparedKey;
        } catch ( ... ) {
            const bool a = !preparedKey.empty() && (preparedKey[0] == '[');
            if ( a ) {
                k = preparedKey;
            } else if ( !preparedKey.empty() ) {
                k = "\"" + preparedKey + "\"";
            }
        }

        const std::string designUID = getDesignUID( designName );
        const std::string url = "/" + name + "/" + designUID + "/_view/" + viewName +
            ( k.empty() ? "" : ("?key=" + k) ) +
            ( otherKey.empty() ? "" : ( ( k.empty() ? "?" : "&") + otherKey ) );

        const Variant var = comm.getData( url );
        //const auto t = var->type().name();
        const Object obj = boost::any_cast< Object >( *var );
        if ( hasError( obj ) ) {
            throw Exception( "View '" + viewName + "': " + error( obj ) );
        }

        return obj;
    }




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




   protected:
      Communication& getCommunication();




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
