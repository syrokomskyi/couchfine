#include "../include/Attachment.h"
#include "../include/Exception.h"

using namespace CouchFine;

Attachment::Attachment(Communication &_comm, const std::string& _db,
                       const std::string& _document, const std::string& _id,
                       const std::string& _revision, const std::string& _contentType)
   : comm(_comm)
   , db(_db)
   , document(_document)
   , id(_id)
   , revision(_revision)
   , contentType(_contentType)
{
}

Attachment::Attachment(const Attachment &attachment)
   : comm(attachment.comm)
   , db(attachment.db)
   , document(attachment.document)
   , id(attachment.id)
   , revision(attachment.revision)
{
}

Attachment::~Attachment(){
}

Attachment& Attachment::operator=(Attachment &attachment){
   comm     = attachment.comm;
   db       = attachment.db;
   document = attachment.document;
   id       = attachment.id;
   revision = attachment.revision;

   return *this;
}

const std::string& Attachment::getID() const{
   return id;
}

const std::string& Attachment::getRevision() const{
   return revision;
}

const std::string& Attachment::getContentType() const{
   return contentType;
}

std::string Attachment::getData() const {
   std::string data;

   if(rawData.size() > 0) {
      data = rawData;
   } else {
      std::string url = "/" + db + "/" + document + "/" + id;
      if(revision.size() > 0)
         url += "?rev=" + revision;
      data = comm.getRawData(url);

      if (data.size() > 0 && data[0] == '{') {
         // check to make sure we did not receive an error
         Object obj = boost::any_cast<Object>( *comm.getData( url ) );
         if ( hasError( obj ) ) {
            throw Exception( "Could not retrieve data for attachment '" + id + "': " + error( obj ) );
         }
      }
   }

   return data;
}





std::ostream& operator<<( std::ostream& out, const CouchFine::Attachment& attachment ) {
   return out << "{id: " << attachment.getID()
              << ", rev: " << attachment.getRevision()
              << ", content-type: " << attachment.getContentType()
              << "}";
}
