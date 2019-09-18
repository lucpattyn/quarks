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
        
        std::string putJson(std::string key, crow::json::rvalue& x);
        
        bool getJson(std::string key, crow::json::wvalue& out);
        
        bool findJson(std::string wild,
                      std::vector<crow::json::wvalue>& matchedResults);
    
        bool filterJson(crow::json::rvalue& filter,
                        std::vector<crow::json::wvalue>& matchedResults);
        
        private:
        
        //std::map<std::string, crow::json::rvalue> _Core;
        
        
    };
    
    
    
}

#include <matrix.hpp>

#endif
