#include <quarks.hpp>

using namespace Quarks;

Cache Cache::_Instance;
Matrix Matrix::_Instance;


int wildcmp(const char *wild, const char *string) {
    // Written by Jack Handy - <A href="mailto:jakkhandy@hotmail.com">jakkhandy@hotmail.com</A>
    const char *cp = NULL, *mp = NULL;
    
    while ((*string) && (*wild != '*')) {
        if ((*wild != *string) && (*wild != '?')) {
            return 0;
        }
        wild++;
        string++;
    }
    
    while (*string) {
        if (*wild == '*') {
            if (!*++wild) {
                return 1;
            }
            mp = wild;
            cp = string+1;
        } else if ((*wild == *string) || (*wild == '?')) {
            wild++;
            string++;
        } else {
            wild = mp;
            string = cp++;
        }
    }
    
    while (*wild == '*') {
        wild++;
    }
    return !*wild;
}

std::string Cache::putJson(std::string key, crow::json::rvalue& x) {
    
    _cache[key] = std::move(x);
    
    return  key;
    
}

bool Cache::getJson(std::string key, crow::json::wvalue& out){
    
    bool ret = false;
    
    std::map<std::string, crow::json::rvalue>::iterator it = _cache.find(key);
    if (it != _cache.end()){
        out = it->second;
        ret = true;
    }
    
    return ret;
    
}

void Cache::findJson(std::string wild, std::vector<crow::json::wvalue>& matchedResults) {

    for (std::map<std::string, crow::json::rvalue>::iterator it = _cache.begin();
         it != _cache.end(); ++it){
            if(wildcmp(wild.c_str(), it->first.c_str()) ){
                crow::json::wvalue w;
                w = it->second;
                CROW_LOG_INFO << "w : " << crow::json::dump(w);
                
                matchedResults.push_back(std::move(w));
            }
    }
    
}

