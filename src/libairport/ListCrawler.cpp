#include "ListCrawler.h"
#include "PaginateCrawler.h"
#include "BasicCrawler.h"
#include "Utils.h"
#include "Url.h"
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <mongo/client/dbclient.h>
#include "Mongo.h"

airport::ListCrawler::ListCrawler(const char *urlCString):startNumber(1), pageStep(DEFAULT_LISTCRAWLER_PAGE_STEP), replaceMarker(DEFAULT_LISTCRAWLER_REPLACE_MARKER), linkRegex(DEFAULT_LISTCRAWLER_LINKREGEX), maxThreads(DEFAULT_LISTCRAWLER_MAX_THREADS), resultPages(0), lastPageHasResults(false), saveInMongo(true), discardExternalDuplicate(true)
{
    set_start_url(urlCString);
}

airport::ListCrawler::ListCrawler(std::string &urlString):startNumber(1), pageStep(DEFAULT_LISTCRAWLER_PAGE_STEP), replaceMarker(DEFAULT_LISTCRAWLER_REPLACE_MARKER), linkRegex(DEFAULT_LISTCRAWLER_LINKREGEX), maxThreads(DEFAULT_LISTCRAWLER_MAX_THREADS), resultPages(0), lastPageHasResults(false), saveInMongo(true), discardExternalDuplicate(true)
{
    set_start_url(urlString);
}

airport::ListCrawler::ListCrawler():startNumber(1), pageStep(DEFAULT_LISTCRAWLER_PAGE_STEP), replaceMarker(DEFAULT_LISTCRAWLER_REPLACE_MARKER), linkRegex(DEFAULT_LISTCRAWLER_LINKREGEX), maxThreads(DEFAULT_LISTCRAWLER_MAX_THREADS), resultPages(0), lastPageHasResults(false), saveInMongo(true), discardExternalDuplicate(true)
{
    
}

airport::ListCrawler::~ListCrawler()
{
    
}

void 
airport::ListCrawler::set_node_name (const char *node_name)
{
    std::string tn(node_name);
    set_node_name(tn);
}

void 
airport::ListCrawler::set_node_name (std::string &node_name)
{
    nodename = node_name;
}

void
airport::ListCrawler::set_start_url(const char *start_url)
{
    std::string tu(start_url);
    set_start_url(tu);
}

void
airport::ListCrawler::set_start_url(std::string &start_url)
{
    startUrl = start_url;
}

void
airport::ListCrawler::set_replace_marker(const char *replace_marker)
{
    std::string rmk(replace_marker);
    set_replace_marker(rmk);
}

void
airport::ListCrawler::set_replace_marker(std::string &replace_marker)
{
    replaceMarker = replace_marker;
}

void 
airport::ListCrawler::set_max_threads(unsigned int max_threads)
{
    maxThreads = max_threads;
}

void 
airport::ListCrawler::set_http_client(airport::HttpClient &httpClient)
{
    this->httpClient = httpClient;
}

void 
airport::ListCrawler::set_start_number(int n)
{
    startNumber = n;
}

void 
airport::ListCrawler::set_end_number(int n)
{
    endNumber = n;
}

void 
airport::ListCrawler::set_page_step(int n)
{
    pageStep = n;
}

void
airport::ListCrawler::set_save_in_mongo(bool s)
{
    saveInMongo = s;
}

void
airport::ListCrawler::set_discard_external_duplicate(bool s)
{
    discardExternalDuplicate = s;
}

void airport::ListCrawler::set_link_regex(const char *link_regex)
{
    std::string tlink(link_regex);
    set_link_regex(link_regex);
}

void 
airport::ListCrawler::set_link_regex(std::string &link_regex)
{
    linkRegex = link_regex;
}

void 
airport::ListCrawler::add_paginate_parser(const char *key, const char *regxString)
{
    std::string tkey(key);
    std::string tRegxString(regxString);
    add_paginate_parser(tkey, tRegxString);
}

void 
airport::ListCrawler::add_paginate_parser(std::string &key, std::string &regxString)
{
    this->paginate_parser.insert(std::map<std::string, std::string>::value_type(key, regxString));
}

void 
airport::ListCrawler::set_paginate_parser(std::map<std::string, std::string> &paginate_parser)
{
    this->paginate_parser = paginate_parser;
}

void 
airport::ListCrawler::reset_paginate_parser()
{
    paginate_parser.clear();
}

void 
airport::ListCrawler::add_parser(const char *key, const char *regxString)
{
    std::string tkey(key);
    std::string tRegxString(regxString);
    add_parser(tkey, tRegxString);
}

void 
airport::ListCrawler::add_parser(std::string &key, std::string &regxString)
{
    this->parser.insert(std::map<std::string, std::string>::value_type(key, regxString));
}

void 
airport::ListCrawler::set_parser(std::map<std::string, std::string> &parser)
{
    this->parser = parser;
}

void 
airport::ListCrawler::reset_parser()
{
    parser.clear();
}

void 
airport::ListCrawler::add_user_dict(const char *key, const char *value)
{
    std::string tkey(key);
    std::string tValue(value);
    add_user_dict(tkey, tValue);
}

void 
airport::ListCrawler::add_user_dict(std::string &key, std::string &value)
{
    this->userDict.insert(std::map<std::string, std::string>::value_type(key, value));
}

void 
airport::ListCrawler::set_user_dict(std::map<std::string, std::string> &user_dict)
{
    this->userDict = user_dict;
}

void 
airport::ListCrawler::add_essential_field(const char *field)
{
    std::string tf(field);
    add_essential_field(tf);
}

void 
airport::ListCrawler::add_essential_field(std::string &field)
{
    essentialFields.push_back(field);
}

void 
airport::ListCrawler::set_essential_fields(std::vector<std::string> &fields)
{
    essentialFields = fields;
}

void 
airport::ListCrawler::add_black_keyword(const char *keyword)
{
    std::string tf(keyword);
    add_black_keyword(tf);
}

void 
airport::ListCrawler::add_black_keyword(std::string &keyword)
{
    blackKeywords.push_back(keyword);
}

void 
airport::ListCrawler::set_black_keywords(std::vector<std::string> &keywords)
{
    blackKeywords = keywords;
}

void 
airport::ListCrawler::add_staticForm_data(std::string &key, std::string &value)
{
    staticFormData.insert(std::map<std::string, std::string>::value_type(key, value));
}

void 
airport::ListCrawler::add_staticForm_data(const char *key, const char *value)
{
    std::string tkey(key);
    std::string tValue(value);
    add_staticForm_data(tkey, tValue);
}
void 
airport::ListCrawler::set_staticForm_data(std::map<std::string, std::string> &data)
{
    staticFormData = data;
}

void 
airport::ListCrawler::add_firstPageForm_data(std::string &key, std::string &value)
{
    firstPageFormData.insert(std::map<std::string, std::string>::value_type(key, value));
}

void 
airport::ListCrawler::add_firstPageForm_data(const char *key, const char *value)
{
    std::string tkey(key);
    std::string tValue(value);
    add_firstPageForm_data(tkey, tValue);
}
void 
airport::ListCrawler::set_firstPageForm_data(std::map<std::string, std::string> &data)
{
    firstPageFormData = data;
}

void 
airport::ListCrawler::add_inpageForm_data(std::string &key, std::string &value)
{
    inpageFormData.insert(std::map<std::string, std::string>::value_type(key, value));
}

void 
airport::ListCrawler::add_inpageForm_data(const char *key, const char *value)
{
    std::string tkey(key);
    std::string tValue(value);
    add_inpageForm_data(tkey, tValue);
}
void 
airport::ListCrawler::set_inpageForm_data(std::map<std::string, std::string> &data)
{
    inpageFormData = data;
}

void 
airport::ListCrawler::add_autoForm_data(std::string &key, const int value)
{
    autoFormData.insert(std::map<std::string, int>::value_type(key, value));
}

void 
airport::ListCrawler::add_autoForm_data(const char *key, const int value)
{
    std::string tkey(key);
    add_autoForm_data(tkey, value);
}
void 
airport::ListCrawler::set_autoForm_data(std::map<std::string, int> &data)
{
    autoFormData = data;
}

template <typename T>
void 
airport::ListCrawler::add_observer(std::string &key, T t, std::string &module)
{
    std::pair<function_type, std::string> p(function_type(t), module);
    observers.insert( std::map<std::string, std::pair<function_type, std::string> > :: value_type(key, p) );
}

void 
airport::ListCrawler::set_observers(std::map<std::string, std::pair<function_type, std::string> > &observers)
{
    this->observers = observers;
}

template <typename T> 
void 
airport::ListCrawler::set_list_observer(T t, std::string &module)
{
    listObserver = std::pair<function_type, std::string>(function_type(t), module);
}

void 
airport::ListCrawler::set_url_pattern(const char *url_pattern)
{
    std::string up(url_pattern);
    set_url_pattern(up);
}

void 
airport::ListCrawler::set_url_pattern(std::string &url_pattern)
{
    urlPattern = url_pattern;
}

std::vector<airport::HttpResponse> 
airport::ListCrawler::start()
{
    std::vector<airport::HttpResponse> httpResponse;
    airport::PaginateCrawler paginateCrawler(startUrl);
    paginateCrawler.set_node_name(nodename);
    paginateCrawler.set_start_number(startNumber);
    paginateCrawler.set_end_number(endNumber);
    paginateCrawler.set_page_step(pageStep);
    paginateCrawler.set_max_threads(maxThreads);
    paginateCrawler.set_replace_marker(replaceMarker);
    paginateCrawler.set_http_client(httpClient);
    paginateCrawler.set_parser(paginate_parser);
    paginateCrawler.set_staticForm_data(staticFormData);
    paginateCrawler.set_firstPageForm_data(firstPageFormData);
    paginateCrawler.set_inpageForm_data(inpageFormData);
    paginateCrawler.set_autoForm_data(autoFormData);
    /*
    std::map<std::string, std::string>::iterator it;
    for (it=paginate_parser.begin(); it!=paginate_parser.end(); ++it)
    {
        std::string key(it->first);
        std::string value(it->second);
        paginateCrawler.add_paginate_parser(key, value);
    }
    */
    std::string lastPageUrl = startUrl;
    if (staticFormData.empty())
    {
        size_t found = startUrl.find(replaceMarker);
        if (found==std::string::npos)
            return httpResponse;
        int markerSize = replaceMarker.size();
        std::string replaceStr = boost::lexical_cast<std::string>(endNumber);
        lastPageUrl.replace(found, markerSize, replaceStr);
    }
    
    
    std::vector<airport::HttpResponse> pageResponse = paginateCrawler.start();
    Mongo mongoDB;
    mongo::DBClientConnection mongoConnection;
    mongoDB.connect(&mongoConnection);
    
    for(std::vector<airport::HttpResponse>::iterator it=pageResponse.begin(); it!=pageResponse.end(); ++it)
    {
        airport::Url currentUrl = it->get_url();
        std::string html = it->get_parsed_data();
        std::vector<std::string> urls = extractUrls(html);
        if (urls.empty())
            continue;
        airport::BasicCrawler basicCrawler;
        basicCrawler.set_node_name(nodename);
        basicCrawler.set_http_client(httpClient);
        basicCrawler.set_essential_fields(essentialFields);
        basicCrawler.set_black_keywords(blackKeywords);
        basicCrawler.set_user_dict(userDict);
        if (!observers.empty())
        {
            basicCrawler.set_observers(observers);
        }
        for(std::vector<std::string>::iterator uit=urls.begin(); uit!=urls.end(); ++ uit)
        {
            std::string tmpurl = (*uit);
            std::string hashStr = airport::Utils::sha1(tmpurl);
            if (std::find( urlHash.begin(), urlHash.end(), hashStr )!=urlHash.end())
                continue;
            if (discardExternalDuplicate && saveInMongo)
            {
                mongo::BSONObj b = mongoDB.getHttpResponseById(mongoConnection, hashStr);
                if (!b.isEmpty())
                {
                    continue;
                }
            }
            basicCrawler.add_url(tmpurl);
            urlHash.push_back(hashStr);
        }
        basicCrawler.set_parser(parser);
        std::vector <airport::HttpResponse> res = basicCrawler.start();
        if (res.empty())
            continue;
        for(std::vector<airport::HttpResponse>::iterator rit=res.begin(); rit!=res.end(); ++rit)
        {
            if (saveInMongo)
            {
                mongo::BSONObj b = mongoDB.httpResponseBSONObj((*rit));
                mongoDB.insertHttpResponse(mongoConnection, b);
            }else{
                httpResponse.push_back((*rit));
            }
        }
        resultPages ++;
        if (lastPageUrl == currentUrl.get_raw_url())
        {
            lastPageHasResults = true;
        }
    }
    return httpResponse;
}

std::vector<std::string>
airport::ListCrawler::extractUrls(std::string &html)
{
    std::vector<std::string> urls;
    boost::regex re(linkRegex.c_str(), boost::regex::normal | boost::regbase::icase);
    boost::regex urlPatternRegex(urlPattern, boost::regex::normal | boost::regex::newline_alt);
    boost::sregex_token_iterator i(html.begin(), html.end(), re, 1);
    boost::sregex_token_iterator j;
    while(i != j) {
        std::string turl((*i++));
        if (!listObserver.second.empty())
        {
            turl = listObserver.first(listObserver.second, turl);
        }
        airport::Url tmpUrl(turl, startUrl);
        std::string normalizedUrl = tmpUrl.get_normalized_url();
        boost::match_results<std::string::const_iterator> what;
        if(0 == boost::regex_match(normalizedUrl, what, urlPatternRegex, boost::match_default | boost::match_partial))
        {
            continue;
        }
        if(!what[0].matched)
        {
            continue;
        }
        urls.push_back(normalizedUrl);
    }
    return urls;
}
