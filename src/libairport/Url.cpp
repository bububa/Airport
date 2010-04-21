#include <iostream>
#include "boost/regex.hpp"
#include "Uriparser.h"
#include "Utils.h"
#include "Url.h"

airport::Url::Url (std::string &url)
{
    normalize_url (url);
}
 
airport::Url::Url (const char *url)
{
    normalize_url (url);
}

airport::Url::Url (std::string &url, std::string &baseurl)
{
    normalize_url_baseurl (url, baseurl);
}
 
airport::Url::Url (const char *url, const char *baseurl)
{
    normalize_url_baseurl (url, baseurl);
}

airport::Url::Url ()
{
 
}
 
airport::Url::~Url ()
{
 
}
 
airport::Url&
airport::Url::operator= (std::string &url)
{
    normalize_url (url);
 
    return *this;
}
 
airport::Url&
airport::Url::operator= (Url &url)
{
    this->rawUrl = url.rawUrl;
    effectiveUrl = url.effectiveUrl;
    normalizedUrl = url.normalizedUrl;
    host = url.host;
    baseUrl = url.baseUrl;
 
    return *this;
}

void
airport::Url::normalize_url (const char *url)
{
    std::string turl (url);
    
    normalize_url (turl);
}
 
void
airport::Url::normalize_url (std::string &url)
{
    rawUrl = url;
    airport::Uriparser uri;
    normalizedUrl = uri.normalize(url);
    host = uri.host();
    if (!uri.host().empty())
        baseUrl = uri.scheme() + "://" + uri.host();
}

void
airport::Url::normalize_url_baseurl (const char *url, const char *baseurl)
{
    std::string turl (url);
    std::string tbaseurl (baseurl);
    normalize_url_baseurl (turl, tbaseurl);
}

void
airport::Url::normalize_url_baseurl (std::string &url, std::string &baseurl)
{
    rawUrl = url;
    airport::Uriparser uri;
    std::string joined_url = uri.join(url, baseurl);
    normalizedUrl = uri.normalize(joined_url);
    host = uri.host();
    if (!uri.host().empty())
        baseUrl = uri.scheme() + "://" + uri.host();
}


void 
airport::Url::set_effective_url(const char *effective_url)
{
    std::string eu(effective_url);
    set_effective_url(eu);
}

void 
airport::Url::set_effective_url(std::string &effective_url)
{
    effectiveUrl = effective_url;
}


std::string
airport::Url::encode (const std::string &url)
{
    std::string str;
 
    for (unsigned int i = 0; i < url.size (); i++) {
        if ((url[i] >= '0' && url[i] <= '9') ||
            (url[i] >= 'A' && url[i] <= 'Z') ||
            (url[i] >= 'a' && url[i] <= 'z')) {
            str += url[i];
        } else {
            if (url[i] & 0xff < 16) {
                str += "%0" + airport::Utils::formatHexInt (url[i] & 0xff);
            } else {
                str += "%" + airport::Utils::formatHexInt (url[i] & 0xff);
            }
        }
    }
 
    return str;
}


 
std::string
airport::Url::decode (const std::string &url)
{
    std::string str;
 
    for (unsigned int i = 0; i < url.size (); i++) {
        if (url[i] == '%') {
            int val = 0;
            int tmp;
            if ((tmp = airport::Utils::getHexDigit (url[i+1])) != -1) {
                val += 16 * tmp;
            } else {
               return "";
            }
 
            if ((tmp = airport::Utils::getHexDigit (url[i+2])) != -1) {
                val += tmp;
            } else {
               return "";
            }
 
            str += (char) val;
            i += 2;
        } else if (url[i] == '+') {
            str += ' ';
        } else {
            str += url[i];
        }
    }
 
    return str;
}