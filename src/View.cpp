#include "View.h"
#include <plustache/include/template.hpp>
#include <plustache/include/context.hpp>


using namespace CouchDB;


View View::valueOf(
    const std::string& folder,
    const std::map< std::string, std::string >&  context
) {
    namespace fs = boost::filesystem;

    assert( fs::exists( folder ) && "Folder is not exists." );
    assert( fs::is_directory( folder ) && "Path is not directory." );

    // Название представления
    const std::string name = fs::basename( folder );

    const std::string f = folder
      + ( (folder[ folder.length() - 1 ] == '/') ? "" : "/" );

    const std::string map = f + "map.js";
    assert( fs::exists( map ) && "File 'map.js' is not exists." );
    std::ifstream fMap( f );
    std::string sMap = std::string(
        std::istreambuf_iterator< char >( fMap ),
        std::istreambuf_iterator< char >()
    );

    // reduce.js - не обязательный файл
    const std::string reduce = f + "reduce.js";
    std::string sReduce = "";
    if ( fs::exists( reduce ) ) {
        std::ifstream fReduce( f );
        sReduce = std::string(
            std::istreambuf_iterator< char >( fReduce ),
            std::istreambuf_iterator< char >()
        );
    }

    // Файлы формируем согласно контексту
    if (context.size() > 0) {
        template_t t;
        sMap = t.render( sMap, context );
        sReduce = t.render( sReduce, context );
    }

    return View( name, sMap, sReduce );
}
