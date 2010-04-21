#include "Inserter.h"
#include <set>
#include <boost/property_tree/info_parser.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include "Config.h"
#include "Mongo.h"


airport::Inserter::Inserter()
{
    
}

airport::Inserter::~Inserter()
{
    
}

std::map<std::string, boost::any>
airport::Inserter::parseConfig(const char *filename)
{
    std::string tf(filename);
    return parseConfig(tf);
}

std::map<std::string, boost::any>
airport::Inserter::parseConfig(std::string &filename)
{
    boost::property_tree::ptree pt;
    boost::property_tree::read_info(filename, pt);
    std::string server, db, user, passwd, table;
    std::map<std::string, std::string> fieldMap;
    std::map<std::string, std::string> defaultValues;
    try
    {
        server = pt.get<std::string>("server");
    }catch(boost::property_tree::ptree_bad_path){}
    try
    {
        db = pt.get<std::string>("db");
    }catch(boost::property_tree::ptree_bad_path){}
    try
    {
        user = pt.get<std::string>("user");
    }catch(boost::property_tree::ptree_bad_path){}
    try
    {
        passwd = pt.get<std::string>("passwd");
    }catch(boost::property_tree::ptree_bad_path){}
    try
    {
        table = pt.get<std::string>("table");
    }catch(boost::property_tree::ptree_bad_path){}
    try
    {
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                    pt.get_child("fieldMap"))
        {
            fieldMap.insert(std::map<std::string, std::string>::value_type(v.first, v.second.data()));
        }
    }catch(boost::property_tree::ptree_bad_path){}
    try
    {
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                    pt.get_child("defaultValues"))
        {
            defaultValues.insert(std::map<std::string, std::string>::value_type(v.first, v.second.data()));
        }
    }catch(boost::property_tree::ptree_bad_path){}
    std::map<std::string, boost::any> response;
    response.insert(std::map<std::string, std::string>::value_type(std::string("server"), server));
    response.insert(std::map<std::string, std::string>::value_type(std::string("db"), db));
    response.insert(std::map<std::string, std::string>::value_type(std::string("user"), user));
    response.insert(std::map<std::string, std::string>::value_type(std::string("passwd"), passwd));
    response.insert(std::map<std::string, std::string>::value_type(std::string("table"), table));
    response.insert(std::map<std::string, std::map<std::string, std::string> >::value_type(std::string("fieldMap"), fieldMap));
    response.insert(std::map<std::string, std::map<std::string, std::string> >::value_type(std::string("defaultValues"), defaultValues));
    return response;
}

mysqlpp::Connection 
airport::Inserter::connect(const char *server, const char *user, const char *passwd, const char *db)
{
    mysqlpp::Connection conn(false);
    conn.set_option( new mysqlpp::SetCharsetNameOption("utf8") );
    conn.set_option( new mysqlpp::SetCharsetNameOption("utf8") );
    conn.set_option(new mysqlpp::ReconnectOption(true));
    if (conn.connect(db, server, user, passwd))
    {
        //mysqlpp::Query query = conn.query("SET NAMES utf8; SET CHARACTER SET utf8; SET COLLATION_CONNECTION='utf8_general_ci';");
        //query.exec();
    }
    return conn;
}

mysqlpp::Connection 
airport::Inserter::connect(std::string &server, std::string &user, const std::string &passwd, const std::string &db)
{
    return connect(server.c_str(), user.c_str(), passwd.c_str(), db.c_str());
}

std::map< std::string, std::string > 
airport::Inserter::structure(mysqlpp::Connection &conn, std::string &table)
{
    std::map< std::string, std::string > response;
    mysqlpp::Query query = conn.query("show fields from " + table);
    if (mysqlpp::StoreQueryResult res = query.store())
    {
        for (size_t i = 0; i < res.num_rows(); ++i)
        {
            std::string field, type;
            res[i]["Field"].to_string (field);
            res[i]["Type"].to_string (type);
            response.insert(std::map< std::string, std::string >::value_type(field, type));
        }
    }
    return response;
}

void
airport::Inserter::insert(mysqlpp::Connection &conn, std::string &table, std::map<std::string, std::string> &fieldMap, std::map<std::string, std::string> &defaultValues)
{
    std::map< std::string, std::string > meta = structure(conn, table);
    if (airport::DEBUG_LEVEL > airport::DEBUG_OFF)
    {
        std::cout << "=====================================" << std::endl;
        std::cout << "MYSQL DATABASE " << table << " STURECTURE" << std::endl;
        for(std::map< std::string, std::string >::iterator it=meta.begin();it!=meta.end();++it)
        {
            std::cout << it->first << ": " << it->second << std::endl;
        }
        std::cout << "=====================================" << std::endl;
    }
    Mongo mongoDB;
    mongo::DBClientConnection mongoConnection;
    mongoDB.connect(&mongoConnection);
    std::auto_ptr<mongo::DBClientCursor> cursor = mongoConnection.query( MONGO_HTTPRESPONSE_COLLECTION, mongo::QUERY("dbid"<<0) );
    while(cursor->more())
    {
        if (airport::DEBUG_LEVEL > airport::DEBUG_OFF)
        {
            std::cout << "-------------------------------------" << std::endl;
        }
        mongo::BSONObj b = cursor->next();
        mongo::BSONObj parsedData = b.getObjectField("parsed_data");
        std::set<std::string> fieldNames;
        parsedData.getFieldNames(fieldNames);
        std::map<std::string, std::string> insertionData;
        std::map<std::string, std::string>::iterator insertionDataIter;
        std::map<std::string, std::string>::iterator metaiter;
        std::map<std::string, std::string>::iterator fieldMapiter;
        std::map<std::string, std::string>::iterator defaultValueiter; 
        for(std::set<std::string>::iterator it=fieldNames.begin();it!=fieldNames.end();++it)
        {
            fieldMapiter = fieldMap.find( (*it) );
            std::string mongoKey, dbKey;
            if (fieldMapiter!=fieldMap.end())
            {
                mongoKey = fieldMapiter->first;
                dbKey = fieldMapiter->second;
            }else{
                metaiter = meta.find( (*it) );
                if (metaiter==meta.end()) continue;
                mongoKey = (*it);
                dbKey = mongoKey;
            }
            insertionData.insert(std::map<std::string, std::string>::value_type(dbKey, parsedData.getStringField( mongoKey.c_str() )));
        }
        for(fieldMapiter=fieldMap.begin();fieldMapiter!=fieldMap.end();++fieldMapiter)
        {
            std::string mongoKey = fieldMapiter->first;
            if (boost::algorithm::starts_with(mongoKey, "parser."))
                continue;
            std::string dbKey = fieldMapiter->second;
            metaiter = meta.find( dbKey );
            if (metaiter==meta.end()) continue;
            mongo::BSONElement e = b.getField(mongoKey.c_str());
            std::string dbValue;
            if (e.type() == mongo::jstOID)
            {
                dbValue = e.__oid().str();
            }else if (e.type() == mongo::NumberInt || e.type() == mongo::Timestamp || e.type() == mongo::JSTypeMax){
                dbValue = boost::lexical_cast<std::string>(b.getIntField( mongoKey.c_str() ));
            }else{
                dbValue = b.getStringField( mongoKey.c_str() );
            }
            insertionData.insert(std::map<std::string, std::string>::value_type(dbKey, dbValue));
        }
        for (defaultValueiter = defaultValues.begin(); defaultValueiter!=defaultValues.end(); ++defaultValueiter)
        {
            insertionDataIter = insertionData.find( defaultValueiter->first );
            if (insertionDataIter != insertionData.end()) continue;
            std::string dbKey = defaultValueiter->first;
            std::string dbValue;
            if (defaultValueiter->second == "now()")
            {
                boost::posix_time::ptime nowt = boost::posix_time::from_time_t(time(NULL));
                dbValue = boost::posix_time::to_iso_extended_string(nowt);
            }else{
                dbValue = defaultValueiter->second;
            }
            insertionData.insert(std::map<std::string, std::string>::value_type(dbKey, dbValue));
        }
        
        //mysqlpp::Query query = conn.query();
        std::string qstr = "INSERT INTO " + table + "(";
        std::string vp;
        mysqlpp::SQLQueryParms sqlp;
        unsigned int i = 0;
        for (std::map<std::string, std::string>::iterator it=insertionData.begin();it!=insertionData.end();++it)
        {
            if (it!=insertionData.begin()) 
            {
                qstr += ",";
                vp += ",";
            }
            qstr += it->first;
            vp += "%" + boost::lexical_cast<std::string>(i++) + "q:" + it->first;
            mysqlpp::SQLTypeAdapter sa(it->second);
            sqlp += sa;
            if (airport::DEBUG_LEVEL > airport::DEBUG_OFF)
            {
                std::cout << it->first << ": " <<  it->second << std::endl;
            }
        }
        qstr += ") VALUES (" + vp + ")";
        mysqlpp::Query query = conn.query(qstr);
        query.parse();
        mysqlpp::SimpleResult res = query.execute(sqlp);
        unsigned int insertId = res.insert_id();
        std::cout <<"ID:"<< insertId<<std::endl;
        if (insertId > 0)
        {
            mongoDB.updateHttpResponseDBID(mongoConnection, b.getStringField("_id"), insertId);
        }
    }
}