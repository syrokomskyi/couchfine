#include "Document.h"
#include "Exception.h"


using namespace CouchDB;


Document::Document(Communication &_comm, const std::string &_db, const std::string &_id,
                   const std::string &_key, const std::string &_rev)
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

Document::~Document(){
}

Document& Document::operator=(Document& doc){
   db       = doc.getDatabase();
   id       = doc.getID();
   key      = doc.getKey();
   revision = doc.getRevision();

   return *this;
}

bool Document::operator==(const Document &doc){
   return (id == doc.getID());
}

Communication& Document::getCommunication(){
   return comm;
}

const std::string& Document::getDatabase() const{
   return db;
}

const std::string& Document::getID() const{
   return id;
}

const std::string& Document::getKey() const{
   return key;
}

const std::string& Document::getRevision() const{
   return revision;
}



void Document::setID( const std::string& id ) {
    this->id = id;
}



void Document::setRevision( const std::string& revision ) {
    this->revision = revision;
}



std::string Document::getURL(bool withRevision) const{
   std::string url = "/" + getDatabase() + "/" + getID();
   if(withRevision && revision.size() > 0)
      url += "?rev=" + revision;
   return url;
}

std::vector<Revision> Document::getAllRevisions(){
   std::vector<Revision> revisions;

   Variant var = comm.getData(getURL(false) + "?revs_info=true");
   Object  obj = boost::any_cast<Object>(*var);

   Array revInfo = boost::any_cast<Array>(*obj["_revs_info"]);

   Array::iterator        revInfoItr = revInfo.begin();
   const Array::iterator &revInfoEnd = revInfo.end();
   for(; revInfoItr != revInfoEnd; ++revInfoItr){
      Object revObj    = boost::any_cast<Object>(**revInfoItr);
      revisions.push_back(Revision(boost::any_cast<std::string>(*revObj["rev"]),
                                   boost::any_cast<std::string>(*revObj["status"])));
   }

   return revisions;
}

Variant Document::getData() const {
   Variant var = comm.getData(getURL(false));
   Object  obj = boost::any_cast<Object>(*var);

   if(obj.find("_id") == obj.end() && obj.find("_rev") == obj.end() &&
      obj.find("error") != obj.end() && obj.find("reason") != obj.end())
      throw Exception("Document '" + getID() + "' not found: " + boost::any_cast<std::string>(*obj["reason"]));

   return var;
}

bool Document::addAttachment(const std::string &attachmentId,
                             const std::string &contentType,
                             const std::string &data){
   std::string url = getURL(false) + "/" + attachmentId;
   if(revision.size() > 0)
      url += "?rev=" + revision;

   Communication::HeaderMap headers;
   headers["Content-Type"] = contentType;

   Variant var = comm.getData(url, headers, "PUT", data);
   Object  obj = boost::any_cast<Object>(*var);

   if(obj.find("error") != obj.end() && obj.find("reason") != obj.end())
      throw Exception("Could not create attachment '" + attachmentId + "': " + boost::any_cast<std::string>(*obj["reason"]));

   revision = boost::any_cast<std::string>(*obj["rev"]);

   return boost::any_cast<bool>(*obj["ok"]);
}

Attachment Document::getAttachment(const std::string &attachmentId){
   Object data = boost::any_cast<Object>(*getData());

   if(data.find("_attachments") == data.end())
      throw Exception("No attachments");

   Object attachments = boost::any_cast<Object>(*data["_attachments"]);
   if(attachments.find(attachmentId) == attachments.end())
      throw Exception("No attachment found with id '" + attachmentId + "'");

   Object attachment = boost::any_cast<Object>(*attachments[attachmentId]);

   return Attachment(comm, db, id, attachmentId, "", boost::any_cast<std::string>(*attachment["content_type"]));
}

std::vector<Attachment> Document::getAllAttachments(){
   Object data = boost::any_cast<Object>(*getData());

   if(data.find("_attachments") == data.end())
      throw Exception("No attachments");

   std::vector<Attachment> vAttachments;

   Object attachments = boost::any_cast<Object>(*data["_attachments"]);

   Object::iterator attachmentItr = attachments.begin();
   const Object::iterator &attachmentEnd = attachments.end();
   for(; attachmentItr != attachmentEnd; ++attachmentItr){
      const std::string &attachmentId = attachmentItr->first;
      Object attachment = boost::any_cast<Object>(*attachmentItr->second);

      vAttachments.push_back(Attachment(comm, db, id, attachmentId, "",
                                        boost::any_cast<std::string>(*attachment["content_type"])));
   }

   return vAttachments;
}

bool Document::removeAttachment(const std::string &attachmentId){
   std::string url = getURL(false) + "/" + attachmentId;
   if(revision.size() > 0)
      url += "?rev=" + revision;

   Variant var = comm.getData(url, "DELETE");
   Object  obj = boost::any_cast<Object>(*var);

   if(obj.find("error") != obj.end() && obj.find("reason") != obj.end())
      throw Exception("Could not delete attachment '" + attachmentId + "': " + boost::any_cast<std::string>(*obj["reason"]));

   revision = boost::any_cast<std::string>(*obj["rev"]);

   return boost::any_cast<bool>(*obj["ok"]);
}




Document Document::copy(const std::string &targetId, const std::string &targetRev){
   Communication::HeaderMap headers;
   if(targetRev.size() > 0)
      headers["Destination"] = targetId + "?rev=" + targetRev;
   else
      headers["Destination"] = targetId;

   Variant var = comm.getData(getURL(true), headers, "COPY");
   Object  obj = boost::any_cast<Object>(*var);

   if(obj.find("error") != obj.end() && obj.find("reason") != obj.end())
      throw Exception("Could not copy document '" + getID() + "' to '" + targetId + "': " + boost::any_cast<std::string>(*obj["reason"]));

   std::string newId = targetId;
   if(obj.find("id") != obj.end())
      newId = boost::any_cast<std::string>(*obj["id"]);

   return Document(comm, db, newId, "", boost::any_cast<std::string>(*obj["rev"]));
}




Document Document::update( Database& db, const Object& jo ) {

    Object o = jo;
    o["id"] = createVariant( this->getID() );
    o["rev"] = createVariant( this->getRevision() );
    const auto doc = db.createDocument( createVariant( o ), this->getID() );

    return doc;
}




bool Document::remove(){
   Variant var = comm.getData(getURL(true), "DELETE");
   Object  obj = boost::any_cast<Object>(*var);

   return boost::any_cast<bool>(*obj["ok"]);
}




std::ostream& operator<<(std::ostream &out, const CouchDB::Document &doc){
   return out << "{id: " << doc.getID() << ", key: " << doc.getKey() << ", rev: " << doc.getRevision() << "}";
}
