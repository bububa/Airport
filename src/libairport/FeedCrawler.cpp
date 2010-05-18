#include "FeedCrawler.h"
#include "BasicCrawler.h"
#include "Utils.h"
//#include "distance.h"
#include "Mongo.h"
#include <time.h>
#include <algorithm>
#include <cctype>

template<class T>
struct less_second
: std::binary_function<T,T,bool>
{
   inline bool operator()(const T& lhs, const T& rhs)
   {
      return lhs.second < rhs.second;
   }
};

airport::FeedCrawler::FeedCrawler():saveInMongo(true), fullRss(false), enableUpdate(true), results(0)
{
    
}

airport::FeedCrawler::~FeedCrawler()
{
    
}

void 
airport::FeedCrawler::set_node_name (const char *node_name)
{
    std::string tn(node_name);
    set_node_name(tn);
}

void 
airport::FeedCrawler::set_node_name (std::string &node_name)
{
    nodename = node_name;
}

void 
airport::FeedCrawler::set_save_in_mongo(bool save)
{
    saveInMongo = save;
}

void 
airport::FeedCrawler::set_full_rss(bool full)
{
    fullRss = full;
}

void
airport::FeedCrawler::set_enable_update(bool update)
{
    enableUpdate = update;
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
    bc.set_node_name(nodename);
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
    if (fullRss)
    {
        Mongo mongoDB;
        mongo::DBClientConnection mongoConnection;
        bool connected;
        if (!enableUpdate)
        {
            connected = mongoDB.connect(&mongoConnection);
        }
        airport::BasicCrawler bc;
        bc.set_node_name(nodename);
        for (std::vector<airport::FeedEntry>::iterator it=response.second.begin();it!=response.second.end();++it)
        {
            airport::FeedEntry entry = (*it);
            std::string url = entry.get_link();
            if (!enableUpdate && connected)
            {
                std::string hashUrl = airport::Utils::sha1(url);
                mongo::BSONObj b = mongoDB.getFeedEntryById(mongoConnection, hashUrl);
                if (b.isEmpty())
                {
                    bc.add_url(url);
                }
            }else{
                bc.add_url(url);
            }
        }
        std::vector<airport::HttpResponse> hr = bc.start();
        for(std::vector<airport::HttpResponse>::iterator hri=hr.begin();hri!=hr.end();++hri)
        {
            airport::Url url = hri->get_url();
            for (std::vector<airport::FeedEntry>::iterator it=response.second.begin();it!=response.second.end();++it)
            {
                airport::FeedEntry entry = (*it);
                if (entry.get_link() == url.get_raw_url())
                {
                    std::string html = hri->get_html_body();
                    std::string title = entry.get_title();
                    std::string desc = entry.get_desc();
                    std::pair<std::string, std::string> p = extractContent(html, title, desc);
                    std::string rtitle = p.first;
                    std::string rdesc = p.second;
                    it->set_title(rtitle);
                    it->set_desc(rdesc);
                    std::cout << rtitle << std::endl;
                    std::cout << rdesc << std::endl;
                }
            }
        }
    }
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

std::pair<std::string, std::string>
airport::FeedCrawler::extractContent(std::string &html, std::string &title, std::string &content)
{
    std::string htmlTidy = airport::Utils::tidy(html);
    TidyDoc tdoc = tidyCreate();
    TidyBuffer errbuf = {0};
    tidySetErrorBuffer( tdoc, &errbuf );
    tidyBufFree( &errbuf );
    tidySetCharEncoding( tdoc, "utf8");
    tidyParseString( tdoc, htmlTidy.c_str() );
    
    std::string responseContent = content;
    std::string responseTitle = title;
    std::string titleContent = title + content;
    airport::nod_t matchNode = bestMatchNode(tdoc, tidyGetBody(tdoc), titleContent);
    if (matchNode.second>=0)
    {
        TidyNode node = matchNode.first;
        ctmbstr name = airport::Utils::nodeName(node);
        TidyBuffer buf = {0};
        tidyNodeGetText (tdoc, node, &buf);
        std::string text(reinterpret_cast<const char *>(buf.bp));
        text.erase(std::remove(text.begin(), text.end(), '\n'), text.end());
        tidyBufFree( &buf );
        responseContent = text;
        text.erase(std::remove(text.begin(), text.end(), ' '), text.end());
        
        airport::nod_t titleMatchNode = bestMatchNode(tdoc, node, title);
        if (titleMatchNode.second>=0)
        {
            TidyNode titleNode = titleMatchNode.first;
            TidyBuffer buf = {0};
            tidyNodeGetText (tdoc, titleNode, &buf);
            std::string titleText(reinterpret_cast<const char *>(buf.bp));
            titleText.erase(std::remove(titleText.begin(), titleText.end(), '\n'), titleText.end());
            tidyBufFree( &buf );
            responseTitle = airport::Utils::stripTags(titleText);
            titleText.erase(std::remove(titleText.begin(), titleText.end(), ' '), titleText.end());
            size_t found = text.find(titleText);
            if (found!=std::string::npos)
            {
                responseContent = airport::Utils::htmlExceptNode(tdoc, node, titleNode);
            }else{
                responseContent = airport::Utils::htmlSafeNode(tdoc, node);
            }
        }
    }
    std::pair<std::string, std::string> response(responseTitle, responseContent);
    return response;
}

airport::nod_t
airport::FeedCrawler::bestMatchNode( TidyDoc tdoc, TidyNode tnod, std::string &request )
{
    TidyNode child;
    airport::nod_t response = airport::nod_t(child, -1);
    std::string reqStr = airport::Utils::stripTags(request);
    if (reqStr.empty()) return response;
    std::string tmpStr;
    if (reqStr.length() > 4) tmpStr = reqStr.substr(0, 4);
    else tmpStr = reqStr;
    size_t found;
    for ( child = tidyGetChild(tnod); child; child = tidyGetNext(child) )
    {
        ctmbstr name = airport::Utils::nodeName(child);
        //assert( name != NULL );
        if ( airport::Utils::isSafeNode(child) )
        {
            TidyBuffer buf = {0};
            tidyNodeGetText (tdoc, child, &buf);
            std::string text(reinterpret_cast<const char *>(buf.bp));
            text.erase(std::remove(text.begin(), text.end(), '\n'), text.end());
            tidyBufFree( &buf );
            std::string domStr = airport::Utils::stripTags(text);
            if (domStr.empty()) continue;
            found=domStr.find(tmpStr);
            if (found==std::string::npos) continue;
            if (domStr.length() > reqStr.length()) domStr = domStr.substr(0, reqStr.length());
            int distance = airport::Utils::distance(reqStr, domStr);
            //std::cout << name << ":" << distance << ", " << s << std::endl;
            if (response.second < 0 || distance < response.second)
            {
                response = nod_t(child, distance);
            }else if (distance == response.second){
                TidyBuffer buf = {0};
                tidyNodeGetText (tdoc, response.first, &buf);
                std::string rtext(reinterpret_cast<const char *>(buf.bp));
                rtext.erase(std::remove(rtext.begin(), rtext.end(), '\n'), rtext.end());
                tidyBufFree( &buf );
                if (text.length() < rtext.length())
                {
                    response = nod_t(child, distance);
                }
            }
            std::string namet(reinterpret_cast<const char *>(name));
            if ( !(namet=="Text" || namet=="Comment" || airport::Utils::childNodeIsText(child) || tidyNodeIsHR(child) || tidyNodeIsBR(child) || tidyNodeIsIMG(child)) )
            {
                airport::nod_t node = bestMatchNode(tdoc, child, request);
                if (node.second < 0) continue;
                TidyBuffer rbuf = {0};
                tidyNodeGetText (tdoc, response.first, &rbuf);
                std::string rtext(reinterpret_cast<const char *>(rbuf.bp));
                rtext.erase(std::remove(rtext.begin(), rtext.end(), '\n'), rtext.end());
                tidyBufFree( &rbuf );
                TidyBuffer nbuf = {0};
                tidyNodeGetText (tdoc, node.first, &nbuf);
                std::string ntext(reinterpret_cast<const char *>(nbuf.bp));
                ntext.erase(std::remove(ntext.begin(), ntext.end(), '\n'), ntext.end());
                tidyBufFree( &nbuf );
                if (response.second < 0 || node.second < response.second || node.second == response.second && ntext.length() < rtext.length())
                {
                    response = node;
                }
            }
        }
    }
    return response;
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
        if (enableUpdate)
        {
            if (mongoDB.updateFeedEntry(mongoConnection, (*it)))
                results ++;
        }else{
            mongoDB.insertFeedEntry(mongoConnection, (*it));
            results ++;
        }
    }
}

void
airport::FeedCrawler::set_http_client(airport::HttpClient &hc)
{
    httpClient = hc;
}