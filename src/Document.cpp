#include "../include/Document.h"
#include "../include/Exception.h"


using namespace CouchFine;


Document::Document(Communication &_comm, const std::string& _db, const std::string& _id,
                   const std::string& _key, const std::string& _rev)
   : comm(_comm)
   , db(_db)
   , id(_id)
   , key(_key)
   , revision(_rev)
{
}




Document::Document(const Document &doc)
   : comm(doc.comm)
   , db(doc.db)
   , id(doc.id)
   , key(doc.key)
   , revision(doc.revision)
{
}

Document::~Document() {
}




Document& Document::operator=(Document& doc) {
   db       = doc.getDatabase();
   id       = doc.getID();
   key      = doc.getKey();
   revision = doc.getRevision();

   return *this;
}




bool Document::operator==(const Document &doc) {
   return (id == doc.getID());
}




Communication& Document::getCommunication() {
   return comm;
}




const std::string& Document::getDatabase() const {
   return db;
}




const std::string& Document::getID() const {
   return id;
}




const std::string& Document::getKey() const {
   return key;
}




const std::string& Document::getRevision() const {
   return revision;
}




void Document::setID( const std::string& id ) {
    this->id = id;
}




void Document::setRevision( const std::string& revision ) {
    this->revision = revision;
}




std::string Document::getURL( bool withRevision ) const {
   std::string url = "/" + getDatabase() + "/" + getID();
   if ( withRevision && !revision.empty() ) {
      url += "?rev=" + revision;
   }
   return url;
}




std::vector<Revision> Document::getAllRevisions() {
   std::vector< Revision > revisions;

   const Variant var = comm.getData( getURL( false ) + "?revs_info=true" );
   const Object obj = boost::any_cast< Object >( *var );
   const Array revInfo = boost::any_cast< Array >( *obj.at( "_revs_info" ) );

   for (auto itr = revInfo.cbegin(); itr != revInfo.cend(); ++itr) {
      const Object revObj = boost::any_cast< Object >( **itr );
      revisions.push_back( Revision(
          CouchFine::revision( revObj ),
          //boost::any_cast<std::string>( *revObj["status"] )
          CouchFine::value< std::string >( revObj, "status" )
      ) );
   }

   return revisions;
}




Variant Document::getData() const {
   const Variant var = comm.getData( getURL( false ) );
   const Object obj = boost::any_cast< Object >( *var );
   /* - Заменено. См. ниже.
   if ( (obj.find( "_id" ) == obj.end()) && (obj.find( "_rev" ) == obj.end()) &&
      (obj.find( "error" ) != obj.end()) && (obj.find( "reason" ) != obj.end()) ) {
      throw Exception("Document '" + getID() + "' not found: " + boost::any_cast<std::string>(*obj["reason"]));
   }
   */
   if ( hasError( obj ) ) {
      throw Exception( "Document '" + getID() + "' not found: " + error( obj ) );
   }

   return var;
}




bool Document::addAttachment(const std::string& attachmentId,
                             const std::string& contentType,
                             const std::string& data) {
   std::string url = getURL(false) + "/" + attachmentId;
   if ( !revision.empty() ) {
      url += "?rev=" + revision;
   }

   Communication::HeaderMap headers;
   headers["Content-Type"] = contentType;

   const Variant var = comm.getData( url, headers, "PUT", data );
   const Object obj = boost::any_cast< Object >( *var );
   if ( hasError( obj ) ) {
      throw Exception( "Could not create attachment '" + attachmentId + "': " + error( obj ) );
   }

   revision = CouchFine::revision( obj );

   return ok( obj );
}




Attachment Document::getAttachment(const std::string& attachmentId) {
   Object data = boost::any_cast<Object>(*getData());

   if(data.find("_attachments") == data.end())
      throw Exception("No attachments");

   Object attachments = boost::any_cast<Object>(*data["_attachments"]);
   if(attachments.find(attachmentId) == attachments.end())
      throw Exception("No attachment found with id '" + attachmentId + "'");

   Object attachment = boost::any_cast<Object>(*attachments[attachmentId]);

   return Attachment(comm, db, id, attachmentId, "", boost::any_cast<std::string>(*attachment["content_type"]));
}




std::vector<Attachment> Document::getAllAttachments() {
   Object data = boost::any_cast<Object>(*getData());

   if(data.find("_attachments") == data.end())
      throw Exception("No attachments");

   std::vector<Attachment> vAttachments;

   Object attachments = boost::any_cast<Object>(*data["_attachments"]);

   Object::iterator attachmentItr = attachments.begin();
   const Object::iterator &attachmentEnd = attachments.end();
   for(; attachmentItr != attachmentEnd; ++attachmentItr) {
      const std::string& attachmentId = attachmentItr->first;
      Object attachment = boost::any_cast<Object>(*attachmentItr->second);

      vAttachments.push_back(Attachment(comm, db, id, attachmentId, "",
                                        boost::any_cast<std::string>(*attachment["content_type"])));
   }

   return vAttachments;
}




bool Document::removeAttachment( const std::string& attachmentId ) {
   std::string url = getURL( false ) + "/" + attachmentId;
   if ( !revision.empty() ) {
      url += "?rev=" + revision;
   }

   const Variant var = comm.getData( url, "DELETE" );
   const Object obj = boost::any_cast< Object >( *var );
   if ( hasError( obj ) ) {
      throw Exception( "Could not delete attachment '" + attachmentId + "': " + error( obj ) );
   }

   revision = CouchFine::revision( obj );

   return ok( obj );
}




Document Document::copy( const uid_t& targetId, const rev_t& targetRev ) {
   Communication::HeaderMap headers;
   /* - Заменено. См. ниже.
   if ( targetRev.size() > 0 ) {
      headers["Destination"] = targetId + "?rev=" + targetRev;
   } else {
      headers["Destination"] = targetId;
   }
   */
   headers["Destination"] = targetRev.empty()
       ? targetId
       : (targetId + "?rev=" + targetRev);

   const Variant var = comm.getData( getURL( true ), headers, "COPY" );
   const Object obj = boost::any_cast< Object >( *var );
   if ( hasError( obj ) ) {
      throw Exception( "Could not copy document '" + getID() + "' to '" + targetId + "': " + error( obj ) );
   }

   const std::string newId = hasUID( obj ) ? uid( obj ) : targetId;

   return Document( comm, db, newId, "", CouchFine::revision( obj ) );
}




Document Document::update( Database& db, const Object& jo ) {

    Object o = jo;
    o["id"] = cjv( getID() );
    o["rev"] = cjv( getRevision() );
    const auto doc = db.createDocument( cjv( o ), getID() );

    return doc;
}




bool Document::remove() {
   const Variant var = comm.getData( getURL( true ), "DELETE" );
   const Object obj = boost::any_cast< Object >( *var );
   return ok( obj );
}




std::ostream& operator<<(std::ostream& out, const CouchFine::Document &doc) {
   return out << "{id: " << doc.getID() << ", key: " << doc.getKey() << ", rev: " << doc.getRevision() << "}";
}
