#include "Revision.h"


using namespace CouchDB;


Revision::Revision(const std::string &_version, const std::string &_status)
   : version(_version)
   , status(_status)
{
}

Revision::Revision(const Revision &revision)
   : version(revision.getVersion())
   , status(revision.getStatus())
{
}

Revision::~Revision(){
}

const std::string& Revision::getVersion() const{
   return version;
}

const std::string& Revision::getStatus() const{
   return status;
}

std::ostream& operator<<(std::ostream &out, const CouchDB::Revision &rev){
   return out << "{rev: " << rev.getVersion() << ", status: " << rev.getStatus() << "}";
}
