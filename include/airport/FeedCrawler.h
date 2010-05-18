#ifndef FEEDCRAWLER_H_UOXA1TSL
#define FEEDCRAWLER_H_UOXA1TSL

#ifndef TIXML_USE_TICPP
#define TIXML_USE_TICPP
#endif

#include "HttpClient.h"
#include "HttpResponse.h"
#include "Url.h"
#include "Feed.h"
#include "FeedEntry.h"
#include <string>
#include <vector>
#include <ticpp/ticpp.h>
#include <ticpp/tinystr.h>
#include <tidy/tidy.h>
#include <tidy/buffio.h>

namespace airport
{
    typedef enum
    {
        INVALID_FEED=0,
        RSS2,
        RSS1,
        ATOM,
        RDF
    } FeedType;
    
    typedef std::pair<TidyNode, int> nod_t;
    
    class FeedCrawler
    {
        airport::HttpClient httpClient;
        std::string nodename;
        bool saveInMongo;
        bool fullRss;
        bool enableUpdate;
        int results;
        airport::nod_t bestMatchNode( TidyDoc tdoc, TidyNode tnod, std::string &request );
    public:
        FeedCrawler();
        ~FeedCrawler();
        void set_node_name(std::string &node_name);
        void set_node_name(const char *node_name);
        void set_save_in_mongo(bool save);
        void set_full_rss(bool full);
        void set_enable_update(bool update);
        void set_http_client(airport::HttpClient &hc);
        std::pair<airport::Feed, std::vector<airport::FeedEntry> > get(const char *url);
        std::pair<airport::Feed, std::vector<airport::FeedEntry> > get(std::string &url);
        std::pair<airport::Feed, std::vector<airport::FeedEntry> > parse(airport::HttpResponse &httpResponse);
        std::pair<airport::Feed, std::vector<airport::FeedEntry> > parseRSS2(ticpp::Document &tdoc, airport::HttpResponse &httpResponse, const airport::FeedType feedType);
        airport::FeedEntry rss2Entry(const ticpp::Element *item, std::string &feedLink, std::string &webLink, std::string &author);
        std::pair<std::string, std::string> extractContent(std::string &html, std::string &title, std::string &content);
        airport::FeedType detectType(ticpp::Document &tdoc);
        void save(airport::Feed &feed, std::vector<airport::FeedEntry> &entries);
        
        int get_new_results() const { return results; }
    };
}

#endif /* end of include guard: FEEDCRAWLER_H_UOXA1TSL */
