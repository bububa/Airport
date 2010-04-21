#ifndef HTTPCLIENT_H_H79PUAOT
#define HTTPCLIENT_H_H79PUAOT

#include <iostream>
#include <sstream>
#include <vector>
#include <list>
#include <utility>
#include <time.h>
#include <boost/any.hpp>
#include "Url.h"
#include "HttpResponse.h"

#include <curlpp/curlpp.hpp>
#include <curlpp/Easy.hpp>

#define DEFAULT_CONNECTTIMEOUT 10
#define DEFAULT_MAXREDIRS 10
#define DEFAULT_TIMEOUT 300


typedef enum {
    STATE_NULL = 0,
    STATE_START,
    STATE_COMPLETED,
    STATE_FAILED,
    STATE_BAD_REQUEST,
} HttpClientState;

static const std::string USER_AGENTS = "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1), Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 1.1.4322), Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 2.0.50727), Mozilla/4.0 (compatible; MSIE 7.0b; Windows NT 5.1), Mozilla/4.0 (compatible; MSIE 7.0b; Win32), Mozilla/4.0 (compatible; MSIE 7.0b; Windows NT 6.0), Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1; SV1; Arcor 5.005; .NET CLR 1.0.3705; .NET CLR 1.1.4322), Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1; YPC 3.0.1; .NET CLR 1.1.4322; .NET CLR 2.0.50727), Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 6.0), Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 6.0; WOW64; SLCC1; .NET CLR 2.0.50727; .NET CLR 3.0.04506; .NET CLR 3.5.21022), Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.0; WOW64; SLCC1; .NET CLR 2.0.50727; .NET CLR 3.0.04506; .NET CLR 3.5.21022), Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.0; WOW64; Trident/4.0; SLCC1; .NET CLR 2.0.50727; .NET CLR 3.5.21022; .NET CLR 3.5.30729; .NET CLR 3.0.30618), Mozilla/5.0 (X11; U; Linux x86_64; en-US; rv:1.8.1) Gecko/20060601 Firefox/2.0 (Ubuntu-edgy), Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.8.1.1) Gecko/20061204 Firefox/2.0.0.1, Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.8.1.2) Gecko/20070220 Firefox/2.0.0.2, Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.8.1.2) Gecko/20070221 SUSE/2.0.0.2-6.1 Firefox/2.0.0.2, Mozilla/5.0 (Windows; U; Windows NT 5.1; en; rv:1.8.1.9) Gecko/20071025 Firefox/2.0.0.9, Mozilla/5.0 (Windows; U; Windows NT 5.1; en; rv:1.8.1.17) Gecko/20080829 Firefox/2.0.0.17, Mozilla/5.0 (Windows; U; Windows NT 5.1; en; rv:1.8.1.19) Gecko/20081201 Firefox/2.0.0.19, Mozilla/5.0 (X11; U; Linux i686 (x86_64); en-US; rv:1.9a1) Gecko/20061204 GranParadiso/3.0a1, Mozilla/5.0 (Windows; U; Windows NT 5.1; en; rv:1.9) Gecko/2008052906 Firefox/3.0, Mozilla/5.0 (Windows; U; Windows NT 5.1; en; rv:1.9.0.1) Gecko/2008070208 Firefox/3.0.1, Mozilla/5.0 (Windows; U; Windows NT 5.1; en; rv:1.9.0.2) Gecko/2008091620 Firefox/3.0.2, Mozilla/5.0 (X11; U; Linux x86_64; en; rv:1.9.0.2) Gecko/2008092702 Gentoo Firefox/3.0.2, Mozilla/5.0 (Windows; U; Windows NT 5.1; en; rv:1.9.0.3) Gecko/2008092417 Firefox/3.0.3, Mozilla/5.0 (Windows; U; Windows NT 5.1; en; rv:1.9.0.3) Gecko/2008092417 Firefox/3.0.3 (.NET CLR 3.5.30729), Mozilla/5.0 (Windows; U; Windows NT 6.0; en; rv:1.9.0.3) Gecko/2008092417 Firefox/3.0.3, Mozilla/5.0 (Windows; U; Windows NT 5.2; en; rv:1.9.0.5) Gecko/2008120122 Firefox/3.0.5, Opera/9.0 (Windows NT 5.1; U; en), Opera/9.01 (X11; Linux i686; U; en), Opera/9.02 (Windows NT 5.1; U; en), Opera/9.10 (Windows NT 5.1; U; en), Opera/9.23 (Windows NT 5.1; U; en), Opera/9.50 (Windows NT 5.1; U; en), Opera/9.50 (Windows NT 6.0; U; en), Opera/9.60 (Windows NT 5.1; U; en) Presto/2.1.1";

namespace airport {
    
    class HttpClient {
        airport::Url url;
        HttpClientState state;
        std::string cookiePath;
        std::vector<std::string> cookieList;
        std::list<std::string> headers;
        boost::any userInfo;
        /* CURL OPTIONS */
        bool resetCookie;
        bool noCookie;
        int connectTimeout;
        int timeout;
        int maxRedirs;
        time_t timeValue;
        std::string userAgent;
        bool noUserAgent;
        std::string proxyUserPwd;
        std::vector<std::string> proxyList;
        curlpp::Forms formParts;
        
        /* PRIVATE FUNCTIONS */
        std::string get_random_useragent();
        std::string get_random_proxy();
        
        airport::HttpResponse callback(curlpp::Easy &request, std::string &htmlBody, std::string &htmlHeader);
        /**
         * Set internal download state.
         * Method will set the internal state flag of the download to the
         * specified state and send out a signal informing listeners that the
         * state has changed.
         * @param state new state to change to
         */
        void set_state (HttpClientState state);
        
    public:
        HttpClient(airport::Url &url);
        HttpClient();
        ~HttpClient();
        
        HttpClient& operator= (HttpClient &httpClient);
        
        airport::HttpResponse get(airport::Url &url);
        airport::HttpResponse get();
        
        void set_cookie_path(std::string &cookie_path);
        void set_cookie_path(const char *cookie_path);
        
        void set_cookie_list (std::vector<std::string> &cookie_list);
        void reset_cookie();
        void set_no_cookie(bool no_cookie);
        
        void set_user_info(boost::any &userInfo);
        
        /* CURL OPTS SETTING METHODS */
        void set_opt_connect_timeout(int connect_timeout);
        void set_opt_timeout(int timeout);
        void set_opt_max_redirs(int max_redirs);
        void set_opt_timevalue(time_t timevalue);
        void set_opt_useragent(const char *useragent);
        void set_opt_useragent(std::string &user_agent);
        void add_opt_header(const char *header);
        void add_opt_header(std::string &header);
        void set_opt_nouseragent(bool no_useragent);
        void set_opt_proxy_userpwd(const char *proxy_userpwd);
        void set_opt_proxy_userpwd(std::string &proxy_userpwd);
        void set_opt_proxy_list(std::vector<std::string> &proxy_list);
        void set_opt_postfield(std::pair<std::string, std::string> &postField);
        void reset_proxyList();
        
        std::vector<std::string> read_cookie_jar(std::string &cookie_path);
        std::vector<std::string> read_cookie_jar(const char *cookie_path);
        
        airport::Url get_url() const { return url; }
        HttpClientState get_state () const { return state; }
    };
    
}

#endif /* end of include guard: HTTPCLIENT_H_H79PUAOT */
