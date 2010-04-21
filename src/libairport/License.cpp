#include "License.h"
#include <botan/botan.h>
#include "Config.h"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <vector>
#include <time.h>

airport::License::License(){}

airport::License::~License(){}

std::pair<std::string, std::string> 
airport::License::encode_license(std::string &info, std::string &secret)
{
    Botan::AutoSeeded_RNG rng;
    std::auto_ptr<Botan::S2K> s2k(Botan::get_s2k("PBKDF2(SHA-1)"));
    s2k->set_iterations(8192);
    s2k->new_random_salt(rng, 8);
    Botan::SymmetricKey key = s2k->derive_key(16, info);
    std::string alg = "AES/CBC/PKCS7";
    Botan::Pipe enc(Botan::get_cipher(alg, key, Botan::ENCRYPTION), new Botan::Hex_Encoder);
    enc.process_msg(secret);
    std::string cipher = enc.read_all_as_string();
    return std::pair<std::string, std::string>(cipher, key.as_string());
}

std::string 
airport::License::decode_license(std::string &cipher, std::string &key)
{
    std::string alg = "AES/CBC/PKCS7";
    Botan::Pipe dec(new Botan::Hex_Decoder, get_cipher(alg, key, Botan::DECRYPTION));
    dec.process_msg(cipher);
    std::string secret = dec.read_all_as_string();
    return secret;
}

bool
airport::License::expired()
{
    if (airport::LICENSE=="0" || airport::ACTIVATE_CODE=="0") return true;
    std::string cipher = airport::LICENSE;
    std::string key = airport::ACTIVATE_CODE;
    std::string secret = airport::License::decode_license(cipher, key);
    std::vector<std::string> parts;
    boost::split(parts, secret, boost::is_any_of("|"));
    if (parts.size()<3) return true;
    if (parts[0] != "com.jipingmi.airport") return true;
    if (parts[1] != airport::AIRPORT_VERSION) return true;
    if (parts[2] == "Unlimited") return false;
    try
    {
        int expire_time = boost::lexical_cast<int>(parts[2]);
        if (expire_time < time(NULL)) return true;
    }catch(boost::bad_lexical_cast){
        return true;
    }
    return false;
}