#include "CouchDB.h"


#define TEST_DB_NAME "db_test1"
#define TEST_ID      "id_test1"


using namespace std;

// TODO: debug why copy is not working correctly below
template<typename T>
void output(const std::vector<T> &v, std::ostream &out){
   typename std::vector<T>::const_iterator i = v.begin();
   const typename std::vector<T>::const_iterator &iEnd = v.end();
   for(; i != iEnd; ++i)
      out << *i << " ";
}


static void printDatabases(CouchDB::Connection &conn){
   std::vector<std::string> dbs = conn.listDatabases();
   copy(dbs.begin(), dbs.end(), ostream_iterator<std::string>(cout, " "));
   cout << endl;
}

static void printDocuments(CouchDB::Database db){
   std::vector<CouchDB::Document> docs = db.listDocuments();
   cout << db.getName() << " has " << docs.size() << " documents: ";
   //copy(docs.begin(), docs.end(), ostream_iterator<CouchDB::Document>(cout, " "));
   output(docs, cout);
   cout << endl;
}


static void printAttachments(CouchDB::Document &doc){
   std::vector<CouchDB::Attachment> attachments = doc.getAllAttachments();
   cout << "Attachments for document " << doc << ": ";
   //copy(attachments.begin(), attachments.end(), ostream_iterator<CouchDB::Attachment>(cout, " "));
   output(attachments, cout);
   cout << endl;
}


static CouchDB::Variant createRecord(int key, const std::string &value,
                                   const std::string &value2, double dValue){
   CouchDB::Object obj;
   obj["key"]    = CouchDB::createVariant(key);
   obj["value"]  = CouchDB::createVariant(value);
   obj["value2"] = CouchDB::createVariant(value2);
   obj["dValue"] = CouchDB::createVariant(dValue);
   return CouchDB::createVariant(obj);
}


int main(){
   //setenv("http_proxy", "", 1);

   try{
      CouchDB::Connection conn;
      cout << "CouchDB version: " << conn.getCouchDBVersion() << endl;

      cout << "Initial database set: ";
      printDatabases(conn);

      cout << "Creating test database" << endl;

      conn.createDatabase(TEST_DB_NAME);

      cout << "New database set: ";
      printDatabases(conn);

      CouchDB::Database db = conn.getDatabase(TEST_DB_NAME);

      CouchDB::Array records;
      records.push_back(createRecord(12345, "T1", "DIE", 5.0));
      records.push_back(createRecord(45678, "T2", "C0" , 6.0));
      records.push_back(createRecord(89012, "T3", "C1" , 7.0));

      CouchDB::Object obj;
      obj["x1"]      = CouchDB::createVariant("A12345");
      obj["x2"]      = CouchDB::createVariant("ABCDEF");
      obj["records"] = CouchDB::createVariant(records);

      CouchDB::Variant data = CouchDB::createVariant(obj);
      cout << "Creating new document" << endl;
      CouchDB::Document doc = db.createDocument(data);
      cout << "Created doc: " << doc << endl;

      printDocuments(db);

      CouchDB::Document testDoc = db.getDocument(doc.getID());

      cout << "Document in database: "
           << testDoc << ": "
           << testDoc.getData() << endl;

      cout << "Comparing docs: " << doc << " and " << testDoc << endl;

      if(doc == testDoc)
         cout << "Verified documents are the same" << endl;
      else
         cout << "Documents differ:" << endl
              << doc.getData() << endl
              << testDoc.getData() << endl;

      cout << "Creating a version of the document with id: " << TEST_ID << endl;
      CouchDB::Document docId = db.createDocument(data, TEST_ID);
      cout << "Created new document: " << docId << endl;

      docId.addAttachment("h1", "text/plain", "hello world");
      docId.addAttachment("h2", "text/plain", "cool world");

      CouchDB::Attachment a = docId.getAttachment("h1");
      cout << "Got attachment: " << a << ": " << a.getData() << endl;

      printAttachments(docId);

      cout << "Removing attachment: " << a << endl;
      docId.removeAttachment(a.getID());

      printAttachments(docId);

      cout << "Trying to access removed attachment: " << a << endl;

      try{
         std::string badData = a.getData();
         cerr << "ERROR: Got valid data for removed attachment: " << badData << endl;
      }
      catch(CouchDB::Exception &e){
         cout << "Unable to retrieve data for removed attachment (GOOD): " << e.what() << endl;
      }

      docId.remove();

      cout << "Copying document: " << doc << endl;
      CouchDB::Document copyDoc = doc.copy(TEST_ID);
      cout << "Got document copy: " << copyDoc << endl;
      copyDoc.remove();

      doc.remove();

      cout << "Docs removed" << endl;

      printDocuments(db);

      cout << "Trying to retrieve data for removed document: " << testDoc << endl;

      try{
         data = testDoc.getData();
         cout << "ERROR: Got valid data for removed document: " << data << endl;
      }
      catch(CouchDB::Exception &e){
         cerr << "Unable to retrieve data for removed document (GOOD): " << e.what() << endl;
      }

      cout << "Removing test database" << endl;

      conn.deleteDatabase(TEST_DB_NAME);

      cout << "Final database set: ";
      printDatabases(conn);

      cout << "All tests passed" << endl;
   }
   catch(exception &e){
      cerr << "Exception: " << e.what() << endl;
   }
   catch(...){
      cerr << "Caught unknown exception in main..." << endl;
   }
}
