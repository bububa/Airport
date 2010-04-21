#ifndef LICENSE_H_L8D72JRM
#define LICENSE_H_L8D72JRM

#include <string>

namespace airport
{
    
    class License
    {
    public:
        License();
        ~License();
        static std::pair<std::string, std::string> encode_license(std::string &info, std::string &secret);
        static std::string decode_license(std::string &cipher, std::string &key);
        static bool expired();
    };
}

#endif /* end of include guard: LICENSE_H_L8D72JRM */
