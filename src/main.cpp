#include <base64.hpp>
#include <fstream>

#ifdef _USE_PLUGINS
#include <filter.hpp>
#endif

#include <main.hpp>
#include <quarks.hpp>
#include <v8engine.hpp>

#include <mutex>


#ifdef _USE_RAPIDAPI

#include <curl/curl.h>
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

#endif

static void handle_runtime_error(const std::runtime_error& error, crow::json::wvalue& out,
   std::vector<crow::json::wvalue>& jsonResults)
{
    std::string errs = R"([{"error" : "runtime parsing error"},)";
    
    errs += "[";
    errs += error.what();
    errs += "],";
    for(size_t i = 0; i < jsonResults.size(); i++){
        errs += crow::json::dump(jsonResults[i]);
    }
    
    errs += "]";
    out["error"] = errs;
}

struct QueryParams{
    std::string wild;
    std::string skip;
    std::string limit;
    
    static QueryParams Parse(const crow::request& req){
        QueryParams q;
        auto x = req.url_params.get("keys");
        q.wild = (x == nullptr || !strlen(x) ? "" : x);
        
        auto s = req.url_params.get("skip");
        q.skip = (s == nullptr || !strlen(s) ? "0" : s);
        
        q.limit = "10";
        auto l = req.url_params.get("limit");
        q.limit = (l == nullptr || !strlen(l)? "-1" : l);
      
        //CROW_LOG_INFO << "qp >> wild: " << q.wild << ", skip: " << q.skip << ", limit: " << q.limit;
        
        return q;
    }

};

class HackWebSocketRule : public crow::BaseRule
{
    using self_t = HackWebSocketRule;
public:
    HackWebSocketRule(std::string rule)
    : BaseRule(std::move(rule))
    {
        
    }
    
    void validate() override
    {
    }
    
    void handle(const crow::request&, crow::response& res, const crow::routing_params&) override
    {
        res = crow::response(404);
        res.end();
    }
    
    void handle_upgrade(const crow::request& req, crow::response&, crow::SocketAdaptor&& adaptor) override
    {
        CROW_LOG_INFO << "custom upgrade ";
        
        crow::websocket::Connection<crow::SocketAdaptor>* conn =
            new crow::websocket::Connection<crow::SocketAdaptor>(req, std::move(adaptor), open_handler_, message_handler_, close_handler_, error_handler_, accept_handler_);
        
        conn->userdata((void*)&req);
    }
#ifdef CROW_ENABLE_SSL
    void handle_upgrade(const crow::request& req, crow::response&, crow::SSLAdaptor&& adaptor) override
    {
        new crow::websocket::Connection<SSLAdaptor>(req, std::move(adaptor), open_handler_, message_handler_, close_handler_, error_handler_, accept_handler_);
    }
#endif
    
    template <typename Func>
    self_t& onopen(Func f)
    {
        open_handler_ = f;
        return *this;
    }
    
    template <typename Func>
    self_t& onmessage(Func f)
    {
        message_handler_ = f;
        return *this;
    }
    
    template <typename Func>
    self_t& onclose(Func f)
    {
        close_handler_ = f;
        return *this;
    }
    
    template <typename Func>
    self_t& onerror(Func f)
    {
        error_handler_ = f;
        return *this;
    }
    
    template <typename Func>
    self_t& onaccept(Func f)
    {
        accept_handler_ = f;
        return *this;
    }
    
    
protected:
    std::function<void(crow::websocket::connection&)> open_handler_;
    std::function<void(crow::websocket::connection&, const std::string&, bool)> message_handler_;
    std::function<void(crow::websocket::connection&, const std::string&)> close_handler_;
    std::function<void(crow::websocket::connection&)> error_handler_;
    std::function<bool(const crow::request&)> accept_handler_;
    
public:
    void reset(HackWebSocketRule* p){
        rule_to_upgrade_.reset(p);
    }
    
};

struct HackTraits : public crow::RuleParameterTraits<crow::TaggedRule<>>{
    crow::WebSocketRule& hackwebsocket() {
        using self_t = crow::TaggedRule<>;
        auto p =new crow::WebSocketRule("/ws");
        //((self_t*)this)->rule_to_upgrade_.reset(p);
        return *p;
    }
};

int main(int argc, char ** argv) {   
   
    crow::SimpleApp app;
    
    Quarks::Core::_Instance.setEnvironment(argc, argv);
    
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
    
    std::mutex mtx;
    std::unordered_set<crow::websocket::connection*> users;
    
    crow::RuleParameterTraits<crow::TaggedRule<>>& traits = CROW_ROUTE(app, "/ws");
    ((HackTraits*)&traits)->websocket()
    .onopen([&](crow::websocket::connection& conn){
        CROW_LOG_INFO << "new websocket connection";
        std::lock_guard<std::mutex> _(mtx);
        users.insert(&conn);
    })
    .onclose([&](crow::websocket::connection& conn, const std::string& reason){
        CROW_LOG_INFO << "websocket connection closed: " << reason;
        std::lock_guard<std::mutex> _(mtx);
        users.erase(&conn);
    })
    .onmessage([&](crow::websocket::connection& /*conn*/, const std::string& data, bool is_binary){
        std::lock_guard<std::mutex> _(mtx);
        for(auto u:users)
            if (is_binary)
                u->send_binary(data);
            else
                u->send_text(data);
    });
    
///////////////////////////////////////////
/////////// core functionalities //////////
///////////////////////////////////////////
    auto post = [](std::string body, std::string& out)
    {
        
        CROW_LOG_INFO << body;
        
        /*auto res = crow::response{os.str()};
         res.add_header("Access-Control-Allow-Origin", "*");
         res.add_header("Access-Control-Allow-Methods", "GET, POST, PATCH, PUT, DELETE, OPTIONS");
         res.add_header("Access-Control-Allow-Headers", "Origin, Content-Type, X-Auth-Token");
         
         
         return res;*/

        
        bool success = Quarks::Core::_Instance.post(body, out);
        
        return success;
        
        
    };
    
    auto put = [](std::string body, std::string& out)
    {
        CROW_LOG_INFO << body;
	
        bool success = Quarks::Core::_Instance.put(body, out);
       
        
    	return success;


    };
   
    auto putatom = [](std::string body, std::string& out)
    {
        CROW_LOG_INFO << body;
        
        bool success = Quarks::Core::_Instance.putAtom(body, out);
        
        return success;
        
    };
    
    auto removeatom = [](std::string body, std::string& out)
    {
        CROW_LOG_INFO << body;
        
        bool success = Quarks::Core::_Instance.removeAtom(body, out);
        
        return success;
        
    };
    
    auto atom = [](std::string body, std::string& out)
    {
        CROW_LOG_INFO << body;
        
        bool success = Quarks::Core::_Instance.atom(body, out);
        
        return success;
        
    };


    auto route_core_postjson_callback =
    [post](const crow::request& req){
        
        std::string out;
        post(req.body, out);
        
        return out;
    };

    
    auto route_core_put_callback =
    [put](const crow::request& req){

        std::string out;

        std::string body = req.body;

        auto x = req.url_params.get("body");
        if(x != nullptr){
            body = x;
        }

        put(body, out);

        return out;

    };
    
    auto route_core_putatom_callback =
    [putatom](const crow::request& req){
        
        std::string out;
        
        std::string body = req.body;
        
        auto x = req.url_params.get("body");
        if(x != nullptr){
            body = x;
        }
        
        putatom(body, out);
        
        return out;
        
    };

    auto route_core_existkey_callback =
    [](const crow::request& req)
    {
        std::string out = "";
        //CROW_LOG_INFO <<"get_key:";
        //CROW_LOG_INFO<<req.url_params;
        
        try{
            
            auto x = req.url_params.get("key");
            std::string key = (x == nullptr ? "" : x);
            
            if(key.size() > 0){
                Quarks::Core::_Instance.exists(key, out);
            }else{
                out = "{\"error\": \"parameter 'key' missing\"}";
            }
            
            
        } catch (const std::runtime_error& error){
            out = R"({"error":"parsing error"})";
        }
        
        return out;
        
    };
    
    auto route_core_getkey_callback =
    [](const crow::request& req)
    {
        std::string out = "";
        //CROW_LOG_INFO <<"get_key:";
        //CROW_LOG_INFO<<req.url_params;
       
        try{	
	   
            auto x = req.url_params.get("key");
            std::string key = (x == nullptr ? "" : x);

            if(key.size() > 0){
                Quarks::Core::_Instance.get(key, out);
            }else{
                out = "{\"error\": \"parameter 'key' missing\"}";
            }
                        
            
        } catch (const std::runtime_error& error){
             out = R"({"error":"parsing error"})";
        }
        
        return out;
        
    };
    
    auto route_core_getall_callback =
    [](const crow::request& req)
    {
        crow::json::wvalue out;
        
        std::vector<crow::json::wvalue> jsonResults;
        
        try{
            auto q = QueryParams::Parse(req);
            
            CROW_LOG_INFO << "wild: " << q.wild << ", skip: " << q.skip << ", limit: " << q.limit;
            
            bool ret = false;
            if(q.wild.size() > 0){
                //Quarks::Core::_Instance.iterJson(wild, jsonResults);
                ret = Quarks::Core::_Instance.getAll(q.wild, jsonResults, std::stoi(q.skip), std::stoi(q.limit));
                
                out["result"] = std::move(jsonResults);
                if(!ret){
                    out["error"] = R"({"error" : "query returned error"})";
                }
                
            }else{
                out["error"] = "{\"error\":\"parameter 'keys' missing\"}";

            }
            
        } catch (const std::runtime_error& error){
            handle_runtime_error(error, out, jsonResults);
        }
        
        return out;
        
    };
    
    auto route_core_getsorted_callback =
    [](const crow::request& req)
    {
        crow::json::wvalue out;
        
        std::vector<crow::json::wvalue> jsonResults;
        
        try{
           
            auto q =  QueryParams::Parse(req);
            
            auto sb = req.url_params.get("sortby");
            std::string sortby = (sb == nullptr || !strlen(sb)) ? "" : sb;
            
            auto des = req.url_params.get("des");
            std::string descending = (des == nullptr || !strlen(des))  ? "false" : des;
            
            CROW_LOG_INFO << "wild: " << q.wild << ", sortby: " << sortby << ", des: " << descending
                << ", skip: " << q.skip << ", limit: " << q.limit;
            
            bool ret = false;
            if(q.wild.size() > 0){
                bool ascending = (descending.compare("true") == 0) ? false : true;
                
                ret = Quarks::Core::_Instance.getSorted(q.wild, sortby, ascending, jsonResults, std::stoi(q.skip), std::stoi(q.limit));
                
                out["result"] = std::move(jsonResults);
                if(!ret){
                    out["error"] = R"({"error" : "query returned error"})";
                }
                
            }else{
                out["error"] = "{\"error\":\"parameter 'keys' missing\"}";

            }
            
        } catch (const std::runtime_error& error){
            handle_runtime_error(error, out, jsonResults);
        }
        
        return out;
        
    };
    
    auto route_core_getkeys_callback =
    [](const crow::request& req)
    {
        crow::json::wvalue out;
        
        std::vector<crow::json::wvalue> jsonResults;
        
        try{
            auto q = QueryParams::Parse(req);
            
            CROW_LOG_INFO << "wild: " << q.wild << " (size: " << q.wild.size() << "), skip: " << q.skip << ", limit: " << q.limit;
            
            bool ret = false;
            if(q.wild.size() > 0){
                auto rev = req.url_params.get("reverse");
                std::string reverse = (rev == nullptr || !strlen(rev))  ? "false" : rev;
                
                if(!reverse.compare("true")){
                    ret = Quarks::Core::_Instance.getKeysReversed(q.wild, jsonResults,
                                                          std::stoi(q.skip), std::stoi(q.limit));
                }else{
                    ret = Quarks::Core::_Instance.getKeys(q.wild, jsonResults,
                                                          std::stoi(q.skip), std::stoi(q.limit));
                }
                
                out["result"] = std::move(jsonResults);
                if(!ret){
                    out["error"] = R"({"error" : "query returned error"})";
                }
                
            }else{
                out["error"] = "{\"error\":\"parameter 'keys' missing\"}";
                
            }
            
        } catch (const std::runtime_error& error){
            handle_runtime_error(error, out, jsonResults);
        }
        
        return out;
        
    };
    
    auto route_core_getcount_callback =
    [](const crow::request& req)
    {
        std::string out;
        long count = 0;
        
        try{
           
            auto q = QueryParams::Parse(req);
           
            bool ret = false;
            if(q.wild.size() > 0){
                //Quarks::Core::_Instance.iterJson(wild, jsonResults);
                ret = Quarks::Core::_Instance.getCount(q.wild, count, std::stoi(q.skip), std::stoi(q.limit));
                out = std::to_string(count);
                
            }else{
                 out = "{\"error\": \"parameter 'key' missing\"}";
            }
            
            if(!ret){
                out = R"({"error" : "counting error"})";
            }
            
        } catch (const std::runtime_error& error){
             out = error.what();
        }
        
        return out;
        
    };
    
    
    auto route_core_iter_callback =
    [](const crow::request& req)
    {
        std::string out = "[";
        try{
            auto q = QueryParams::Parse(req);
            
            CROW_LOG_INFO << "wild: " << q.wild << ", skip: " << q.skip << ", limit: " << q.limit;
            
            std::vector<std::string> strResults;
            Quarks::Core::_Instance.iter(q.wild, strResults, std::stoi(q.skip), std::stoi(q.limit));
            
            for(auto v : strResults){
                out += v;
                out += ",";
            }
            
            out[out.size() -1] = ']'; // replace the last ',' with ']'
            
        } catch (const std::runtime_error& error){
            out = "runtime error during iteration";
        }
        
        return out;
        
    };
    
    auto route_core_removekey_callback =
    [](const crow::request& req)
    {
        std::string out = "";
        //CROW_LOG_INFO <<"get_key:";
        //CROW_LOG_INFO<<req.url_params;
        
        try{
            
            auto x = req.url_params.get("key");
            std::string key = (x == nullptr ? "" : x);
            
            if(key.size() > 0){
                Quarks::Core::_Instance.remove(key, out);
                
            }else{
                out = "{\"error\": \"parameter 'key' missing\"}";
            }
            
            
        } catch (const std::runtime_error& error){
            out = error.what();
        }
        
        return out;
        
    };
    
    auto route_core_removekeys_callback =
    [](const crow::request& req)
    {
        crow::json::wvalue out;
        
        try{
            auto q = QueryParams::Parse(req);
            
            CROW_LOG_INFO << "wild: " << q.wild << ", skip: " << q.skip << ", limit: " << q.limit;
           
            if(q.wild.size() > 0){
                //Quarks::Core::_Instance.iterJson(wild, jsonResults);
                int removeCount = Quarks::Core::_Instance.removeAll(q.wild, std::stoi(q.skip), std::stoi(q.limit));
                out["result"] = removeCount;
                
            }else{
                out["error"] = "{\"error\":\"parameter 'keys' missing\"}";
            }
            
        } catch (const std::runtime_error& error){
            out["error"] = error.what();
        }
        
        return out;
        
    };
    
    auto route_core_removeatom_callback =
    [removeatom](const crow::request& req){
        
        std::string out;
        
        std::string body = req.body;
        
        auto x = req.url_params.get("body");
        if(x != nullptr){
            body = x;
        }
        
        removeatom(body, out);
        
        return out;
        
    };

    /*auto putjson = [](std::string body, crow::json::wvalue& out)
    {	

	    auto x = crow::json::load(body);
        if (!x){
            CROW_LOG_INFO << body;
            out["error"] = "invalid post body";

            return;
	
        }

        bool success = false;
        
        try{
            std::string s = x["key"].s();
            crow::json::rvalue v = x["value"];
            
            success = Quarks::Core::_Instance.putJson(s, v, out);
            
        } catch (const std::runtime_error& error){
            out["error"] = error.what();
        }
       
        //auto res = crow::response{os.str()};
        //res.add_header("Access-Control-Allow-Origin", "*");
        //res.add_header("Access-Control-Allow-Methods", "GET, POST, PATCH, PUT, DELETE, OPTIONS");
        //res.add_header("Access-Control-Allow-Headers", "Origin, Content-Type, X-Auth-Token");
         

        //return res;

        if(!success){
            out["error"] = "save failed";

        }


    };*/

    auto route_core_putjson_callback =
    [put](const crow::request& req){

        //crow::json::wvalue out;
        //putjson(req.body, out);

        std::string out;
        put(req.body, out);
        
        return out;

        /*auto x = crow::json::load(req.body);        
        if (!x){
            CROW_LOG_INFO << req.body;
            out["error"] = "invalid post body";
	
            return out;
        }

        std::string s = x["key"].s();
        crow::json::rvalue v = x["value"];        
               
        bool success = Quarks::Core::_Instance.putJson(s, v, out);*/
               
        
        /*auto res = crow::response{os.str()};
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, PATCH, PUT, DELETE, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Origin, Content-Type, X-Auth-Token");
         

        return res;*/

        /*if(!success){
            out["error"] = "save failed";

        }

        return out;*/

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
            
        } catch (const std::runtime_error& error){
             out["error"] = "parameter 'key' missing";
        }
        
        return out;
        
    };
    
    auto route_core_getlist_callback =
    [](const crow::request& req)
    {
        crow::json::wvalue w;
        w["result"] = "";
        
        std::string body = req.body;
        auto p = req.url_params.get("body");
        if(p != nullptr){
            body = p;
        }
        
        auto x = crow::json::load(body);
        if (!x){
            w["error"] = "invalid parameters";
            return w;
        }
        
        auto q = QueryParams::Parse(req);
        
        std::vector<crow::json::wvalue> jsonResults;
        bool ret = Quarks::Core::_Instance.getList(x, jsonResults, std::stoi(q.skip), std::stoi(q.limit));
        
        if(jsonResults.size()){
            //w["result"] = jsonResults[0].s();
            //CROW_LOG_INFO << "jsonResults[0] : "
            //   <<  crow::json::dump(jsonResults[0])
            
            w["result"] = std::move(jsonResults);
        }
        
        if(!ret){
             w["error"] = "runtime error";
        }
        
        return w;
        
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
        
        auto q = QueryParams::Parse(req);
        
        std::vector<crow::json::wvalue> jsonResults;
        Quarks::Core::_Instance.searchJson(x, jsonResults, std::stoi(q.skip), std::stoi(q.limit));
        
        if(jsonResults.size()){
            //w["result"] = jsonResults[0].s();
            //CROW_LOG_INFO << "jsonResults[0] : "
            //   <<  crow::json::dump(jsonResults[0])
            
            w["result"] = std::move(jsonResults);
        }
        
        return w;
        
    };
    
    auto route_core_atom_callback =
    [atom](const crow::request& req){
        
        std::string out;
        
        std::string body = req.body;
        
        auto x = req.url_params.get("body");
        if(x != nullptr){
            body = x;
        }
        
        atom(body, out);
        
        return out;
        
    };

    auto route_core_filetransfer_callback=
    [](/*const crow::request& req*/){
        bool ret = Quarks::Core::_Instance.fileTransfer("filetransfer", "transfer", "sendfile", "remotedes");

        if(ret){
            return R"({"result":"transferred"})";
        }else{
            return R"({"error":"not transferred"})";
        }
	
    };
    
    auto route_core_opentcpsocket_callback=
    [](/*const crow::request& req*/){
        bool ret = Quarks::Core::_Instance.openTCPSocketClient();
        
        if(ret){
            return R"({"result":"opened socket"})";
        }else{
            return R"({"error":"socket closed"})";
        }
        
    };

    // html stuff 
    /*
    auto uriDecode = [](const std::string& in, std::string& out){
        out.clear();
        out.reserve(in.size());
        for (std::size_t i = 0; i < in.size(); ++i)
        {
            if (in[i] == '%')
            {
                if (i + 3 <= in.size())
                {
                    int value = 0;
                    std::istringstream is(in.substr(i + 1, 2));
                    if (is >> std::hex >> value)
                    {
                        out += static_cast<char>(value);
                        i += 2;
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    return false;
                }
            }
            else if (in[i] == '+')
            {
                out += ' ';
            }
            else
            {
                out += in[i];
            }
        }
        return true;
        
        
    };*/
    
    auto route_ai_callback =
    [](const crow::request& req){
        const char* q = req.url_params.get("msg");
        std::string params;
        //uriDecode(q, params);
        
        CROW_LOG_INFO << "uri params:" << params << q;

#ifdef _USE_RAPIDAPI       
        
        CURL *curl;
        //CURLcode cres;
        std::string readBuffer;
        
        curl = curl_easy_init();
        if(curl) {
            
            char *output = curl_easy_escape(curl, q, 0);
            std::string url = std::string("https://acobot-brainshop-ai-v1.p.rapidapi.com/get?bid=178&key=sX5A2PcYZbsN5EY6&uid=mashape&msg=") + output;
            
            curl_free(output);
            
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
            //cres = curl_easy_perform(curl);
            curl_easy_perform(curl);

            curl_easy_cleanup(curl);
            
            std::cout << readBuffer << std::endl;
        }
        
        std::ostringstream os;
        os << readBuffer;
#else
	std::string readBuffer  = "Feature not available";
        std::ostringstream os;
	os << readBuffer; 
#endif 
        
        auto res = crow::response{os.str()};
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, PATCH, PUT, DELETE, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Origin, Content-Type, X-Auth-Token");
        
        return res;
    };
    
    CROW_ROUTE(app, "/put")
    (route_core_put_callback);

    CROW_ROUTE(app, "/exists")
    (route_core_existkey_callback);

    CROW_ROUTE(app, "/get")
    (route_core_getkey_callback);
   
    CROW_ROUTE(app, "/getall")
    (route_core_getall_callback);
    
    CROW_ROUTE(app, "/getsorted")
    (route_core_getsorted_callback);
   
    CROW_ROUTE(app, "/getkeys")
    (route_core_getkeys_callback);
    
    CROW_ROUTE(app, "/getcount")
    (route_core_getcount_callback);
    
    CROW_ROUTE(app, "/iter")
    (route_core_iter_callback);
    
    CROW_ROUTE(app, "/remove")
    (route_core_removekey_callback);
  
    CROW_ROUTE(app, "/removeall")
    (route_core_removekeys_callback);
    
    CROW_ROUTE(app, "/remove/atom")
    .methods("GET"_method, "POST"_method)(route_core_removeatom_callback);

    CROW_ROUTE(app, "/putjson")
    .methods("POST"_method)(route_core_putjson_callback);
    
    CROW_ROUTE(app, "/postjson")
    .methods("POST"_method)(route_core_postjson_callback);
    
    CROW_ROUTE(app, "/put/atom")
    .methods("GET"_method, "POST"_method)(route_core_putatom_callback);
    
    CROW_ROUTE(app, "/atom")
     .methods("GET"_method, "POST"_method)(route_core_atom_callback);
  
    CROW_ROUTE(app, "/getjson")
    .methods("GET"_method, "POST"_method)(route_core_getjson_callback);
    
    CROW_ROUTE(app, "/getlist")
    .methods("GET"_method, "POST"_method)(route_core_getlist_callback);
    
    CROW_ROUTE(app, "/searchjson")
    .methods("GET"_method, "POST"_method)(route_core_searchjson_callback);
    
    
    CROW_ROUTE(app, "/opentcpsocket")
    .methods("GET"_method, "POST"_method)(route_core_opentcpsocket_callback);

    CROW_ROUTE(app, "/filetransfer")
    .methods("GET"_method, "POST"_method)(route_core_filetransfer_callback);
    
    
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
   
    auto defaultPageLoader = [&resourceLoader, &readFile/*, &uriDecode*/](const crow::request& req, std::string defaultPage){
        
        std::string result;
        
        const char* q = req.url_params.get("q");
        
        //bool useMustache = false;
        bool stylesheet = false;
        bool javascript = false;
        
        if(q == nullptr){
            crow::mustache::context x;
            auto page = resourceLoader(x, defaultPage);
            result = page.render(x);
            
        }else{
            std::string asset;
            asset = q;
            //uriDecode(q, asset);
            

            if(asset.size() > 3){
                size_t js = asset.size() - 3;
                //CROW_LOG_INFO << asset.substr(css);
                if(!asset.substr(js).compare(".js")){
                    javascript = true;
                }
            
            }

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
            
        } // end of else ..
        
        std::ostringstream os;
        os << result;
        
        //CROW_LOG_INFO << result;
        
        auto res = crow::response{os.str()};

        if(javascript){
           res.add_header("Content-type", "application/javascript");

        }else if(stylesheet){
            //CROW_LOG_INFO << "stylesheet requested";
            res.add_header("Content-type", "text/css");
        }else{
            res.set_header("Content-Type", "text/html; charset=UTF-8");
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
        auto res = defaultPageLoader(req, "console/index.html");
        return res;
    });
    
    CROW_ROUTE(app, "/home")
    ([&resourceLoader](){
        crow::mustache::context x;
        auto page = resourceLoader(x, "home.html");
        return page.render(x);
    });
    
    CROW_ROUTE(app, "/chat")
    ([&resourceLoader](){
        crow::mustache::context x;
        auto page = resourceLoader(x, "ws.html");
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

    app.port(Quarks::Core::_Instance.getPort()).multithreaded().run();
    
#ifdef _V8_LATEST
    v8EngineShutdownInMain();
#endif
    
    Quarks::Core::_Instance.shutDown();
    
    return 0;
}
