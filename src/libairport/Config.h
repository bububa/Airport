#ifndef CONFIG_H_LG5O98UQ
#define CONFIG_H_LG5O98UQ

#include <string>

namespace airport
{
    typedef enum {
        DEBUG_OFF = 0,
        DEBUG_NORMAL,
        DEBUG_WARNING,
        DEBUG_ERROR,
        DEBUG_CRITICAL,
    } AirportDebugLevel;
    
    static const std::string AIRPORT_VERSION("1.0");
    static const std::string DATA_PATH("/Users/syd/Documents/Works/codes/Airport/data/");
    static const std::string MONGODB_HOST("localhost");
    static const int DEBUG_LEVEL = airport::DEBUG_NORMAL;
    static const std::string MYSQL_SERVER("localhost");
    static const std::string MYSQL_DB("airport");
    static const std::string MYSQL_USER("root");
    static const std::string MYSQL_PASSWD("00320398");
    static const std::string MYSQL_TABLE("httpresponse");
    static const std::string LICENSE("E310379A08F5158EE027CC80C570AA075F1474177FE6906633E2C1095AE8E9BFABF76CC013FF638831E840A017B5EC10");
    static const std::string ACTIVATE_CODE("97C310EDE6AB26605027FA61648944F2");
}

#endif /* end of include guard: CONFIG_H_LG5O98UQ */
