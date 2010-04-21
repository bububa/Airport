#include <botan/botan.h>
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <boost/lexical_cast.hpp>
#include "airport/License.h"
#include "airport/Config.h"

int main()
{
    Botan::LibraryInitializer init;
    std::string user, unlimited_choose, days, dur, secret, license, key;
    std::cout << "user: ";
    std::cin >> user;
    std::cout << "unlimited licsense?(Y/N)";
    std::cin >> unlimited_choose;
    std::transform(unlimited_choose.begin(), unlimited_choose.end(), unlimited_choose.begin(), (int(*)(int)) std::toupper);
    if (unlimited_choose == "Y")
    {
        dur = "Unlimited";
    }else{
        int expire_days;
        while(true)
        {
            std::cout << "authorize days: ";
            std::cin >> days;
            try
            {
                expire_days = boost::lexical_cast<int>(days);
                break;
            }catch(boost::bad_lexical_cast){}
        }
        int expire_time = expire_days * 24 * 60 + time(NULL);
        dur = boost::lexical_cast<std::string>(expire_time);
    }
    secret = "com.jipingmi.airport|" + airport::AIRPORT_VERSION + "|" + dur + "|" + user;
    
    std::pair<std::string, std::string> p = airport::License::encode_license(user, secret);
    std::cout << "LICENSE: " << p.first << std::endl;
    std::cout << "ACTIVATE CODE: " << p.second << std::endl;
}