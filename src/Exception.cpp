#include "../include/Exception.h"


using namespace CouchFine;


Exception::Exception(const std::string& _message) {
   message = _message;
}

Exception::~Exception() throw() {
}

const char* Exception::what() const throw() {
   return message.c_str();
}
