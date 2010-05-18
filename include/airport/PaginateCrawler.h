#ifndef PAGINATECRAWLER_H_9HIADEI0
#define PAGINATECRAWLER_H_9HIADEI0

#include <string>
#include <vector>
#include <map>
#include <boost/function.hpp>
#include "HttpClient.h"
#include "HttpResponse.h"

#define DEFAULT_PAGINATE_MAX_THREADS 10
#define DEFAULT_PAGINATE_PAGE_STEP 1
const std::string DEFAULT_PAGINATE_REPLACE_MARKER("{NUMBER}");

namespace airport
{
    
    class PaginateCrawler
    {
        typedef boost::function<std::string(std::string, std::string)> function_type;
        airport::HttpClient httpClient;
        std::string startUrl;
        std::string nodename;
        int startNumber;
        int endNumber;
        int pageStep;
        std::string replaceMarker;
        std::map<std::string, std::string> parser;
        std::map<std::string, std::string> userDict;
        std::vector<std::string> essentialFields;
        std::map<std::string, std::string> staticFormData;
        std::map<std::string, std::string> firstPageFormData;
        std::map<std::string, std::string> inpageFormData;
        std::map<std::string, int> autoFormData;
        std::map<std::string, std::pair<function_type, std::string> > observers;
        
        unsigned int maxThreads;
    public:
        PaginateCrawler(const char *startUrlCString);
        PaginateCrawler(std::string &startUrlString);
        PaginateCrawler();
        ~PaginateCrawler();
        
        void set_node_name(std::string &node_name);
        void set_node_name(const char *node_name);
        
        void set_start_url(const char *start_url);
        void set_start_url(std::string &start_url);
        
        void set_replace_marker(const char *replace_marker);
        void set_replace_marker(std::string &replace_marker);
        
        void set_start_number(int n);
        void set_end_number(int n);
        void set_page_step(int n);
        
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
        
        template <typename T> void add_observer(std::string &key, T t, std::string &module);
        
        void set_max_threads(unsigned int max_threads);
        
        void set_http_client(airport::HttpClient &httpClient);
        
        std::map<std::string, std::string> parse(std::string &htmlBody, std::map<std::string, std::string> &formData);
        
        std::vector<airport::HttpResponse> start();
    };
}
#endif /* end of include guard: PAGINATECRAWLER_H_9HIADEI0 */
