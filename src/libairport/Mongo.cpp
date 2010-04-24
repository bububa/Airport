#include <json/json.h>
#include "Config.h"
#include "Mongo.h"
#include <iostream>
#include <memory>
#include <time.h>
#include "Url.h"
#include "Utils.h"

airport::Mongo::Mongo():host(airport::MONGODB_HOST)
{
    
}

airport::Mongo::~Mongo()
{
    
}

bool 
airport::Mongo::connect(mongo::DBClientConnection *c)
{
    return connect(c, host);
}

bool 
airport::Mongo::connect(mongo::DBClientConnection *c, const char *hostCStr)
{
    try{
        c->connect(hostCStr);
        return true;
    }catch( mongo::DBException &e ) {
        if (airport::DEBUG_LEVEL > airport::DEBUG_NORMAL)
        {
            std::cout << "Can't connect to mongodb!" << std::endl;
        }
    }
    return false;
}

bool 
airport::Mongo::connect(mongo::DBClientConnection *c, std::string &hostStr)
{
    return connect(c, hostStr.c_str());
}

mongo::BSONObj 
airport::Mongo::httpResponseBSONObj(airport::HttpResponse &httpResponse)
{
    mongo::BSONObjBuilder pb;
    mongo::BSONObjBuilder b;
    std::string js = httpResponse.get_raw_parsed_data();
    if(!js.empty())
    {
        Json::Value root;   // will contains the root value after parsing.
        Json::Reader reader;
        bool parsingSuccessful = reader.parse( js, root );
        if (parsingSuccessful)
        {
            Json::Value::Members members = root.getMemberNames();
            Json::Value::Members::iterator it;
            for (it=members.begin();it!=members.end();++it)
            {
                std::string key = (*it);
                std::string value = root[key].asString();
                pb.append(key.c_str(), value);
            }
            b.append("parsed_data", pb.obj());
        }else{
            b.append("parsed_data", js);
        }
    }else{
        b.append("parsed_data", js);
    }
    
    airport::Url url = httpResponse.get_url();
    b.append("_id", airport::Utils::sha1(url.get_effective_url()));
    b.append("domain", url.get_base_url());
    b.append("effective_url", url.get_effective_url());
    b.append("code", (int)httpResponse.get_response_code());
    b.append("total_time", httpResponse.get_total_time());
    b.append("html_body", httpResponse.get_html_body());
    b.append("html_header", httpResponse.get_html_header());
    //b.append("parsed_data", httpResponse.get_parsed_data());
    b.append("updated_time", (long long int)httpResponse.get_updated_time());
    b.append("dbid", (long long int)0);
    mongo::BSONObj p = b.obj();
    return p;
}

void 
airport::Mongo::insertHttpResponse(mongo::DBClientConnection &c, airport::HttpResponse &httpResponse)
{
    mongo::BSONObj b = httpResponseBSONObj(httpResponse);
    insertHttpResponse(c, b);
}

void 
airport::Mongo::insertHttpResponse(mongo::DBClientConnection &c, mongo::BSONObj &b)
{
    c.insert(MONGO_HTTPRESPONSE_COLLECTION, b);
}

mongo::BSONObj 
airport::Mongo::getHttpResponseById(mongo::DBClientConnection &c, std::string &id)
{
    mongo::BSONObj b = c.findOne(MONGO_HTTPRESPONSE_COLLECTION, mongo::QUERY("_id" << id));
    return b;
}

void
airport::Mongo::updateHttpResponseDBID(mongo::DBClientConnection &c, const char *id, unsigned int dbid)
{
    std::string tid(id);
    updateHttpResponseDBID(c, tid, dbid);
}

void
airport::Mongo::updateHttpResponseDBID(mongo::DBClientConnection &c, std::string &id, unsigned int dbid)
{
    c.update(MONGO_HTTPRESPONSE_COLLECTION, BSON("_id"<<id), BSON("$set"<<BSON("dbid"<< dbid)));
}

mongo::BSONObj 
airport::Mongo::dispatcherBSONObj(const char *workerid, const char *rulename, const char *rule)
{
    std::string tw(workerid);
    std::string trn(rulename);
    std::string tr(rule);
    return dispatcherBSONObj(tw, trn, tr);
}

mongo::BSONObj 
airport::Mongo::dispatcherBSONObj(std::string &servername, std::string &rulename, std::string &rule)
{
    mongo::BSONObjBuilder b;
    b.append("servername", servername);
    b.append("rulename", rulename);
    b.append("rule", rule);
    b.append("inserted_datetime", (long long int)time(NULL));
    b.append("started_datetime", 0);
    b.append("completed_datetime", 0);
    b.append("archived", false);
    mongo::BSONObj p = b.obj();
    return p;
}

void 
airport::Mongo::insertDispatcher(mongo::DBClientConnection &c, const char *servername, const char *rulename, const char *rule)
{
    mongo::BSONObj b = dispatcherBSONObj(servername, rulename, rule);
    insertDispatcher(c, b);
}

void 
airport::Mongo::insertDispatcher(mongo::DBClientConnection &c, std::string &servername, std::string &rulename, std::string &rule)
{
    mongo::BSONObj b = dispatcherBSONObj(servername, rulename, rule);
    insertDispatcher(c, b);
}

void 
airport::Mongo::insertDispatcher(mongo::DBClientConnection &c, mongo::BSONObj &b)
{
    c.insert(MONGO_DISPATCHER_COLLECTION, b);
}

void 
airport::Mongo::archiveDispatcher(mongo::DBClientConnection &c, mongo::OID &oid)
{
    c.update( MONGO_DISPATCHER_COLLECTION, BSON("_id"<<oid), BSON("$set"<<BSON("archived"<<true)));
}

bool 
airport::Mongo::isRunningJob(mongo::DBClientConnection &c, std::string &rulename)
{
    c.ensureIndex(MONGO_DISPATCHER_COLLECTION, mongo::fromjson("{rulename:1, completed_datetime:1, archived:1}"));
    mongo::BSONObj b = c.findOne(MONGO_DISPATCHER_COLLECTION, mongo::QUERY("rulename" << rulename << "completed_datetime" << 0 << "archived"<<false));
    return b.isEmpty()?false:true;
}

int 
airport::Mongo::serverJobsCount(mongo::DBClientConnection &c, std::string &servername) 
{
    c.ensureIndex(MONGO_DISPATCHER_COLLECTION, mongo::fromjson("{servername:1, completed_datetime:1, archived:1}"));
    return c.count( MONGO_DISPATCHER_COLLECTION, BSON("servername"<<servername<<"completed_datetime"<<0<<"archived"<<false) );
}

mongo::BSONObj 
airport::Mongo::serversJobsCount(mongo::DBClientConnection &c)
{
    mongo::BSONObj b;
    c.ensureIndex(MONGO_DISPATCHER_COLLECTION, mongo::fromjson("{servername:1, rulename:1, completed_datetime:1, archived:1}"));
    //std::string groupcmd ("{group:{ns:\"dispatcher\",cond:{completed_datetime:0},key:{servername:1},initial:{count:0},$reduce:function(obj,prev){prev.count++;}}}");
    mongo::BSONObj cmd = BSON("group" << BSON("ns" << "dispatcher" << "cond" << BSON("completed_datetime"<<0<<"archived"<<false) << "key" << BSON("servername"<<1) << "initial" << BSON("count"<<0) << "$reduce" << "function(obj,prev){prev.count++;}" ) );
    c.runCommand(MONGO_DB_NAME, cmd, b);
    return b;
}

void 
airport::Mongo::serverJobComplete(mongo::DBClientConnection &c, std::string &servername, std::string &rulename, std::string &rule) 
{
    c.ensureIndex(MONGO_DISPATCHER_COLLECTION, mongo::fromjson("{servername:1, rulename:1, completed_datetime:1, archived:1}"));
    c.update( MONGO_DISPATCHER_COLLECTION, BSON("servername"<<servername<<"rulename"<<rulename<<"completed_datetime"<<0<<"archived"<<false), BSON("$set"<<BSON("rule"<<rule<<"completed_datetime"<<(long long int)time(NULL))) );
}

void 
airport::Mongo::mountServer(mongo::DBClientConnection &c, std::string &servername)
{
    c.insert(MONGO_SERVER_COLLECTION, BSON("_id"<<servername<<"ping"<<(long long int)time(NULL)));
}

void 
airport::Mongo::unmountServer(mongo::DBClientConnection &c, std::string &servername)
{
    c.remove(MONGO_SERVER_COLLECTION, BSON("_id"<<servername));
    c.ensureIndex(MONGO_DISPATCHER_COLLECTION, mongo::fromjson("{servername:1, rulename:1, completed_datetime:1, archived:1}"));
    c.update( MONGO_DISPATCHER_COLLECTION, BSON("servername"<<servername<<"completed_datetime"<<0<<"archived"<<false), BSON("$set"<<BSON("archived"<<true)));
}

void 
airport::Mongo::pingServer(mongo::DBClientConnection &c, std::string &servername)
{
    c.update(MONGO_SERVER_COLLECTION, BSON("_id"<<servername), BSON("$set"<<BSON("ping"<<(long long int)time(NULL))));
}

std::map<std::string, std::pair<int, int> > 
airport::Mongo::serversStatus(mongo::DBClientConnection &c)
{
    mongo::BSONObj b = serversJobsCount(c);
    mongo::BSONObj retval = b.getObjectField("retval");
    std::map<std::string, std::pair<int, int> > servers;
    if (!retval.isEmpty())
    {
        mongo::BSONObjIterator it (retval);
        while(it.more())
        {
            mongo::BSONObj server = it.next().embeddedObject();
            std::string servername = server.getStringField("servername");
            int count = server.getIntField("count");
            std::pair<int, int> v(count, 0);
            servers.insert(std::map<std::string, std::pair<int, int> >::value_type(servername, v));
        }
    }
    mongo::BSONObj emptyObj;
    std::auto_ptr<mongo::DBClientCursor> cursor = c.query(MONGO_SERVER_COLLECTION, emptyObj );
    while( cursor->more() )
    {
        mongo::BSONObj b = cursor->next();
        std::string servername = b.getStringField("_id");
        std::map<std::string, std::pair<int, int> >::iterator it = servers.find(servername);
        if (it!=servers.end())
        {
            std::pair<int, int> v((it->second).first, b.getIntField("ping"));
            servers[servername] = v;
        }else{
            std::pair<int, int> v(0, b.getIntField("ping"));
            servers.insert(std::map<std::string, std::pair<int, int> >::value_type(servername, v));
        }
    }
    return servers;
}

std::vector< boost::tuple<std::string, std::string, double> >
airport::Mongo::getKeywords(mongo::DBClientConnection &c, std::string &collection, std::string &json)
{
    std::vector< boost::tuple<std::string, std::string, double> > response;
    std::auto_ptr<mongo::DBClientCursor> cursor = c.query(collection, mongo::fromjson(json));
    while( cursor->more() )
    {
        mongo::BSONObj b = cursor->next();
        std::string name = b.getStringField("name");
        std::string cat = b.getStringField("category");
        mongo::BSONElement e = b.getField("weight");
        double weight = e.number();
        response.push_back( boost::tuple<std::string, std::string, double>(name, cat, weight) );
    }
    return response;
}

mongo::BSONObj 
airport::Mongo::getFeedById(mongo::DBClientConnection &c, std::string &id)
{
    mongo::BSONObj b = c.findOne(MONGO_FEED_COLLECTION, mongo::QUERY("_id" << id));
    return b;
}

void 
airport::Mongo::updateFeed(mongo::DBClientConnection &c, airport::Feed &feed)
{
    std::string hashUrl = airport::Utils::sha1(feed.get_url());
    mongo::BSONObj item = getFeedById(c, hashUrl);
    mongo::BSONObjBuilder b;
    b.append("url", feed.get_url());
    b.append("title", feed.get_title());
    b.append("desc", feed.get_desc());
    b.append("link", feed.get_link());
    b.append("etag", feed.get_etag());
    b.append("last_modified", feed.get_last_modified());
    b.append("pubdate", (long long int)feed.get_pubdate());
    b.append("updated_datetime", (long long int)time(NULL));
    if (item.isEmpty())
    {
        b.append("_id", hashUrl);
        b.append("inserted_datetime", (long long int)time(NULL));
        mongo::BSONObj p = b.obj();
        c.insert(MONGO_FEED_COLLECTION, p);
    }else{
        mongo::BSONObj p = b.obj();
        c.update(MONGO_FEED_COLLECTION, BSON("_id"<<hashUrl), BSON("$set"<<p));
    }
}

mongo::BSONObj 
airport::Mongo::getFeedEntryById(mongo::DBClientConnection &c, std::string &id)
{
    mongo::BSONObj b = c.findOne(MONGO_FEEDENTRY_COLLECTION, mongo::QUERY("_id" << id));
    return b;
}

void 
airport::Mongo::insertFeedEntry(mongo::DBClientConnection &c, airport::FeedEntry &entry)
{
    std::string hashUrl = airport::Utils::sha1(entry.get_link());
    mongo::BSONObjBuilder b;
    b.append("title", entry.get_title());
    b.append("desc", entry.get_desc());
    b.append("link", entry.get_link());
    b.append("pubdate", (long long int)entry.get_pubdate());
    b.append("updated_datetime", (long long int)time(NULL));
    b.append("feed_link", entry.get_feed_link());
    b.append("web_link", entry.get_web_link());
    b.append("author", entry.get_author());
    b.append("category", entry.get_category());
    b.append("comments", entry.get_comments());
    b.append("guid", entry.get_guid());
    b.append("_id", hashUrl);
    b.append("inserted_datetime", (long long int)time(NULL));
    mongo::BSONObj p = b.obj();
    c.insert(MONGO_FEEDENTRY_COLLECTION, p);
}

bool 
airport::Mongo::updateFeedEntry(mongo::DBClientConnection &c, airport::FeedEntry &entry)
{
    bool insert = false;
    std::string hashUrl = airport::Utils::sha1(entry.get_link());
    mongo::BSONObj item = getFeedEntryById(c, hashUrl);
    mongo::BSONObjBuilder b;
    b.append("title", entry.get_title());
    b.append("desc", entry.get_desc());
    b.append("link", entry.get_link());
    b.append("pubdate", (long long int)entry.get_pubdate());
    b.append("updated_datetime", (long long int)time(NULL));
    b.append("feed_link", entry.get_feed_link());
    b.append("web_link", entry.get_web_link());
    b.append("author", entry.get_author());
    b.append("category", entry.get_category());
    b.append("comments", entry.get_comments());
    b.append("guid", entry.get_guid());
    if (item.isEmpty())
    {
        b.append("_id", hashUrl);
        b.append("inserted_datetime", (long long int)time(NULL));
        mongo::BSONObj p = b.obj();
        c.insert(MONGO_FEEDENTRY_COLLECTION, p);
        insert = true;
    }else{
        mongo::BSONObj p = b.obj();
        c.update(MONGO_FEEDENTRY_COLLECTION, BSON("_id"<<hashUrl), BSON("$set"<<p));
    }
    return insert;
}