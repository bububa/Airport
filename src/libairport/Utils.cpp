/*
 *      utils.cc
 *
 *      Copyright 2010 Brett Mravec <brett.mravec@gmail.com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */
 
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <iconv.h>
#include <errno.h>
#include <sys/stat.h>
#include <json/json.h>

#include <botan/botan.h>
#include <botan/filters.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/find_iterator.hpp>

//#include "app-log.h"
#include "Utils.h"
//#include "app-config.h"

#ifdef DEBUG
#define TRACE(fmt, args...) fprintf(stderr, "%s:%s:%d:"fmt, \
 __FILE__, __FUNCTION__, __LINE__, ##args)
#else
#define TRACE(fmt, args...)
#endif

template<class T>
struct more_second
: std::binary_function<T,T,bool>
{
   inline bool operator()(const T& lhs, const T& rhs)
   {
      return lhs.second > rhs.second;
   }
};

std::string
airport::Utils::size_to_string (size_t size)
{
    const char *ss[] = { "KB", "MB", "GB", "TB" };
 
    char digits = 2;
    char str[50];
    double num = size;
 
    if (size < 1024) {
        sprintf (str, "%d B", (int)size);
        return std::string (str);
    }
 
    for (int i = 0; i < 3; i++) {
        num /= 1024.0;
 
        if (num < 1024) {
            if (num > 100) {
                digits = 1;
            }
 
            sprintf (str, "%.*f %s", digits, num, ss[i]);
            return std::string (str);
        }
    }
 
    if (num > 1000) {
        digits = 0;
    } else if (num > 100) {
        digits = 1;
    }
 
    sprintf (str, "%.*f %s", digits, num, ss[3]);
    return std::string (str);
}
 
int
airport::Utils::parseInt (const std::string &str)
{
    if (str[0] >= '0' && str[0] <= '9' || str[0] == '-') {
        int val;
        std::istringstream iss (str);
        iss >> val;
        return val;
    } else {
        return 0;
    }
}
 
int
airport::Utils::parseHexInt (const std::string &str)
{
    if ((str[0] >= '0' && str[0] <= '9') ||
        (str[0] >= 'a' && str[0] <= 'f') ||
        (str[0] >= 'A' && str[0] <= 'F')) {
        int val;
        std::istringstream iss (str);
        iss >> std::hex >> val;
        return val;
    } else {
        return 0;
    }
}
 
std::string
airport::Utils::formatInt (int val)
{
    std::stringstream out;
    out << val;
    return out.str ();
}
 
std::string
airport::Utils::formatHexInt (int val)
{
    std::stringstream out;
    out << std::hex << val;
    return out.str ();
}

int
airport::Utils::getHexDigit (char b)
{
    if (b >= '0' && b <= '9') {
        return b - '0';
    } else if (b >= 'A' && b <= 'F') {
        return 10 + b - 'A';
    } else if (b >= 'a' && b <= 'f') {
        return 10 + b - 'a';
    }
 
    return 0;
}

std::string
airport::Utils::formatDouble (double val)
{
    std::stringstream out;
    out << val;
    return out.str ();
}

std::wstring 
airport::Utils::widen( const std::string &str )
{
    using namespace std ;
    wostringstream wstm ;
    const ctype<wchar_t>& ctfacet = 
                        use_facet< ctype<wchar_t> >( wstm.getloc() ) ;
    for( size_t i=0 ; i<str.size() ; ++i ) 
              wstm << ctfacet.widen( str[i] ) ;
    return wstm.str() ;
}

std::string 
airport::Utils::narrow( const std::wstring &str )
{
    using namespace std ;
    ostringstream stm ;
    const ctype<char>& ctfacet = 
                         use_facet< ctype<char> >( stm.getloc() ) ;
    for( size_t i=0 ; i<str.size() ; ++i ) 
                  stm << ctfacet.narrow( str[i], 0 ) ;
    return stm.str() ;
}

int 
airport::Utils::iconv_string(const char *from, const char *to, const char *src, size_t len, 
std::string& result, int c = 0, size_t buf_size = 512)
{
    iconv_t cd;
    
    char *pinbuf = const_cast< char* >(src);
    size_t inbytesleft = len;
    char *poutbuf = NULL;
    size_t outbytesleft = buf_size;
    
    char *dst = NULL;
    size_t retbytes = 0;
    int done = 0;
    int errno_save = 0;
    
    if ((iconv_t)-1 == (cd = iconv_open(to, from)))
    {
        return -1;
    }
    
    dst = new char[buf_size];
    
    while(inbytesleft > 0 && !done)
    {
        poutbuf = dst;
        outbytesleft = buf_size;
        
        TRACE("TARGET - in:%p pin:%p left:%d\n", src, pinbuf, inbytesleft);
        retbytes = iconv(cd, &pinbuf, &inbytesleft, &poutbuf, &outbytesleft);
        errno_save = errno;
        
        if (dst != poutbuf) // we have something to write
        {
            TRACE("OK - in:%p pin:%p left:%d done:%d buf:%d\n", 
            src, pinbuf, inbytesleft, pinbuf-src, poutbuf-dst);
            result.append(dst, poutbuf-dst);
        }
        
        if (retbytes != (size_t)-1)
        {
            poutbuf = dst;
            outbytesleft = buf_size;
            (void)iconv(cd, NULL, NULL, &poutbuf, &outbytesleft);
            
            if (dst != poutbuf) // we have something to write
            {
                TRACE("OK - in:%p pin:%p left:%d done:%d buf:%d\n", 
                src, pinbuf, inbytesleft, pinbuf-src, poutbuf-dst);
                result.append(dst, poutbuf-dst);
            }
            
            errno_save = 0;
            break;
        }
        
        TRACE("FAIL - in:%p pin:%p left:%d done:%d buf:%d\n", 
        src, pinbuf, inbytesleft, pinbuf-src, poutbuf-dst);
        
        switch(errno_save)
        {
            case E2BIG: 
                TRACE("E E2BIG\n");
                break;
            case EILSEQ:
                TRACE("E EILSEQ\n");
                if (c) {
                    errno_save = 0;
                    inbytesleft = len-(pinbuf-src); // forward one illegal byte
                    inbytesleft--;
                    pinbuf++;
                    break;
                }
                done = 1;
                break;
            case EINVAL:
                TRACE("E EINVAL\n");
                done = 1;
                break;
            default:
                TRACE("E Unknown:[%d]%s\n", errno, strerror(errno));
                done = 1;
                break;
            }
        }
        
        delete[] dst;
        iconv_close(cd);
        
        errno = errno_save;
        return (errno_save) ? -1 : 0;
}

int 
airport::Utils::getRand(int size)
{
    int lower = 0, upper = size;
    do
    {
        int mid = (lower + upper) / 2;
        
        if (rand () > RAND_MAX / 2) // not a great test, perhaps use parity of rand ()?
        {
            lower = mid;
        }else{
            upper = mid;
        }
    } while (upper != lower);
    return lower;
}

/*
std::string
airport::Utils::createDownloadFilename (const std::string &name)
{
    std::string path = Config::Instance ().get_property (Config::DOWNLOAD_DIRECTORY);
 
    mkdir (path.c_str (), 0700);
    path += "/" + name;
 
    return path;
}
*/
int
airport::Utils::getFileSize (const std::string &name)
{
    struct stat ostat;
    if (stat (name.c_str (), &ostat) == 0) {
        return ostat.st_size;
    } else {
        return -1;
    }
}
 
bool
airport::Utils::removePath (const std::string &name)
{
    struct stat ostat;
 
    if (stat (name.c_str (), &ostat) == 0) {
        if (S_ISDIR (ostat.st_mode)) {
            struct dirent *dp;
            DIR *dir = opendir (name.c_str ());
            while ((dp = readdir (dir)) != NULL) {
                if (!strcmp (dp->d_name, ".") ||
                    !strcmp (dp->d_name, "..")) {
                    continue;
                }
 
                removePath (name + "/" + dp->d_name);
            }
            closedir (dir);
 
            //LOG_INFO ("Remove Dir: " + name);
            remove (name.c_str ());
        } else {
            //LOG_INFO ("Remove File: " + name);
            remove (name.c_str ());
        }
 
        return true;
    } else {
        return false;
    }
}
 
std::string
airport::Utils::createConfigFilename (const std::string &name)
{
    std::string path (getenv ("HOME"));
    path += "/.config/downman";
 
    mkdir (path.c_str (), 0700);
 
    path += "/" + name;
 
    return path;
}
 
std::string
airport::Utils::getDefaultDownloadDirectory ()
{
    std::string path (getenv ("HOME"));
    path += "/Downloads";
 
    return path;
}
/*
std::string
airport::Utils::getImageResource (const std::string &name)
{
    std::string path = SHARE_DIR;
 
    path += "/imgs/";
    path += name;
 
    return path;
}
*/

std::string
airport::Utils::sha1(std::string request)
{
    Botan::Pipe pipe (new Botan::Hash_Filter("SHA-1"), new Botan::Hex_Encoder);
    pipe.process_msg(request);
    std::string res = pipe.read_all_as_string(0);
    return res;
}

void 
airport::Utils::sleep( clock_t wait )
{
    clock_t goal;
    goal = wait + clock();
    while( goal > clock() )
        ;
}

bool 
airport::Utils::endsWith (std::string const &fullString, std::string const &ending)
{
    if (fullString.length() > ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

std::string
airport::Utils::tidy(std::string &request, const TidyOptionId outOptId)
{
    if (request.empty()) return request;
    TidyBuffer output = {0};
    TidyBuffer errbuf = {0};
    int rc = -1;
    Bool ok;
    
    TidyDoc tdoc = tidyCreate();                     // Initialize "document"
    ok = tidyOptSetBool( tdoc, outOptId, yes );  // Convert to XHTML
    if (outOptId == TidyXmlOut)
        ok = tidyOptSetBool( tdoc, TidyXmlTags, yes );
    if ( ok )
        rc = tidySetErrorBuffer( tdoc, &errbuf );      // Capture diagnostics
    if (rc >= 0)
        rc = tidySetCharEncoding( tdoc, "utf8");
    if ( rc >= 0 )
        rc = tidyParseString( tdoc, request.c_str() );           // Parse the input
    if ( rc >= 0 )
        rc = tidyCleanAndRepair( tdoc );               // Tidy it up!
    if ( rc >= 0 )
        rc = tidyRunDiagnostics( tdoc );               // Kvetch
    if ( rc > 1 )                                    // If error, force output.
        rc = ( tidyOptSetBool(tdoc, TidyForceOutput, yes) ? rc : -1 );
    if ( rc >= 0 )
        rc = tidySaveBuffer( tdoc, &output );          // Pretty Print
    
    if ( rc >= 0 )
    {
        //if ( rc > 0 )
            //printf( "\nDiagnostics:\n\n%s", errbuf.bp );
        //printf( "\nAnd here is the result:\n\n%s", output.bp );
        std::string res(reinterpret_cast<const char *>(output.bp));
        return res;
    }else
        //printf( "A severe error (%d) occurred.\n", rc );
    
    tidyBufFree( &output );
    tidyBufFree( &errbuf );
    tidyRelease( tdoc );
    return request;
}

std::vector<std::string>
airport::Utils::fileToList(std::string &filename)
{
    std::vector<std::string> response;
    std::ifstream in(filename.c_str());
    std::string line;
    while(getline(in, line))
    {
        response.push_back(line);
    }
    return response;
}

std::map<std::string, std::string>
airport::Utils::fileToDict(std::string &filename)
{
    std::map<std::string, std::string> response;
    std::ifstream in(filename.c_str());
    std::string line;
    while(getline(in, line))
    {
        std::vector<std::string> strs;
        boost::split(strs, line, boost::is_any_of("="));
        std::string value;
        std::string key;
        for(std::vector<std::string>::iterator it=strs.begin(); it!=strs.end(); ++it)
        {
            if (it == strs.begin())
            {
                key = (*it);
            }else{
                value += (*it);
                if (it != strs.end())
                    value += " ";
            }
        }
        response.insert(std::map<std::string, std::string>::value_type(key, value));
    }
    return response;
}

std::map<std::string, std::string>
airport::Utils::jsonDecode(std::string &js)
{
    Json::Value root;   // will contains the root value after parsing.
    Json::Reader reader;
    bool parsingSuccessful = reader.parse( js, root );
    std::map<std::string, std::string> d;
    if (parsingSuccessful)
    {
        Json::Value::Members members = root.getMemberNames();
        Json::Value::Members::iterator it;
        for (it=members.begin();it!=members.end();++it)
        {
            std::string key = (*it);
            std::string value = root[key].asString();
            d.insert(std::map<std::string, std::string>::value_type(key, value));
        }
    }
    return d;
}

std::string
airport::Utils::htmlSanitize(std::string &req)
{
    std::string html = airport::Utils::tidy(req);
    TidyDoc tdoc = tidyCreate();
    TidyBuffer errbuf = {0};
    tidySetErrorBuffer( tdoc, &errbuf );
    tidyBufFree( &errbuf );
    tidySetCharEncoding( tdoc, "utf8");
    tidyParseString( tdoc, html.c_str() );
    html = airport::Utils::htmlSafeNode(tdoc, tidyGetBody(tdoc));
    return html;
}

std::string
airport::Utils::stripTags(std::string &req)
{
    std::string html = airport::Utils::tidy(req);
    TidyDoc tdoc = tidyCreate();
    TidyBuffer errbuf = {0};
    tidySetErrorBuffer( tdoc, &errbuf );
    tidyBufFree( &errbuf );
    tidySetCharEncoding( tdoc, "utf8");
    tidyParseString( tdoc, html.c_str() );
    html = airport::Utils::htmlNodeText(tdoc, tidyGetBody(tdoc));
    return html;
}

std::string
airport::Utils::htmlNodeText( TidyDoc tdoc, TidyNode tnod )
{
    std::string response;
    TidyNode child;
    for ( child = tidyGetChild(tnod); child; child = tidyGetNext(child) )
    {
        ctmbstr name = airport::Utils::nodeName(child);
        //assert( name != NULL );
        if ( airport::Utils::isSafeNode(child) )
        {
            if (name=="Text")
            {
                TidyBuffer buf = {0};
                tidyNodeGetText (tdoc, child, &buf);
                std::string text(reinterpret_cast<const char *>(buf.bp));
                text.erase(std::remove(text.begin(), text.end(), '\n'), text.end());
                response += text;
                tidyBufFree( &buf );
            }
            response += airport::Utils::htmlNodeText( tdoc, child );
        }
    }
    return response;
}

std::string
airport::Utils::htmlSafeNode( TidyDoc tdoc, TidyNode tnod )
{
    std::string response;
    TidyNode child;
    for ( child = tidyGetChild(tnod); child; child = tidyGetNext(child) )
    {
        ctmbstr name = airport::Utils::nodeName(child);
        //assert( name != NULL );
        if ( airport::Utils::isSafeNode(child) )
        {
            TidyNode cchild = tidyGetChild(child);
            ctmbstr cname = airport::Utils::nodeName(cchild);
            std::string namet(reinterpret_cast<const char *>(name));
            std::string cnamet(reinterpret_cast<const char *>(cname));
            if (namet=="Text" || cnamet=="Text" || tidyNodeIsHR(child) || tidyNodeIsBR(child) || tidyNodeIsIMG(child))
            {
                TidyBuffer buf = {0};
                tidyNodeGetText (tdoc, child, &buf);
                std::string text(reinterpret_cast<const char *>(buf.bp));
                text.erase(std::remove(text.begin(), text.end(), '\n'), text.end());
                response += text;
                tidyBufFree( &buf );
            }else{
                response += airport::Utils::htmlSafeNode( tdoc, child );
            }
        }
    }
    return response;
}

bool
airport::Utils::isSafeNode( TidyNode tnod )
{
    return !( tidyNodeIsHTML(tnod) || tidyNodeIsHEAD(tnod) || tidyNodeIsTITLE(tnod) || tidyNodeIsBASE(tnod) || tidyNodeIsMETA(tnod) || tidyNodeIsBODY(tnod) || tidyNodeIsFRAMESET(tnod) || tidyNodeIsFRAME(tnod) || tidyNodeIsIFRAME(tnod) || tidyNodeIsNOFRAMES(tnod) || tidyNodeIsLINK(tnod) || tidyNodeIsOPTION(tnod) || tidyNodeIsAREA(tnod) || tidyNodeIsNOBR(tnod) || tidyNodeIsSTYLE(tnod) || tidyNodeIsSCRIPT(tnod) || tidyNodeIsNOSCRIPT(tnod) || tidyNodeIsFORM(tnod) || tidyNodeIsTEXTAREA(tnod) || tidyNodeIsAPPLET(tnod) || tidyNodeIsOBJECT(tnod) || tidyNodeIsINPUT(tnod) || tidyNodeIsXMP(tnod) || tidyNodeIsSELECT(tnod) || tidyNodeIsEMBED(tnod) || tidyNodeIsMENU(tnod) );
}

ctmbstr
airport::Utils::nodeName(TidyNode tnod)
{
    ctmbstr name;
    switch ( tidyNodeGetType(tnod) )
    {
        case TidyNode_Root:       name = "Root";                    break;
        case TidyNode_DocType:    name = "DOCTYPE";                 break;
        case TidyNode_Comment:    name = "Comment";                 break;
        case TidyNode_ProcIns:    name = "Processing Instruction";  break;
        case TidyNode_Text:       name = "Text";                    break;
        case TidyNode_CDATA:      name = "CDATA";                   break;
        case TidyNode_Section:    name = "XML Section";             break;
        case TidyNode_Asp:        name = "ASP";                     break;
        case TidyNode_Jste:       name = "JSTE";                    break;
        case TidyNode_Php:        name = "PHP";                     break;
        case TidyNode_XmlDecl:    name = "XML Declaration";         break;
        case TidyNode_Start:
        case TidyNode_End:
        case TidyNode_StartEnd:
        default:
            name = tidyNodeGetName( tnod );
            break;
    }
    return name;
}

std::vector< boost::tuple<std::string, std::string, double> >
airport::Utils::bestMatchKeywords(std::vector< boost::tuple<std::string, std::string, double> > &keywords, std::string &str)
{
    std::vector< boost::tuple<std::string, std::string, double> > response;
    typedef boost::tuple<std::string, std::string, double> kdata_t;
    typedef std::pair<kdata_t, double> tfidf_t;
    typedef boost::find_iterator<std::string::iterator> string_find_iterator;
    std::vector<tfidf_t> vec;
    for (std::vector<kdata_t>::iterator it=keywords.begin();it!=keywords.end();++it)
    {
        kdata_t keyword = (*it);
        unsigned int counter = 0;
        for(string_find_iterator It=make_find_iterator(str, first_finder(keyword.get<0>(), boost::is_iequal()));It!=string_find_iterator();++It)
        {
            counter ++;
        }
        double tf = (double)keyword.get<0>().size() / (double)str.size() * (double)counter;
        double tfidf = tf * keyword.get<2>();
        tfidf_t p(keyword, tfidf);
        vec.push_back(p);
    }
    std::sort(vec.begin(), vec.end(), more_second<tfidf_t>());
    for (std::vector<tfidf_t>::iterator it=vec.begin();it!=vec.end();++it)
    {
        response.push_back(it->first);
    }
    return response;
}

int
airport::Utils::check_end (const char *p)
{
    if (!p) return 0;
    while (ISSPACE (*p)) ++p;
    if (!*p 
        || (p[0] == 'G' && p[1] == 'M' && p[2] == 'T')
        || ((p[0] == '+' || p[1] == '-') && ISDIGIT (p[1])))
        return 1;
    else
        return 0;
}

time_t
airport::Utils::mktime_from_utc (struct tm *t)
{
    time_t tl, tb;
    struct tm *tg;
    tl = mktime (t);
    if (tl == -1)
    {
        t->tm_hour--;
        tl = mktime (t);
        if (tl == -1)
            return -1; /* can't deal with output from strptime */
        tl += 3600;
    }
    tg = gmtime (&tl);
    tg->tm_isdst = 0;
    tb = mktime (tg);
    if (tb == -1)
    {
        tg->tm_hour--;
        tb = mktime (tg);
        if (tb == -1)
            return -1; /* can't deal with output from gmtime */
        tb += 3600;
    }
    return (tl - (tb - tl));
}

time_t
airport::Utils::dateParser(std::string &datetime)
{
    struct tm parsed;
    parsed.tm_isdst = -1;
    time_t mtime;
    /* RFC 1123 */
    if ( check_end(strptime(datetime.c_str(), "%a, %d %b %Y %H:%M:%S GMT", &parsed))
    /* RFC 850 */
    || check_end(strptime(datetime.c_str(), "%a, %d-%b-%y %H:%M:%S GMT", &parsed))
    /* RFC 822 */
    || check_end(strptime(datetime.c_str(), "%d %b %Y %T", &parsed))
    || check_end(strptime(datetime.c_str(), "%a, %d %b %Y %H:%M:%S %z", &parsed))
    /* REST */
    || check_end(strptime(datetime.c_str(), "%m/%d/%Y", &parsed))
    || check_end(strptime(datetime.c_str(), "%Y-%m-%d", &parsed))
    || check_end(strptime(datetime.c_str(), "%Y-%m-%d %H:%M:%S", &parsed))
    /* asctime */
    || check_end(strptime(datetime.c_str(), "%a, %b %d %H:%M:%S %Y", &parsed)) ) 
    {
        mtime = mktime_from_utc(&parsed);
        //mtime = timegm(&parsed);
    }
    return mtime;
}