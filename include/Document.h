#pragma once

#include "configure.h"
#include "Revision.h"
#include "Attachment.h"
#include "Database.h"


namespace CouchDB{

class Document{
   friend class Database;

   protected:
      Document(Communication&, const std::string&,
               const std::string&, const std::string&, const std::string&);

   public:
      Document(const Document&);
      ~Document();

      Document& operator=(Document&);
      bool operator==(const Document&);

      const std::string& getID() const;
      const std::string& getKey() const;
      const std::string& getRevision() const;

      void setID( const std::string& id );
      void setRevision( const std::string& revision );

      std::vector<Revision> getAllRevisions();

      Variant getData() const;

      bool addAttachment(const std::string&, const std::string&,
                         const std::string&);
      Attachment getAttachment(const std::string&);
      std::vector<Attachment> getAllAttachments();
      bool removeAttachment(const std::string&);

      Document copy( const std::string&, const std::string &rev = "" );
      Document update( Database& db, const Object& jo );
      bool remove();

   protected:
      Communication& getCommunication();
      const std::string& getDatabase() const;

      std::string getURL(bool) const;

   private:
      Communication &comm;
      std::string   db;
      std::string   id;
      std::string   key;
      std::string   revision;
};

}


std::ostream& operator<<(std::ostream&, const CouchDB::Document&);
