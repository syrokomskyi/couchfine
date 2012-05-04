#pragma once

#include "configure.h"


namespace CouchDB{

class Exception : public std::exception{
   public:
      Exception(const std::string&);
      virtual ~Exception() throw();
      virtual const char* what() const throw();

   private:
      std::string message;
};

}
