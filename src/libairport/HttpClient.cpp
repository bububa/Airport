#include "Config.h"
#include "HttpClient.h"
#include "HttpResponse.h"
#include "Url.h"

#include "Utils.h"
#include <stdio.h>
#include <sstream>
#include <istream>
#include <fstream>

#include <time.h>
#include <utility>

#include <chardet.h>

#include <iconv.h>
#include <errno.h>
#include <string>
#include <vector>

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/threadpool.hpp>

#define CURLPP_ALLOW_NOT_AVAILABLE
#include <curlpp/curlpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>
#include <curlpp/Infos.hpp>

#define CHARSET_MAX 256

boost::mutex m_io_monitor;

class WriteFunctor {
  public:
    WriteFunctor() {
      stream_.reset(new std::ostringstream);
    }
    ~WriteFunctor() {
    }
    size_t Write(char* ptr, size_t size, size_t nmemb) {
      std::streamsize sizes = size * nmemb;
      stream_->write((const char*)ptr, sizes);
      return sizes;
    }
    std::string GetResponse() {
      return stream_->str();
    }
  private:
    boost::scoped_ptr< std::ostringstream > stream_;
};

class YesNo {
public:
    explicit YesNo(bool yn) : yesno(yn) {}
    std::string operator()() const {
        return yesno ? "Yes" : "No";
    }
    friend std::ostream &operator<<(std::ostream &strm, const YesNo &yn) {
        strm << yn();
        return strm;
    }
private:
    bool yesno;
};

struct Cookie {
    std::string name;
    std::string value;
    std::string domain;
    std::string path;
    std::time_t expires;
    bool tail;
    bool secure;
};

std::ostream &
operator<<(std::ostream &strm, const Cookie &cook)
{
    strm << "Cookie: '" << cook.name << "' (secure: " << YesNo(cook.secure) << ", tail: "
        << YesNo(cook.tail) << ") for domain: '" << cook.domain << "', "
        << "path: '" << cook.path << "'.\n";
    strm << "Value: '" << cook.value << "'.\n";
    strm << "Expires: '" << ctime(&cook.expires) << "'.\n";

    return strm;
}

std::vector<std::string> &
split_cookie_str(const std::string &str, std::vector<std::string> &in)
{
    std::string part;

    std::istringstream strm(str);
    while (std::getline(strm, part, '\t'))
        in.push_back(part);

    return in;
}

std::vector<std::string>
splitCookieStr(const std::string &str)
{
    std::vector<std::string> split;
    split_cookie_str(str, split);
    return split;
}

std::vector<std::string> &
splitCookieStr(const std::string &str, std::vector<std::string> &in)
{
    return split_cookie_str(str, in);
}

int StrToInt(const std::string &str)
{
    std::istringstream strm(str);
    int i = 0;
    if (!(strm >> i)) {
        throw cURLpp::RuntimeError("Unable to convert string '" + str + "' to integer!");
    }
    return i;
}

Cookie
MakeCookie(const std::string &str_cookie)
{
    std::vector<std::string> vC = splitCookieStr(str_cookie);
    Cookie cook;

    cook.domain = vC[0];
    cook.tail = vC[1] == "TRUE";
    cook.path = vC[2];
    cook.secure = vC[3] == "TRUE";
    cook.expires = StrToInt(vC[4]);
    cook.name = vC[5];
    cook.value = vC[6];

    return cook;
}

airport::HttpClient::HttpClient (airport::Url &url):
resetCookie(false), noCookie(true), connectTimeout(DEFAULT_CONNECTTIMEOUT), timeout(DEFAULT_TIMEOUT), maxRedirs(DEFAULT_MAXREDIRS), noUserAgent(false)
{
    this->url = url;
    set_opt_timevalue(time(NULL));
    std::string cookie_path = airport::DATA_PATH + "cookies/";
    set_cookie_path(cookie_path);
}

airport::HttpClient::HttpClient():
resetCookie(false), noCookie(true), connectTimeout(DEFAULT_CONNECTTIMEOUT), timeout(DEFAULT_TIMEOUT), maxRedirs(DEFAULT_MAXREDIRS), noUserAgent(false)
{
    set_opt_timevalue(time(NULL));
    std::string cookie_path = airport::DATA_PATH + "cookies/";
    set_cookie_path(cookie_path);
}

airport::HttpClient::~HttpClient()
{
    
}

airport::HttpClient&
airport::HttpClient::operator= (HttpClient &httpClient)
{   
    this->url = httpClient.url;
    cookiePath = httpClient.cookiePath;
    cookieList = httpClient.cookieList;
    resetCookie = httpClient.resetCookie;
    noCookie = httpClient.noCookie;
    connectTimeout = httpClient.connectTimeout;
    timeout = httpClient.timeout;
    maxRedirs = httpClient.maxRedirs;
    timeValue = httpClient.timeValue;
    userAgent = httpClient.userAgent;
    noUserAgent = httpClient.noUserAgent;
    proxyUserPwd = httpClient.proxyUserPwd;
    proxyList = httpClient.proxyList;
    formParts = httpClient.formParts;
    userInfo = httpClient.userInfo;
    headers = httpClient.headers;
    return *this;
}

airport::HttpResponse
airport::HttpClient::get(airport::Url &url)
{
    this->url = url;
    return get();
}

airport::HttpResponse
airport::HttpClient::get()
{
    set_state(STATE_START);
    struct timeval startTime;
    struct timezone timeZone;
    gettimeofday(&startTime, &timeZone);
    try
    {
        WriteFunctor headerWriteFunctor;
        WriteFunctor htmlWriteFunctor;
        
        std::ostringstream responseStream;
        curlpp::Cleanup myCleanup;
        curlpp::options::Url myUrl(url.get_normalized_url());
        curlpp::Easy request;
        
        request.setOpt(myUrl);
        if (!headers.empty())
        {
            request.setOpt(curlpp::options::HttpHeader(headers));
        }
        
        if (!noCookie)
        {
            std::string cookieFile = cookiePath + url.get_host();
            if (resetCookie)
            {
                cookieList.clear();
                resetCookie = false;
            }else{
                if (cookieList.empty())
                    cookieList = read_cookie_jar(cookieFile);
                for (std::vector<std::string>::iterator it = cookieList.begin();
                    it != cookieList.end();
                    ++it)
                {
        	        request.setOpt(curlpp::options::CookieList(*it));
                }
            }
            request.setOpt(curlpp::options::CookieJar(cookieFile.c_str()));
        }
        
        if (!noUserAgent)
        {
            if (userAgent.size()>0)
            {
                request.setOpt(curlpp::options::UserAgent(userAgent));
            }else{
                request.setOpt(curlpp::options::UserAgent(get_random_useragent()));
            }
        }
        
        if (!proxyList.empty())
        {
            proxyUserPwd = get_random_proxy();
        }
        if (proxyUserPwd.size() > 0)
        {
            request.setOpt(curlpp::options::ProxyUserPwd(proxyUserPwd));
        }
        
        if (formParts.size() > 0)
        {
            request.setOpt(curlpp::options::HttpPost(formParts));
        }else{
            request.setOpt(curlpp::options::HttpGet(true));
        }
        
        #ifdef HAVE_BOOST
            // 参考 curlpp-0.7.2\examples\example18.cpp
            request.setOpt(curlpp::options::BoostHeaderFunction(
              boost::bind(&WriteFunctor::Write, &headerWriteFunctor, _1, _2, _3) ) );
            request.setOpt(curlpp::options::BoostWriteFunction(
              boost::bind(&WriteFunctor::Write, &htmlWriteFunctor, _1, _2, _3) ) );
        #else
            // 其实这里使用 functor 方式足矣, boost::bind 虽然不错， 但用这里有点多余，
            // 而且 _1 这种占位符看起来还是不舒服， 不熟的人如读天书。
            curlpp::Types::WriteFunctionFunctor functor2(&headerWriteFunctor, &WriteFunctor::Write);
            request.setOpt( curlpp::options::HeaderFunction(functor2));
            curlpp::Types::WriteFunctionFunctor functor(&htmlWriteFunctor, &WriteFunctor::Write);
            request.setOpt( curlpp::options::WriteFunction(functor) );
        #endif
        //request.setOpt(new curlpp::options::NoBody(true));
        //request.setOpt(new curlpp::options::FileTime(true));
        //request.setOpt(new curlpp::options::WriteStream(&responseStream));
        request.setOpt(curlpp::options::NoSignal(true));
        request.setOpt(curlpp::options::MaxRedirs(maxRedirs));
        request.setOpt(curlpp::options::AutoReferer(true));
        request.setOpt(curlpp::options::FollowLocation(true));
        request.setOpt(curlpp::options::ConnectTimeout(connectTimeout));
        request.setOpt(curlpp::options::Timeout(timeout));
        request.setOpt(curlpp::options::SslVerifyPeer(true));
        request.setOpt(curlpp::options::SslVerifyHost(true));
        request.setOpt(curlpp::options::TimeValue(timeValue));
        
        request.perform();
        if (airport::DEBUG_LEVEL > airport::DEBUG_OFF)
        {
            boost::mutex::scoped_lock lock1(m_io_monitor);
            std::cout << "GOT: " << url.get_normalized_url() << std::endl;
        }
        //return airport::HttpResponse();
        
        std::string htmlBody = htmlWriteFunctor.GetResponse();
        std::string htmlHeader = headerWriteFunctor.GetResponse();
        airport::HttpResponse httpResponse = this->callback(request, htmlBody, htmlHeader);
        
        double totalTime;
        struct timeval endTime;
        gettimeofday(&endTime, &timeZone);
        totalTime = endTime.tv_sec + static_cast<double>(endTime.tv_usec)/1000000 - startTime.tv_sec - static_cast<double>(startTime.tv_usec)/1000000;
        
        httpResponse.set_total_time(totalTime);
        httpResponse.set_user_info(userInfo);
        httpResponse.set_updated_time(time(NULL));
        set_state(STATE_COMPLETED);
        return httpResponse;
        
    }
    catch(curlpp::RuntimeError & e)
    {
        if (airport::DEBUG_LEVEL > airport::DEBUG_NORMAL)
        {
            boost::mutex::scoped_lock lock2(m_io_monitor);
            std::cout << e.what() << url.get_normalized_url() << std::endl;
        }
    }
    catch(curlpp::LogicError & e)
    {
        if (airport::DEBUG_LEVEL > airport::DEBUG_NORMAL)
        {
            boost::mutex::scoped_lock lock3(m_io_monitor);
            std::cout << e.what() << url.get_normalized_url() << std::endl;
        }
    }
    set_state(STATE_FAILED);
    double totalTime;
    struct timeval endTime;
    gettimeofday(&endTime, &timeZone);
    totalTime = endTime.tv_sec + static_cast<double>(endTime.tv_usec)/1000000 - startTime.tv_sec - static_cast<double>(startTime.tv_usec)/1000000;
    std::string htmlBody("");
    airport::HttpResponse httpResponse(url, htmlBody, 408, totalTime, time(NULL));
    return httpResponse;
}

airport::HttpResponse 
airport::HttpClient::callback(curlpp::Easy &request, std::string &htmlBody, std::string &htmlHeader)
{
    airport::HttpResponse httpResponse;
    chardet_t pdet = NULL;
    int dataSize = strlen(htmlBody.c_str()) * sizeof(char);
    int is_need_convert = 0;
    char charset[CHARSET_MAX];
    
    if( dataSize > 0 && chardet_create(&pdet) == 0)
    {
        if(chardet_handle_data(pdet, htmlBody.c_str(), dataSize) == 0 && chardet_data_end(pdet) == 0 )
        {
            chardet_get_charset(pdet, charset, CHARSET_MAX);
            if( memcmp(charset, "UTF-8", 5) != 0 && strlen(charset) != 0 ) is_need_convert = 1;
        }
        chardet_destroy(pdet);
    }
    std::string resultPage;
    if (is_need_convert == 1)
    {
        if (airport::Utils::iconv_string(charset, "UTF-8", htmlBody.c_str(), htmlBody.size(), resultPage, 1, 512) < 0)
         {
             resultPage = htmlBody;
         }
    }else{
        resultPage = htmlBody;
    }
    
    std::string effURL;
    long responseCode;
    curlpp::infos::EffectiveUrl::get(request, effURL);
    curlpp::infos::ResponseCode::get(request, responseCode);
    url.set_effective_url(effURL);
    httpResponse.set_url(url);
    httpResponse.set_response_code(responseCode);
    httpResponse.set_html_body(resultPage);
    httpResponse.set_html_header(htmlHeader);
    //std::cout << "CONVERTED: \n" << httpResponse.get_html_body() << std::endl;
    /*
    double totalTime, speedDownload, sizeDownload;
    curlpp::infos::TotalTime::get(request, totalTime);
    curlpp::infos::SpeedDownload::get(request, speedDownload);
    curlpp::infos::SizeDownload::get(request, sizeDownload);
    std::cout << "CHARSET: " << charset << " CONVERT: " << is_need_convert << std::endl;
    std::cout << "Effective URL: " << effURL << std::endl;
    std::cout << "Response code: " << responseCode << std::endl;
    std::cout << "SpeedDownload: " << speedDownload << std::endl;
    std::cout << "TotalTime: " << totalTime << std::endl;
    std::cout << "SizeDownload: " << sizeDownload << std::endl;
    */
    
    /*
    std::cout << "Cookies from cookie engine:" << std::endl;
    std::list< std::string > cookies;
    curlpp::infos::CookieList::get(request, cookies);
    unsigned int i = 1;
    for (std::list< std::string >::const_iterator it = cookies.begin();
        it != cookies.end();
        ++it, i++)
    {
        std::cout << "[" << i << "]: " << MakeCookie(*it) << std::endl;
    }
    */
    return httpResponse;
}

std::vector<std::string> 
airport::HttpClient::read_cookie_jar (const char *cookie_path)
{
    std::string tcp(cookie_path);
    return read_cookie_jar(tcp);
}

std::vector<std::string>
airport::HttpClient::read_cookie_jar(std::string &cookie_path)
{
    std::string line;
    std::vector<std::string> cl;
    std::ifstream in(cookie_path.c_str());
    while(getline(in, line))
    {
        cl.push_back(line);
    }
    return cl;
}

void 
airport::HttpClient::set_cookie_path (const char *cookie_path)
{
    std::string tcp(cookie_path);
    set_cookie_path(tcp);
}

void 
airport::HttpClient::set_cookie_path (std::string &cookie_path)
{
    cookiePath = cookie_path;
}

void
airport::HttpClient::set_cookie_list (std::vector<std::string> &cookie_list)
{
    cookieList = cookie_list;
}

void
airport::HttpClient::reset_cookie()
{
    resetCookie = true;
}

void
airport::HttpClient::set_no_cookie(bool no_cookie)
{
    noCookie = no_cookie;
}

void 
airport::HttpClient::set_opt_connect_timeout(int connect_timeout)
{
    connectTimeout = connect_timeout;
}

void 
airport::HttpClient::set_opt_timeout(int to)
{
    timeout = to;
}

void 
airport::HttpClient::set_opt_max_redirs(int max_redirs)
{
    maxRedirs = max_redirs;
}

void 
airport::HttpClient::set_opt_timevalue(time_t timevalue)
{
    timeValue = timevalue;
}

void 
airport::HttpClient::set_user_info(boost::any &user_info)
{
    userInfo = user_info;
}

std::string 
airport::HttpClient::get_random_useragent()
{
    std::vector<std::string> agents;
    boost::split(agents, USER_AGENTS, boost::is_any_of(","));
    int randIt = airport::Utils::getRand(agents.size());
    return agents[randIt];
}

void 
airport::HttpClient::set_opt_useragent(const char *useragent)
{
    std::string ua(useragent);
    set_opt_useragent(ua);
}

void 
airport::HttpClient::set_opt_useragent(std::string &useragent)
{
    userAgent = useragent;
}

void 
airport::HttpClient::set_opt_nouseragent(bool nouseragent)
{
    noUserAgent = nouseragent;
}

void 
airport::HttpClient::add_opt_header(const char *header)
{
    std::string ua(header);
    add_opt_header(ua);
}

void 
airport::HttpClient::add_opt_header(std::string &header)
{
    headers.push_back(header);
}

void 
airport::HttpClient::set_opt_proxy_userpwd(const char *proxy_userpwd)
{
    std::string pu(proxy_userpwd);
    set_opt_proxy_userpwd(pu);
}

void 
airport::HttpClient::set_opt_proxy_userpwd(std::string &proxy_userpwd)
{
    proxyUserPwd = proxy_userpwd;
}

void
airport::HttpClient::set_opt_proxy_list(std::vector<std::string> &proxy_list)
{
    proxyList = proxy_list;
}

std::string 
airport::HttpClient::get_random_proxy()
{
    int randIt = airport::Utils::getRand(proxyList.size());
    return proxyList[randIt];
}

void
airport::HttpClient::reset_proxyList()
{
    proxyList.clear();
}

void 
airport::HttpClient::set_opt_postfield(std::pair<std::string, std::string> &postField)
{
    formParts.push_back(new curlpp::FormParts::Content(postField.first, postField.second));
}

void
airport::HttpClient::set_state (HttpClientState state)
{
    this->state = state;
 
    //state_changed (this, state);
}

std::map<std::string, std::string>
airport::HttpClient::parse_headers(std::string &header)
{
    std::map<std::string, std::string> headers;
    std::string line;
    std::istringstream in(header);
    while(std::getline(in, line))
    {
        std::vector<std::string> strs;
        boost::split(strs, line, boost::is_any_of(":"));
        if (strs.size()>1)
        {
            boost::trim(strs[1]);
            headers.insert(std::map<std::string, std::string>::value_type(strs[0], strs[1]));
        }
    }
    //boost::split(lines, header, boost::is_any_of("="));
    return headers;
}