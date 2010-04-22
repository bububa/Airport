#ifndef FEEDENTRY_H_5J8OGPG2
#define FEEDENTRY_H_5J8OGPG2

#include <string>
#include <time.h>

namespace airport
{
    
    class FeedEntry
    {
        std::string title;
        std::string link;
        std::string description;
        std::string author;
        std::string category;
        std::string comments;
        time_t pubDate;
        std::string guid;
        std::string feedLink;
        std::string webLink;
    public:
        FeedEntry();
        ~FeedEntry();
        void set_title(std::string &title);
        void set_desc(std::string &desc);
        void set_link(std::string &link);
        void set_author(std::string &author);
        void set_category(std::string &category);
        void set_comments(std::string &comments);
        void set_guid(std::string &guid);
        void set_pubdate(time_t pubDate);
        void set_feed_link(std::string &link);
        void set_web_link(std::string &link);
        std::string get_title() const { return title; }
        std::string get_desc() const { return description; }
        std::string get_link() const { return link; }
        time_t get_pubdate() const { return pubDate; }
        std::string get_author() const { return author; }
        std::string get_category() const { return category; }
        std::string get_comments() const { return comments; }
        std::string get_guid() const { return guid; }
        std::string get_feed_link() const { return feedLink; }
        std::string get_web_link() const { return webLink; }
    };
}

#endif /* end of include guard: FEEDENTRY_H_5J8OGPG2 */
