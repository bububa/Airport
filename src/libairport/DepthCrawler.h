#ifndef DEPTHCRAWLER_H_DO4REOWW
#define DEPTHCRAWLER_H_DO4REOWW

#include <string>
#include <vector>
#include <map>
#include <boost/function.hpp>
#include <mongo/client/dbclient.h>
#include "HttpClient.h"
#include "HttpResponse.h"

#define DEFAULT_DEPTHCRAWLER_MAX_THREADS 10
#define DEFAULT_DEPTHCRAWLER_MAX_DEPTH 3
namespace airport
{
    
    class DepthCrawler
    {
        typedef boost::function<std::string(std::string, std::string)> function_type;
        airport::HttpClient httpClient;
        std::string startUrl;
        std::string targetUrlPattern;
        std::string acceptUrlPattern;
        std::string rejectUrlPattern;
        std::map<std::string, std::string> parser;
        std::map<std::string, std::string> acceptPageParser;
        std::map<std::string, std::string> userDict;
        std::vector<std::string> essentialFields;
        std::map<std::string, std::pair<function_type, std::string> > observers;
        unsigned int maxThreads;
        unsigned int resultPages;
        unsigned int maxDepth;
        /* PRIVATE METHODS */
        std::vector<std::string> extractUrls(std::string &html);
        void saveResponse(std::vector<airport::HttpResponse> &httpResponse, mongo::DBClientConnection &mongoConnection, int depth);
    public:
        DepthCrawler(const char *startUrlCString);
        DepthCrawler(std::string &startUrlString);
        DepthCrawler();
        ~DepthCrawler();
        
        void set_start_url(const char *start_url);
        void set_start_url(std::string &start_url);
        
        void add_accept_page_parser(std::string &key, std::string &regxString);
        void add_accept_page_parser(const char *key, const char *regxString);
        void set_accept_page_parser(std::map<std::string, std::string> &accept_page_parser);
        void reset_accept_page_parser();
        
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
        
        template <typename T> void add_observer(std::string &key, T t, std::string &module);
        
        void set_target_url_pattern(const char *urlPattern);
        void set_target_url_pattern(std::string &urlPattern);
        
        void set_accept_url_pattern(const char *urlPattern);
        void set_accept_url_pattern(std::string &urlPattern);
        
        void set_reject_url_pattern(const char *urlPattern);
        void set_reject_url_pattern(std::string &urlPattern);
        
        void set_max_threads(unsigned int max_threads);
        
        void set_http_client(airport::HttpClient &httpClient);
        
        unsigned int get_result_pages() const { return resultPages; }
        
        void start();
    };
}

#endif /* end of include guard: DEPTHCRAWLER_H_DO4REOWW */
