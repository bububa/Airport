#include "DepthCrawler.h"
#include "BasicCrawler.h"
#include "Utils.h"
#include "Url.h"
#include <boost/regex.hpp>
#include <mongo/client/dbclient.h>
#include "Mongo.h"

airport::DepthCrawler::DepthCrawler(const char *urlCString):maxThreads(DEFAULT_DEPTHCRAWLER_MAX_THREADS), resultPages(0), maxDepth(DEFAULT_DEPTHCRAWLER_MAX_DEPTH)
{
    set_start_url(urlCString);
}

airport::DepthCrawler::DepthCrawler(std::string &urlString):maxThreads(DEFAULT_DEPTHCRAWLER_MAX_THREADS), resultPages(0), maxDepth(DEFAULT_DEPTHCRAWLER_MAX_DEPTH)
{
    set_start_url(urlString);
}

airport::DepthCrawler::DepthCrawler():maxThreads(DEFAULT_DEPTHCRAWLER_MAX_THREADS), resultPages(0), maxDepth(DEFAULT_DEPTHCRAWLER_MAX_DEPTH)
{
    
}

airport::DepthCrawler::~DepthCrawler()
{
    
}

void
airport::DepthCrawler::set_start_url(const char *start_url)
{
    std::string tu(start_url);
    set_start_url(tu);
}

void
airport::DepthCrawler::set_start_url(std::string &start_url)
{
    startUrl = start_url;
}

void 
airport::DepthCrawler::set_max_threads(unsigned int max_threads)
{
    maxThreads = max_threads;
}

void 
airport::DepthCrawler::set_http_client(airport::HttpClient &httpClient)
{
    this->httpClient = httpClient;
}

void 
airport::DepthCrawler::add_accept_page_parser(const char *key, const char *regxString)
{
    std::string tkey(key);
    std::string tRegxString(regxString);
    add_accept_page_parser(tkey, tRegxString);
}

void 
airport::DepthCrawler::add_accept_page_parser(std::string &key, std::string &regxString)
{
    this->acceptPageParser.insert(std::map<std::string, std::string>::value_type(key, regxString));
}

void 
airport::DepthCrawler::set_accept_page_parser(std::map<std::string, std::string> &accept_page_parser)
{
    this->acceptPageParser = accept_page_parser;
}

void 
airport::DepthCrawler::reset_accept_page_parser()
{
    acceptPageParser.clear();
}

void 
airport::DepthCrawler::add_parser(const char *key, const char *regxString)
{
    std::string tkey(key);
    std::string tRegxString(regxString);
    add_parser(tkey, tRegxString);
}

void 
airport::DepthCrawler::add_parser(std::string &key, std::string &regxString)
{
    this->parser.insert(std::map<std::string, std::string>::value_type(key, regxString));
}

void 
airport::DepthCrawler::set_parser(std::map<std::string, std::string> &parser)
{
    this->parser = parser;
}

void 
airport::DepthCrawler::reset_parser()
{
    parser.clear();
}

void 
airport::DepthCrawler::add_user_dict(const char *key, const char *value)
{
    std::string tkey(key);
    std::string tValue(value);
    add_user_dict(tkey, tValue);
}

void 
airport::DepthCrawler::add_user_dict(std::string &key, std::string &value)
{
    this->userDict.insert(std::map<std::string, std::string>::value_type(key, value));
}

void 
airport::DepthCrawler::set_user_dict(std::map<std::string, std::string> &user_dict)
{
    this->userDict = user_dict;
}

void 
airport::DepthCrawler::add_essential_field(const char *field)
{
    std::string tf(field);
    add_essential_field(tf);
}

void 
airport::DepthCrawler::add_essential_field(std::string &field)
{
    essentialFields.push_back(field);
}

void 
airport::DepthCrawler::set_essential_fields(std::vector<std::string> &fields)
{
    essentialFields = fields;
}

template <typename T>
void 
airport::DepthCrawler::add_observer(std::string &key, T t, std::string &module)
{
    std::pair<function_type, std::string> p(function_type(t), module);
    observers.insert( std::map<std::string, function_type> :: value_type(key, p) );
}

void 
airport::DepthCrawler::set_target_url_pattern(const char *url_pattern)
{
    std::string up(url_pattern);
    set_target_url_pattern(up);
}

void 
airport::DepthCrawler::set_target_url_pattern(std::string &url_pattern)
{
    targetUrlPattern = url_pattern;
}

void 
airport::DepthCrawler::set_accept_url_pattern(const char *url_pattern)
{
    std::string up(url_pattern);
    set_accept_url_pattern(up);
}

void 
airport::DepthCrawler::set_accept_url_pattern(std::string &url_pattern)
{
    acceptUrlPattern = url_pattern;
}

void 
airport::DepthCrawler::set_reject_url_pattern(const char *url_pattern)
{
    std::string up(url_pattern);
    set_reject_url_pattern(up);
}

void 
airport::DepthCrawler::set_reject_url_pattern(std::string &url_pattern)
{
    rejectUrlPattern = url_pattern;
}

void
airport::DepthCrawler::start()
{
    Mongo mongoDB;
    mongo::DBClientConnection mongoConnection;
    mongoDB.connect(&mongoConnection);
    for(unsigned int depth=0; depth < maxDepth; depth++)
    {
        std::cout << "DEPTH: " << depth << std::endl;
        airport::BasicCrawler basicCrawler;
        basicCrawler.set_http_client(httpClient);
        basicCrawler.set_user_dict(userDict);
        basicCrawler.set_parser(acceptPageParser);
        if (!observers.empty())
        {
            basicCrawler.set_observers(observers);
        }
        if(depth == 0)
        {
            basicCrawler.add_url(startUrl);
            std::vector<airport::HttpResponse> httpResponse = basicCrawler.start();
            saveResponse(httpResponse, mongoConnection, depth);
        }else{
            std::string startUrlHash = airport::Utils::sha1(startUrl);
            int nToReturn = 100;
            for (int nToSkip=0; ;nToSkip+=nToReturn)
            {
                mongoConnection.ensureIndex(MONGO_DEPTHRESPONSE_COLLECTION, mongo::fromjson("{start_url:1, depth:1, parsed:1}"));
                mongo::BSONObj b;
                std::auto_ptr<mongo::DBClientCursor> cursor = mongoConnection.query(MONGO_DEPTHRESPONSE_COLLECTION, mongo::QUERY( "start_url" << startUrlHash << "depth" << depth << "parsed" << false), nToReturn, nToSkip);
                unsigned int url_count = 0;
                while( cursor->more() )
                {
                    b = cursor->next();
                    basicCrawler.add_url(b.getStringField("effective_url"));
                    url_count ++;
                }
                if (url_count == 0)
                    break;
                std::vector<airport::HttpResponse> httpResponse = basicCrawler.start();
                saveResponse(httpResponse, mongoConnection, depth);
            }
        }
    }
}

void
airport::DepthCrawler::saveResponse(std::vector<airport::HttpResponse> &httpResponse, mongo::DBClientConnection &mongoConnection, int depth)
{
    if (httpResponse.empty())
        return;
    std::vector<std::string> urls;
    std::string startUrlHash = airport::Utils::sha1(startUrl);
    for(std::vector<airport::HttpResponse>::iterator it=httpResponse.begin(); it!=httpResponse.end(); ++it)
    {
        airport::HttpResponse hr = (*it);
        airport::Url url = hr.get_url();
        std::cout << "SAVE: " << url.get_raw_url() << std::endl;
        bool isTarget = true;
        if (targetUrlPattern.size())
        {
            boost::match_results<std::string::const_iterator> what;
            boost::regex targetUrlPatternRegex(targetUrlPattern, boost::regex::normal | boost::regex::newline_alt);
            if(0 == boost::regex_match(url.get_effective_url(), what, targetUrlPatternRegex, boost::match_default | boost::match_partial))
            {
                isTarget = false;
            }
            if(!what[0].matched)
            {
                isTarget = false;
            }
        }
        mongo::BSONObjBuilder b;
        b.append("_id", airport::Utils::sha1(url.get_raw_url()));
        b.append("effective_url", url.get_effective_url());
        b.append("code", (int)hr.get_response_code());
        b.append("total_time", hr.get_total_time());
        b.append("html_body", hr.get_html_body());
        b.append("html_header", hr.get_html_header());
        b.append("parsed_data", hr.get_parsed_data());
        b.append("updated_time", (long long int)hr.get_updated_time());
        b.append("start_url", startUrlHash);
        b.append("depth", depth);
        b.append("parsed", true);
        b.append("is_target", isTarget);
        mongo::BSONObj p = b.obj();
        mongoConnection.insert(MONGO_DEPTHRESPONSE_COLLECTION, p);
        std::string html = hr.get_parsed_data();
        std::vector<std::string> tmpurls = extractUrls(html);
        if(tmpurls.empty())
            continue;
        std::vector<std::string>::iterator urlit = urls.end();
        urls.insert(urlit, tmpurls.begin(), tmpurls.end());
    }
    if (urls.empty())
        return;
    for(std::vector<std::string>::iterator it=urls.begin(); it!=urls.end(); ++it)
    {
        Mongo mongoDB;
        std::string hashStr = airport::Utils::sha1((*it));
        mongo::BSONObj b = mongoDB.getHttpResponseById(mongoConnection, hashStr);
        if (!b.isEmpty())
        {
            continue;
        }
        mongo::BSONObjBuilder builder;
        builder.append("_id", airport::Utils::sha1((*it)));
        builder.append("start_url", startUrlHash);
        builder.append("depth", depth);
        builder.append("parsed", false);
        mongo::BSONObj p = builder.obj();
        mongoConnection.insert(MONGO_DEPTHRESPONSE_COLLECTION, p);
    }
}

std::vector<std::string>
airport::DepthCrawler::extractUrls(std::string &html)
{
    std::vector<std::string> urls;
    boost::regex re("<\\s*A\\s+[^>]*href\\s*=\\s*[\"|\'](.*?)[\"|\']", boost::regex::normal | boost::regbase::icase);
    if (acceptUrlPattern.size())
        boost::regex acceptUrlPatternRegex(acceptUrlPattern, boost::regex::normal | boost::regex::newline_alt);
    boost::sregex_token_iterator i(html.begin(), html.end(), re, 1);
    boost::sregex_token_iterator j;
    while(i != j) {
        std::string turl((*i++));
        airport::Url tmpUrl(turl, startUrl);
        std::string normalizedUrl = tmpUrl.get_normalized_url();
        if (acceptUrlPattern.size())
        {
            boost::match_results<std::string::const_iterator> what;
            boost::regex acceptUrlPatternRegex(acceptUrlPattern, boost::regex::normal | boost::regex::newline_alt);
            if(0 == boost::regex_match(normalizedUrl, what, acceptUrlPatternRegex, boost::match_default | boost::match_partial))
            {
                continue;
            }
            if(!what[0].matched)
            {
                continue;
            }
        }
        if (rejectUrlPattern.size())
        {
            boost::match_results<std::string::const_iterator> what;
            boost::regex rejectUrlPatternRegex(rejectUrlPattern, boost::regex::normal | boost::regex::newline_alt);
            if(0 == boost::regex_match(normalizedUrl, what, rejectUrlPatternRegex, boost::match_default | boost::match_partial))
            {
                continue;
            }
            if(!what[0].matched)
            {
                continue;
            }
        }
        urls.push_back(normalizedUrl);
    }
    return urls;
}