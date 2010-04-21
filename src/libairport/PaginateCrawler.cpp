#include "PaginateCrawler.h"
#include "BasicCrawler.h"

#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>


airport::PaginateCrawler::PaginateCrawler(const char *urlCString):startNumber(1), pageStep(DEFAULT_PAGINATE_PAGE_STEP), replaceMarker(DEFAULT_PAGINATE_REPLACE_MARKER), maxThreads(DEFAULT_PAGINATE_MAX_THREADS)
{
    set_start_url(urlCString);
}

airport::PaginateCrawler::PaginateCrawler(std::string &urlString):startNumber(1), pageStep(DEFAULT_PAGINATE_PAGE_STEP), replaceMarker(DEFAULT_PAGINATE_REPLACE_MARKER), maxThreads(DEFAULT_PAGINATE_MAX_THREADS)
{
    set_start_url(urlString);
}

airport::PaginateCrawler::PaginateCrawler():startNumber(1), pageStep(DEFAULT_PAGINATE_PAGE_STEP), replaceMarker(DEFAULT_PAGINATE_REPLACE_MARKER), maxThreads(DEFAULT_PAGINATE_MAX_THREADS)
{
    
}

airport::PaginateCrawler::~PaginateCrawler()
{
    
}

void
airport::PaginateCrawler::set_start_url(const char *start_url)
{
    std::string tu(start_url);
    set_start_url(tu);
}

void
airport::PaginateCrawler::set_start_url(std::string &start_url)
{
    startUrl = start_url;
}

void
airport::PaginateCrawler::set_replace_marker(const char *replace_marker)
{
    std::string rmk(replace_marker);
    set_replace_marker(rmk);
}

void
airport::PaginateCrawler::set_replace_marker(std::string &replace_marker)
{
    replaceMarker = replace_marker;
}

void 
airport::PaginateCrawler::set_max_threads(unsigned int max_threads)
{
    maxThreads = max_threads;
}

void 
airport::PaginateCrawler::set_http_client(airport::HttpClient &httpClient)
{
    this->httpClient = httpClient;
}
void 
airport::PaginateCrawler::set_start_number(int n)
{
    startNumber = n;
}

void 
airport::PaginateCrawler::set_end_number(int n)
{
    endNumber = n;
}

void 
airport::PaginateCrawler::set_page_step(int n)
{
    pageStep = n;
}

void 
airport::PaginateCrawler::add_parser(const char *key, const char *regxString)
{
    std::string tkey(key);
    std::string tRegxString(regxString);
    add_parser(tkey, tRegxString);
}

void 
airport::PaginateCrawler::add_parser(std::string &key, std::string &regxString)
{
    this->parser.insert(std::map<std::string, std::string>::value_type(key, regxString));
}

void 
airport::PaginateCrawler::set_parser(std::map<std::string, std::string> &parser)
{
    this->parser = parser;
}

void 
airport::PaginateCrawler::reset_parser()
{
    parser.clear();
}

void 
airport::PaginateCrawler::add_staticForm_data(std::string &key, std::string &value)
{
    staticFormData.insert(std::map<std::string, std::string>::value_type(key, value));
}

void 
airport::PaginateCrawler::add_staticForm_data(const char *key, const char *value)
{
    std::string tkey(key);
    std::string tValue(value);
    add_staticForm_data(tkey, tValue);
}
void 
airport::PaginateCrawler::set_staticForm_data(std::map<std::string, std::string> &data)
{
    staticFormData = data;
}

void 
airport::PaginateCrawler::add_firstPageForm_data(std::string &key, std::string &value)
{
    firstPageFormData.insert(std::map<std::string, std::string>::value_type(key, value));
}

void 
airport::PaginateCrawler::add_firstPageForm_data(const char *key, const char *value)
{
    std::string tkey(key);
    std::string tValue(value);
    add_firstPageForm_data(tkey, tValue);
}
void 
airport::PaginateCrawler::set_firstPageForm_data(std::map<std::string, std::string> &data)
{
    firstPageFormData = data;
}

void 
airport::PaginateCrawler::add_inpageForm_data(std::string &key, std::string &value)
{
    inpageFormData.insert(std::map<std::string, std::string>::value_type(key, value));
}

void 
airport::PaginateCrawler::add_inpageForm_data(const char *key, const char *value)
{
    std::string tkey(key);
    std::string tValue(value);
    add_inpageForm_data(tkey, tValue);
}
void 
airport::PaginateCrawler::set_inpageForm_data(std::map<std::string, std::string> &data)
{
    inpageFormData = data;
}

void 
airport::PaginateCrawler::add_autoForm_data(std::string &key, const int value)
{
    autoFormData.insert(std::map<std::string, int>::value_type(key, value));
}

void 
airport::PaginateCrawler::add_autoForm_data(const char *key, const int value)
{
    std::string tkey(key);
    add_autoForm_data(tkey, value);
}
void 
airport::PaginateCrawler::set_autoForm_data(std::map<std::string, int> &data)
{
    autoFormData = data;
}

void 
airport::PaginateCrawler::add_user_dict(const char *key, const char *value)
{
    std::string tkey(key);
    std::string tValue(value);
    add_user_dict(tkey, tValue);
}

void 
airport::PaginateCrawler::add_user_dict(std::string &key, std::string &value)
{
    this->userDict.insert(std::map<std::string, std::string>::value_type(key, value));
}

void 
airport::PaginateCrawler::set_user_dict(std::map<std::string, std::string> &user_dict)
{
    this->userDict = user_dict;
}

void 
airport::PaginateCrawler::add_essential_field(const char *field)
{
    std::string tf(field);
    add_essential_field(tf);
}

void 
airport::PaginateCrawler::add_essential_field(std::string &field)
{
    essentialFields.push_back(field);
}

void 
airport::PaginateCrawler::set_essential_fields(std::vector<std::string> &fields)
{
    essentialFields = fields;
}

template <typename T>
void 
airport::PaginateCrawler::add_observer(std::string &key, T t, std::string &module)
{
    std::pair<function_type, std::string> p(function_type(t), module);
    observers.insert( std::map<std::string, std::pair<function_type, std::string> >:: value_type(key, p) );
}

std::vector<airport::HttpResponse> 
airport::PaginateCrawler::start()
{
    if (!staticFormData.empty())
    {
        std::vector<airport::HttpResponse> httpResponse;
        std::map<std::string, std::string> pageForm;
        for (int i=startNumber; i<=endNumber*pageStep; i+=pageStep)
        {
            airport::BasicCrawler basicCrawler;
            airport::HttpClient hc = httpClient;
            std::map<std::string, std::string>::iterator it;
            for (it=parser.begin(); it!=parser.end(); ++it)
            {
                std::string key(it->first);
                std::string value(it->second);
                basicCrawler.add_parser(key, value);
            }
            if (i == startNumber)
            {
                if (!firstPageFormData.empty())
                {
                    for(std::map<std::string, std::string>::iterator it=firstPageFormData.begin();it!=firstPageFormData.end();++it)
                    {
                        std::string k = it->first;
                        std::string v = it->second;
                        std::pair<std::string, std::string> p(k, v);
                        hc.set_opt_postfield(p);
                    }
                }
            }else{
                if (!staticFormData.empty())
                {
                    for(std::map<std::string, std::string>::iterator it=staticFormData.begin();it!=staticFormData.end();++it)
                    {
                        std::string k = it->first;
                        std::string v = it->second;
                        std::pair<std::string, std::string> p(k, v);
                        hc.set_opt_postfield(p);
                    }
                }
                if (!pageForm.empty())
                {
                    for(std::map<std::string, std::string>::iterator it=pageForm.begin();it!=pageForm.end();++it)
                    {
                        std::string k = it->first;
                        std::string v = it->second;
                        std::pair<std::string, std::string> p(k, v);
                        hc.set_opt_postfield(p);
                    }
                }
                for(std::map<std::string, int>::iterator it=autoFormData.begin();it!=autoFormData.end();++it)
                {
                    int page = it->second + i-pageStep;
                    std::string k = it->first;
                    std::string v = boost::lexical_cast<std::string>(page);
                    std::pair<std::string, std::string> p(k, v);
                    hc.set_opt_postfield(p);
                }
            }
            basicCrawler.add_url(startUrl);
            basicCrawler.set_http_client(hc);
            basicCrawler.set_max_threads(maxThreads);
            basicCrawler.set_essential_fields(essentialFields);
            basicCrawler.set_user_dict(userDict);
            if (!observers.empty())
            {
                basicCrawler.set_observers(observers);
            }
            std::vector <airport::HttpResponse> hr = basicCrawler.start();
            if (hr.empty())
                continue;
            httpResponse.push_back(hr[0]);
            if (!inpageFormData.empty())
            {
                std::string html = hr[0].get_html_body();
                pageForm = parse(html, inpageFormData);
            }
        }
        return httpResponse;
    }
    
    size_t found = startUrl.find(replaceMarker);
    if (found==std::string::npos)
        return std::vector <airport::HttpResponse>();
    int markerSize = replaceMarker.size();
    airport::BasicCrawler basicCrawler;
    std::map<std::string, std::string>::iterator it;
    for (it=parser.begin(); it!=parser.end(); ++it)
    {
        std::string key(it->first);
        std::string value(it->second);
        basicCrawler.add_parser(key, value);
    }
    basicCrawler.set_http_client(httpClient);
    basicCrawler.set_max_threads(maxThreads);
    basicCrawler.set_essential_fields(essentialFields);
    basicCrawler.set_user_dict(userDict);
    if (!observers.empty())
    {
        basicCrawler.set_observers(observers);
    }
    for (int i=startNumber; i<=endNumber*pageStep; i+=pageStep)
    {
        std::string urlStr = startUrl;
        std::string replaceStr = boost::lexical_cast<std::string>(i);
        urlStr.replace(found, markerSize, replaceStr);
        //std::cout << "U: " << markerSize << ", S: " << urlStr << std::endl;
        basicCrawler.add_url(urlStr);
    }
    std::vector <airport::HttpResponse> httpResponse = basicCrawler.start();
    return httpResponse;
}

std::map<std::string, std::string>
airport::PaginateCrawler::parse(std::string &htmlBody, std::map<std::string, std::string> &formData)
{
    std::map<std::string, std::string> response;
    std::map<std::string, std::string>::iterator it;
    for (it=formData.begin(); it!=formData.end(); ++it)
    {
        boost::regex reg(it->second);
        boost::smatch m;
        if (boost::regex_search(htmlBody, m, reg)) {
            if (m[1].matched)
            {
                std::string matchedStr = m[1].str();
                std::string key = it->first;
                response.insert(std::map<std::string, std::string>::value_type(key, matchedStr) );
            }
        }
    }
    return response;
}