#ifndef _QUARKS_H_
#define _QUARKS_H_

#include <string>
#include <map>

#include <crow.h>

/**
 * @brief This namespace refers to implementation of core functionalities from the service
 * 
 */
namespace Quarks {

    /**
     * @brief Solves the shortcomings of Redis - Sort, Expiry, Serialize
     * 
     */
    class Core {
        public:
        static Core _Instance;
        
        Core();
        ~Core();
        
        void setEnvironment(int argc, std::string argv);
        
        bool putJson(std::string key, crow::json::rvalue& x, crow::json::wvalue& out);
        bool getJson(std::string key, crow::json::wvalue& out);
        
        bool getValue(std::string key, std::string& value);
        
        bool iterJson(std::string wild,
                      std::vector<crow::json::wvalue>& matchedResults);
    
        bool searchJson(crow::json::rvalue& args,
                        std::vector<crow::json::wvalue>& matchedResults);
        
        
        
        private:
        
        std::string _argv;
        int _argc;
        
        //std::map<std::string, crow::json::rvalue> _Core;
        
        
    };
    
    
    
}

#include <matrix.hpp>

#endif
