#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include "airport/Config.h"
#include "airport/Inserter.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

bool find_file( const fs::path & dir_path,         // in this directory,
                const std::string & file_name, // search for this name,
                fs::path & path_found )            // placing path here if found
{
    if ( !fs::exists( dir_path ) ) return false;
    fs::directory_iterator end_itr; // default construction yields past-the-end
    for ( fs::directory_iterator itr( dir_path ); itr != end_itr;++itr )
    {
        if ( fs::is_directory(itr->status()) )
        {
            if ( find_file( itr->path(), file_name, path_found ) ) return true;
        }else if ( itr->leaf() == file_name ){
            path_found = itr->path();
            return true;
        }
    }
    return false;
}

void
insert(std::string &config_file, std::string &server, std::string &user, std::string &passwd, std::string &db, std::string &table)
{
    airport::Inserter inserter;
    std::map<std::string, boost::any> config = inserter.parseConfig(config_file);
    if (!boost::any_cast<std::string>(config["server"]).empty()) server = boost::any_cast<std::string>(config["server"]);
    if (!boost::any_cast<std::string>(config["user"]).empty()) user = boost::any_cast<std::string>(config["user"]);
    if (!boost::any_cast<std::string>(config["passwd"]).empty()) passwd = boost::any_cast<std::string>(config["passwd"]);
    if (!boost::any_cast<std::string>(config["db"]).empty()) db = boost::any_cast<std::string>(config["db"]);
    if (!boost::any_cast<std::string>(config["table"]).empty()) table = boost::any_cast<std::string>(config["table"]);
    try {
        mysqlpp::Connection conn = inserter.connect(server, user, passwd, db);
        std::map<std::string, std::string> fieldMap = boost::any_cast< std::map<std::string, std::string> >(config["fieldMap"]);
        std::map<std::string, std::string> defaultValues = boost::any_cast< std::map<std::string, std::string> >(config["defaultValues"]);
        inserter.insert(conn, table, fieldMap, defaultValues);
    }catch(const mysqlpp::ConnectionFailed& err){
        std::cerr << "Failed to connect to database server: " << err.what() << std::endl;
    }catch (const mysqlpp::Exception& er) {
        std::cerr << "Error: " << er.what() << std::endl;
    }
}

int main(int argc, char *argv[])
{
    std::string server = airport::MYSQL_SERVER;
    std::string db = airport::MYSQL_DB;
    std::string user = airport::MYSQL_USER;
    std::string passwd = airport::MYSQL_PASSWD;
    std::string table = airport::MYSQL_TABLE;
    
    /* PARSE COMMAND LINE */
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("config,c", po::value<std::string>(), "set config file")
        ("server,s", po::value<std::string>(), "set server, host:port")
        ("db,d", po::value<std::string>(), "set database name")
        ("user,u", po::value<std::string>(), "set username")
        ("passwd,p", po::value<std::string>(), "set password")
        ("table,t", po::value<std::string>(), "set table")
        ("status", "show status")
    ;
    
    po::variables_map vm;
    try
    {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    }catch(po::invalid_command_line_syntax const &){
        std::cout << "Invalid command, please use --help." << std::endl;
        return 0;
    }
    
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }
    
    if (!vm.count("config")) {
        std::cout << "Need config file." << std::endl;
        return 0;
    }
    std::string config_file = vm["config"].as<std::string>();
    if (vm.count("server")) {
        server = vm["server"].as<std::string>();
    }
    if (vm.count("db")) {
        db = vm["db"].as<std::string>();
    }
    if (vm.count("user")) {
        user = vm["user"].as<std::string>();
    }
    if (vm.count("passwd")) {
        passwd = vm["passwd"].as<std::string>();
    }
    if (vm.count("table")) {
        passwd = vm["table"].as<std::string>();
    }
    
    fs::path data_path(airport::DATA_PATH);
    fs::path config_path;
    if (!find_file(data_path, config_file, config_path))
    {
        std::cout<< "Can't find config file file!" << std::endl;
        return 0;
    }
    std::string filename(config_path.string());
    while(true)
    {
        insert(filename, server, user, passwd, db, table);
        std::cout << "sleeping 30 sec..." << std::endl;
        sleep(30);
    }
}