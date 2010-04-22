#ifndef MONGO_H_DX4X2HKL
#define MONGO_H_DX4X2HKL

#include <mongo/client/dbclient.h>
#include <string>
#include <map>
#include <boost/tuple/tuple.hpp>
#include "HttpResponse.h"
#include "Feed.h"
#include "FeedEntry.h"

#define MONGO_DB_NAME "airport"
#define MONGO_HTTPRESPONSE_COLLECTION "airport.httpresponse"
#define MONGO_DEPTHRESPONSE_COLLECTION "airport.depthresponse"
#define MONGO_FEED_COLLECTION "airport.feed"
#define MONGO_FEEDENTRY_COLLECTION "airport.feedentry"
#define MONGO_DISPATCHER_COLLECTION "airport.dispatcher"
#define MONGO_SERVER_COLLECTION "airport.server"

namespace airport 
{
    
    class Mongo {
        std::string host;
    public:
        Mongo();
        ~Mongo();
        
        bool connect(mongo::DBClientConnection *c);
        bool connect(mongo::DBClientConnection *c, const char *host);
        bool connect(mongo::DBClientConnection *c, std::string &host);
        mongo::BSONObj httpResponseBSONObj(airport::HttpResponse &httpResponse);
        void insertHttpResponse(mongo::DBClientConnection &c, airport::HttpResponse &httpResponse);
        void insertHttpResponse(mongo::DBClientConnection &c, mongo::BSONObj &b);
        mongo::BSONObj getHttpResponseById(mongo::DBClientConnection &c, std::string &id);
        void updateHttpResponseDBID(mongo::DBClientConnection &c, const char *id, unsigned int dbid);
        void updateHttpResponseDBID(mongo::DBClientConnection &c, std::string &id, unsigned int dbid);
        mongo::BSONObj dispatcherBSONObj(const char *servername, const char *rulename, const char *rule);
        mongo::BSONObj dispatcherBSONObj(std::string &servername, std::string &rulename, std::string &rule);
        void insertDispatcher(mongo::DBClientConnection &c, const char *servername, const char *rulename, const char *rule);
        void insertDispatcher(mongo::DBClientConnection &c, std::string &servername, std::string &rulename, std::string &rule);
        void insertDispatcher(mongo::DBClientConnection &c, mongo::BSONObj &b);
        void archiveDispatcher(mongo::DBClientConnection &c, mongo::OID &oid);
        bool isRunningJob(mongo::DBClientConnection &c, std::string &rulename);
        int serverJobsCount(mongo::DBClientConnection &c, std::string &servername);
        mongo::BSONObj serversJobsCount(mongo::DBClientConnection &c);
        void serverJobComplete(mongo::DBClientConnection &c, std::string &servername, std::string &rulename, std::string &rule);
        void mountServer(mongo::DBClientConnection &c, std::string &servername);
        void unmountServer(mongo::DBClientConnection &c, std::string &servername);
        void pingServer(mongo::DBClientConnection &c, std::string &servername);
        std::map<std::string, std::pair<int, int> > serversStatus(mongo::DBClientConnection &c);
        std::vector< boost::tuple<std::string, std::string, double> > getKeywords(mongo::DBClientConnection &c, std::string &collection, std::string &json);
        
        mongo::BSONObj getFeedById(mongo::DBClientConnection &c, std::string &id);
        void updateFeed(mongo::DBClientConnection &c, airport::Feed &feed);
        mongo::BSONObj getFeedEntryById(mongo::DBClientConnection &c, std::string &id);
        bool updateFeedEntry(mongo::DBClientConnection &c, airport::FeedEntry &entry);
    };
}

#endif /* end of include guard: MONGO_H_DX4X2HKL */
