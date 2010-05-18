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
    static const std::string MONGODB_HOST("localhost:27017");
    static const std::string LOG_HOST("localhost:27017");
    static const int DEBUG_LEVEL = airport::DEBUG_NORMAL;
    static const std::string MYSQL_SERVER("localhost");
    static const std::string MYSQL_DB("airport");
    static const std::string MYSQL_USER("syd");
    static const std::string MYSQL_PASSWD("00320398");
    static const std::string MYSQL_TABLE("httpresponse");
    static const std::string LICENSE("2511BDF6E517E8D5C853A79B863744B637A1E3389276DD3E3B82A9C0FA0C0CB5E3C7BB9E717F817F7CEC419B05EDFFB5");
    static const std::string ACTIVATE_CODE("9505D3EEF376D30BEA1478F04FCAE836");
}

#endif /* end of include guard: CONFIG_H_LG5O98UQ */
