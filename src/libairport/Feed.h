#ifndef FEED_H_NASKCLR0
#define FEED_H_NASKCLR0

#include <string>
#include <time.h>

namespace airport
{
    
    class Feed
    {
        std::string url;
        std::string title;
        std::string description;
        std::string link;
        time_t pubDate;
        std::string lastModified;
        std::string etag;
    public:
        Feed();
        ~Feed();
        void set_url(std::string &url);
        void set_title(std::string &title);
        void set_desc(std::string &desc);
        void set_link(std::string &link);
        void set_pubdate(time_t pubDate);
        void set_last_modified(std::string &last_modified);
        void set_etag(std::string &etag);
        std::string get_url() const { return url; }
        std::string get_title() const { return title; }
        std::string get_desc() const { return description; }
        std::string get_link() const { return link; }
        time_t get_pubdate() const { return pubDate; }
        std::string get_last_modified() const { return lastModified; }
        std::string get_etag() const { return etag; }
    };
}

#endif /* end of include guard: FEED_H_NASKCLR0 */
