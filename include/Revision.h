#pragma once

#include "configure.h"


namespace CouchDB{

class Revision{
   public:
      Revision(const std::string&, const std::string&);
      Revision(const Revision&);
      ~Revision();

      const std::string& getVersion() const;
      const std::string& getStatus() const;

   private:
      std::string version;
      std::string status;
};

}

std::ostream& operator<<(std::ostream&, const CouchDB::Revision&);
