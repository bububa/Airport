#ifndef INSERTER_H_G063XSWV
#define INSERTER_H_G063XSWV

#include <mysql++/mysql++.h>
#include <string>
#include <map>
#include <boost/any.hpp>

namespace airport
{
    
    class Inserter
    {
    public:
        Inserter();
        ~Inserter();
        std::map<std::string, boost::any> parseConfig(const char *filename);
        std::map<std::string, boost::any> parseConfig(std::string &filename);
        mysqlpp::Connection connect(const char *server, const char *user, const char *passwd="", const char *db=0);
        mysqlpp::Connection connect(std::string &server, std::string &user, const std::string &passwd="", const std::string &db="");
        std::map< std::string, std::string > structure(mysqlpp::Connection &conn, std::string &table);
        void insert(mysqlpp::Connection &conn, std::string &table, std::map<std::string, std::string> &fieldMap, std::map<std::string, std::string> &defaultValues);
    };
}


#endif /* end of include guard: INSERTER_H_G063XSWV */
