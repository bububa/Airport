#include "Feed.h"

airport::Feed::Feed()
{
    
}

airport::Feed::~Feed()
{
    
}

void 
airport::Feed::set_url(std::string &url)
{
    this->url = url;
}

void 
airport::Feed::set_title(std::string &title)
{
    this->title = title;
}

void 
airport::Feed::set_desc(std::string &desc)
{
    description = desc;
}

void 
airport::Feed::set_link(std::string &link)
{
    this->link = link;
}

void 
airport::Feed::set_pubdate(time_t pubDate)
{
    this->pubDate = pubDate;
}

void 
airport::Feed::set_last_modified(std::string &last_modified)
{
    lastModified = last_modified;
}

void 
airport::Feed::set_etag(std::string &etag)
{
    this->etag = etag;
}