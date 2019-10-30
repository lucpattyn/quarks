#include <base64.hpp>
#include <fstream>

#ifdef _USE_PLUGINS
#include <filter.hpp>
#endif

#include <main.hpp>
#include <quarks.hpp>
#include <v8engine.hpp>

#include <curl/curl.h>

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

int main(int argc, char ** argv) {
   
    std::vector<std::string> arguments(argv + 1, argv + argc);

    crow::SimpleApp app;
    
    Quarks::Core::_Instance.setEnvironment(argc, argv[0]);
    
    /*v8::V8::InitializeICUDefaultLocation(argv[0]);
    v8::V8::InitializeExternalStartupData(argv[0]);
    std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());
    v8::V8::Initialize();*/
    
#ifdef _V8_LATEST
    v8EngineInitializeInMain(argc, argv);
#endif
    
#ifdef _USE_PLUGINS

    auto route_filter_gaussian_callback =
        [](const crow::request &req) {
          auto body = req.body;
          crow::response res;
          res.add_header("Content-type", "image/png");
          res.body = Core::Filter::gaussian(body);
          return res;
        };
    
    
    auto route_filter_adjust_callback =
    [](const crow::request &req) {
        auto body = req.body;
        crow::response res;
        res.add_header("Content-type", "image/png");
        res.body = Core::Filter::adjust(body);
        return res;
    };

    auto route_filter_clahe_callback =
    [](const crow::request &req) {
        auto body = req.body;
        crow::response res;
        res.add_header("Content-type", "image/png");
        res.body = Core::Filter::clahe(body);
        return res;
    };
    
#endif
    
    
    // core functionalities
    auto route_core_putjson_callback =
    [](const crow::request& req){

	crow::json::wvalue out;

        auto x = crow::json::load(req.body);        
	if (!x){	   
	   CROW_LOG_INFO << req.body;	  
	   out["error"] = "invalid post body";
	
	   return out;
        }

        std::string s = x["key"].s();
        crow::json::rvalue v = x["value"];        
               
        bool success = Quarks::Core::_Instance.putJson(s, v, out);
               
       
	/*auto res = crow::response{os.str()};
	res.add_header("Access-Control-Allow-Origin", "*");
	res.add_header("Access-Control-Allow-Methods", "GET, POST, PATCH, PUT, DELETE, OPTIONS");
	res.add_header("Access-Control-Allow-Headers", "Origin, Content-Type, X-Auth-Token");
        

        return res;*/

	if(!success){
	     out["error"] = "save failed";

	}

	return out;

    };
    
    auto route_core_getjson_callback =
    [](const crow::request& req)
    {
        crow::json::wvalue out;
        auto x = crow::json::load(req.body);
        if (!x){
            out["error"] = "invalid parameters";
            return out;
        }
                
        try{
            std::string key = x["key"].s();
            Quarks::Core::_Instance.getJson(key, out);
            
        }catch (const std::runtime_error& error){
             out["error"] = "parameter 'key' missing";
        }
        
        return out;
        
    };

    auto route_core_getkey_callback =
    [](const crow::request& req)
    {
        crow::json::wvalue out;
	//CROW_LOG_INFO <<"get_key:";
	//CROW_LOG_INFO<<req.url_params;
       
        try{
	
	    crow::json::wvalue jsonResult;
	
	    std::vector<crow::json::wvalue> jsonResults;

	    auto x = req.url_params.get("key");
	    std::string key = (x == nullptr ? "" : x); 

	    if(key.size() > 0){           
                Quarks::Core::_Instance.getJson(key, jsonResult);
	    }

	    auto k = req.url_params.get("keys");
	    std::string wild = (k == nullptr ? "" : k);
            //CROW_LOG_INFO << "wild-card : " << wild;
            
            
	    if(wild.size() > 0){
            	Quarks::Core::_Instance.iterJson(wild, jsonResults);
	    }

	    if(key.size() > 0){
	    	jsonResults.push_back(std::move(jsonResult));
	    }
            
            if(jsonResults.size() > 0){
                out["result"] = std::move(jsonResults);
            }
            
        }catch (const std::runtime_error& error){
             out["error"] = "parameter 'key' or 'keys' missing";
        }
        
        return out;
        
    };

    auto route_core_getkeys_callback =
    [](const crow::request& req)
    {
        crow::json::wvalue out;
        
	try{
	    auto x = req.url_params.get("keys");
	    std::string wild = (x == nullptr ? "" : x);
            //CROW_LOG_INFO << "wild-card : " << wild;
            
            std::vector<crow::json::wvalue> jsonResults;

	    if(wild.size() > 0){
            	Quarks::Core::_Instance.iterJson(wild, jsonResults);
	    }
            
            if(jsonResults.size()){
                out["result"] = std::move(jsonResults);
            }
            
        }catch (const std::runtime_error& error){
            out["error"] = "parameter 'keys' missing";
        }
        
        return out;
        
    };


    
    auto route_core_iterjson_callback =
    [](const crow::request& req)
    {
        crow::json::wvalue out;
        out["result"] = "";
        
        auto x = crow::json::load(req.body);
        if (!x){
            out["error"] = "invalid parameters";
            return out;
        }        
        
        try{
            std::string wild = x["keys"].s();
            //CROW_LOG_INFO << "wild-card : " << wild;
            
            std::vector<crow::json::wvalue> jsonResults;
            Quarks::Core::_Instance.iterJson(wild, jsonResults);
            
            if(jsonResults.size()){
                out["result"] = std::move(jsonResults);
            }
            
        }catch (const std::runtime_error& error){
            out["error"] = "parameter 'keys' missing";
        }
        
        return out;
        
    };
    
    auto route_core_searchjson_callback =
    [](const crow::request& req){
        
        crow::json::wvalue w;
        w["result"] = "";
        
        auto x = crow::json::load(req.body);
        if (!x){
            w["error"] = "invalid parameters";
            return w;
        }
        
        std::vector<crow::json::wvalue> jsonResults;
        Quarks::Core::_Instance.searchJson(x, jsonResults);
        
        if(jsonResults.size()){
            //w["result"] = jsonResults[0].s();
            //CROW_LOG_INFO << "jsonResults[0] : "
            //   <<  crow::json::dump(jsonResults[0])
            
            w["result"] = std::move(jsonResults);
        }
        
        return w;
        
    };    
    
    auto route_ai_callback =
    [](const crow::request& req){
        const char* q = req.url_params.get("msg");
        
        std::string url = std::string("https://acobot-brainshop-ai-v1.p.rapidapi.com/get?bid=178&key=sX5A2PcYZbsN5EY6&uid=mashape&msg=") + q;
        
        CURL *curl;
        CURLcode cres;
        std::string readBuffer;
        
        curl = curl_easy_init();
        if(curl) {
            
            struct curl_slist *headers = NULL;
            headers = curl_slist_append(headers, "Accept: application/json");
            headers = curl_slist_append(headers, "Content-Type: application/json; charset=utf-8");
            headers = curl_slist_append(headers, "X-RapidAPI-Key: 89cd5a2676msh7cf7274830b66fbp1e09d8jsn21a8f654a6e9");
            
            
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            
            //curl_easy_setopt(curl, CURLOPT_POST, 1);
            //curl_easy_setopt(curl, CURLOPT_POSTFIELDS, R"({\"queryInput\":{\"text\":{\"text\":\"any good jokes\",\"languageCode\":\"en\"}},\"queryParams\":{\"timeZone\":\"Asia/Dhaka\"}})");
            
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            cres = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            
            std::cout << readBuffer << std::endl;
        }
        
        std::ostringstream os;
        os << readBuffer;
        
        auto res = crow::response{os.str()};
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, PATCH, PUT, DELETE, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Origin, Content-Type, X-Auth-Token");
        
        return res;
    };
    
    CROW_ROUTE(app, "/quarks/core/putjson")
    .methods("POST"_method)(route_core_putjson_callback);
  
    CROW_ROUTE(app, "/quarks/core/getjson")
    .methods("GET"_method, "POST"_method)(route_core_getjson_callback);


    CROW_ROUTE(app, "/quarks/core/get")
    (route_core_getkey_callback);

    CROW_ROUTE(app, "/quarks/core/getall")
    (route_core_getkeys_callback);

    
    CROW_ROUTE(app, "/quarks/core/iterjson")
    .methods("GET"_method, "POST"_method)(route_core_iterjson_callback);
    
    CROW_ROUTE(app, "/quarks/core/searchjson")
    .methods("GET"_method, "POST"_method)(route_core_searchjson_callback);
    
    
    //auto& v = Quarks::Matrix::_Instance; // we will work with the matrix data struct
                                            // in later api calls
    
    auto resourceLoader = [](crow::mustache::context& x, std::string filename,
				 const char* base = nullptr) {
        char name[256];
        gethostname(name, 256);
        x["servername"] = name;

	crow::mustache::set_base(".");
	if(base == nullptr){
	    crow::mustache::set_base("templates"); 
	}else{
	    crow::mustache::set_base(base);
	}           
        
        return crow::mustache::load(filename);
    };
    
    // generic file serving from templates

    auto readFile = [](std::string name){
	
		std::ifstream f(name.c_str(), std::ifstream::in|std::ifstream::binary|std::ifstream::ate);
		if(!f.is_open()) {return std::string("");};
		std::streampos size = f.tellg();
		char* image = new char [size];
		f.seekg (0, std::ifstream::beg);
		f.read (image, size);
		f.close();

		std::string s(image, size);

		delete[] image;

		return s;

	
    	/*FILE* file = fopen(name.c_str(), "rb");
    	if (file == NULL) return std::string("");
    
    	fseek(file, 0, SEEK_END);
    	int size = ftell(file);
    	rewind(file);
    
    	char* chars = new char[size + 1];
        chars[size] = '\0';
	
    	for (int i = 0; i < size;)
    	{
        	int read = fread(&chars[i], 1, size - i, file);
        	i += read;
    	}
    	fclose(file);

        std::string strChars(chars, size);
        delete[] chars;

        return strChars;*/

    };

    auto defaultPageLoader = [&resourceLoader, &readFile](const crow::request& req, std::string defaultPage){
        
        std::string result;
        
        const char* q = req.url_params.get("q");
        
        //bool useMustache = false;
        bool stylesheet = false;
        
        if(q == nullptr){
            crow::mustache::context x;
            auto page = resourceLoader(x, defaultPage);
            result = page.render(x);
            
        }else{
            std::string asset = q;
            
            if(asset.size() > 4){
                size_t css = asset.size() - 4;
                //CROW_LOG_INFO << asset.substr(css);
                if(!asset.substr(css).compare(".css")){
                    stylesheet = true;
                }
            }
            
            /*
             
             std::size_t found = asset.find_last_of("/\\");
             std::string fileName = asset.substr(found + 1);
             
             if(found != std::string::npos) {
             if (fileName.find(".css") != std::string::npos){
             useMustache = true;
             }
             
             }
             
             
             if(useMustache){
             std::string base = asset.substr(0, found);
             //CROW_LOG_INFO << base;
             crow::mustache::context x;
             auto page = resourceLoader(x, fileName, base.c_str());
             result = page.render(x);
             
             }else{
             result = readFile(asset);
             }*/
            
            result = readFile(asset);
        }
        
        std::ostringstream os;
        os << result;
        
        //CROW_LOG_INFO << result;
        
        auto res = crow::response{os.str()};
        if(stylesheet){
            //CROW_LOG_INFO << "stylesheet requested";
            res.add_header("Content-type", "text/css");
        }
        
        return res;
        
    };
    
    CROW_ROUTE(app, "/")
    ([&defaultPageLoader](const crow::request& req){
        auto res = defaultPageLoader(req, "index.html");
        return res;
    });


    // html serving
    /*CROW_ROUTE(app, "/")
    ([&resourceLoader](){
        crow::mustache::context x;	
        auto page = resourceLoader(x, "index.html");
        return page.render(x);
    });*/
    
    CROW_ROUTE(app, "/console")
    ([&defaultPageLoader](const crow::request& req){
        //crow::mustache::context x;
        //auto page = resourceLoader(x, "console/index.html");
        //return page.render(x);
        auto res = defaultPageLoader(req, "console/index.html");
        return res;
    });
    
    CROW_ROUTE(app, "/home")
    ([&resourceLoader](){
        crow::mustache::context x;
        auto page = resourceLoader(x, "home.html");
        return page.render(x);
    });

        
    CROW_ROUTE(app, "/home/<int>")
    ([&resourceLoader](int resId){
        crow::mustache::context x;
        auto page = resourceLoader(x, std::to_string(resId) + ".html");
        return page.render(x);
    });
    
    // js serving
    CROW_ROUTE(app, "/js/<int>")
    ([&resourceLoader](int resId){
        crow::mustache::context x;
        auto page = resourceLoader(x, std::to_string(resId) + ".js");
        return page.render(x);
    });
    
    // css serving
    CROW_ROUTE(app, "/css/<int>")
    ([&resourceLoader](int resId){
        crow::mustache::context x;
        auto page = resourceLoader(x, std::to_string(resId) + ".css");
        return page.render(x);
    });


#ifdef _USE_PLUGINS
    CROW_ROUTE(app, "/filter/gaussian")
        .methods("POST"_method)(route_filter_gaussian_callback);
    
    CROW_ROUTE(app, "/filter/adjust")
        .methods("POST"_method)(route_filter_adjust_callback);
    
    CROW_ROUTE(app, "/filter/clahe")
        .methods("POST"_method)(route_filter_clahe_callback);
#endif

// AI calls
    
    CROW_ROUTE(app, "/ai")
    .methods("GET"_method)(route_ai_callback);
    
    
    std::cout << "running .." << std::endl;

    app.port(18080).run();
    
#ifdef _V8_LATEST
    v8EngineShutdownInMain();
#endif
    
    return 0;
}
