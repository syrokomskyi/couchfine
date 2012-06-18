#include "../include/Communication.h"
#include "../external/tinyjson/tinyjson.hpp"
#include <boost/assign.hpp>

using namespace CouchFine;

const std::string DEFAULT_COUCHDB_URL = "http://localhost:5984";


// @source http://blooberry.com/indexdot/html/topics/urlencoding.htm
// (!) Порядок важен: иначе '%' может быть перекодирован лишний раз.
const std::map< std::string, std::string >  Communication::FROM_RFC1738 = boost::assign::list_of
    ( std::make_pair( "%25", "%" ) )
    // специальные
    ( std::make_pair( "%0A", "\n" ) )
    // зарезервированные
    ( std::make_pair( "%24", "$" ) )
    ( std::make_pair( "%26", "&" ) )
    ( std::make_pair( "%2B", "+" ) )
    ( std::make_pair( "%2C", "," ) )
    ( std::make_pair( "%2F", "/" ) )
    ( std::make_pair( "%3A", ":" ) )
    ( std::make_pair( "%3B", ";" ) )
    ( std::make_pair( "%3D", "=" ) )
    ( std::make_pair( "%3F", "?" ) )
    ( std::make_pair( "%40", "@" ) )
    // ненадёжные
    ( std::make_pair( "%20", " " ) )
    ( std::make_pair( "%22", "\"" ) )
    ( std::make_pair( "%3C", "<" ) )
    ( std::make_pair( "%3E", ">" ) )
    ( std::make_pair( "%23", "#" ) )
    ( std::make_pair( "%7B", "{" ) )
    ( std::make_pair( "%7D", "}" ) )
    ( std::make_pair( "%7C", "|" ) )
    ( std::make_pair( "%5C", "\\" ) )
    ( std::make_pair( "%5E", "^" ) )
    ( std::make_pair( "%7E", "~" ) )
    ( std::make_pair( "%5B", "[" ) )
    ( std::make_pair( "%5D", "]" ) )
    ( std::make_pair( "%60", "`" ) )
    // прочие
    ( std::make_pair( "%28", "(" ) )
    ( std::make_pair( "%29", ")" ) )
;






static Variant parseData( const std::string& buffer ) {
   Variant var = json::parse( buffer.begin(), buffer.end() );

   /* - Быстрее. Проще. Обходимся.
   // Преобразуем результат в наш Object: есть Object, есть Array...
   // Надо пройти по всей глубине и заменить Object на Object.
   transformObject( &var );
   */
   /* - Заменено на transformObject().
   const auto& type = var->type();
   if (type == typeid( Object ) ) {
       const Object oldObj = boost::any_cast< Object >( *var );
       const Object obj = Object( oldObj );
       var = cjv( obj );

   } else {
       // оставляем без изменений
   }
   */


#ifdef COUCH_DB_DEBUG
   std::cout << "Data:" << std::endl
             << var << std::endl;
#endif

   return var;
}




static int writer( char *data, size_t size, size_t nmemb, std::string* dest ) {
    int written = 0;
    if ( dest ){
        written = size * nmemb;
        dest->append( data, written );
    }

    return written;
}




static size_t reader( void* ptr, size_t size, size_t nmemb, std::string* stream ) {
    int actual  = (int)stream->size();
    int written = size * nmemb;
    if(written > actual)
        written = actual;
    memcpy(ptr, stream->c_str(), written);
    stream->erase(0, written);
    return written;
}




Communication::Communication() {
   init( DEFAULT_COUCHDB_URL );
}




Communication::Communication( const std::string& url ) {
   init( url );
}




void Communication::init( const std::string& url ) {
   curl_global_init( CURL_GLOBAL_DEFAULT );

   curl = curl_easy_init();
   if ( !curl )
      throw Exception( "Unable to create CURL object" );

   if (curl_easy_setopt( curl, CURLOPT_NOPROGRESS, 1L ) != CURLE_OK)
      throw Exception( "Unable to set NOPROGRESS option." );

#ifdef _DEBUG
   // @test
   curl_easy_setopt( curl, CURLOPT_VERBOSE, 1L );
   //curl_easy_setopt( curl, CURLOPT_HEADER, 1L );
#endif

   if (curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, writer ) != CURLE_OK)
      throw Exception( "Unable to set writer function" );

   if (curl_easy_setopt( curl, CURLOPT_WRITEDATA, &buffer ) != CURLE_OK)
      throw Exception( "Unable to set write buffer" );

   if (curl_easy_setopt( curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1 ) != CURLE_OK)
      throw Exception( "Unable to set http-version" );

   if (curl_easy_setopt( curl, CURLOPT_NOSIGNAL, 1 ) != CURLE_OK)
      throw Exception( "Unable to set NOSIGNAL option." );

   if (curl_easy_setopt( curl, CURLOPT_FAILONERROR, 0 ) != CURLE_OK)
      throw Exception( "Unable to set FAILONERROR option." );

   // (!) Первое подключение к CouchDB после включения компьютера требует
   // неск. секунд. Источник: мой компьютер :)
   if (curl_easy_setopt( curl, CURLOPT_TIMEOUT, 5 ) != CURLE_OK)
      throw Exception( "Unable to set TIMEOUT option." );

   baseURL = url;
}




Communication::~Communication() {
    if ( curl ) {
        curl_easy_cleanup( curl );
    }
    curl_global_cleanup();
}




Variant Communication::getData(
    const std::string& url,
    const std::string& method,
    const std::string& data
) {
   HeaderMap headers;
   return getData(url, method, data, headers);
}




Variant Communication::getData(
    const std::string& url,
    const HeaderMap &headers,
    const std::string& method,
    const std::string& data
) {
   return getData( url, method, data, headers );
}




std::string Communication::getRawData( const std::string& url ) {
   HeaderMap headers;
   getRawData( url, "GET", "", headers );
   return buffer;
}




Variant Communication::getData(
    const std::string& url,
    const std::string& method,
    std::string data,
    const HeaderMap &headers
) {
   getRawData( url, method, data, headers );
   return parseData( buffer );
}




void Communication::getRawData(
    const std::string& _url,
    const std::string& method,
    const std::string& data,
    const HeaderMap& headers
) {
   /* - Лишнее. Передаём уже подготовленный ключ.
   std::string preparedURL = curl_easy_escape( curl, _url.c_str(), _url.length() );
   boost::replace_all( preparedURL, "%26", "&" );
   boost::replace_all( preparedURL, "%2F", "/" );
   boost::replace_all( preparedURL, "%3D", "=" );
   boost::replace_all( preparedURL, "%3F", "?" );
   const std::string url = baseURL + preparedURL;
   */
   const std::string url = baseURL + _url;

   const bool presentData = !data.empty();

   // Ниже - тяжёлое преобразование. Делаем попытку пропустить его.
   const auto needSafe = [] ( const std::string& s ) -> bool {
       for (auto itr = s.cbegin(); itr != s.cend(); ++itr) {
           const char ch = *itr;
           // русские буквы
           if ( ( (ch >= 'а' ) && (ch <= 'я') ) || ( (ch >= 'А' ) && (ch <= 'Я') ) ) {
               return true;
           }
       }
       return false;
   };


   std::string preparedData;
   if ( needSafe( data ) ) {
       // @todo fine optimize Как передавать серверу русские символы без этого костыля?
       //       Попробовать через файл? См. http://wiki.apache.org/couchdb/Quirks_on_Windows
       /* - Заменено. См. ниже.
       std::string preparedData = data;
       const bool utf8 = isUTF8( (unsigned char*)preparedData.c_str(), preparedData.size() );
       if ( utf8 ) {
            char* preparedDataPtr = curl_easy_escape( curl, data.c_str(), data.length() );
            preparedData = (std::string)preparedDataPtr;
            boost::replace_all( preparedData, "%7B", "{" );
            boost::replace_all( preparedData, "%7D", "}" );
            boost::replace_all( preparedData, "%0A", "\n" );
            boost::replace_all( preparedData, "%20", " " );
            boost::replace_all( preparedData, "%22", "\"" );
            boost::replace_all( preparedData, "%3A", ":" );
            curl_free( preparedDataPtr );
       }
       const long sizePreparedData = (long)preparedData.size();
       *//* - Заменено. См. ниже.
       std::string preparedData = curl_easy_escape( curl, data.c_str(), data.length() );
       char* preparedDataPtr = curl_easy_escape( curl, data.c_str(), data.length() );
       preparedData = (std::string)preparedDataPtr;
       // @source Escape code - http://blooberry.com/indexdot/html/topics/urlencoding.htm
       boost::replace_all( preparedData, "%7B", "{" );
       boost::replace_all( preparedData, "%7D", "}" );
       boost::replace_all( preparedData, "%0A", "\n" );
       boost::replace_all( preparedData, "%20", " " );
       boost::replace_all( preparedData, "%22", "\"" );
       boost::replace_all( preparedData, "%3A", ":" );
       curl_free( preparedDataPtr );
       const long sizePreparedData = (long)preparedData.size();
       */
       /* - Заменено. См. ниже.
       preparedData = curl_easy_escape( curl, data.c_str(), data.length() );
       char* preparedDataPtr = curl_easy_escape( curl, data.c_str(), data.length() );
       preparedData = (std::string)preparedDataPtr;
       std::for_each( FROM_RFC1738.cbegin(), FROM_RFC1738.cend(),
           //[ &preparedData ] ( const std::pair< std::string, std::string >&  code ) {
           [ &preparedData ] ( const std::map< std::string, std::string >::value_type&  code ) {
               boost::replace_all( preparedData, code.first, code.second );
       } );
       curl_free( preparedDataPtr );
       */
       preparedData = curl_easy_escape( curl, data.c_str(), data.length() );
       std::for_each( FROM_RFC1738.cbegin(), FROM_RFC1738.cend(),
           [ &preparedData ] ( const std::map< std::string, std::string >::value_type&  code ) {
               boost::replace_all( preparedData, code.first, code.second );
       } );

   } else {
       preparedData = data;
   }

   // Всегда экранируем символы перевода строки, иначе - ошибка при передаче
   /* - Не работает: ошибка при передаче.
   boost::replace_all( preparedData, "\n", "\\n" );
   */

   const long sizePreparedData = static_cast< long >( preparedData.size() );


#ifdef COUCH_DB_DEBUG
   std::cout << "Getting data: " << url << " [" << method << "]" << std::endl;
#endif

   buffer.clear();

   if (headers.size() > 0 || presentData){
      struct curl_slist* chunk = nullptr;

      HeaderMap::const_iterator header = headers.begin();
      const HeaderMap::const_iterator& headerEnd = headers.end();
      for(; header != headerEnd; ++header){
         std::string headerStr = header->first + ": " + header->second;
         chunk = curl_slist_append(chunk, headerStr.c_str());
      }

      if (presentData && ( (headers.find("Content-Type") == headers.end()) || (headers.find("charset") == headers.end()) ) ) {
            //chunk = curl_slist_append(chunk, "Content-Type: application/json; charset: UTF-8");
            chunk = curl_slist_append( chunk, "Accept: application/json" );
            chunk = curl_slist_append( chunk, "Content-Type: application/json; charset: UTF-8" );
            //chunk = curl_slist_append( chunk, "charset: utf-8" );
      }

      if (curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk) != CURLE_OK)
          throw Exception( "Unable to set custom header" );
   }

   if (curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str()) != CURLE_OK)
      throw Exception( "Unable to set HTTP method: " + method );

   if (curl_easy_setopt(curl, CURLOPT_URL, url.c_str()) != CURLE_OK)
      throw Exception( "Unable to set URL: " + url );

   if( presentData ) {
#ifdef COUCH_DB_DEBUG
      std::cout << "Sending data: " << preparedData << std::endl;
#endif

      if (curl_easy_setopt(curl, CURLOPT_READFUNCTION, reader) != CURLE_OK)
         throw Exception( "Unable to set read function" );

      if (curl_easy_setopt(curl, CURLOPT_READDATA, &preparedData) != CURLE_OK)
         throw Exception( "Unable to set data: " + preparedData);

      if (curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L) != CURLE_OK)
         throw Exception( "Unable to set upload request" );

      if (curl_easy_setopt(curl, CURLOPT_INFILESIZE, sizePreparedData) != CURLE_OK)
         throw Exception( "Unable to set content size" );
   }

   /* - Заменено. См. ниже.
   if(curl_easy_perform(curl) != CURLE_OK)
      throw Exception("Unable to load URL: " + url);
   */
   const auto errorPerform = curl_easy_perform( curl );
   if (errorPerform != CURLE_OK) {
       CURLINFO info = CURLINFO_NONE;
       const CURLcode codeError = curl_easy_getinfo( curl, info );
       const auto strError = curl_easy_strerror( codeError );
       std::cerr << strError << std::endl;
       throw Exception( "Unable to load URL: " + url );
   }


   if ( presentData || (headers.size() > 0) ) {
      if (curl_easy_setopt( curl, CURLOPT_UPLOAD, 0L ) != CURLE_OK)
         throw Exception( "Unable to reset upload request" );

      if (curl_easy_setopt( curl, CURLOPT_HTTPHEADER, NULL ) != CURLE_OK)
         throw Exception( "Unable to reset custom headers" );
   }

#ifdef COUCH_DB_DEBUG
   long responseCode;
   if (curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &responseCode ) != CURLE_OK)
      throw Exception( "Unable to get response code" );

   std::cout << "Response code: " << responseCode << std::endl;
   std::cout << "Raw buffer: " << buffer;
#endif

}
