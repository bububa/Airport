#ifndef HTTPRESPONSE_H_VKQOJT6K
#define HTTPRESPONSE_H_VKQOJT6K

#include <string>
#include <time.h>
#include <map>
#include "Url.h"

namespace airport
{
    
    class HttpResponse
    {
        airport::Url url;
        std::string htmlBody;
        std::string htmlHeader;
        long responseCode;
        double totalTime;
        time_t updatedTime;
        std::string parsedData;
        std::map<std::string, std::string> userInfo;
    public:
        HttpResponse(airport::Url &url, std::string &htmlBody, long responseCode, double totalTime, time_t updatedTime);
        HttpResponse();
        ~HttpResponse();
        
        HttpResponse& operator= (HttpResponse &httpResponse);
        
        void set_url(airport::Url &url);
        void set_html_body(std::string &htmlBody);
        void set_html_header(std::string &htmlHeader);
        void set_response_code(long responseCode);
        void set_total_time(double totalTime);
        void set_updated_time(time_t updatedTime);
        void set_parsed_data(std::string &parsedData);
        void set_user_info(std::map<std::string, std::string> &userInfo);
        
        airport::Url get_url() const { return url; };
        std::string get_html_body() const { return htmlBody; };
        std::string get_html_header() const { return htmlHeader; };
        long get_response_code() const { return responseCode; };
        double get_total_time() const { return totalTime; };
        time_t get_updated_time() const { return updatedTime; };
        std::string get_parsed_data() const { return parsedData.empty()?htmlBody:parsedData; };
        std::string get_raw_parsed_data() const {return parsedData;};
    };
    
}

#endif /* end of include guard: HTTPRESPONSE_H_VKQOJT6K */
