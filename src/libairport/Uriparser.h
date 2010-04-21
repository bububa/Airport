#ifndef URIPARSER_H_UYM10A7G
#define URIPARSER_H_UYM10A7G

#include <string>
#include <cstdlib>

namespace airport
{
    
    class Uriparser 
    {
        std::string uri_;
        std::string host_;
        std::string scheme_;
        int port_;
        std::string path_query_;
        std::string fragment_;
        
        bool _parse(const char*src, size_t src_len);
        int nu_parse_uri(const char* _buf, size_t len, const char** scheme, size_t *scheme_len, const char **host, size_t *host_len, int *port, const char **path_query, int*path_query_len, const char **fragment, int*fragment_len);
        std::string canonical_path(std::string &path_query);
    public:
        Uriparser();
        ~Uriparser();
        
        std::string normalize(const char *url);
        std::string normalize(const std::string &url);
        
        std::string join(const std::string &targetUrl, const std::string &baseUrl);
        
        bool parse(const char *src);
        bool parse(const std::string &src);
        
        std::string host() const { return host_; }
        std::string scheme() const { return scheme_; }
        int port() const { return port_; }
        std::string path_query() const { return path_query_; }
        std::string fragment() const { return fragment_; }
        std::string as_string() const { return uri_; }
        operator bool() const {
            return !this->uri_.empty();
        }
    };
}

#endif /* end of include guard: URIPARSER_H_UYM10A7G */
