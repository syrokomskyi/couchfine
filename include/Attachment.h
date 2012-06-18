#pragma once

#include "configure.h"
#include "Communication.h"


namespace CouchFine {

class Attachment{
   friend class Document;

   protected:
      Attachment(Communication&, const std::string&, const std::string&,
                 const std::string&, const std::string&, const std::string&);

   public:
      Attachment(const Attachment&);
      ~Attachment();

      Attachment& operator=(Attachment&);

      const std::string& getID() const;
      const std::string& getRevision() const;
      const std::string& getContentType() const;

      std::string getData() const;

   private:
      Communication &comm;
      std::string   db;
      std::string   document;
      std::string   id;
      std::string   revision;
      std::string   contentType;
      std::string   rawData;
};

}

std::ostream& operator<<(std::ostream&, const CouchFine::Attachment&);
