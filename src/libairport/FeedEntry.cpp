#include "FeedEntry.h"

airport::FeedEntry::FeedEntry()
{
    
}

airport::FeedEntry::~FeedEntry()
{
    
}

void 
airport::FeedEntry::set_title(std::string &title)
{
    this->title = title;
}

void 
airport::FeedEntry::set_desc(std::string &desc)
{
    description = desc;
}

void 
airport::FeedEntry::set_link(std::string &link)
{
    this->link = link;
}

void 
airport::FeedEntry::set_pubdate(time_t pubDate)
{
    this->pubDate = pubDate;
}

void 
airport::FeedEntry::set_author(std::string &author)
{
    this->author = author;
}

void 
airport::FeedEntry::set_category(std::string &category)
{
    this->category = category;
}

void 
airport::FeedEntry::set_comments(std::string &comments)
{
    this->comments = comments;
}

void 
airport::FeedEntry::set_guid(std::string &guid)
{
    this->guid = guid;
}

void 
airport::FeedEntry::set_feed_link(std::string &link)
{
    this->feedLink = link;
}

void 
airport::FeedEntry::set_web_link(std::string &link)
{
    this->webLink = link;
}