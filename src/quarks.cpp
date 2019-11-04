#include <quarks.hpp>
#include <v8engine.hpp>

#include "rocksdb/db.h"

#include <iomanip>

using namespace Quarks;

Core Core::_Instance;
Matrix Matrix::_Instance;

rocksdb::DB* db = nullptr;
rocksdb::Status dbStatus;

void initDB(){
    rocksdb::Options options;
    options.create_if_missing = true;
    dbStatus = rocksdb::DB::Open(options, "quarks_db", &db);
        
}

void closeDB(){
    // close the database
    delete db;
}

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

Core::Core(){
    initDB();
}

Core::~Core(){
    closeDB();
}

void Core::setEnvironment(int argc, std::string argv){
    _argv = argv;
    _argc = argc;
}

bool Core::put(std::string body, std::string& out) {

    auto x = crow::json::load(body);        
    if (!x){
    	CROW_LOG_INFO << "invalid put body" << body;
        //out = "invalid put body";
	 
	out = "{\"error\": \"Invalid put parameters\"}";

	return false;
	
    }

    std::string key = x["key"].s();
    //std::string value = x["value"].s();  
    auto r = x["value"];
    if(!r){
	out = "{\"error\": \"Parameter 'value' missing\"}";
	return false;
    }
    
    crow::json::wvalue w = std::move(r);
    std::string value = crow::json::dump(w);

    CROW_LOG_INFO << "put body : " << body << "key : " << key << ", value : " << value << "\n";
        
    bool ret = false;    
        
    rocksdb::Slice keySlice = key;
        
    // modify the database
    if (dbStatus.ok()){
	ret = true;

        rocksdb::Status status = db->Put(rocksdb::WriteOptions(), keySlice, value);
        if(!status.ok()){
            key = "{\"error\":\"data failed to save\"}";
	    ret = false;
        }
    
    }else{
        key = "{\"error\":\"db status error\"}";
        ret = false;
    }

    //std::stringstream ss;
    //ss<< "{" << std::quoted("result") << ":" << std::quoted(key) << "}";    
    if(ret){
    	out = std::string("{") + R"("result")" + std::string(":\"")  + key + std::string("\"}");
    }

    return  ret;
    
}


bool Core::putJson(std::string key, crow::json::rvalue& x, crow::json::wvalue& out) {
    
    bool ret = true;
    
    crow::json::wvalue w = std::move(x);
    std::string value = crow::json::dump(w);
    
    rocksdb::Slice keySlice = key;
        
    // modify the database
    if (dbStatus.ok()){
	
        rocksdb::Status status = db->Put(rocksdb::WriteOptions(), keySlice, value);
        if(!status.ok()){
            key = "";
	    ret = false;
        }
    
    }else{
        key = "";
        ret = false;
    }

    std::stringstream ss;
    ss<< "{" << std::quoted("result") << ":" << std::quoted(key) << "}";    
    
    out = crow::json::load(ss.str());

    return  ret;
    
}

bool Core::getJson(std::string key, crow::json::wvalue& out){
    
    bool ret = false;
    
    /*std::map<std::string, crow::json::rvalue>::iterator it = _Core.find(key);
    if (it != _Core.end()){
        out = it->second;
        ret = true;
    }*/
    if (dbStatus.ok()){
        std::string value;
        rocksdb::Status status = db->Get(rocksdb::ReadOptions(), key, &value);
    
        if(status.ok()){
            out =  crow::json::load(value);
            ret = true;
        }
    }
    
    return ret;
    
}

bool Core::get(std::string key, std::string& value){
    
    bool ret = false;
    
    if (dbStatus.ok()){
        
        rocksdb::Status status = db->Get(rocksdb::ReadOptions(), key, &value);
        
        if(status.ok()){
            ret = true;
        }
    }
    
    return ret;
    
}

bool Core::iterJson(std::string wild, std::vector<crow::json::wvalue>& matchedResults) {

    /*for (std::map<std::string, crow::json::rvalue>::iterator it = _Core.begin();
         it != _Core.end(); ++it){
            if(wildcmp(wild.c_str(), it->first.c_str()) ){
                crow::json::wvalue w;
                w = it->second;
                CROW_LOG_INFO << "w : " << crow::json::dump(w);
                
                matchedResults.push_back(std::move(w));
            }
    }*/
    
    if (dbStatus.ok()){
    // create new iterator
        rocksdb::Iterator* it = db->NewIterator(rocksdb::ReadOptions());
        
        // iterate all entries
        for (it->SeekToFirst(); it->Valid(); it->Next()) {
            //CROW_LOG_INFO << "iterate : " << it->key().ToString()
            //<< ": " << it->value().ToString() ; //<< endl;
            
            if(wildcmp(wild.c_str(), it->key().ToString().c_str())){
                crow::json::wvalue w;
                auto x = crow::json::load(it->value().ToString());
                //CROW_LOG_INFO << "w : " << crow::json::dump(w);

		if(!x){
		    w =  crow::json::load(std::string("[\"") + 
						it->value().ToString() + std::string("\"]"));

		}else{
		    w = x;
		}
                
                matchedResults.push_back(std::move(w));
                
            }
        }
        //assert(it->status().ok());  // check for any errors found during the scan
        
        return it->status().ok();
    }
    
    return false;
    
}

bool Core::searchJson(crow::json::rvalue& args,
                       std::vector<crow::json::wvalue>& matchedResults) {
    
    
    if (dbStatus.ok()){
        
        Core& core = *this;
        
        v8Engine ve([](std::string log){
            CROW_LOG_INFO << log.c_str();
        });
        
        std::string sArgs =  crow::json::dump(args);
        CROW_LOG_INFO << "args : " << sArgs;
        
        std::string wild = args["keys"].s();
        CROW_LOG_INFO << "keys : " << wild.c_str();
        
        crow::json::wvalue wArgs;
        wArgs["include"] = args["include"];
        CROW_LOG_INFO << "include : " << crow::json::dump(wArgs);
        
        crow::json::wvalue wFilter;
        wFilter["include"]= std::move(wArgs["include"]);
        CROW_LOG_INFO << "include : " << crow::json::dump(wFilter["include"]);
        
        std::string sFilter = crow::json::dump(wFilter["include"]);
        crow::json::rvalue filter = crow::json::load(sFilter);
        
        std::string mapField = filter["map"]["field"].s();
        std::string mapAs = filter["map"]["as"].s();
        
        std::string moduleName = filter["module"].s();
        std::string funcName = filter["filter"].s();
        std::string params = filter["params"].s();
        
        auto funcOnV8EngineLoaded = [&core, &ve, &wild, &funcName, &params, &mapField, &mapAs, &matchedResults]
            (v8Engine::v8Context& v8Ctx) -> int {
            
            // create new iterator
            rocksdb::Iterator* it = db->NewIterator(rocksdb::ReadOptions());
            
            // iterate all entries
            for (it->SeekToFirst(); it->Valid(); it->Next()) {
                
                std::string elem = it->value().ToString();
                
                //CROW_LOG_INFO << "iterate : " << it->key().ToString()
                //<< ": " <<  elem; //<< endl;
                
                if(wildcmp(wild.c_str(), it->key().ToString().c_str())){
                    //crow::json::rvalue r;
                    auto r = crow::json::load(it->value().ToString());
		    if(!r){
		    	r =  crow::json::load(std::string("[\"") + 
						it->value().ToString() + std::string("\"]"));


		    }

                    
                    //CROW_LOG_INFO << "r : " << elem;
                    
                    std::string mapKey = r[mapField.c_str()].s();
                    crow::json::wvalue mapJson;
                    
                    crow::json::wvalue w = r;
                    if(core.getJson(mapKey, mapJson)){
                        w[mapAs] = std::move(mapJson);
                    }
                    
                    //std::string allow = ve.invoke("matcher", sfilter.c_str(), //filterParams.c_str());
                    std::string e = crow::json::dump(w);
                    std::string allow = ve.invoke(v8Ctx, funcName, e, params.c_str());
                    if(atoi(allow.c_str()) == 1){
                        matchedResults.push_back(std::move(w));
                    }
                    
                }
            }
            
            return (it->status().ok());
                
            
        }; // funcOnV8EngineLoaded
        
        std::string moduleJS = moduleName + ".js";
        return ve.load(moduleJS, funcOnV8EngineLoaded);
        
    }
    
    return false;
    
}

bool Core::fileTransfer(std::string moduleName, std::string funcName, std::string channelName,
		 std::string remoteDescription) {
    
	Core& core = *this;
        
        v8Engine ve([](std::string log){
            CROW_LOG_INFO << log.c_str();
        });
        
        
        auto v8Loaded = [&core, &ve, &funcName, &channelName, &remoteDescription]
            (v8Engine::v8Context& v8Ctx) -> int {
            
            std::string rawData = ve.invoke(v8Ctx, funcName, channelName, remoteDescription);
            
            //writeFile(fileName, rawData);
	    CROW_LOG_INFO << rawData;
                
	    return (rawData.size() > 0);
            
        }; 
        
        std::string moduleJS = moduleName + ".js";
	CROW_LOG_INFO << moduleJS;

        return ve.load(moduleJS, v8Loaded);        
      // return true;
}

