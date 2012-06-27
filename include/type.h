#pragma once

#include "configure.h"
#include <json-type.h>
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/function.hpp>

// @todo optimize option Перенесено в typelib. Дополнить typelib этой возможностью.
#ifdef SAVE_ORDER_FIELDS
    #include <map>
#else
    #include <boost/unordered_map.hpp>
#endif


/**
* Вспомогательные типы данных и методы для удобной работы с ними.
*/


namespace CouchFine {

/**
* UID документа.
*/
typedef std::string uid_t;


/**
* Ревизия документа.
*/
typedef std::string rev_t;


typedef std::pair< uid_t, rev_t >  uidrev_t;



/**
* Типы, удобные для работы с TinyJSON.
*//* - Заменены на типы из typelib. См. ниже.
typedef boost::shared_ptr< boost::any >  Variant;
typedef std::deque< Variant >            Array;

#ifdef SAVE_ORDER_FIELDS
typedef std::map< std::string, Variant >  Object;
#else
typedef boost::unordered_map< std::string, Variant >  Object;
#endif
*/

typedef typelib::json::Variant  Variant;
typedef typelib::json::Array    Array;
typedef typelib::json::Object   Object;

// @see typelib::json::cjv()



/**
* Функция для создания строки JSON из полученного значения.
* Может быть переопределена, чтобы транслировать собственные классы.
*/
typedef boost::function< std::string( const CouchFine::Variant& v ) >  fnCreateJSON_t;



// Методы для взаимодействия с Object

/**
* @return Преобразованный к заданному типу Variant (static_cast).
*//* - @todo fine Добавить static_cast к Variant. Наследовать от boost::any?
inline operator const Object& ( const Variant& var ) {
    return boost::any_cast< Object& >( *var );
}
*/



/**
* @return Значение поля объекта, преобразованное к заданному типу или
*         'def' если поле с заданным именем в объекте не найдено.
*/
template < typename T >
static inline T v( const Object& o, const std::string& field, const T& def = T() ) {
    const auto ftr = o.find( field );
    return (ftr == o.cend())
        ? def : boost::any_cast< T >( *ftr->second );
}


// @alias v()
template < typename T >
static inline T value( const Object& o, const std::string& field, const T& def = T() ) {
    return v< T >( o, field, def );
}




/**
* @return UID объекта или пустая строка, если UID не задан.
*/
static inline uid_t uid( const Object& o ) {
    auto ftr = o.find( "_id" );
    if (ftr != o.cend()) {
        return boost::any_cast< std::string >( *ftr->second );
    }
    // При получении д., UID идёт без префикса '_'
    ftr = o.find( "id" );
    if (ftr != o.cend()) {
        return boost::any_cast< std::string >( *ftr->second );
    }
    return "";
}


/**
* Устанавливает UID для объекта.
* @return Ссылку на объект, чтобы можно было писать код так:
*           store >> doc.uid( "MyID", "MyRevision" )
*         Такой код извлечёт документ из хранилища в 'doc'.
*/
static inline Object& uid( Object& o, const uid_t& u, const rev_t& r = "" ) {
    assert( !u.empty() && "Пустой UID." );
    assert( (o.find( "id" ) == o.cend())
        && "Обнаружено поле 'id'. Используйте '_id' для корректного сохранения документа в хранилище без потери производительности." );
    o[ "_id" ] = typelib::json::cjv( u );
    if ( !r.empty() ) {
        o[ "_rev" ] = typelib::json::cjv( r );
    }
    return o;
}


static inline Object& uid( Object& o, const uidrev_t& ur ) {
    return uid( o, ur.first, ur.second );
}



/**
* @return true, если задан UID.
*/
static inline bool hasUID( const Object& o ) {
    assert( (o.find( "id" ) == o.cend())
        && "Обнаружено поле 'id'. Вероятно, ошибка: используйте '_id' для корректного сохранения документа в хранилище." );
    const auto ftr = o.find( "_id" );
    return (ftr != o.cend());
}



/**
* @return Ревизия объекта или пустая строка, если ревизия не задана.
*/
static inline rev_t revision( const Object& o ) {
    auto ftr = o.find( "_rev" );
    if (ftr != o.cend()) {
        return boost::any_cast< std::string >( *ftr->second );
    }
    // При получении д., ревизия идёт без префикса '_'
    ftr = o.find( "rev" );
    if (ftr != o.cend()) {
        return boost::any_cast< std::string >( *ftr->second );
    }
    return "";
}



/**
* Устанавливает ревизию для объекта.
* @return Ссылку на объект, чтобы можно было писать код так:
*           store >> doc.rev( "MyRevision" )
*         Такой код извлечёт документ из хранилища в 'doc'.
*/
static inline Object& revision( Object& o, const rev_t& r ) {
    assert( !r.empty() && "Пустая ревизия." );
    o[ "_rev" ] = typelib::json::cjv( r );
    return o;
}



/**
* @return true, если задана ревизия.
*/
static inline bool hasRevision( const Object& o ) {
    const auto ftr = o.find( "_rev" );
    return (ftr != o.cend());
}




/**
* @return Сообщение об ошибке или пустая строка, если ошибки нет.
*         Удобно для проверки после обращения с запросом к хранилищу.
*/
static inline std::string error( const Object& o ) {
    const auto fte = o.find( "error" );
    if (fte != o.cend()) {
        std::string e = boost::any_cast< std::string >( *fte->second );
        const auto ftr = o.find( "reason" );
        if (ftr != o.cend()) {
            e += boost::any_cast< std::string >( *ftr->second );
        }
    }
    return "";
}



static inline std::string error( const Variant& var ) {
    return ( var && (var->type() == typeid( Object )) )
        ? error( boost::any_cast< Object >( *var ) )
        : "";
}





/**
* @return true, если объект является сообщением CouchDB об ошибке.
*/
static inline bool hasError( const Object& o ) {
    return (o.find( "error" ) != o.cend());
}


static inline bool hasError( const Variant& var ) {
    return var && (var->type() == typeid( Object ))
        && hasError( boost::any_cast< Object >( *var ) );
}


/**
* @return true, если объект содержит поле 'reason' (детализирует ошибку).
*/
static inline bool hasReason( const Object& o ) {
    return (o.find( "reason" ) != o.cend());
}





/**
* @return Значение поля 'ok' или false, если поле не найдено. Это поле часто
*         идёт в ответе сервера.
*/
static inline bool ok( const Object& o ) {
    const auto ftr = o.find( "ok" );
    return (ftr == o.cend())
        ? false
        : boost::any_cast< bool >( *ftr->second );
}



} // CouchFine





/**
* Записывает в объект пару "ключ : значение".
*//* - Появляется неоднозначность для operator<<( Object& , uidrev_t& ).
inline CouchFine::Object& operator<<(
    CouchFine::Object& o,
    const std::pair< const std::string&, const boost::any& >&  kv
) {
    // не создаём матрёшек: обеспечивает typelib::json::cjv()
    o[ kv.first ] = typelib::json::cjv( kv.second );
    return o;
}



template< typename T >
inline CouchFine::Object& operator<<(
    CouchFine::Object& o,
    const std::pair< std::string, const T& >&  kv
) {
    o[ kv.first ] = typelib::json::cjv( kv.second );
    return o;
}
*/




inline CouchFine::Object& operator<<(
    CouchFine::Object& o,
    const CouchFine::uidrev_t& ur
) {
    CouchFine::uid( o, ur );
    return o;
}
