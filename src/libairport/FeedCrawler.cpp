#include "FeedCrawler.h"
#include "BasicCrawler.h"
#include "Utils.h"
#include "Mongo.h"
#include <time.h>
#include <algorithm>
#include <cctype>

airport::FeedCrawler::FeedCrawler():saveInMongo(true), results(0)
{
    
}

airport::FeedCrawler::~FeedCrawler()
{
    
}

void 
airport::FeedCrawler::set_save_in_mongo(bool save)
{
    saveInMongo = save;
}

std::pair<airport::Feed, std::vector<airport::FeedEntry> >
airport::FeedCrawler::get(const char *url)
{
    std::string turl(url);
    return get(turl);
}

std::pair<airport::Feed, std::vector<airport::FeedEntry> >
airport::FeedCrawler::get(std::string &url)
{
    std::pair<airport::Feed, std::vector<airport::FeedEntry> > response;
    std::string hashUrl = airport::Utils::sha1(url);
    int lastTime = time(NULL);
    std::string etag, lastModified;
    bool connected = false;
    {
        Mongo mongoDB;
        mongo::DBClientConnection mongoConnection;
        connected = mongoDB.connect(&mongoConnection);
        if (connected)
        {
            mongo::BSONObj b = mongoDB.getFeedById(mongoConnection, hashUrl);
            if (!b.isEmpty())
            {
                lastTime = b.getIntField("updated_datetime");
                etag = b.getStringField("etag");
                lastModified = b.getStringField("last_modified");
            }
        }
    }
    airport::BasicCrawler bc;
    airport::HttpClient hc = httpClient;
    hc.set_opt_timevalue((time_t)lastTime);
    if (!etag.empty())
    {
        std::string etagHeader = "If-None-Match: \"" + etag + "\"";
        hc.add_opt_header(etagHeader);
    }
    if (!lastModified.empty())
    {
        std::string lastModifiedHeader = "If-Modified-Since: " + lastModified;
        hc.add_opt_header(lastModifiedHeader);
    }
    hc.set_opt_nouseragent(true);
    bc.set_http_client(hc);
    bc.add_url(url);
    std::vector<airport::HttpResponse> hr = bc.start();
    if (hr.empty()) return response;
    airport::HttpResponse httpResponse = hr[0];
    //std::cout << "FEED: " << httpResponse.get_html_body() << std::endl;
    response = parse(httpResponse);
    if (saveInMongo && connected)
    {
        airport::Feed feed = response.first;
        std::vector<airport::FeedEntry> entries = response.second;
        save(feed, entries);
    }else{
        results = response.second.size();
    }
    return response;
}

std::pair<airport::Feed, std::vector<airport::FeedEntry> >
airport::FeedCrawler::parse(airport::HttpResponse &httpResponse)
{
    std::pair<airport::Feed, std::vector<airport::FeedEntry> > response;
    ticpp::Document tdoc;
    tdoc.Parse(httpResponse.get_html_body().c_str());
    airport::FeedType ft = detectType(tdoc);
    if (ft == airport::INVALID_FEED) return response;
    response = parseRSS2(tdoc, httpResponse, ft);
    return response;
}
std::pair<airport::Feed, std::vector<airport::FeedEntry> >
airport::FeedCrawler::parseRSS2(ticpp::Document &tdoc, airport::HttpResponse &httpResponse, const airport::FeedType feedType)
{
    ticpp::Element* pChanel = tdoc.FirstChildElement()->FirstChildElement();
    
    ticpp::Iterator< ticpp::Element > child;
    airport::Feed feed;
    airport::Url url = httpResponse.get_url();
    std::string furl = url.get_effective_url();
    feed.set_url(furl);
    std::string header = httpResponse.get_html_header();
    std::map<std::string, std::string> headers = airport::HttpClient::parse_headers(header);
    std::map<std::string, std::string>::iterator it;
    it = headers.find("Last-Modified");
    if (it!=headers.end())
    {
        feed.set_last_modified(it->second);
    }
    it = headers.find("ETag");
    if (it!=headers.end())
    {
        feed.set_etag(it->second);
    }
    std::vector<airport::FeedEntry> entries;
    std::string author;
    for ( child = child.begin( pChanel ); child != child.end(); child++ )
    {
        std::string tag = child->Value();
        std::transform(tag.begin(), tag.end(), tag.begin(), (int(*)(int)) std::tolower);
        std::string value = child->GetTextOrDefault("");
        if (tag == "title")
        {
            feed.set_title(value);
        }else if (tag == "description") {
            std::string desc = airport::Utils::htmlSanitize(value);
            feed.set_desc(desc);
        }else if (tag == "link" && feed.get_link().empty()) {
            feed.set_link(value);
        }else if (tag == "pubdate") {
            feed.set_pubdate(airport::Utils::dateParser(value));
        }else if (tag == "lastbuilddate" && feed.get_pubdate() == 0) {
            feed.set_pubdate(airport::Utils::dateParser(value));
        }else if (tag == "dc:date" && feed.get_pubdate() == 0) {
            feed.set_pubdate(airport::Utils::dateParser(value));
        }else if (tag == "feedburner:info") {
            child->GetAttribute("uri", &author);
        }else if (tag == "item" && feedType == airport::RSS2) {
            std::string webLink = feed.get_link();
            entries.push_back( rss2Entry(child.Get(), furl, webLink, author) );
        }
    }
    if (feedType == airport::RDF)
    {
        ticpp::Element* rdf = tdoc.FirstChildElement();
        for ( child = child.begin( rdf ); child != child.end(); child++ )
        {
            if (child->Value() != "item") continue;
            std::string webLink = feed.get_link();
            entries.push_back( rss2Entry(child.Get(), furl, webLink, author) );
        }
    }
    std::pair<airport::Feed, std::vector<airport::FeedEntry> > response(feed, entries);
    return response;
}

airport::FeedEntry
airport::FeedCrawler::rss2Entry(const ticpp::Element *item, std::string &feedLink, std::string &webLink, std::string &author)
{
    airport::FeedEntry entry;
    ticpp::Iterator< ticpp::Element > child;
    for ( child = child.begin( item ); child != child.end(); child++ )
    {
        std::string tag = child->Value();
        std::transform(tag.begin(), tag.end(), tag.begin(), (int(*)(int)) std::tolower);
        std::string value = child->GetTextOrDefault("");
        entry.set_feed_link(feedLink);
        entry.set_web_link(webLink);
        if (tag == "title")
        {
            entry.set_title(value);
        }else if (tag == "description") {
            std::string content = value + entry.get_desc();
            entry.set_desc(value);
        }else if (tag == "link") {
            entry.set_link(value);
        }else if (tag == "feedburner:origlink") {
            entry.set_link(value);
        }else if (tag == "pubdate") {
            entry.set_pubdate(airport::Utils::dateParser(value));
        }else if (tag == "author" && entry.get_author().empty()) {
            if (value.empty())
            {
                value = child.Get()->FirstChildElement()->GetTextOrDefault("");
            }
            entry.set_author(value);
        }else if (tag == "category") {
            std::string cat = entry.get_category();
            if (cat.empty()) cat = value;
            else cat += "," + value;
            entry.set_category(cat);
        }else if (tag == "dc:subject") {
            entry.set_category(value);
        }else if (tag == "comments") {
            entry.set_comments(value);
        }else if (tag == "dc:creator" && entry.get_author().empty()) {
            entry.set_author(value);
        }else if (tag == "content:encoded") {
            std::string content = entry.get_desc() + value;
            entry.set_desc(content);
        }else if (tag == "dc:date") {
            entry.set_pubdate(airport::Utils::dateParser(value));
        }else if (tag == "guid") {
            entry.set_guid(value);
        }
        if (entry.get_author().empty()) entry.set_author(author);
    }
    return entry;
}

airport::FeedType
airport::FeedCrawler::detectType(ticpp::Document &tdoc)
{
    airport::FeedType ft = airport::INVALID_FEED;
    ticpp::Element* pElem = tdoc.FirstChildElement();
    if (pElem->Value() == "rss")
    {
        std::string version = pElem->GetAttribute("version");
        if (version == "2.0")
        {
            ft = airport::RSS2;
            return ft;
        }
    }else if (pElem->Value() == "rdf:RDF") {
        ft = airport::RDF;
        return ft;
    }
    return ft;
}

void 
airport::FeedCrawler::save(airport::Feed &feed, std::vector<airport::FeedEntry> &entries)
{
    Mongo mongoDB;
    mongo::DBClientConnection mongoConnection;
    bool connected = mongoDB.connect(&mongoConnection);
    if (!connected) 
    {
        results = entries.size();
        return;
    }
    mongoDB.updateFeed(mongoConnection, feed);
    std::vector<airport::FeedEntry>::iterator it;
    for (it=entries.begin();it!=entries.end();++it)
    {
        if (mongoDB.updateFeedEntry(mongoConnection, (*it)))
            results ++;
    }
}

void
airport::FeedCrawler::set_http_client(airport::HttpClient &hc)
{
    httpClient = hc;
}