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
#include "airport/Utils.h"
#include "airport/Mongo.h"
#include "airport/Rule.h"
#include "airport/License.h"

namespace po = boost::program_options;

#define MAX_CONCURRENT_JOBS 5
#define MAX_JOB_IDLE_TIMESTAMP 1800
#define MAX_SERVER_IDLE_TIMESTAMP 300

pid_t 
subprocess(std::string &rule, std::string &node)
{
    pid_t pid, child_pid;
    pid = fork();
    switch(pid)
    {
        case -1: // ERROR!
            printf("Error with code %i\n", errno);
            //return errno;
            return 0;
        case 0: // Child
            child_pid = getpid();
            printf("This is the child: pid = %d\n", child_pid);
            rule = "eva --rule=" + rule + " --node=" + node;
            system(rule.c_str());
            exit(0);
    }
    return pid;
}

void 
check_jobs(std::string &alias, std::map<pid_t, std::string> & running_processes, const int max_concurrent_jobs)
{
    airport::Mongo mongoDB;
    mongo::DBClientConnection mongoConnection;
    mongoDB.connect(&mongoConnection);
    mongoConnection.ensureIndex(MONGO_DISPATCHER_COLLECTION, mongo::fromjson("{servername:1, started_datetime:1, completed_datetime:1, archived:1}"));
    std::auto_ptr<mongo::DBClientCursor> cursor = mongoConnection.query( MONGO_DISPATCHER_COLLECTION, mongo::QUERY("servername"<<alias<<"started_datetime"<<0<<"completed_datetime"<<0<<"archived"<<false) );
    while(cursor->more())
    {
        mongo::BSONObj b = cursor->next();
        if (running_processes.size() >= max_concurrent_jobs)
            continue;
        std::string rulename = b.getStringField("rulename");
        std::string rulecontent = b.getStringField("rule");
        std::string rule_path = airport::DATA_PATH + "rules/" + rulename;
        std::ofstream rulefile (rule_path.c_str());
        rulefile << rulecontent;
        rulefile.close();
        pid_t pid = subprocess( rulename, alias );
        if (pid > 0)
        {
            running_processes.insert(std::map<pid_t, std::string>::value_type(pid, rulename) );
            mongo::BSONElement e;
            bool getId = b.getObjectID (e);
            if (getId)
            {
                mongo::OID oid = e.__oid();
                mongoConnection.update( MONGO_DISPATCHER_COLLECTION, BSON("_id"<<oid) , BSON("$set"<<BSON("started_datetime"<<(long long int)time(NULL))) );
            }
        }
        sleep(2);
    }
}

std::vector<boost::filesystem::path> 
find_rules(const boost::filesystem::path &dir_path)
{
    std::vector<boost::filesystem::path> rules;
    boost::filesystem::directory_iterator end_itr;
    for ( boost::filesystem::directory_iterator itr( dir_path ); itr != end_itr; ++itr )
    {
        if ( boost::filesystem::is_directory(itr->status()) )
        {
            std::vector<boost::filesystem::path> rs = find_rules( itr->path());
            if ( !rs.empty() )
            {
                for(std::vector<boost::filesystem::path>::iterator i=rs.begin(); i!=rs.end(); ++i)
                {
                    rules.push_back((*i));
                }
            }
        }else if ( itr->path().extension() == ".rule" ) {
            airport::Rule rule;
            std::string found_path = itr->path().string();
            if (rule.runable(found_path))
            {
                rules.push_back(itr->path());
            }
        }
    }
    
    return rules;
}

/* GABAGE COLLECT */
void 
gabageCollect()
{
    /* UNMOUNT UNAVALIABALE SERVERS */
    airport::Mongo mongoDB;
    mongo::DBClientConnection mongoConnection;
    mongoDB.connect(&mongoConnection);
    std::map<std::string, std::pair<int, int> > servers = mongoDB.serversStatus(mongoConnection);
    std::map<std::string, std::pair<int, int> >::iterator it;
    for(it=servers.begin();it!=servers.end();++it)
    {
        int elasped = (time(NULL) - (it->second).second)/60;
        if (elasped > MAX_SERVER_IDLE_TIMESTAMP)
        {
            std::string alias = it->first;
            mongoDB.unmountServer(mongoConnection, alias);
            std::cout << "Unmount server alias: "<< alias << std::endl;
        }
    }
    /* ARCHIVE FAILED DISPATCHER */
    mongoConnection.ensureIndex(MONGO_DISPATCHER_COLLECTION, mongo::fromjson("{completed_datetime:1, archived:1}"));
    std::auto_ptr<mongo::DBClientCursor> cursor = mongoConnection.query(MONGO_DISPATCHER_COLLECTION, mongo::QUERY("completed_datetime"<<0<<"archived"<<false) );
    while(cursor->more())
    {
        mongo::BSONObj b = cursor->next();
        int inserted_datetime = b.getIntField("inserted_datetime");
        if ( (time(NULL) - inserted_datetime) > MAX_JOB_IDLE_TIMESTAMP )
        {
            mongo::BSONElement e;
            bool getId = b.getObjectID (e);
            if (getId)
            {
                mongo::OID oid = e.__oid();
                mongoDB.archiveDispatcher(mongoConnection, oid);
                std::cout << "Archive job: " << oid.str() << std::endl;
            }
        }
    }
    
}

/* RUN AS ATC */
void 
run_as_atc(const int max_concurrent_jobs)
{
    while(true)
    {
        if (airport::License::expired())
        {
            std::cout << "Your license has been expired." << std::endl;
            return;
        }
        {
            boost::filesystem::path dir_path(airport::DATA_PATH);
            std::vector<boost::filesystem::path> rules = find_rules(dir_path);
            if (rules.empty())
            {
                gabageCollect();
                std::cout << "sleeping..." << std::endl;
                sleep(60*2);
            }
            airport::Mongo mongoDB;
            mongo::DBClientConnection mongoConnection;
            mongoDB.connect(&mongoConnection);
            std::map<std::string, std::pair<int, int> > servers = mongoDB.serversStatus(mongoConnection);
            std::map<std::string, std::pair<int, int> >::iterator its;
            if (servers.empty())
            {
                gabageCollect();
                std::cout << "sleeping..." << std::endl;
                sleep(60*2);
            }
            for(its=servers.begin();its!=servers.end();++its)
            {
                int sc = (its->second).first;
                if (sc >= max_concurrent_jobs)
                    continue;
                std::vector<boost::filesystem::path>::iterator it;
                int pos = 0;
                for(it=rules.begin(); it!=rules.end(); ++it)
                {
                    std::string rule_name = it->filename();
                    if (sc++ >= max_concurrent_jobs || mongoDB.isRunningJob(mongoConnection, rule_name))
                        break;
                    pos ++;
                    std::string server = its->first;
                    std::cout << "SERVER: " << server << " RULE: " << it->filename() << std::endl;
                    airport::Rule rule;
                    std::string rule_path = it->string();
                    std::string rule_content = rule.readinfo(rule_path);
                    mongoDB.insertDispatcher(mongoConnection, server, rule_name, rule_content);
                }
                rules.erase(rules.begin() + pos);
            }
            
            std::auto_ptr<mongo::DBClientCursor> cursor = mongoConnection.query( MONGO_DISPATCHER_COLLECTION, mongo::QUERY("completed_datetime"<<BSON("$gt"<<0)<<"archived"<<false) );
            while(cursor->more())
            {
                mongo::BSONObj b = cursor->next();
                std::string rulename = b.getStringField("rulename");
                std::string rulecontent = b.getStringField("rule");
                std::string rule_path = airport::DATA_PATH + "rules/" + rulename;
                std::ofstream rulefile (rule_path.c_str());
                rulefile << rulecontent;
                rulefile.close();
                mongo::BSONElement e;
                bool getId = b.getObjectID (e);
                if (getId)
                {
                    mongo::OID oid = e.__oid();
                    mongoDB.archiveDispatcher(mongoConnection, oid);
                }
            }
        }
        gabageCollect();
        std::cout << "sleeping..." << std::endl;
        sleep(60*2);
    }
}

/* RUN AS AIRPORT */
void 
run_as_airport(const int max_concurrent_jobs, std::string &alias)
{
    while(true)
    {
        if (airport::License::expired())
        {
            std::cout << "Your license has been expired." << std::endl;
            return;
        }
        {
            airport::Mongo mongoDB;
            mongo::DBClientConnection mongoConnection;
            mongoDB.connect(&mongoConnection);
            mongoDB.mountServer(mongoConnection, alias);
            mongoConnection.ensureIndex(MONGO_DISPATCHER_COLLECTION, mongo::fromjson("{servername:1, rulename:1, completed_datetime:1, archived:1}"));
            int status;
            std::map<pid_t, std::string> running_processes;
            check_jobs(alias, running_processes, max_concurrent_jobs);
            while(!running_processes.empty())
            {
                pid_t pid = wait(&status);
                if (pid == -1) break;
                std::string rulename = running_processes[pid];
                //std::cout << "PID: " << pid << " RULE: "<< rulename <<" finished." << std::endl;
                std::map<pid_t, std::string>::iterator it;
                it=running_processes.find(pid);
                running_processes.erase(it);
                airport::Rule rule;
                std::string filepath = airport::DATA_PATH + "rules/" + rulename;
                std::string rulecontent = rule.readinfo(filepath);
                mongoDB.serverJobComplete(mongoConnection, alias, rulename, rulecontent);
                check_jobs(alias, running_processes, max_concurrent_jobs);
            }
        }
        std::cout << "sleeping..." << std::endl;
        sleep(30);
    }
}

/* UNMOUNT SERVER ALIAS */
void
unmount(std::string &alias)
{
    airport::Mongo mongoDB;
    mongo::DBClientConnection mongoConnection;
    mongoDB.connect(&mongoConnection);
    mongoDB.unmountServer(mongoConnection, alias);
}

/* PING ATC SERVER */
void
ping(std::string &alias)
{
    airport::Mongo mongoDB;
    mongo::DBClientConnection mongoConnection;
    mongoDB.connect(&mongoConnection);
    mongoDB.pingServer(mongoConnection, alias);
}

/* SHOW STATUS */
void
status()
{
    airport::Mongo mongoDB;
    mongo::DBClientConnection mongoConnection;
    mongoDB.connect(&mongoConnection);
    std::map<std::string, std::pair<int, int> > servers = mongoDB.serversStatus(mongoConnection);
    std::map<std::string, std::pair<int, int> >::iterator it;
    std::cout << "ALIAS\tJOBS\tPING" << std::endl;
    for(it=servers.begin();it!=servers.end();++it)
    {
        std::cout << it->first << "\t" << (it->second).first << "\t" << (time(NULL) - (it->second).second)/60 << std::endl;
    }
}

int main(int argc, char *argv[])
{
    if (airport::License::expired())
    {
        std::cout << "Your license has been expired." << std::endl;
    }
    int max_concurrent_jobs = MAX_CONCURRENT_JOBS;
    bool isATC;
    std::string alias;
    /* PARSE COMMAND LINE */
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("version,v", "show airport version")
        ("atc,A", "set the server to be ATC")
        ("alias,a", po::value<std::string>(), "set server alias name")
        ("concurrent,c", po::value<int>(), "set max concurrent jobs")
        ("unmount", po::value<std::string>(), "unmount an airport")
        ("ping", po::value<std::string>(), "airport ping ATC")
        ("status", "show airports status")
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
    
    if (vm.count("version")) {
        std::cout << airport::AIRPORT_VERSION << std::endl;
        return 1;
    }
    
    /* NAME SERVER OPTIONS */
    if (vm.count("unmount"))
    {
        std::string unmountAlias = vm["unmount"].as<std::string>();
        unmount(unmountAlias);
        std::cout << "Unmount server alias: " << unmountAlias << std::endl;
        return 1;
    }
    
    if (vm.count("ping"))
    {
        std::string pingAlias = vm["ping"].as<std::string>();
        ping(pingAlias);
        std::cout << "Airport: " << pingAlias << " ping ATC."<< std::endl;
        return 1;
    }
    
    if (vm.count("status"))
    {
        status();
        return 1;
    }
    
    /* ATC & airport SERVER OPTIONS */
    if (vm.count("atc")) {
        isATC = true;
        std::cout << "This server running as ATC." << std::endl;
    }else if (vm.count("alias")) {
        std::cout << "This server running as airport." << std::endl;
        std::cout << "Alias: " << vm["alias"].as<std::string>() << std::endl;
        alias = vm["alias"].as<std::string>();
    }else{
        std::cout << "Please specify this server's name!" << std::endl;
        return 0;
    }
    if (vm.count("concurrent")) {
        max_concurrent_jobs = vm["concurrent"].as<int>();
    }
    
    /* RUN SERVER OPTIONS */
    if (isATC)
    {
        run_as_atc(max_concurrent_jobs);
    }else{
        run_as_airport(max_concurrent_jobs, alias);
    }
    return 0;
}