#pragma once

#include "type.h"
#include "Exception.h"
#include <map>
#include <boost/algorithm/string.hpp>
#include <boost/function.hpp>


/**
* (!) Внутри design-методов должны использоваться только одинарные кавычки.
*/


namespace CouchFine {

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
        *var = typelib::json::cjv( obj );

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
          val = boost::any( CouchFine::Object( oldObj ) );
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
      // Object* может быть получен при анализе содержимого Pool
      // @see Pool& operator<<( Pool&, Object* )
      else if ( (type == typeid( Object )) || (type == typeid( Object* )) ) {

         const Object obj = (type == typeid( Object ))
             ? boost::any_cast< Object >( val )
             : *boost::any_cast< Object* >( val );

         out << "{";
#ifdef COUCH_DB_DEBUG
         out << std::endl;
#endif

         bool addComma = false;
         auto data = obj.cbegin();
         const auto& data_end = obj.cend();
         for(; data != data_end; ++data) {
            if ( addComma ) {
               out << ", ";
            } else {
               addComma = true;
            }
#ifdef COUCH_DB_DEBUG
            out << childIndent;
#endif
            out << '"' << data->first << "\": ";
            printHelper( out, *data->second, childIndent );
#ifdef COUCH_DB_DEBUG
            out << std::endl;
#endif
         }

#ifdef COUCH_DB_DEBUG
         out << indent;
#endif
         out << "}";
      }
      else if(type == typeid(Array)) {
         Array array = boost::any_cast<Array>(value);

         out << "[";
#ifdef COUCH_DB_DEBUG
         out << std::endl;
#endif

         bool addComma = false;
         Array::iterator        data     = array.begin();
         const Array::iterator &data_end = array.end();
         for( ; data != data_end; ++data) {
            if ( addComma ) {
               out << ", ";
            } else {
               addComma = true;
            }
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
         const std::string& t = type.name();
         std::cerr << "CouchFine::Communication::printHelper(). Unrecognized type " << type.name() << std::endl;
         throw Exception( "Unrecognized type: " + t );
      }

   }

}



inline void printHelper( std::ostream& out, const CouchFine::Object& obj, const std::string& indent ) {
    printHelper( out,  boost::any( obj ),  indent );
}






class Communication {
public:
    /**
    * Ключ для перевода из RFC1738.
    *
    * @see http://blooberry.com/indexdot/html/topics/urlencoding.htm
    */
    static const std::map< std::string, std::string >  FROM_RFC1738;

    static const std::map< std::string, std::string >  FROM_RFC1738_SLASH;


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




typedef std::pair< std::string /* indent */, CouchFine::Variant >  indentVariant_t;
typedef std::pair< std::string /* indent */, CouchFine::Object >   indentObject_t;

} // namespace CouchFine






inline std::ostream& operator<<(
    std::ostream& out,
    const CouchFine::indentVariant_t& value
) {
    CouchFine::printHelper( out, *value.second, value.first );
    return out;
}



inline std::ostream& operator<<(
    std::ostream& out,
    const CouchFine::indentObject_t& value
) {
    out << CouchFine::indentVariant_t( value.first, typelib::json::cjv( value.second ) );
    return out;
}



inline std::ostream& operator<<(
    std::ostream& out,
    const CouchFine::Variant& value
) {
    out << CouchFine::indentVariant_t( "", value );
    return out;
}



inline std::ostream& operator<<(
    std::ostream& out,
    const CouchFine::Object& value
) {
    out << CouchFine::indentObject_t( "", value );
    return out;
}



/*
inline std::ostream& operator<<(
    std::ostream& out,
    const CouchFine::Array& value
) {
    for (auto itr = value.cbegin(); itr != value.cend(); ++itr) {
        out << *itr;
    }
    return out;
}
*/
