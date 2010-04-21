#ifndef URL_AIRPORT_H
#define URL_AIRPORT_H

#include <string>
namespace airport {
    
    class Url {
    
        std::string host;
        std::string baseUrl;
        std::string rawUrl;
        std::string normalizedUrl;
        std::string effectiveUrl;
    
    public:
        Url (std::string &url);
        Url (const char *url);
        Url (std::string &url, std::string &baseurl);
        Url (const char *url, const char *baseurl);
        Url ();
        ~Url ();
    
        Url &operator= (std::string &url);
        Url &operator= (Url &url);
    
        void normalize_url (std::string &url);
        void normalize_url (const char *url);
    
        void normalize_url_baseurl (std::string &url, std::string &baseurl);
        void normalize_url_baseurl (const char *url, const char *baseurl);
    
        std::string get_host() const { return host; }
        std::string get_base_url() const { return baseUrl; }
        std::string get_raw_url () const { return rawUrl; }
        std::string get_normalized_url () const { return normalizedUrl; }
        std::string get_effective_url () const { return effectiveUrl; }
        
        void set_effective_url(const char *effective_url);
        void set_effective_url(std::string &effective_url);
        
        static std::string encode (const std::string &url);
        static std::string decode (const std::string &url);
        
    };

}

#endif /* end of include guard: URL_AIRPORT_H */
