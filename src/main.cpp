#include <base64.hpp>

#ifdef _USE_PLUGINS
#include <filter.hpp>
#endif

#include <main.hpp>

#include <quarks.hpp>

#include <v8engine.hpp>

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
        auto x = crow::json::load(req.body);
        if (!x)
            return crow::response(400);
        //int sum = x["a"].i()+x["b"].i();
        
        std::string s = x["key"].s();
        crow::json::rvalue v = x["value"];
        
        
        //crow::json::wvalue w = std::move(x);
        //CROW_LOG_INFO << "w : " << crow::json::dump(w);
        
        
        std::string result = Quarks::Core::_Instance.putJson(s, v);
        //std::string result = Quarks::Core::CoreJson(x["a"].s(), x);
        
        std::ostringstream os;
        //os << result << sum;
        os << result;
        
        return crow::response{os.str()};
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
        //int sum = x["a"].i()+x["b"].i();
        
        try{
            std::string key = x["key"].s();
            Quarks::Core::_Instance.getJson(key, out);
            
        }catch (const std::runtime_error& error){
             out["error"] = "parameter 'key' missing";
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
        
        //int sum = x["a"].i()+x["b"].i();
        try{
            std::string wild = x["keys"].s();
            CROW_LOG_INFO << "wild-card : " << wild;
            
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
            w["error"] = "invalid filter value";
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
    
#ifdef _USE_PLUGINS
    CROW_ROUTE(app, "/filter/gaussian")
        .methods("POST"_method)(route_filter_gaussian_callback);
    
    CROW_ROUTE(app, "/filter/adjust")
        .methods("POST"_method)(route_filter_adjust_callback);
    
    CROW_ROUTE(app, "/filter/clahe")
        .methods("POST"_method)(route_filter_clahe_callback);
#endif
    
    CROW_ROUTE(app, "/quarks/core/putjson")
    .methods("POST"_method)(route_core_putjson_callback);
  
    CROW_ROUTE(app, "/quarks/core/getjson")
    .methods("GET"_method, "POST"_method)(route_core_getjson_callback);
    
    CROW_ROUTE(app, "/quarks/core/iterjson")
    .methods("GET"_method, "POST"_method)(route_core_iterjson_callback);
    
    CROW_ROUTE(app, "/quarks/core/searchjson")
    .methods("GET"_method, "POST"_method)(route_core_searchjson_callback);
    
    
    //auto& v = Quarks::Matrix::_Instance; // we will work with the matrix data struct
                                            // in later api calls
    
    auto resourceLoader = [](crow::mustache::context& x, std::string filename) {
        char name[256];
        gethostname(name, 256);
        x["servername"] = name;
        
        return crow::mustache::load(filename);
    };
    
    // html serving
    CROW_ROUTE(app, "/")
    ([&resourceLoader](){
        crow::mustache::context x;
        auto page = resourceLoader(x, "index.html");
        return page.render(x);
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
    
    std::cout << "running .." << std::endl;

    app.port(18080).run();
    
#ifdef _V8_LATEST
    v8EngineShutdownInMain();
#endif
    
    return 0;
}
