#pragma once

#include "configure.h"
#include "Exception.h"
#include <map>
#include <boost/algorithm/string.hpp>

/**
* (!) Внутри design-методов должны использоваться только одинарные кавычки.
*/


// @todo optimize live Заменить tinyjson на живую и свежую альтернативу.

namespace CouchDB {

// some data helpers aligned with TinyJSON implementation
typedef boost::shared_ptr< boost::any >  Variant;
typedef std::deque< Variant >            Array;

// Старый объект оставлен, чтобы приводить извлечённые из JSON-файлов данные
// к новому типу - Object
#ifdef SAVE_ORDER_FIELDS
typedef std::map< std::string, Variant >  Object;
#else
typedef std::unordered_map< std::string, Variant >  Object;
#endif


/**
* @return Значение, обёрнутое в CouchDB::Variant.
*
* @alias createVariant()
*/
template< typename T >
inline Variant cjv( T value ) {
    return Variant( new boost::any( value ) );
}


// @alias cjv()
template< typename T >
inline Variant createVariant( T value ) {
    return Variant( new boost::any( value ) );
}



#if 0
// - Заменено на внешние методы. См. ниже.
/**
* Структура для более удобной работы с Object.
*/
struct WrapObject
#ifdef SAVE_ORDER_FIELDS
    : public std::map< std::string, Variant >
#else
    : public std::unordered_map< std::string, Variant >
#endif
{
    inline WrapObject() : std::map< std::string, Variant >() {
    }


    explicit
    inline WrapObject( const Object& oldObj ) {
        this->insert( oldObj.cbegin(), oldObj.cend() );
    }


    /* - Достаточно конструктора.
    inline Object& operator=( const Object& oldObj ) {
        this->insert( oldObj.cbegin(), oldObj.cend() );
        return *this;
    }
    */


    /**
    * @return UID объекта или пустая строка, если UID не задан.
    */
    inline std::string uid() const {
        const auto ftr = this->find( "_id" );
        return (ftr == this->cend())
            ? "" : boost::any_cast< std::string >( *ftr->second );
    }


    /**
    * Устанавливает UID для объекта.
    * @return Ссылку на объект, чтобы можно было писать след.:
    *           store >> doc.uid( "MyID" )
    *         Эта запись извлечёт документ из хранилища в 'doc'.
    */
    inline WrapObject& uid( const std::string& s ) {
        (*this)[ "_id" ] = cjv( s );
        return *this;
    }


    /**
    * @return true, если задан UID.
    */
    inline bool hasUID() const {
        const auto ftr = this->find( "_id" );
        return (ftr != this->cend());
    }



    /**
    * @return Ревизия объекта или пустая строка, если ревизия не задана.
    */
    inline std::string rev() const {
        const auto ftr = this->find( "_rev" );
        return (ftr == this->cend())
            ? "" : boost::any_cast< std::string >( *ftr->second );
    }


    /**
    * Устанавливает ревизию для объекта.
    * @return Ссылку на объект, чтобы можно было писать след.:
    *           store >> doc.rev( "MyRevision" )
    *         Эта запись извлечёт документ из хранилища в 'doc'.
    */
    inline WrapObject& rev( const std::string& s ) {
        (*this)[ "_rev" ] = cjv( s );
        return *this;
    }


    /**
    * @return true, если задан UID.
    */
    inline bool hasRev() const {
        const auto ftr = this->find( "_rev" );
        return (ftr != this->cend());
    }



    /**
    * @return Сообщение об ошибке или пустая строка, если ошибки нет.
    *         Удобно для проверки после обращения с запросом к хранилищу.
    */
    inline std::string error() const {
        std::string e = "";
        auto ftr = this->find( "error" );
        if (ftr != this->cend()) {
            e += boost::any_cast< std::string >( *ftr->second );
        }
        ftr = this->find( "reason" );
        if (ftr != this->cend()) {
            e += " > " + boost::any_cast< std::string >( *ftr->second );
        }
        return e;
    }


    /**
    * @return true, если объект является сообщением CouchDB об ошибке.
    */
    inline bool hasError() const {
        const auto fte = this->find( "error" );
        const auto ftr = this->find( "reason" );
        return ( (fte != this->cend()) || (ftr != this->cend()) );
    }

};
#endif




// Методы для взаимодействия с Object

/**
* @return Значение поля объекта, преобразованное к заданному типу или
*         'def' если поле с заданным именем в объекте не найдено.
*/
template < typename T >
inline T v( const Object& o, const std::string field, const T& def = T() ) {
    const auto ftr = o.find( field );
    return (ftr == o.cend())
        ? def : boost::any_cast< T >( *ftr->second );
}



/**
* @return UID объекта или пустая строка, если UID не задан.
*/
inline std::string uid( const Object& o ) {
    auto ftr = o.find( "_id" );
    if (ftr != o.cend()) {
        return boost::any_cast< std::string >( *ftr->second );
    }
    // При получении д. из представления, UID идёт без префикса '_'
    ftr = o.find( "id" );
    if (ftr != o.cend()) {
        return boost::any_cast< std::string >( *ftr->second );
    }
    return "";
}


/**
* Устанавливает UID для объекта.
* @return Ссылку на объект, чтобы можно было писать след.:
*           store >> doc.uid( "MyID" )
*         Эта запись извлечёт документ из хранилища в 'doc'.
*/
inline Object& uid( Object& o, const std::string& s ) {
    assert( (o.find( "id" ) == o.cend())
        && "Обнаружено поле 'id'. Вероятно, ошибка: используйте '_id' для корректного сохранения документа в хранилище." );
    o[ "_id" ] = cjv( s );
    return o;
}


/**
* @return true, если задан UID.
*/
inline bool hasUID( const Object& o ) {
    assert( (o.find( "id" ) == o.cend())
        && "Обнаружено поле 'id'. Вероятно, ошибка: используйте '_id' для корректного сохранения документа в хранилище." );
    const auto ftr = o.find( "_id" );
    return (ftr != o.cend());
}



/**
* @return Ревизия объекта или пустая строка, если ревизия не задана.
*/
inline std::string rev( const Object& o ) {
    const auto ftr = o.find( "_rev" );
    return (ftr == o.cend())
        ? "" : boost::any_cast< std::string >( *ftr->second );
}


/**
* Устанавливает ревизию для объекта.
* @return Ссылку на объект, чтобы можно было писать след.:
*           store >> rev( doc, "MyRevision" )
*         Эта запись извлечёт документ из хранилища в 'doc'.
*/
inline Object& rev( Object& o, const std::string& s ) {
    o[ "_rev" ] = cjv( s );
    return o;
}


/**
* @return true, если задан UID.
*/
inline bool hasRev( const Object& o ) {
    const auto ftr = o.find( "_rev" );
    return (ftr != o.cend());
}




/**
* @return Сообщение об ошибке или пустая строка, если ошибки нет.
*         Удобно для проверки после обращения с запросом к хранилищу.
*/
inline std::string error( const Object& o ) {
    std::string e = "";
    const auto fte = o.find( "error" );
    if (fte != o.cend()) {
        e += boost::any_cast< std::string >( *fte->second );
    }
    const auto ftr = o.find( "reason" );
    if (ftr != o.cend()) {
        e += " > " + boost::any_cast< std::string >( *ftr->second );
    }
    return e;
}


/**
* @return true, если объект является сообщением CouchDB об ошибке.
*/
inline bool hasError( const Object& o ) {
    const auto fte = o.find( "error" );
    if (fte != o.cend()) {
        return true;
    }
    const auto ftr = o.find( "reason" );
    if (ftr != o.cend()) {
        return true;
    }
    return false;
}






/**
* Заменяет объекты Object на Object. Наследство от tinyJSON.
*//* - Медленно. Лишнее. Не используется.
inline void transformObject( Variant* var ) {
    const auto& type = ( *var )->type();
    if (type == typeid( Object )) {
        Object& oldObj = boost::any_cast< Object >( **var );
        // объект может содержать другие объекты
        for (auto itr = oldObj.begin(); itr != oldObj.end(); ++itr) {
            transformObject( &itr->second );
        }
        const Object& obj = Object( oldObj );
        *var = cjv( obj );

    } else if (type == typeid( Array )) {
        // список может содержать другие объекты
        Array& a = boost::any_cast< Array >( **var );
        for (auto itr = a.begin(); itr != a.end(); ++itr) {
            auto& t = *itr;
            transformObject( &t );
        }

    } else {
        // оставляем без изменений
    }
}
*/





inline void printHelper( std::ostream& out, const boost::any& value, const std::string& indent ) {
   std::string childIndent = indent + "   ";

   boost::any val = value;

   if ( val.empty() )
      out << "null";

   else {
      // @todo optimize Расположить в порядке востребованности.
      //const auto& test0 = typeid( val ).name();
      //const auto& test1 = typeid( Object ).name();
      //const auto& test2 = typeid( std::map< std::string, Variant > ).name();

      /* - Обходимся без перобразования.
      // @todo optimize fine Попадаются старые объекты в возвращаемых значениях,
      //       несмотря на вызов transformObject(). Разобраться.
      if (val.type() == typeid( Object )) {
          // преобразуем в Object
          const Object oldObj = boost::any_cast< Object >( val );
          val = boost::any( CouchDB::Object( oldObj ) );
      }
      */

      const type_info& type = val.type();
      if (type == typeid( const char* ))
          out << '"' << boost::any_cast< const char* >( val ) << '"';
      else if (type == typeid( std::string ))
          out << '"' << boost::any_cast< std::string >( val ) << '"';
      else if (type == typeid( bool ))
          out << boost::any_cast< bool >( val );
      else if (type == typeid( int ))
          out << boost::any_cast< int >( val );
      else if (type == typeid( size_t ))
          out << boost::any_cast< size_t >( val );
      else if (type == typeid( long ))
          out << boost::any_cast< long >( val );
      else if (type == typeid( float ))
          out << boost::any_cast< float >( val );
      else if (type == typeid( double ))
          out << boost::any_cast< double >( val );
      else if (type == typeid( char ))
          out << boost::any_cast< char >( val );
      else if (type == typeid( unsigned char ))
          out << boost::any_cast< unsigned char >( val );
      /* - Заменено на превентивную типизацию. См. выше.
      else if (type == typeid( Object ))
          throw Exception( "Use Object instead Object." );
      */
      else if (type == typeid( Object )) {

         Object obj = boost::any_cast< Object >( val );

         out << "{";
#ifdef COUCH_DB_DEBUG
         out << std::endl;
#endif

         bool addComma = false;
         Object::iterator        data     = obj.begin();
         const Object::iterator &data_end = obj.end();
         for(; data != data_end; ++data){
            if(addComma)
               out << ",";
            else
               addComma = true;
#ifdef COUCH_DB_DEBUG
            out << childIndent;
#endif
            out << '"' << data->first << "\": ";
            printHelper(out, *data->second, childIndent);
#ifdef COUCH_DB_DEBUG
            out << std::endl;
#endif
         }

#ifdef COUCH_DB_DEBUG
         out << indent;
#endif
         out << "}";
      }
      else if(type == typeid(Array)){
         Array array = boost::any_cast<Array>(value);

         out << "[";
#ifdef COUCH_DB_DEBUG
         out << std::endl;
#endif

         bool addComma = false;
         Array::iterator        data     = array.begin();
         const Array::iterator &data_end = array.end();
         for(; data != data_end; ++data){
            if(addComma)
               out << ",";
            else
               addComma = true;
#ifdef COUCH_DB_DEBUG
            out << childIndent;
#endif
            printHelper(out, **data, childIndent);
#ifdef COUCH_DB_DEBUG
            out << std::endl;
#endif
         }

#ifdef COUCH_DB_DEBUG
         out << indent;
#endif
         out << "]";
      }

      else {
         const std::string t = type.name();
         //out << "unrecognized type (" << type.name() << ")";
         throw Exception( "Unrecognized type: " + t );
      }

   }

}



inline void printHelper( std::ostream& out, const CouchDB::Object& obj, const std::string& indent ) {
    printHelper( out,  new boost::any( obj ),  indent );
}





class Communication {
public:
    /**
    * Ключ для перевода из RFC1738.
    *
    * @see http://blooberry.com/indexdot/html/topics/urlencoding.htm
    */
    static const std::map< std::string, std::string >  FROM_RFC1738;


public:

#ifdef SAVE_ORDER_FIELDS
      typedef std::map< std::string, std::string >  HeaderMap;
#else
      typedef std::unordered_map< std::string, std::string >  HeaderMap;
#endif


      Communication();
      Communication(const std::string&);
      ~Communication();

      Variant getData(const std::string&, const std::string& method = "GET",
                      const std::string& data = "");
      Variant getData(const std::string&, const HeaderMap&,
                      const std::string& method = "GET",
                      const std::string& data = "");

      std::string getRawData(const std::string&);




   private:
      void init(const std::string&);
      Variant getData(const std::string&, const std::string&,
                      std::string, const HeaderMap&);
      void getRawData(const std::string&, const std::string&,
                      const std::string&, const HeaderMap&);


        /**
        * @return true, когда строка в UTF-8.
        *
        * @source http://forum.sources.ru/index.php?s=b01af20d3b1760bcb86e1182798dab65&amp;showtopic=169548&view=findpost&p=1438394
        *//* - не работает
        static inline bool isUTF8( unsigned char* src, size_t n ) {
            const unsigned char *slim = src + n;
            int is_baseascii = 1;
            while (src < slim) {
                int wc;
                if (*src < 0x80) {
                    src++;
                }
                else {
                    is_baseascii = 0;
                    if ((*src & 0xE0) == 0xC0) {
                        if ((slim - src) < 2) return 0;
                        wc = (*src++ & 0x1F) << 6;
                        if ((*src & 0xC0) != 0x80) {
                            return false;
                        } else {
                            wc |= *src & 0x3F;
                        }
                        if (wc < 0x80) {
                            return false;
                        }
                        src++;
                    } else if ((*src & 0xF0) == 0xE0) {
                    // less common
                        if ((slim - src) < 3) return 0;
                        wc = (*src++ & 0x0F) << 12;
                        if ((*src & 0xC0) != 0x80) {
                            return false;
                        } else {
                            wc |= (*src++ & 0x3F) << 6;
                            if ((*src & 0xC0) != 0x80) {
                                return false;
                            } else {
                                wc |= *src & 0x3F;
                            }
                        }
                        if (wc < 0x800) {
                            return false;
                        }
                        src++;
                    } else {
                    // very unlikely
                    return false;
                    }
                }
            }
            return (is_baseascii == 0);
        }
        */





      CURL        *curl;
      std::string baseURL;
      std::string buffer;
};




typedef std::pair< std::string /* indent */, CouchDB::Variant >  indentVariant_t;
typedef std::pair< std::string /* indent */, CouchDB::Object >   indentObject_t;

} // namespace CouchDB




inline std::ostream& operator<<(
    std::ostream &out,
    const CouchDB::indentVariant_t& value
) {
    CouchDB::printHelper( out, *value.second, value.first );
    return out;
}



inline std::ostream& operator<<(
    std::ostream &out,
    const CouchDB::indentObject_t& value
) {
    out << CouchDB::indentVariant_t( value.first, CouchDB::cjv( value.second ) );
    return out;
}



inline std::ostream& operator<<(
    std::ostream &out,
    const CouchDB::Variant& value
) {
    out << CouchDB::indentVariant_t( "", value );
    return out;
}



inline std::ostream& operator<<(
    std::ostream &out,
    const CouchDB::Object& value
) {
    out << CouchDB::indentObject_t( "", value );
    return out;
}



/*
inline std::ostream& operator<<(
    std::ostream &out,
    const CouchDB::Array& value
) {
    for (auto itr = value.cbegin(); itr != value.cend(); ++itr) {
        out << *itr;
    }
    return out;
}
*/
