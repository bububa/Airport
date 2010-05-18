#include <iostream>
#include <string>
#include <botan/botan.h>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include "airport/Config.h"
#include "airport/Rule.h"
#include "airport/License.h"


namespace po = boost::program_options;
namespace fs = boost::filesystem;

bool find_file( const fs::path & dir_path,         // in this directory,
                const std::string & file_name, // search for this name,
                fs::path & path_found )            // placing path here if found
{
    if ( !fs::exists( dir_path ) ) return false;
    fs::directory_iterator end_itr; // default construction yields past-the-end
    for ( fs::directory_iterator itr( dir_path ); itr != end_itr;++itr )
    {
        if ( fs::is_directory(itr->status()) )
        {
            if ( find_file( itr->path(), file_name, path_found ) ) return true;
        }else if ( itr->leaf() == file_name ){
            path_found = itr->path();
            return true;
        }
    }
    return false;
}

int main(int argc, char *argv[]) {
    Botan::LibraryInitializer init;
    
    if (airport::License::expired())
    {
        std::cout << "Your license has been expired." << std::endl;
    }
    /* PARSE COMMAND LINE */
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("rule,r", po::value<std::string>(), "set rule name")
        ("node,n", po::value<std::string>(), "set node name")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }
    if (vm.count("rule")) {
        std::cout << "Execute rule " << vm["rule"].as<std::string>() << std::endl;
    } else {
        std::cout << "Rule file was not set." << std::endl;
        return 0;
    }
    std::string node;
    if (vm.count("node")) {
        node = vm["node"].as<std::string>();
        std::cout << "Node name " << node << std::endl;
    }else{
        std::cout << "Node name is empty" << std::endl;
        node = "";
    }
    /* FIND RULE FILE */
    fs::path data_path(airport::DATA_PATH);
    fs::path rule_path;
    if (!find_file(data_path, vm["rule"].as<std::string>(), rule_path))
    {
        std::cout<< "Can't find rule file!" << std::endl;
        return 0;
    }
    
    /* EXECUTE RULE */
    airport::Rule rule;
    rule.set_node_name(node);
    std::string filename(rule_path.string());
    if (airport::DEBUG_LEVEL > airport::DEBUG_NORMAL)
    {
        std::string info = rule.readinfo(filename);
        std::cout << info << std::endl;
    }
    rule.parse(filename);
    return 1;
}