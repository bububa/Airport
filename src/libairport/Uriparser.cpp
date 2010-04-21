#include "Uriparser.h"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <string>
#include <vector>
#include <iostream>

#define CHECK_EOF() \
  if (buf == buf_end) { \
    return -2;      \
  }

#define EXPECT(ch)    \
  CHECK_EOF();        \
  if (*buf++ != ch) { \
    return -1;        \
  }
  
airport::Uriparser::Uriparser():uri_(""), host_(""), scheme_(""), port_(0), path_query_(""), fragment_(""){}
airport::Uriparser::~Uriparser(){}

std::string 
airport::Uriparser::normalize(const char *url)
{
    std::string turl(url);
    return normalize(turl);
}

std::string 
airport::Uriparser::normalize(const std::string &url)
{
    parse(url);
    if (!host_.empty() ) 
    {
        uri_ = scheme_ + "://" + host_;
        if (port_ > 0)
            uri_ += ':' + boost::lexical_cast<std::string>(port_);
    }
    uri_ += path_query_;
    if (!fragment_.empty())
        uri_ += "#" + fragment_;
    return uri_;
}

std::string 
airport::Uriparser::join(const std::string &u1, const std::string &u2)
{
    airport::Uriparser targetUrl;
    airport::Uriparser baseUrl;
    targetUrl.normalize(u1);
    baseUrl.normalize(u2);
    std::string res;
    if (!baseUrl.host().empty()) 
    {
        res = baseUrl.scheme() + "://" + baseUrl.host();
        if (baseUrl.port() > 0)
            res += ':' + boost::lexical_cast<std::string>(baseUrl.port());
    }
    if (boost::starts_with(targetUrl.path_query(), "/"))
    {
        res += targetUrl.path_query();
    }else{
        std::vector<std::string> pathes;
        std::string path_query;
        std::string bquery = baseUrl.path_query();
        boost::split(pathes, bquery, boost::is_any_of("/"));
        for(std::vector<std::string>::iterator it=pathes.end()-1; it>=pathes.begin(); --it)
        {
            if (it==pathes.end()-1)
            {
                size_t found=(*it).find(".");
                if (found!=std::string::npos)
                {
                    continue;
                }
            }
            if (path_query.empty())
            {
                path_query = (*it);
            }else{
                path_query = (*it) + "/" + path_query;
            }
        }
        res += path_query;
        if (!targetUrl.path_query().empty())
            res += "/../" + targetUrl.path_query();
    }
    if (!targetUrl.fragment().empty())
        res += "#" + targetUrl.fragment();
    res = normalize(res);
    return res;
}

bool 
airport::Uriparser::parse(const char *src)
{
    std::string sc(src);
    return parse(sc);
}

bool 
airport::Uriparser::parse(const std::string &src)
{
    std::string sc(src);
    return _parse(src.c_str(), src.size());
}

bool 
airport::Uriparser::_parse(const char*src, size_t src_len)
{
    const char * scheme;
    size_t scheme_len;
    const char * host;
    size_t host_len;
    const char *path_query;
    int path_query_len;
    const char *fragment;
    int fragment_len;
    int ret = nu_parse_uri(src, src_len, &scheme, &scheme_len, &host, &host_len, &port_, &path_query, &path_query_len, &fragment, &fragment_len);
    if (ret != 0) {
        path_query_.assign(src, src_len);
        path_query_ = canonical_path(path_query_);
        return false; // parse error
    }
    uri_.assign(src, src_len);
    host_.assign(host, host_len);
    path_query_.assign(path_query, path_query_len);
    scheme_.assign(scheme, scheme_len);
    fragment_.assign(fragment, fragment_len);
    path_query_ = canonical_path(path_query_);
    return true;
}

std::string 
airport::Uriparser::canonical_path(std::string &path_query)
{
    std::vector<std::string> pathes;
    boost::split(pathes, path_query, boost::is_any_of("/"));
    std::string res;
    unsigned int counter = 0;
    for(std::vector<std::string>::iterator it=pathes.end()-1; it>=pathes.begin(); --it)
    {
        if (it==pathes.end()-1)
        {
            res = (*it);
        }else{
            if ((*it) == "..")
            {
                counter ++;
            }else if(counter==0 && (*it).size()>0){
                res = (*it) + "/" + res;
            }else if((*it).size()>0){
                counter --;
            }
        }
    }
    
    if(boost::starts_with(path_query, "/")){
        res = "/" + res;
    }else if (counter > 0 && host_.size()>0)
    {
        for(unsigned int i=0;i<counter;i++)
        {
            res = "../" + res;
        }
    }
    return res;
}

int 
airport::Uriparser::nu_parse_uri(const char* _buf, size_t len, const char** scheme, size_t *scheme_len, const char **host, size_t *host_len, int *port, const char **path_query, int*path_query_len, const char **fragment, int*fragment_len) {
    const char * buf = _buf, * buf_end = buf + len;

    *scheme = buf;
    for (;;++buf) {
        CHECK_EOF();
        if (':' == *buf) {
            break;
        }
    }
    *scheme_len = buf - *scheme;
    EXPECT(':'); EXPECT('/'); EXPECT('/');

    *host = buf;
    *port = 0;
    *host_len = 0;
    *path_query_len = 0;
    *fragment_len = 0;
    for (;;++buf) {
        if (buf == buf_end) {
            *host_len = buf - *host;
            return 0;
        }
        if (':' == *buf) { /* with port */
            *host_len = buf - *host;
            buf++;

            *port = 0;
            for (;'0' <= *buf && *buf <= '9';buf++) {
                if (buf == buf_end) {
                    return 0;
                }
                *port = *port * 10 + (*buf - '0');
            }
            if (buf == buf_end) {
                return 0;
            }
            break;
        }
        if ('/' == *buf) { /* no port */
            *host_len = buf - *host;
            break;
        }
    }
    if (*host_len==0)
    {
        buf = _buf;
    }
    *path_query = buf;
    *path_query_len = buf_end - buf;
    for (;;++buf) {
        if (buf == buf_end) {
            return 0;
        }
        if ('#' == *buf)
        {
            *path_query_len = buf - *path_query;
            buf ++;
            if (buf == buf_end) {
                return 0;
            }
            *fragment = buf;
            *fragment_len = buf_end - buf;
            break;
        }
    }
    return 0;
}