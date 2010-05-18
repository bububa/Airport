#ifndef BASICCRAWLER_H_E91JNXBB
#define BASICCRAWLER_H_E91JNXBB

#include <string>
#include <vector>
#include <map>
#include <boost/any.hpp>
#include <boost/function.hpp>
#include "HttpClient.h"
#include "HttpResponse.h"
#include "Url.h"

#define DEFAULT_CRAWLER_MAX_THREADS 10
#define DEFAULT_CRAWLER_RETRY 5

namespace airport
{
    
    class BasicCrawler 
    {
        typedef boost::function<std::string(std::string, std::string)> function_type;
        airport::HttpClient httpClient;
        std::string nodename;
        std::vector<airport::Url> request;
        std::vector<std::string> blackKeywords;
        std::map<std::string, std::string> parser;
        unsigned int maxThreads;
        std::map<std::string, std::string> userInfo;
        std::map<std::string, std::string> userDict;
        std::vector<std::string> essentialFields;
        std::map<std::string, std::pair<function_type, std::string> > observers;
        /*Private Methods*/
        std::string parse(std::string &htmlBody);
        airport::HttpResponse fetch(airport::Url &url);
        std::vector<airport::HttpResponse> chunk_fetch(std::vector<airport::Url> &urls);
    public:
        BasicCrawler(const char *urlCString);
        BasicCrawler(std::string &urlString);
        BasicCrawler(airport::Url &url);
        BasicCrawler();
        ~BasicCrawler();
        
        void set_node_name(std::string &node_name);
        void set_node_name(const char *node_name);
        
        void add_url(const char *urlCString);
        void add_url(std::string &urlString);
        void add_url(airport::Url &url);
        void reset_request();
        
        void add_parser(std::string &key, std::string &regxString);
        void add_parser(const char *key, const char *regxString);
        void set_parser(std::map<std::string, std::string> &parser);
        void reset_parser();
        
        void add_user_dict(std::string &key, std::string &value);
        void add_user_dict(const char *key, const char *value);
        void set_user_dict(std::map<std::string, std::string> &user_dict);
        
        void add_essential_field(const char *field);
        void add_essential_field(std::string &field);
        void set_essential_fields(std::vector<std::string> &fields);
        
        void add_black_keyword(const char *keyword);
        void add_black_keyword(std::string &keyword);
        void set_black_keywords(std::vector<std::string> &keywords);
        
        template <typename T> void add_observer(std::string &key, T t, std::string &module);
        
        void set_observers(std::map<std::string, std::pair<function_type, std::string> > &observers);
        
        void set_max_threads(unsigned int max_threads);
        void set_user_info(std::map<std::string, std::string> &userInfo);
        
        void set_http_client(airport::HttpClient &httpClient);
        
        std::vector<airport::HttpResponse> start();
    };
    
}

#endif /* end of include guard: BASICCRAWLER_H_E91JNXBB */
