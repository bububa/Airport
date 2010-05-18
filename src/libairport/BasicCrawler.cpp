#include "BasicCrawler.h"
#include "Config.h"
#include "Utils.h"

#include <boost/bind.hpp>
#include <boost/threadpool.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/regex.hpp>
#include <json/json.h>

#define CURLPP_ALLOW_NOT_AVAILABLE
#include <curlpp/curlpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>
#include <curlpp/Infos.hpp>


#define DEFAULT_CHUNK_SIZE 50
boost::mutex m_io_monitor2;

template<class R>
class callback_task
{
    typedef boost::function<void (R)> callback;
    typedef boost::function<R ()> function;

private:
    callback c_;
    function f_;

public:
    //F: 任务执行函数 C：结果回调函数
    template<class F,class C>
    callback_task(F f,C c)
    {
        f_ = f;
        c_ = c;
    }

    void operator()()
    {
        c_(f_());
    }
};

template<typename Thp, typename Func>
boost::shared_future< typename boost::result_of<Func()>::type >
submit_job(Thp& thp, Func f)
{
  typedef typename boost::result_of<Func()>::type result;
  typedef boost::packaged_task<result> packaged_task;
  typedef boost::shared_ptr<boost::packaged_task<result> > packaged_task_ptr;
 
  packaged_task_ptr task(new packaged_task(f));
  boost::shared_future<result> res(task->get_future());
  boost::threadpool::schedule(thp, boost::bind(&packaged_task::operator(), task));
  return res;
}

airport::BasicCrawler::BasicCrawler(const char *urlCString):maxThreads(DEFAULT_CRAWLER_MAX_THREADS)
{
    add_url(urlCString);
}

airport::BasicCrawler::BasicCrawler(std::string &urlString):maxThreads(DEFAULT_CRAWLER_MAX_THREADS)
{
    add_url(urlString);
}

airport::BasicCrawler::BasicCrawler(airport::Url &url):maxThreads(DEFAULT_CRAWLER_MAX_THREADS)
{
    add_url(url);
}

airport::BasicCrawler::BasicCrawler():maxThreads(DEFAULT_CRAWLER_MAX_THREADS)
{
    
}

airport::BasicCrawler::~BasicCrawler()
{
    
}

void 
airport::BasicCrawler::set_node_name (const char *node_name)
{
    std::string tn(node_name);
    set_node_name(tn);
}

void 
airport::BasicCrawler::set_node_name (std::string &node_name)
{
    nodename = node_name;
}

void 
airport::BasicCrawler::add_url(const char *urlCString)
{
    airport::Url url(urlCString);
    add_url(url);
}

void 
airport::BasicCrawler::add_url(std::string &urlString)
{
    airport::Url url(urlString);
    add_url(url);
}

void 
airport::BasicCrawler::add_url(airport::Url &url)
{
    request.push_back(url);
}

void 
airport::BasicCrawler::reset_request()
{
    request.clear();
}

void 
airport::BasicCrawler::add_parser(const char *key, const char *regxString)
{
    std::string tkey(key);
    std::string tRegxString(regxString);
    add_parser(tkey, tRegxString);
}

void 
airport::BasicCrawler::add_parser(std::string &key, std::string &regxString)
{
    this->parser.insert(std::map<std::string, std::string>::value_type(key, regxString));
}

void 
airport::BasicCrawler::set_parser(std::map<std::string, std::string> &parser)
{
    this->parser = parser;
}

void 
airport::BasicCrawler::reset_parser()
{
    parser.clear();
}

void 
airport::BasicCrawler::add_user_dict(const char *key, const char *value)
{
    std::string tkey(key);
    std::string tValue(value);
    add_user_dict(tkey, tValue);
}

void 
airport::BasicCrawler::add_user_dict(std::string &key, std::string &value)
{
    this->userDict.insert(std::map<std::string, std::string>::value_type(key, value));
}

void 
airport::BasicCrawler::set_user_dict(std::map<std::string, std::string> &user_dict)
{
    this->userDict = user_dict;
}

void 
airport::BasicCrawler::add_essential_field(const char *field)
{
    std::string tf(field);
    add_essential_field(tf);
}

void 
airport::BasicCrawler::add_essential_field(std::string &field)
{
    essentialFields.push_back(field);
}

void 
airport::BasicCrawler::set_essential_fields(std::vector<std::string> &fields)
{
    essentialFields = fields;
}

void 
airport::BasicCrawler::add_black_keyword(const char *keyword)
{
    std::string tf(keyword);
    add_black_keyword(tf);
}

void 
airport::BasicCrawler::add_black_keyword(std::string &keyword)
{
    blackKeywords.push_back(keyword);
}

void 
airport::BasicCrawler::set_black_keywords(std::vector<std::string> &keywords)
{
    blackKeywords = keywords;
}

void 
airport::BasicCrawler::set_max_threads(unsigned int max_threads)
{
    maxThreads = max_threads;
}

void 
airport::BasicCrawler::set_user_info(std::map<std::string, std::string> &user_info)
{
    userInfo = user_info;
}

void 
airport::BasicCrawler::set_http_client(airport::HttpClient &httpClient)
{
    this->httpClient = httpClient;
}

template <typename T> 
void 
airport::BasicCrawler::add_observer(std::string &key, T t, std::string &module)
{
    std::pair<function_type, std::string> p (function_type(t), module);
    observers.insert( std::map<std::string, std::map<std::string, std::pair<function_type, std::string> > > :: value_type(key, p) );
}

void 
airport::BasicCrawler::set_observers(std::map<std::string, std::pair<function_type, std::string> > &observers)
{
    this->observers = observers;
}

std::vector<airport::HttpResponse> 
airport::BasicCrawler::start()
{
    std::vector <airport::HttpResponse> httpResponse;
    if (request.empty())
        return httpResponse;
    httpClient.set_node_name(nodename);
    httpClient.set_user_info(userInfo);
    if (request.size() == 1 || maxThreads==0)
    {
        airport::HttpResponse res = fetch(request[0]);
        if (res.get_html_body().size()>0)
        {
            if (parser.size()>0)
            {
                std::string htmlBody = res.get_html_body();
                std::string parsedData = parse(htmlBody);
                if (!essentialFields.empty() && parsedData.empty())
                    return httpResponse;
                res.set_parsed_data(parsedData);
            }
            httpResponse.push_back(res);
        }
    }else{
        unsigned int urls_count = request.size();
        unsigned int chunks = urls_count/DEFAULT_CHUNK_SIZE;
        for(unsigned int i=0; i<=chunks; i++)
        {
            std::vector<airport::Url> urls;
            for (unsigned int ii=0; ii<DEFAULT_CHUNK_SIZE; ii++)
            {
                unsigned int m = i*DEFAULT_CHUNK_SIZE+ii;
                if (m < urls_count)
                {
                    urls.push_back(request[m]);
                }
            }
            std::vector<airport::HttpResponse> res = chunk_fetch(urls);
            for (std::vector<airport::HttpResponse>::iterator it=res.begin(); it!=res.end(); ++it)
            {
                //airport::HttpResponse hr = it->get();
                httpResponse.push_back(*it);
            }
        }
    }
    /*
    std::cout << "Finished: " << httpResponse.size() << std::endl;
    for (std::vector<airport::HttpResponse>::iterator it=httpResponse.begin(); it!=httpResponse.end(); ++it)
    {
        std::cout << "===========================\n" << it->get_parsed_data() << std::endl;
    }
    */
    return httpResponse;
}

airport::HttpResponse 
airport::BasicCrawler::fetch(airport::Url &url)
{
    int retry = DEFAULT_CRAWLER_RETRY;
    while(true)
    {
        airport::HttpClient hc = httpClient;
        airport::HttpResponse httpResponse = hc.get(url);
        if (hc.get_state() == STATE_COMPLETED)
            return httpResponse;
        else
            retry --;
        if (retry <=0) return httpResponse;
        int randWait = airport::Utils::getRand(2*1000*1000);
        airport::Utils::sleep((clock_t)randWait);
    }
}

std::vector<airport::HttpResponse>
airport::BasicCrawler::chunk_fetch(std::vector<airport::Url> &urls)
{
    std::vector<airport::HttpResponse> httpResponse;
    boost::threadpool::pool tp(maxThreads);
    unsigned int urls_count = urls.size();
    boost::shared_future <airport::HttpResponse> res[urls_count];
    unsigned int i = 0;
    for (std::vector<airport::Url>::iterator it=urls.begin(); it!=urls.end(); ++it) 
    {
        airport::Url tu = *it;
        res[i] = submit_job(tp, boost::bind(&airport::BasicCrawler::fetch, this, tu));
        //tp.schedule(boost::bind(&airport::HttpClient::get, httpClient, tu));
        i ++;
    }
    
    tp.wait();
    
    if (!parser.empty())
    {
        boost::shared_future <std::string> pres[urls_count];
        for(unsigned int i=0; i<urls_count; i++)
        {
            airport::HttpResponse hr = res[i].get();
            std::string htmlBody = hr.get_html_body();
            if (!htmlBody.empty())
                pres[i] = submit_job(tp, boost::bind(&airport::BasicCrawler::parse, this, htmlBody));
        }
        tp.wait();
        for(unsigned int i=0; i<urls_count; i++)
        {
            std::string parsedData = pres[i].get();
            if (!essentialFields.empty() && parsedData.empty())
                continue;
             airport::HttpResponse hr = res[i].get();
             hr.set_parsed_data(parsedData);
             httpResponse.push_back(hr);
        }
        return httpResponse;
    }
    
    for(unsigned int i=0; i<urls_count; i++)
    {
        airport::HttpResponse hr = res[i].get();
        
        if(!hr.get_html_body().empty())
        {
            httpResponse.push_back(hr);
        }
    }
    return httpResponse;
}

std::string 
airport::BasicCrawler::parse(std::string &htmlBody)
{
    Json::Value root;
    std::map<std::string, std::string>::iterator it;
    for (it=parser.begin(); it!=parser.end(); ++it)
    {
        boost::regex reg(it->second);
        boost::smatch m;
        if (boost::regex_search(htmlBody, m, reg)) {
            if (m[1].matched)
            {
                std::string matchedStr = m[1].str();
                std::string key = it->first;
                root[key] = matchedStr;
            }else{
                root[it->first] = std::string("");
            }
        }
    }
    if (!userDict.empty())
    {
        std::map<std::string, std::string>::iterator uit;
        for(uit=userDict.begin(); uit!=userDict.end(); ++ it)
        {
            root[uit->first] = uit->second;
        }
    }
    
    if (!essentialFields.empty() || !observers.empty())
    {
        Json::Value::Members members = root.getMemberNames();
        Json::Value::Members::iterator jit;
        for (jit=members.begin();jit!=members.end();++jit)
        {
            std::string key = (*jit);
            std::string value = root[key].asString();
            if (!essentialFields.empty() && std::find(essentialFields.begin(), essentialFields.end(), key) != essentialFields.end() && value.empty())
            {
                if (airport::DEBUG_LEVEL > airport::DEBUG_OFF)
                {
                    std::cout << "NEED FIELD: " << key << std::endl;
                }
                return std::string("");
            }
            std::map<std::string, std::pair<function_type, std::string> >::iterator iter = observers.find(key);
            if (!observers.empty() && iter != observers.end())
            {
                std::pair<function_type, std::string> func = iter->second;
                value = func.first(func.second, value);
                root[key] = value;
            }
        }
    }
    
    if (!blackKeywords.empty())
    {
        Json::Value::Members members = root.getMemberNames();
        Json::Value::Members::iterator jit;
        for (jit=members.begin();jit!=members.end();++jit)
        {
            std::string key = (*jit);
            std::string value = root[key].asString();
            for (std::vector<std::string>::iterator bit=blackKeywords.begin();bit!=blackKeywords.end();++bit)
            {
                size_t found = value.find( (*bit) );
                if (found!=std::string::npos)
                {
                    if (airport::DEBUG_LEVEL > airport::DEBUG_OFF)
                    {
                        std::cout << "FOUND BLACK WORDS: " << (*bit) << std::endl;
                    }
                    return std::string("");
                }
            }
        }
    }
    
    Json::FastWriter writer;
    std::string jsonres = writer.write(root);
    return jsonres;
}