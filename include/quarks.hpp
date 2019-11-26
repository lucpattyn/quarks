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
        
        
        bool put(std::string body, std::string& out);
        bool get(std::string key, std::string& value);
        bool getAll(std::string wild,
                            std::vector<crow::json::wvalue>& matchedResults,
                            int skip = 0, int limit = -1);
        bool remove(std::string key);
        int removeAll(std::string wild, int skip = 0, int limit = -1);
        
        bool putJson(std::string key, crow::json::rvalue& x, crow::json::wvalue& out);
        bool getJson(std::string key, crow::json::wvalue& out);
        
        
        bool iterJson(std::string wild,
                      std::vector<crow::json::wvalue>& matchedResults,
                      int skip = 0, int limit = -1);
    
        bool searchJson(crow::json::rvalue& args,
                        std::vector<crow::json::wvalue>& matchedResults,
                        int skip = 0, int limit = -1);

        bool openTCPSocketClient();
        
        bool fileTransfer(std::string moduleName, std::string funcName,
			std::string channelName, std::string remoteDescription);
        
               
        private:
        
        std::string _argv;
        int _argc;
        
        //std::map<std::string, crow::json::rvalue> _Core;
        
        
    };
    
    
    
}

#include <matrix.hpp>

#endif
