#include "Rule.h"
#include "Config.h"
#include "Utils.h"
#include "Mongo.h"
#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/function.hpp>
#include <boost/regex.hpp>
#include <boost/threadpool.hpp>
#include <boost/lexical_cast.hpp>
#include <fstream>
#include <sstream>

boost::mutex mr_io_monitor;

airport::Rule::Rule()
{
    
}

airport::Rule::~Rule()
{
    
}

std::string 
airport::Rule::operator()(std::string &module, std::string request)
{
    std::string p = "modules." + module;
    std::string modName = pt.get<std::string>(p);
    std::string response = boost::any_cast<std::string>(element(modName, p, module, request));
    return response;
}

void 
airport::Rule::set_node_name(const char *nodename)
{
    std::string tn(nodename);
    set_node_name(tn);
}

void 
airport::Rule::set_node_name(std::string &nodename)
{
    this->nodename = nodename;
}

std::string 
airport::Rule::readinfo(const char *filename)
{
    std::string tf(filename);
    return readinfo(tf);
}

std::string 
airport::Rule::readinfo(std::string &filename)
{
    std::string info;
    //boost::filesystem::path cpath=boost::filesystem::current_path();
    boost::filesystem::path info_path( filename );
    if(!boost::filesystem::exists(info_path)) 
    {
        std::cout << info_path.file_string() << std::endl;
        return info;
    }
    std::ifstream infofs(info_path.file_string().c_str());
    std::ostringstream tmp; 
    tmp << infofs.rdbuf();
    info = tmp.str();
    //infofs.rdbuf() >> info;
    /*while(   infofs >> info   )
    {
        std::cout   <<   info   <<   std::endl;
    }*/
    infofs.close();
    return info;
}

bool 
airport::Rule::runable(const char *filename)
{
    std::string tf(filename);
    return runable(tf);
}

bool 
airport::Rule::runable(std::string &filename)
{
    boost::property_tree::read_info(filename, pt);
    time_t duration = pt.get<time_t>("duration", 0);
    time_t updated_time = pt.get<time_t>("update_time", time(NULL));
    std::string starter;
    try
    {
        starter = pt.get<std::string>("starter");
    }catch(boost::property_tree::ptree_bad_path){ 
        return false;
    }
    std::string p = "modules." + starter;
    std::string modName = pt.get<std::string>(p);
    return runable(modName, duration, updated_time);
}

void
airport::Rule::parse(const char *filename)
{
    std::string tf(filename);
    return parse(tf);
}

void
airport::Rule::parse(std::string &filename)
{
    boost::property_tree::read_info(filename, pt);
    BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                pt.get_child("modules"))
    {
        if (v.first[0] == '_')
        {
            std::string modName = v.second.data();
            std::string p = "modules." + v.first;
            boost::any value = false;
            staticModules.insert(std::map<std::string, boost::any>::value_type(v.first, value) );
        }
    }
    time_t duration = pt.get<time_t>("duration", 0);
    time_t updated_time = pt.get<time_t>("update_time", time(NULL));
    std::string starter;
    try
    {
        starter = pt.get<std::string>("starter");
    }catch(boost::property_tree::ptree_bad_path){
        return;
    }
    
    std::string p = "modules." + starter;
    std::string modName = pt.get<std::string>(p);
    
    if (runable(modName, duration, updated_time))
    {
        boost::any response = element(modName, p, starter);
        update_rule(filename, modName, response, duration, p);
        if (airport::DEBUG_LEVEL > airport::DEBUG_OFF && modName == "ListCrawler")
        {
            std::pair<airport::ListCrawler, std::vector<airport::HttpResponse> > res = boost::any_cast< std::pair<airport::ListCrawler, std::vector<airport::HttpResponse> > > (response);
            for (std::vector<airport::HttpResponse>::iterator it=res.second.begin();it!=res.second.end();++it)
            {
                showResult((*it));
            }
        }
    }
}

void 
airport::Rule::showResult(airport::HttpResponse &hr)
{
    airport::Url url = hr.get_url();
    std::cout << "URL: " << url.get_raw_url() << std::endl;
    std::cout << "PARSED DATA" << std::endl;
    std::string js = hr.get_raw_parsed_data();
    if(!js.empty())
    {
        std::map<std::string, std::string> d = airport::Utils::jsonDecode(js);
        if (d.empty())
        {
            std::cout << hr.get_parsed_data() << std::endl;
        }else{
            for(std::map<std::string, std::string>::iterator it=d.begin();it!=d.end();++it)
            {
                std::cout << it->first << ": " << it->second << std::endl;
            }
        }
    }else{
        std::cout << hr.get_parsed_data() << std::endl;
    }
    std::cout << "===============================\n" << std::endl;
}

bool
airport::Rule::runable(std::string &module, time_t duration, time_t updated_time)
{
    if (module == "ListCrawler" && (updated_time + duration) > time(NULL))
    {
        return false;
    }else if (module == "FeedCrawler" && (updated_time + duration) > time(NULL)) {
        return false;
    }
    return true;
}

void
airport::Rule::update_rule(std::string &filename, std::string &module, boost::any &response, time_t duration, std::string &path)
{
    if (module == "ListCrawler")
    {
        bool saveInMongo = pt.get<bool>(path + ".save_in_mongo", true);
        if (!saveInMongo) return;
        std::pair< airport::ListCrawler, std::vector<airport::HttpResponse> > res = boost::any_cast< std::pair< airport::ListCrawler, std::vector<airport::HttpResponse> > >(response);
        airport::ListCrawler lc = res.first;
        int endNumber;
        if (lc.get_last_page_has_results() > 0)
        {
            endNumber = pt.get<int>(path + ".end_number");
            endNumber ++;
            duration = (duration-600) > 600 ? (duration-600) : 600;
        }else{
            endNumber = lc.get_result_pages() > 3 ? lc.get_result_pages() : 3;
            duration += 600;
        }
        pt.put(path + ".end_number", endNumber);
        pt.put("update_time", time(NULL));
        pt.put("duration", duration);
        boost::property_tree::write_info(filename, pt);
    }else if (module == "FeedCrawler") {
        bool saveInMongo = pt.get<bool>(path + ".save_in_mongo", true);
        if (!saveInMongo) return;
        std::pair< airport::FeedCrawler, std::pair<airport::Feed, std::vector<airport::FeedEntry> > > res = boost::any_cast< std::pair< airport::FeedCrawler, std::pair<airport::Feed, std::vector<airport::FeedEntry> > > >(response);
        airport::FeedCrawler fc = res.first;std::cout << fc.get_new_results() << std::endl;
        if (fc.get_new_results() > 0)
        {
            duration = (duration-600) > 600 ? (duration-600) : 600;
        }else{
            duration += 600;
        }
        pt.put("update_time", time(NULL));
        pt.put("duration", duration);
        boost::property_tree::write_info(filename, pt);
    }
}

boost::any
airport::Rule::element(std::string &module, std::string &path, std::string &name, const boost::any &request)
{
    boost::any response;
    if (airport::DEBUG_LEVEL > airport::DEBUG_OFF)
    {
        boost::mutex::scoped_lock lock(mr_io_monitor);
        std::string req;
        try
        {
            req = boost::any_cast<std::string>(request);
        }catch(boost::bad_any_cast const &){}
        std::cout << "MODULE: " << module << " PATH: " << path << " REQ: "<< req << std::endl;
    }
    bool staticAvailable = false;
    if (!staticModules.empty())
    {
        try
        {
            staticAvailable = boost::any_cast<bool>(staticModules[name]);
        }catch(const boost::bad_any_cast &){
            staticAvailable = true;
        }
    }
    if (name[0]=='_' && staticAvailable)
    {
        response = staticModules[name];
    }else if (module == "HttpClient") {
        response = httpClient(path);
    }else if (module == "BasicCrawler") {
        response = basicCrawler(path, boost::any_cast<std::string>(request));
    }else if (module == "PaginateCrawler") {
        response = paginateCrawler(path);
    }else if (module == "ListCrawler") {
        response = listCrawler(path);
    }else if (module == "FeedCrawler") {
        response = feedCrawler(path);
    }else if (module == "List") {
        response = List(path, boost::any_cast<std::string>(request));
    }else if (module == "Dict") {
        response = Dict(path);
    }else if (module == "ListMember") {
        response = ListMember(path, boost::any_cast<std::string>(request));
    }else if (module == "DictMember") {
        response = DictMember(path, boost::any_cast<std::string>(request));
    }else if (module == "HttpResponse") {
        response = httpResponse(path, boost::any_cast<std::string>(request));
    }else if (module == "JsonDecode") {
        response = JsonDecode(path, boost::any_cast<std::string>(request));
    }else if (module == "Regex") {
        response = Regex(path, boost::any_cast<std::string>(request));
    }else if (module == "StringJoin") {
        response = StringJoin(path, boost::any_cast<std::string>(request));
    }else if (module == "Tidy") {
        response = Tidy(path, boost::any_cast<std::string>(request));
    }else if (module == "FileToList") {
        response = FileToList(path, boost::any_cast<std::string>(request));
    }else if (module == "FileToDict") {
        response = FileToDict(path, boost::any_cast<std::string>(request));
    }else if (module == "StripTags") {
        response = StripTags(path, boost::any_cast<std::string>(request));
    }else if (module == "HtmlSanitize") {
        response = HtmlSanitize(path, boost::any_cast<std::string>(request));
    }else if (module == "MongoKeywords") {
        response = MongoKeywords(path, boost::any_cast<std::string>(request));
    }
    if (name[0] == '_')
        staticModules[name] = response;
    return response;
}

airport::HttpClient
airport::Rule::httpClient(std::string &path)
{
    airport::HttpClient hc;
    hc.set_node_name(nodename);
    /* SET NOCOOKIE */
    bool noCookie = pt.get(path + ".no_cookie", true);
    hc.set_no_cookie(noCookie);
    /* SET USER_AGENT */
    try
    {
        std::string userAgent = pt.get<std::string>(path + ".user_agent");
        if (!userAgent.empty())
            hc.set_opt_useragent(userAgent);
    }catch(boost::property_tree::ptree_bad_path){ }
    /* SET CONNECTION_TIMEOUT */
    try
    {
        int ct = pt.get<int>(path + ".connection_timeout");
        hc.set_opt_connect_timeout(ct);
    }catch(boost::property_tree::ptree_bad_path){ }
    /* SET TIMEOUT */
    try
    {
        int to = pt.get<int>(path + ".timeout");
        hc.set_opt_timeout(to);
    }catch(boost::property_tree::ptree_bad_path){ }
    /* SET MAX_REDIRS */
    try
    {
        int mr = pt.get<int>(path + ".max_redirs");
        hc.set_opt_timeout(mr);
    }catch(boost::property_tree::ptree_bad_path){ }
    /* SET TIMEVALUE */
    try
    {
        int tv = pt.get<int>(path + ".time_value");
        hc.set_opt_timevalue((time_t)tv);
    }catch(boost::property_tree::ptree_bad_path){ }
    /* SET NO_USERAGENT */
    bool no_useragent = pt.get<bool>(path + ".no_useragent", true);
    hc.set_opt_nouseragent(no_useragent);
    /* SET PROXY_LIST */
    std::string proxyListModule = pt.get<std::string>(path + ".proxy_list", "");
    if (!proxyListModule.empty())
    {
        std::string p = "modules." + proxyListModule;
        std::string modName = pt.get<std::string>(p);
        std::vector<std::string> proxyList = boost::any_cast< std::vector<std::string> >( element(modName, p, proxyListModule) );
        hc.set_opt_proxy_list(proxyList);
    }else{
        try
        {
            std::vector<std::string> proxyList;
            BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                        pt.get_child(path + ".proxy_list"))
            {
                if (v.first == "item")
                    proxyList.push_back(v.second.data());
            }
            hc.set_opt_proxy_list(proxyList);
        }catch(boost::property_tree::ptree_bad_path){ }
    }
    /* SET POST_FIELD */
    std::string postFieldModule = pt.get<std::string>(path + ".post_field", "");
    if (!postFieldModule.empty())
    {
        std::string p = "modules." + postFieldModule;
        std::string modName = pt.get<std::string>(p);
        std::map<std::string, std::string> post_field = boost::any_cast< std::map<std::string, std::string> >( element(modName, p, postFieldModule) );
        std::map<std::string, std::string>::iterator it;
        for (it=post_field.begin(); it!=post_field.end(); ++it)
        {
            std::string key(it->first);
            std::string value(it->second);
            std::pair< std::string, std::string > pf(key, value);
            hc.set_opt_postfield(pf);
        }
    }else{
        try
        {
            BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                        pt.get_child(path + ".post_field"))
            {
                std::string key(v.first);
                std::string value(v.second.data());
                std::pair< std::string, std::string > pf(key, value);
                hc.set_opt_postfield(pf);
            }
        }catch(boost::property_tree::ptree_bad_path){ }
    }
    return hc;
}

std::pair< airport::PaginateCrawler, std::vector<airport::HttpResponse> >
airport::Rule::paginateCrawler(std::string &path)
{
    airport::PaginateCrawler pc;
    pc.set_node_name(nodename);
    try{
        std::string startUrl = pt.get<std::string>(path + ".starturl");
        pc.set_start_url(startUrl);
    }catch(boost::property_tree::ptree_bad_path){
        if (airport::DEBUG_LEVEL > airport::DEBUG_OFF)
        {
            boost::mutex::scoped_lock lock(mr_io_monitor);
            std::cout << "ERROR: need starturl!" << std::endl;
            //throw exception;
        }
    }
    
    std::string replaceMarker = pt.get<std::string>(path + ".replace_marker", "");
    if (!replaceMarker.empty())
        pc.set_replace_marker(replaceMarker);
    try
    {
        int startNumber = pt.get<int>(path + ".start_number");
        pc.set_start_number(startNumber);
    }catch(boost::property_tree::ptree_bad_path){ }
    try
    {
        int endNumber = pt.get<int>(path + ".end_number");
        pc.set_end_number(endNumber);
    }catch(boost::property_tree::ptree_bad_path){ }
    try
    {
        int page_step = pt.get<int>(path + ".page_step");
        pc.set_page_step(page_step);
    }catch(boost::property_tree::ptree_bad_path){ }
    /* SET ESSENTIAL FIELDS */
    std::string eFieldsModule = pt.get<std::string>(path + ".essential_fields", "");
    if (!eFieldsModule.empty())
    {
        std::string p = "modules." + eFieldsModule;
        std::string modName = pt.get<std::string>(p);
        std::vector<std::string> fields = boost::any_cast< std::vector<std::string> >( element(modName, p, eFieldsModule) );
        BOOST_FOREACH(std::string field, fields)
        {
            pc.add_essential_field(field);
        }
    }else{
        try
        {
            BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                        pt.get_child(path + ".essential_fields"))
            {
                pc.add_essential_field(v.second.data());
            }
        }catch(boost::property_tree::ptree_bad_path){ }
    }
    /* SET PARSERS */
    std::string parsersModule = pt.get<std::string>(path + ".parsers", "");
    if (!parsersModule.empty())
    {
        std::string p = "modules." + parsersModule;
        std::string modName = pt.get<std::string>(p);
        std::map<std::string, std::string> parsers = boost::any_cast< std::map<std::string, std::string> >( element(modName, p, parsersModule) );
        std::map<std::string, std::string>::iterator it;
        for (it=parsers.begin(); it!=parsers.end(); ++it)
        {
            std::string key(it->first);
            std::string value(it->second);
            pc.add_parser(key, value);
        }
    }else{
        try
        {
            BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                        pt.get_child(path + ".parsers"))
            {
                std::string key(v.first);
                std::string value(v.second.data());
                pc.add_parser(key, value);
            }
        }catch(boost::property_tree::ptree_bad_path){ }
    }
    /* SET STATIC FORM DATA */
    try
    {
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                    pt.get_child(path + ".static_form"))
        {
            std::string key(v.first);
            std::string value(v.second.data());
            pc.add_staticForm_data(key, value);
        }
    }catch(boost::property_tree::ptree_bad_path){ }
    /* SET FIRST PAGE FORM DATA */
    try
    {
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                    pt.get_child(path + ".first_page_form"))
        {
            std::string key(v.first);
            std::string value(v.second.data());
            pc.add_firstPageForm_data(key, value);
        }
    }catch(boost::property_tree::ptree_bad_path){ }
    /* SET INPAGE FORM DATA */
    try
    {
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                    pt.get_child(path + ".inpage_form"))
        {
            std::string key(v.first);
            std::string value(v.second.data());
            pc.add_inpageForm_data(key, value);
        }
    }catch(boost::property_tree::ptree_bad_path){ }
    /* SET AUTO FORM DATA */
    try
    {
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                    pt.get_child(path + ".auto_form"))
        {
            std::string key(v.first);
            std::string value(v.second.data());
            pc.add_autoForm_data(key, boost::lexical_cast<int>(value));
        }
    }catch(boost::property_tree::ptree_bad_path){ }
    /* SET USERDICT */
    std::string userDictModule = pt.get<std::string>(path + ".user_dict", "");
    if (!userDictModule.empty())
    {
        std::string p = "modules." + userDictModule;
        std::string modName = pt.get<std::string>(p);
        std::map<std::string, std::string> user_dict = boost::any_cast< std::map<std::string, std::string> >( element(modName, p, userDictModule) );
        std::map<std::string, std::string>::iterator it;
        for (it=user_dict.begin(); it!=user_dict.end(); ++it)
        {
            std::string key(it->first);
            std::string value(it->second);
            pc.add_user_dict(key, value);
        }
    }else{
        try
        {
            BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                        pt.get_child(path + ".user_dict"))
            {
                std::string key(v.first);
                std::string value(v.second.data());
                pc.add_user_dict(key, value);
            }
        }catch(boost::property_tree::ptree_bad_path){ }
    }
    /* SET MAXTHREADS */
    int maxthreads = pt.get<int>(path + ".maxthreads", 0);
    if (maxthreads > 0)
    {
        pc.set_max_threads(maxthreads);
    }
    
    /* SET HTTPCLIENT */
    std::string httpClientModule = pt.get<std::string>(path + ".httpclient", "");
    if (!httpClientModule.empty())
    {
        std::string p = "modules." + httpClientModule;
        std::string modName = pt.get<std::string>(p);
        airport::HttpClient httpClient = boost::any_cast< airport::HttpClient >( element(modName, p, httpClientModule) );
        pc.set_http_client(httpClient);
    }
    std::vector<airport::HttpResponse> hr = pc.start();
    std::pair< airport::PaginateCrawler, std::vector<airport::HttpResponse> > rs(pc, hr);
    return rs;
}

std::pair< airport::ListCrawler, std::vector<airport::HttpResponse> >
airport::Rule::listCrawler(std::string &path)
{
    airport::ListCrawler lc;
    lc.set_node_name(nodename);
    try{
        std::string startUrl = pt.get<std::string>(path + ".starturl");
        lc.set_start_url(startUrl);
    }catch(boost::property_tree::ptree_bad_path){ 
        if (airport::DEBUG_LEVEL > airport::DEBUG_OFF)
        {
            boost::mutex::scoped_lock lock(mr_io_monitor);
            std::cout << "ERROR: need starturl" << std::endl;
            //throw exception;
        }
    }
    try{
        std::string urlPattern = pt.get<std::string>(path + ".url_pattern");
        lc.set_url_pattern(urlPattern);
    }catch(boost::property_tree::ptree_bad_path){
        if (airport::DEBUG_LEVEL > airport::DEBUG_OFF)
        {
            boost::mutex::scoped_lock lock(mr_io_monitor);
            std::cout << "ERROR: need url_pattern" << std::endl;
            //throw exception;
        }
    }
    std::string replaceMarker = pt.get<std::string>(path + ".replace_marker", "");
    if (!replaceMarker.empty())
        lc.set_replace_marker(replaceMarker);
    try{
        std::string linkRegex = pt.get<std::string>(path + ".link_regex");
        lc.set_link_regex(linkRegex);
    }catch(boost::property_tree::ptree_bad_path){}
    try
    {
        int startNumber = pt.get<int>(path + ".start_number");
        lc.set_start_number(startNumber);
    }catch(boost::property_tree::ptree_bad_path){ }
    try
    {
        int endNumber = pt.get<int>(path + ".end_number");
        lc.set_end_number(endNumber);
    }catch(boost::property_tree::ptree_bad_path){ }
    try
    {
        int page_step = pt.get<int>(path + ".page_step");
        lc.set_page_step(page_step);
    }catch(boost::property_tree::ptree_bad_path){ }
    try
    {
        bool saveInMongo = pt.get<bool>(path + ".save_in_mongo");
        lc.set_save_in_mongo(saveInMongo);
    }catch(boost::property_tree::ptree_bad_path){ }
    try
    {
        bool discardExternalDuplicate = pt.get<bool>(path + ".discard_external_duplicate");
        lc.set_discard_external_duplicate(discardExternalDuplicate);
    }catch(boost::property_tree::ptree_bad_path){ }
    /* SET ESSENTIAL FIELDS */
    std::string eFieldsModule = pt.get<std::string>(path + ".essential_fields", "");
    if (!eFieldsModule.empty())
    {
        std::string p = "modules." + eFieldsModule;
        std::string modName = pt.get<std::string>(p);
        std::vector<std::string> fields = boost::any_cast< std::vector<std::string> >( element(modName, p, eFieldsModule) );
        BOOST_FOREACH(std::string field, fields)
        {
            lc.add_essential_field(field);
        }
    }else{
        try
        {
            BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                        pt.get_child(path + ".essential_fields"))
            {
                lc.add_essential_field(v.second.data());
            }
        }catch(boost::property_tree::ptree_bad_path){ }
    }
    /* SET BLACK KEYWORDS */
    std::string bFieldsModule = pt.get<std::string>(path + ".black_keywords", "");
    if (!bFieldsModule.empty())
    {
        std::string p = "modules." + bFieldsModule;
        std::string modName = pt.get<std::string>(p);
        std::vector<std::string> keywords = boost::any_cast< std::vector<std::string> >( element(modName, p, bFieldsModule) );
        BOOST_FOREACH(std::string keyword, keywords)
        {
            lc.add_black_keyword(keyword);
        }
    }else{
        try
        {
            BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                        pt.get_child(path + ".black_keywords"))
            {
                lc.add_black_keyword(v.second.data());
            }
        }catch(boost::property_tree::ptree_bad_path){ }
    }
    /* SET PAGINATE PARSERS */
    std::string paginatearsersModule = pt.get<std::string>(path + ".paginate_parsers", "");
    if (!paginatearsersModule.empty())
    {
        std::string p = "modules." + paginatearsersModule;
        std::string modName = pt.get<std::string>(p);
        std::map<std::string, std::string> parsers = boost::any_cast< std::map<std::string, std::string> >( element(modName, p, paginatearsersModule) );
        std::map<std::string, std::string>::iterator it;
        for (it=parsers.begin(); it!=parsers.end(); ++it)
        {
            std::string key(it->first);
            std::string value(it->second);
            lc.add_paginate_parser(key, value);
        }
    }else{
        try
        {
            BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                        pt.get_child(path + ".paginate_parsers"))
            {
                std::string key(v.first);
                std::string value(v.second.data());
                lc.add_paginate_parser(key, value);
            }
        }catch(boost::property_tree::ptree_bad_path){ }
    }
    /* SET PARSERS */
    std::string parsersModule = pt.get<std::string>(path + ".parsers", "");
    if (!parsersModule.empty())
    {
        std::string p = "modules." + parsersModule;
        std::string modName = pt.get<std::string>(p);
        std::map<std::string, std::string> parsers = boost::any_cast< std::map<std::string, std::string> >( element(modName, p, parsersModule) );
        std::map<std::string, std::string>::iterator it;
        for (it=parsers.begin(); it!=parsers.end(); ++it)
        {
            std::string key(it->first);
            std::string value(it->second);
            lc.add_parser(key, value);
        }
    }else{
        try
        {
            BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                        pt.get_child(path + ".parsers"))
            {
                std::string key(v.first);
                std::string value(v.second.data());
                lc.add_parser(key, value);
            }
        }catch(boost::property_tree::ptree_bad_path){ }
    }
    /* SET STATIC FORM DATA */
    try
    {
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                    pt.get_child(path + ".static_form"))
        {
            std::string key(v.first);
            std::string value(v.second.data());
            lc.add_staticForm_data(key, value);
        }
    }catch(boost::property_tree::ptree_bad_path){ }
    /* SET FIRST PAGE FORM DATA */
    try
    {
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                    pt.get_child(path + ".first_page_form"))
        {
            std::string key(v.first);
            std::string value(v.second.data());
            lc.add_firstPageForm_data(key, value);
        }
    }catch(boost::property_tree::ptree_bad_path){ }
    /* SET INPAGE FORM DATA */
    try
    {
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                    pt.get_child(path + ".inpage_form"))
        {
            std::string key(v.first);
            std::string value(v.second.data());
            lc.add_inpageForm_data(key, value);
        }
    }catch(boost::property_tree::ptree_bad_path){ }
    /* SET AUTO FORM DATA */
    try
    {
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                    pt.get_child(path + ".auto_form"))
        {
            std::string key(v.first);
            std::string value(v.second.data());
            lc.add_autoForm_data(key, boost::lexical_cast<int>(value));
        }
    }catch(boost::property_tree::ptree_bad_path){ }
    /* SET USERDICT */
    std::string userDictModule = pt.get<std::string>(path + ".user_dict", "");
    if (!userDictModule.empty())
    {
        std::string p = "modules." + userDictModule;
        std::string modName = pt.get<std::string>(p);
        std::map<std::string, std::string> user_dict = boost::any_cast< std::map<std::string, std::string> >( element(modName, p, userDictModule) );
        std::map<std::string, std::string>::iterator it;
        for (it=user_dict.begin(); it!=user_dict.end(); ++it)
        {
            std::string key(it->first);
            std::string value(it->second);
            lc.add_user_dict(key, value);
        }
    }else{
        try
        {
            BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                        pt.get_child(path + ".user_dict"))
            {
                std::string key(v.first);
                std::string value(v.second.data());
                lc.add_user_dict(key, value);
            }
        }catch(boost::property_tree::ptree_bad_path){ }
    }
    /* SET MAXTHREADS */
    int maxthreads = pt.get<int>(path + ".maxthreads", 0);
    if (maxthreads > 0)
    {
        lc.set_max_threads(maxthreads);
    }
    
    /* SET HTTPCLIENT */
    std::string httpClientModule = pt.get<std::string>(path + ".httpclient", "");
    if (!httpClientModule.empty())
    {
        std::string p = "modules." + httpClientModule;
        std::string modName = pt.get<std::string>(p);
        airport::HttpClient httpClient = boost::any_cast< airport::HttpClient >( element(modName, p, httpClientModule) );
        lc.set_http_client(httpClient);
    }
    
    /* SET OBSERVERS */
    try
    {
        typedef boost::function<std::string(std::string, std::string)> function_type;
        std::map<std::string, std::pair<function_type, std::string> > observers;
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                    pt.get_child(path + ".observers"))
        {
            std::string key(v.first);
            std::string obModule(v.second.data());
            std::string p = "modules." + obModule;
            std::pair<function_type, std::string> pr((*this), obModule);
            observers.insert( std::map<std::string, std::pair<function_type, std::string> >::value_type(key, pr));
        }
        lc.set_observers(observers);
    }catch(boost::property_tree::ptree_bad_path){ }
    
    std::vector<airport::HttpResponse> hr = lc.start();
    std::pair< airport::ListCrawler, std::vector<airport::HttpResponse> > rs(lc, hr);
    return rs;
}


std::pair< airport::FeedCrawler, std::pair<airport::Feed, std::vector<airport::FeedEntry> > >
airport::Rule::feedCrawler(std::string &path)
{
    airport::FeedCrawler fc;
    fc.set_node_name(nodename);
    std::pair<airport::Feed, std::vector<airport::FeedEntry> > result;
    /* SET HTTPCLIENT */
    std::string httpClientModule = pt.get<std::string>(path + ".httpclient", "");
    if (!httpClientModule.empty())
    {
        std::string p = "modules." + httpClientModule;
        std::string modName = pt.get<std::string>(p);
        airport::HttpClient httpClient = boost::any_cast< airport::HttpClient >( element(modName, p, httpClientModule) );
        fc.set_http_client(httpClient);
    }
    try
    {
        bool saveInMongo = pt.get<bool>(path + ".save_in_mongo");
        fc.set_save_in_mongo(saveInMongo);
    }catch(boost::property_tree::ptree_bad_path){ }
    
    try
    {
        bool enableUpdate = pt.get<bool>(path + ".enable_update");
        fc.set_enable_update(enableUpdate);
    }catch(boost::property_tree::ptree_bad_path){ }
    
    try
    {
        bool fullRss = pt.get<bool>(path + ".full");
        fc.set_full_rss(fullRss);
    }catch(boost::property_tree::ptree_bad_path){ }
    
    try{
        std::string startUrl = pt.get<std::string>(path + ".starturl");
        result = fc.get(startUrl);
    }catch(boost::property_tree::ptree_bad_path){ 
        if (airport::DEBUG_LEVEL > airport::DEBUG_OFF)
        {
            boost::mutex::scoped_lock lock(mr_io_monitor);
            std::cout << "ERROR: need starturl" << std::endl;
            //throw exception;
        }
    }
    std::pair< airport::FeedCrawler, std::pair<airport::Feed, std::vector<airport::FeedEntry> > > rs(fc, result);
    return rs;
}

std::pair< airport::BasicCrawler, std::vector<airport::HttpResponse> >
airport::Rule::basicCrawler(std::string &path, const std::string &start_url)
{
    airport::BasicCrawler bc;
    bc.set_node_name(nodename);
    /* SET URLS */
    if (start_url.empty())
    {
        std::string urlsModule = pt.get<std::string>(path + ".urls", "");
        if (!urlsModule.empty())
        {
            std::string p = "modules." + urlsModule;
            std::string modName = pt.get<std::string>(p);
            std::vector<std::string> urls = boost::any_cast< std::vector<std::string> >( element(modName, p, urlsModule) );
            BOOST_FOREACH(std::string url, urls)
            {
                bc.add_url(url);
            }
        }else{
            try
            {
                BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                            pt.get_child(path + ".urls"))
                {
                    bc.add_url(v.second.data());
                }
            }catch(boost::property_tree::ptree_bad_path){ 
                if (airport::DEBUG_LEVEL > airport::DEBUG_OFF)
                {
                    boost::mutex::scoped_lock lock(mr_io_monitor);
                    std::cout << "ERROR: need urls" << std::endl;
                    //throw exception;
                }
            }
        }
    }else{
        std::string urlModule = pt.get<std::string>(path + ".url", "");
        if (!urlModule.empty())
        {
            std::string p = "modules." + urlModule;
            std::string modName = pt.get<std::string>(p);
            std::string url = boost::any_cast< std::string >( element(modName, p, urlModule, start_url) );
            bc.add_url(url);
        }else{
            bc.add_url(start_url.c_str());
        }
        
    }
    /* SET ESSENTIAL FIELDS */
    std::string eFieldsModule = pt.get<std::string>(path + ".essential_fields", "");
    if (!eFieldsModule.empty())
    {
        std::string p = "modules." + eFieldsModule;
        std::string modName = pt.get<std::string>(p);
        std::vector<std::string> fields = boost::any_cast< std::vector<std::string> >( element(modName, p, eFieldsModule) );
        BOOST_FOREACH(std::string field, fields)
        {
            bc.add_essential_field(field);
        }
    }else{
        try
        {
            BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                        pt.get_child(path + ".essential_fields"))
            {
                bc.add_essential_field(v.second.data());
            }
        }catch(boost::property_tree::ptree_bad_path){ }
    }
    /* SET PARSERS */
    std::string parsersModule = pt.get<std::string>(path + ".parsers", "");
    if (!parsersModule.empty())
    {
        std::string p = "modules." + parsersModule;
        std::string modName = pt.get<std::string>(p);
        std::map<std::string, std::string> parsers = boost::any_cast< std::map<std::string, std::string> >( element(modName, p, parsersModule) );
        std::map<std::string, std::string>::iterator it;
        for (it=parsers.begin(); it!=parsers.end(); ++it)
        {
            std::string key(it->first);
            std::string value(it->second);
            bc.add_parser(key, value);
        }
    }else{
        try
        {
            BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                        pt.get_child(path + ".parsers"))
            {
                std::string key(v.first);
                std::string value(v.second.data());
                bc.add_parser(key, value);
            }
        }catch(boost::property_tree::ptree_bad_path){ }
    }
    /* SET USERDICT */
    std::string userDictModule = pt.get<std::string>(path + ".user_dict", "");
    if (!userDictModule.empty())
    {
        std::string p = "modules." + userDictModule;
        std::string modName = pt.get<std::string>(p);
        std::map<std::string, std::string> user_dict = boost::any_cast< std::map<std::string, std::string> >( element(modName, p, userDictModule) );
        std::map<std::string, std::string>::iterator it;
        for (it=user_dict.begin(); it!=user_dict.end(); ++it)
        {
            std::string key(it->first);
            std::string value(it->second);
            bc.add_user_dict(key, value);
        }
    }else{
        try
        {
            BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                        pt.get_child(path + ".user_dict"))
            {
                std::string key(v.first);
                std::string value(v.second.data());
                bc.add_user_dict(key, value);
            }
        }catch(boost::property_tree::ptree_bad_path){ }
    }
    /* SET MAXTHREADS */
    int maxthreads = pt.get<int>(path + ".maxthreads", 0);
    if (maxthreads > 0)
    {
        bc.set_max_threads(maxthreads);
    }
    
    /* SET HTTPCLIENT */
    std::string httpClientModule = pt.get<std::string>(path + ".httpclient", "");
    if (!httpClientModule.empty())
    {
        std::string p = "modules." + httpClientModule;
        std::string modName = pt.get<std::string>(p);
        airport::HttpClient httpClient = boost::any_cast< airport::HttpClient >( element(modName, p, httpClientModule) );
        bc.set_http_client(httpClient);
    }
    
    /* SET OBSERVERS */
    try
    {
        typedef boost::function<std::string(std::string, std::string)> function_type;
        std::map<std::string, std::pair<function_type, std::string> > observers;
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                    pt.get_child(path + ".observers"))
        {
            std::string key(v.first);
            std::string obModule(v.second.data());
            std::string p = "modules." + obModule;
            std::pair<function_type, std::string> pr((*this), obModule);
            observers.insert( std::map<std::string, std::pair<function_type, std::string> >::value_type(key, pr));
            //std::string modNam = pt.get<std::string>(p);
            //element(modName, p, obModule);
            //boost::function<std::string(std::string, std::string)> func = boost::ref(*this);
            //func(key, obModule);
            //bc.add_observer(key, rule, obModule);
        }
        bc.set_observers(observers);
    }catch(boost::property_tree::ptree_bad_path){ }
    
    std::vector<airport::HttpResponse> hr = bc.start();
    std::pair< airport::BasicCrawler, std::vector<airport::HttpResponse> > rs(bc, hr);
    return rs;
}

boost::any
airport::Rule::List(std::string &path, const std::string &request)
{
    boost::any response;
    std::string subMod = pt.get(path + "._list", "");
    if (!subMod.empty())
    {
        std::string p = "modules." + subMod;
        std::string modName = pt.get<std::string>(p);
        response = element(modName, p, subMod, request);
    }else{
        std::vector<std::string> l;
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                    pt.get_child(path))
        {
            if (v.first == "item")
                l.push_back(v.second.data());
            if (v.first == "_item")
            {
                std::string subMod = v.second.data();
                std::string p = "modules." + subMod;
                std::string modName = pt.get<std::string>(p);
                l.push_back( boost::any_cast<std::string>(element(modName, p, subMod, request)) );
            }
        }
        response = l;
    }
    return response;
}

boost::any
airport::Rule::Dict(std::string &path, const std::string &request)
{
    boost::any response;
    std::string subMod = pt.get(path + "._dict", "");
    if (!subMod.empty())
    {
        std::string p = "modules." + subMod;
        std::string modName = pt.get<std::string>(p);
        response = element(modName, p, subMod, request);
    }else{
        std::map<std::string, std::string> d;
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                    pt.get_child(path))
        {
            if (v.first!="_dict")
            {
                d.insert(std::map<std::string, std::string>::value_type(v.first, v.second.data()));
            }
        }
        response = d;
    }
    return response;
}

boost::any
airport::Rule::ListMember(std::string &path, const std::string &request)
{
    boost::any response;
    std::string subMod = pt.get(path + ".list", "");
    int member;
    try
    {
        member = pt.get<int>(path + ".member");
    }catch(boost::property_tree::ptree_bad_path){
        member = boost::lexical_cast<int>(request);
    }
    
    if (!subMod.empty())
    {
        std::string p = "modules." + subMod;
        std::string modName = pt.get<std::string>(p);
        std::string memberType = pt.get(path + ".member_type", "string");
        if (memberType == "string")
        {
            std::vector<std::string> l = boost::any_cast< std::vector<std::string> >(element(modName, p, subMod, request));
            if (member>=l.size())
                response = l.end();
            else
                response = l[member];
        }else if (memberType == "basiccrawler"){
            std::pair< airport::BasicCrawler, std::vector<airport::HttpResponse> > l = boost::any_cast< std::pair< airport::BasicCrawler, std::vector<airport::HttpResponse> > >(element(modName, p, subMod, request));
            if (member == 0) response = l.first;
            else response = l.second;
        }else if (memberType == "listcrawler"){
            std::pair< airport::ListCrawler, std::vector<airport::HttpResponse> > l = boost::any_cast< std::pair< airport::ListCrawler, std::vector<airport::HttpResponse> > >(element(modName, p, subMod, request));
            if (member == 0) response = l.first;
            else response = l.second;
        }else if (memberType == "paginatecrawler"){
            std::pair< airport::PaginateCrawler, std::vector<airport::HttpResponse> > l = boost::any_cast< std::pair< airport::PaginateCrawler, std::vector<airport::HttpResponse> > >(element(modName, p, subMod, request));
            if (member == 0) response = l.first;
            else response = l.second;
        }else if (memberType == "httpresponse"){
            std::vector<airport::HttpResponse> l = boost::any_cast < std::vector<airport::HttpResponse> >(element(modName, p, subMod, request));
            if (member>=l.size())
                response = l.end();
            else
                response = l[member];
        }
    }else{
        std::vector<std::string> l;
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                    pt.get_child(path))
        {
            if (v.first == "item")
                l.push_back(v.second.data());
        }
        response = l[member];
    }
    return response;
}

boost::any 
airport::Rule::DictMember(std::string &path, const std::string &request)
{
    boost::any response;
    std::string subMod = pt.get(path + ".dict", "");
    std::string member = pt.get<std::string>(path + ".member", "");
    if (member.empty()) member = request;
    if (!subMod.empty())
    {
        std::string p = "modules." + subMod;
        std::string modName = pt.get<std::string>(p);
        std::map<std::string, std::string> d = boost::any_cast< std::map<std::string, std::string> >(element(modName, p, subMod, request));
        response = d[member];
    }else{
        std::map<std::string, std::string> d;
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                    pt.get_child(path))
        {
            d.insert(std::map<std::string, std::string>::value_type(v.first, v.second.data()));
        }
        response = d[member];
    }
    return response;
}

boost::any
airport::Rule::httpResponse(std::string &path, const std::string &request)
{
    boost::any response;
    std::string subMod = pt.get<std::string>(path + ".httpresponse");
    std::string member = pt.get<std::string>(path + ".member");
    std::string p = "modules." + subMod;
    std::string modName = pt.get<std::string>(p);
    airport::HttpResponse hr = boost::any_cast< airport::HttpResponse >(element(modName, p, subMod, request));
    if (member == "parsed_data")
    {
        response = hr.get_parsed_data();
    }else if (member == "html_body") {
        response = hr.get_html_body();
    }else if (member == "html_header") {
        response = hr.get_html_header();
    }
    return response;
}

boost::any
airport::Rule::JsonDecode(std::string &path, const std::string &request)
{
    boost::any response;
    std::string subMod = pt.get<std::string>(path + ".json");
    std::string member = pt.get<std::string>(path + ".member", "");
    std::string p = "modules." + subMod;
    std::string modName = pt.get<std::string>(p);
    std::string js = boost::any_cast< std::string >(element(modName, p, subMod, request));
    std::map<std::string, std::string> d = airport::Utils::jsonDecode(js);
    if (member.empty())
        response = d;
    else
        response = d[member];
    return response;
}

boost::any
airport::Rule::Regex(std::string &path, const std::string &request)
{
    boost::any response;
    std::string baseStr = pt.get<std::string>(path + ".string", "");
    std::string subMod = pt.get<std::string>(path + ".SUBMOD", "");
    std::string regex = pt.get<std::string>(path + ".regex");
    bool matchAll = pt.get<bool>(path + ".matchall", false);
    std::string resStr;
    std::vector<std::string> matches;
    if (baseStr.empty() && subMod.empty())
    {
        baseStr = request;
        boost::regex reg(regex);
        if (matchAll)
        {
            boost::sregex_token_iterator i(baseStr.begin(), baseStr.end(), reg, 1);
            boost::sregex_token_iterator j;
            while(i != j) {
                std::string m = (*i++);
                if (!m.empty())
                    matches.push_back(m);
            }
        }else{
            boost::smatch m;
            if (boost::regex_search(baseStr, m, reg, boost::match_extra)) {
                if (m[1].matched)
                {
                    resStr = m[1].str();
                }
            }
        }
    }else if(!subMod.empty()) {
        std::string p = "modules." + subMod;
        std::string modName = pt.get<std::string>(p);
        if (baseStr.empty()){
            baseStr = boost::any_cast< std::string >(element(modName, p, subMod, request));
            boost::regex reg(regex);
            if (matchAll)
            {
                boost::sregex_token_iterator i(baseStr.begin(), baseStr.end(), reg, 1);
                boost::sregex_token_iterator j;
                while(i != j) {
                    std::string m = (*i++);
                    if (!m.empty())
                        matches.push_back(m);
                }
            }else{
                boost::smatch m;
                if (boost::regex_search(baseStr, m, reg)) {
                    if (m[1].matched)
                    {
                        resStr = m[1].str();
                    }
                }
            }
        }else{
            boost::regex reg(regex);
            if (matchAll)
            {
                boost::sregex_token_iterator i(baseStr.begin(), baseStr.end(), reg, 1);
                boost::sregex_token_iterator j;
                while(i != j) {
                    std::string m = (*i++);
                    if (!m.empty())
                        matches.push_back(m);
                }
            }else{
                boost::smatch m;
                if (boost::regex_search(baseStr, m, reg)) {
                    if (m[1].matched)
                    {
                        baseStr = m[1].str();
                    }
                }
            }
            if (matchAll)
            {
                matches = boost::any_cast< std::vector<std::string> >(element(modName, p, subMod, matches));
                std::string _regex = pt.get<std::string>(path + "._regex", "");
                if (!_regex.empty())
                {
                    boost::regex _reg(_regex);
                    boost::smatch _m;
                    for(unsigned int i = 0; i < matches.size(); ++i)
                    {
                        if (boost::regex_search(resStr, _m, _reg)) {
                            if (_m[1].matched)
                            {
                                matches[i] = _m[1].str();
                            }
                        }
                    }
                }
            }else{
                resStr = boost::any_cast< std::string >(element(modName, p, subMod, baseStr));
                std::string _regex = pt.get<std::string>(path + "._regex", "");
                if (!_regex.empty())
                {
                    boost::regex _reg(_regex);
                    boost::smatch _m;
                    if (boost::regex_search(resStr, _m, _reg)) {
                        if (_m[1].matched)
                        {
                            resStr = _m[1].str();
                        }
                    }
                }
            }
            
        }
    }
    if (matchAll)
        response = matches;
    else
        response = resStr;
    return response;
}

std::string
airport::Rule::StringJoin(std::string &path, const std::string &request)
{
    std::string subMod = pt.get(path + ".strings", "");
    std::string spliter = pt.get(path + ".spliter", "");
    std::string str;
    if (!subMod.empty())
    {
        std::string p = "modules." + subMod;
        std::string modName = pt.get<std::string>(p);
        std::vector<std::string> strs = boost::any_cast< std::vector<std::string> >(element(modName, p, subMod, request));
        for(std::vector<std::string>::iterator it=strs.begin(); it!=strs.end(); ++it)
        {
            if (it!=strs.begin() && !spliter.empty())
                str += spliter;
            str += (*it);
        }
    }else{
        int i = 0;
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                    pt.get_child(path + ".strings"))
        {
            if (i!=0 && !spliter.empty())
                str += spliter;
            if (v.first == "item")
                str += v.second.data();
            else if (v.first == "_item")
                str += request;
            i ++ ;
        }
    }
    return str;
}

std::vector<std::string>
airport::Rule::FileToList(std::string &path, const std::string &request)
{
    std::string filename = pt.get(path + ".filename", "");
    if (filename.empty())
    {
        try
        {
            std::string subMod = pt.get<std::string>(path + "._filename");
            std::string p = "modules." + subMod;
            std::string modName = pt.get<std::string>(p);
            filename = boost::any_cast< std::string >(element(modName, p, subMod, request));
        }catch(boost::property_tree::ptree_bad_path){
            if (airport::DEBUG_LEVEL > airport::DEBUG_OFF)
            {
                std::cout << "ERROR: need filename" << std::endl;
            }
            //throw exception
        }
    }
    return airport::Utils::fileToList(filename);
}

std::map<std::string, std::string>
airport::Rule::FileToDict(std::string &path, const std::string &request)
{
    std::string filename = pt.get(path + ".filename", "");
    if (filename.empty())
    {
        try
        {
            std::string subMod = pt.get<std::string>(path + "._filename");
            std::string p = "modules." + subMod;
            std::string modName = pt.get<std::string>(p);
            filename = boost::any_cast< std::string >(element(modName, p, subMod, request));
        }catch(boost::property_tree::ptree_bad_path){
            if (airport::DEBUG_LEVEL > airport::DEBUG_OFF)
            {
                std::cout << "ERROR: need filename" << std::endl;
            }
            //throw exception
        }
    }
    return airport::Utils::fileToDict(filename);
}

std::string
airport::Rule::Tidy(std::string &path, const std::string &request)
{
    std::string s = request;
    return airport::Utils::tidy(s);
}

std::string
airport::Rule::StripTags(std::string &path, const std::string &request)
{
    std::string s = request;
    return airport::Utils::stripTags(s);
}

std::string
airport::Rule::HtmlSanitize(std::string &path, const std::string &request)
{
    std::string s = request;
    return airport::Utils::htmlSanitize(s);
}

std::vector< boost::tuple<std::string, std::string, double> >
airport::Rule::MongoKeywords(std::string &path, const std::string &request)
{
    std::vector< boost::tuple<std::string, std::string, double> > response;
    std::string collection = pt.get<std::string>(path + ".collection");
    std::string query;
    
    std::string subMod = pt.get<std::string>(path + "._query", "");
    if (!subMod.empty())
    {
        std::string p = "modules." + subMod;
        std::string modName = pt.get<std::string>(p);
        query = boost::any_cast< std::string >(element(modName, p, subMod, request));
    }else{
        query = pt.get<std::string>(path + ".query", "");
        if (query.empty())
        {
            query = request;
        }
    }
    
    Mongo mongoDB;
    mongo::DBClientConnection mongoConnection;
    mongoDB.connect(&mongoConnection);
    response = mongoDB.getKeywords(mongoConnection, collection, query);
    return response;
}