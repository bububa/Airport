#ifndef LISTCRAWLER_H_E7TX6T7I
#define LISTCRAWLER_H_E7TX6T7I

#include <string>
#include <vector>
#include <map>
#include <boost/function.hpp>
#include "HttpClient.h"
#include "HttpResponse.h"

#define DEFAULT_LISTCRAWLER_MAX_THREADS 10
#define DEFAULT_LISTCRAWLER_PAGE_STEP 1
const std::string DEFAULT_LISTCRAWLER_REPLACE_MARKER("{NUMBER}");
const std::string DEFAULT_LISTCRAWLER_LINKREGEX("<\\s*A\\s+[^>]*href\\s*=\\s*[\"|\'](.*?)[\"|\']");

namespace airport
{
    
    class ListCrawler
    {
        typedef boost::function<std::string(std::string, std::string)> function_type;
        airport::HttpClient httpClient;
        std::string startUrl;
        int startNumber;
        int endNumber;
        int pageStep;
        std::string replaceMarker;
        std::string urlPattern;
        std::string linkRegex;
        std::map<std::string, std::string> parser;
        std::map<std::string, std::string> paginate_parser;
        std::map<std::string, std::string> userDict;
        std::vector<std::string> essentialFields;
        std::vector<std::string> blackKeywords;
        std::map<std::string, std::string> staticFormData;
        std::map<std::string, std::string> firstPageFormData;
        std::map<std::string, std::string> inpageFormData;
        std::map<std::string, int> autoFormData;
        std::map<std::string, std::pair<function_type, std::string> > observers;
        std::pair<function_type, std::string> listObserver;
        unsigned int maxThreads;
        unsigned int resultPages;
        bool lastPageHasResults;
        std::vector<std::string> urlHash;
        bool saveInMongo;
        bool discardExternalDuplicate;
        /* PRIVATE METHODS */
        std::vector<std::string> extractUrls(std::string &html);
    public:
        ListCrawler(const char *startUrlCString);
        ListCrawler(std::string &startUrlString);
        ListCrawler();
        ~ListCrawler();
        
        void set_start_url(const char *start_url);
        void set_start_url(std::string &start_url);
        
        void set_replace_marker(const char *replace_marker);
        void set_replace_marker(std::string &replace_marker);
        
        void set_start_number(int n);
        void set_end_number(int n);
        void set_page_step(int n);
        void set_save_in_mongo(bool s);
        void set_discard_external_duplicate(bool s);
        
        void set_link_regex(const char *link_regex);
        void set_link_regex(std::string &link_regex);
        
        void add_paginate_parser(std::string &key, std::string &regxString);
        void add_paginate_parser(const char *key, const char *regxString);
        void set_paginate_parser(std::map<std::string, std::string> &paginate_parser);
        void reset_paginate_parser();
        
        void add_parser(std::string &key, std::string &regxString);
        void add_parser(const char *key, const char *regxString);
        void set_parser(std::map<std::string, std::string> &parser);
        void reset_parser();
        
        void add_staticForm_data(std::string &key, std::string &value);
        void add_staticForm_data(const char *key, const char *value);
        void set_staticForm_data(std::map<std::string, std::string> &data);
        
        void add_firstPageForm_data(std::string &key, std::string &value);
        void add_firstPageForm_data(const char *key, const char *value);
        void set_firstPageForm_data(std::map<std::string, std::string> &data);
        
        void add_inpageForm_data(std::string &key, std::string &value);
        void add_inpageForm_data(const char *key, const char *value);
        void set_inpageForm_data(std::map<std::string, std::string> &data);
        
        void add_autoForm_data(std::string &key, const int value);
        void add_autoForm_data(const char *key,  const int value);
        void set_autoForm_data(std::map<std::string, int> &data);
        
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
        
        template <typename T> void set_list_observer(T t, std::string &module);
        
        void set_url_pattern(const char *urlPattern);
        void set_url_pattern(std::string &urlPattern);
        
        void set_max_threads(unsigned int max_threads);
        
        void set_http_client(airport::HttpClient &httpClient);
        
        unsigned int get_result_pages() const { return resultPages; }
        bool get_last_page_has_results() const { return lastPageHasResults; }
        
        std::vector<airport::HttpResponse> start();
    };
}

#endif /* end of include guard: LISTCRAWLER_H_E7TX6T7I */
