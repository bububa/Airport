#include "HttpResponse.h"
#include "Url.h"
#include <stdio.h>
#include <string>
#include <time.h>

airport::HttpResponse::HttpResponse (airport::Url &url, std::string &htmlBody, long responseCode, double totalTime, time_t updatedTime)
{
    this->url = url;
    this->htmlBody = htmlBody;
    this->responseCode = responseCode;
    this->totalTime = totalTime;
    this->updatedTime = updatedTime;
}

airport::HttpResponse::HttpResponse()
{
    
}

airport::HttpResponse::~HttpResponse()
{
    
}

airport::HttpResponse&
airport::HttpResponse::operator= (HttpResponse &httpResponse)
{
    this->url = httpResponse.url;
    htmlBody = httpResponse.htmlBody;
    responseCode = httpResponse.responseCode;
    totalTime = httpResponse.totalTime;
    updatedTime = httpResponse.updatedTime;
    parsedData = httpResponse.parsedData;
    userInfo = httpResponse.userInfo;
    return *this;
}

void 
airport::HttpResponse::set_url(airport::Url &url)
{
    this->url = url;
}

void 
airport::HttpResponse::set_html_body(std::string &htmlBody)
{
    this->htmlBody = htmlBody;
}

void 
airport::HttpResponse::set_html_header(std::string &htmlHeader)
{
    this->htmlHeader = htmlHeader;
}

void 
airport::HttpResponse::set_response_code(long responseCode)
{
    this->responseCode = responseCode;
}

void 
airport::HttpResponse::set_total_time(double totalTime)
{
    this->totalTime = totalTime;
}

void 
airport::HttpResponse::set_updated_time(time_t updatedTime)
{
    this->updatedTime = updatedTime;
}

void 
airport::HttpResponse::set_parsed_data(std::string &parsedData)
{
    this->parsedData = parsedData;
}

void 
airport::HttpResponse::set_user_info(boost::any &user_info)
{
    userInfo = user_info;
}