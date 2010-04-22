#ifndef RULE_H_H1X49E8L
#define RULE_H_H1X49E8L

#include <string>
#include <map>
#include <boost/any.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/tuple/tuple.hpp>
#include "HttpResponse.h"
#include "HttpClient.h"
#include "ListCrawler.h"
#include "BasicCrawler.h"
#include "PaginateCrawler.h"
#include "FeedCrawler.h"
#include "Feed.h"
#include "FeedEntry.h"

namespace airport
{
    
    class Rule
    {
        boost::property_tree::ptree pt;
        std::map<std::string, boost::any> staticModules;
        /* PRIVATE METHODS */
        bool runable(std::string &module, time_t duration, time_t updated_time);
        void update_rule(std::string &filename, std::string &module, boost::any &response, time_t duration, std::string &path);
        void showResult(airport::HttpResponse &hr);
        boost::any element(std::string &module, std::string &path, std::string &name, const boost::any &request="");
        airport::HttpClient httpClient(std::string &path);
        std::pair< airport::BasicCrawler, std::vector<airport::HttpResponse> > basicCrawler(std::string &path, const std::string& start_url="");
        std::pair< airport::PaginateCrawler, std::vector<airport::HttpResponse> > paginateCrawler(std::string &path);
        std::pair< airport::ListCrawler, std::vector<airport::HttpResponse> > listCrawler(std::string &path);
        std::pair< airport::FeedCrawler, std::pair<airport::Feed, std::vector<airport::FeedEntry> > > feedCrawler(std::string &path);
        boost::any List(std::string &path, const std::string &request="");
        boost::any Dict(std::string &path, const std::string &request="");
        boost::any ListMember(std::string &path, const std::string &request="");
        boost::any DictMember(std::string &path, const std::string &request="");
        boost::any httpResponse(std::string &path, const std::string &request="");
        boost::any JsonDecode(std::string &path, const std::string &request="");
        boost::any Regex(std::string &path, const std::string &request="");
        std::string StringJoin(std::string &path, const std::string &request="");
        std::string Tidy(std::string &path, const std::string &request);
        std::vector<std::string> FileToList(std::string &path, const std::string &request="");
        std::map<std::string, std::string> FileToDict(std::string &path, const std::string &request="");
        std::string StripTags(std::string &path, const std::string &reques);
        std::string HtmlSanitize(std::string &path, const std::string &request);
        std::vector< boost::tuple<std::string, std::string, double> > MongoKeywords(std::string &path, const std::string &request);
    public:
        Rule();
        ~Rule();
        std::string operator()(std::string &module, std::string request);
        static std::string readinfo(const char *filename);
        static std::string readinfo(std::string &filename);
        void parse(const char *filename);
        void parse(std::string &filename);
        bool runable(const char *filename);
        bool runable(std::string &filename);
    };
}

#endif /* end of include guard: RULE_H_H1X49E8L */
