#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <boost/filesystem.hpp>
#include <boost/property_tree/info_parser.hpp>
#include "airport/Config.h"

namespace fs = boost::filesystem;

int main()
{
    std::string data_path, mongodb_host, log_host, debug_level, mysql_server, mysql_user, mysql_passwd, mysql_db, mysql_table, license, activate_code;
    std::cout << "This is a crawler system designed for vertical search engine.\n" << std::endl;
    std::cout << "CODE name: Airport" << std::endl;
    std::cout << "Version: " << airport::AIRPORT_VERSION << std::endl;
    std::cout << "Created on 04/21/2009" << std::endl;
    std::cout << "Last modified on 04/21/2009" << std::endl;
    std::cout << "Author: prof.syd.xu@gmail.com, who is a developer in JiPingMi.com living in China.\n" << std::endl;
    std::cout << "Please complete the following environment parameters..." << std::endl;
    /* SETTING DATA_PATH */
    while(true)
    {
        std::cout << "1. [Data Path] is the path on your server to store the files which will be used by the system like cookies, crawler rules and inserter rules etc. Default [Data Path] is \"/var/data/airport/\" if you leave it empty." << std::endl;
        std::cin >> data_path;
        if (data_path.empty())
        {
            data_path = "/var/data/airport/";
        }
        fs::path info_path( data_path );
        if(fs::exists(info_path)) 
        {
            break;
        }
        std::cout << "[Data Path]: " << data_path << " does not exist!" << std::endl;
    }
    /* SETTING MONGODB HOST */
    std::cout << "2. [MongoDB Host] is the ip:port address for mongodb which is the storage system for the crawler. You could get more information about mongodb at mongodb.org." << std::endl;
    std::cin >> mongodb_host;
    /* SETTING MONGODB HOST */
    std::cout << "3. [LOG Host] is the ip:port address for log server." << std::endl;
    std::cin >> log_host;
    /* SETTING DEBUG LEVEL */
    while(true)
    {
        std::cout << "4. [DEBUG Level] (off/normal/warning/error/critical). Default [DEBUG Level] is normal." << std::endl;
        std::cin >> debug_level;
        std::transform(debug_level.begin(), debug_level.end(), debug_level.begin(), (int(*)(int)) std::tolower);
        if (debug_level == "off" || debug_level == "normal" || debug_level == "warning" || debug_level == "error" || debug_level == "critical")
            break;
    }
    
    /* SETTING MySQL Server */
    std::cout << "5. [MySQL Server]: ";
    std::cin >> mysql_server;
    std::cout << "6. [MySQL Database]: ";
    std::cin >> mysql_db;
    std::cout << "7. [MySQL User]: ";
    std::cin >> mysql_user;
    std::cout << "8. [MySQL Password]:";
    std::cin >> mysql_passwd;
    std::cout << "9. [MySQL Table]: ";
    std::cin >> mysql_table;
    
    /* ACTIVATION CODE AND LISENSE */
    std::cout << "10. [LISENSE]: ";
    std::cin >> license;
    std::cout << "11. [ACTIVATION CODE]: ";
    std::cin >> activate_code;
    
    /* WRITE configure.info */
    boost::property_tree::ptree pt;
    pt.put("DATA_PATH", data_path);
    pt.put("MONGODB_HOST", mongodb_host);
    pt.put("LOG_HOST", log_host);
    pt.put("DEBUG_LEVEL", debug_level);
    pt.put("MYSQL_SERVER", mysql_server);
    pt.put("MYSQL_DB", mysql_db);
    pt.put("MYSQL_USER", mysql_user);
    pt.put("MYSQL_PASSWD", mysql_passwd);
    pt.put("MYSQL_TABLE", mysql_table);
    pt.put("LICENSE", license);
    pt.put("ACTIVATE_CODE", activate_code);
    boost::property_tree::write_info("configure.info", pt);
    return 0;
}