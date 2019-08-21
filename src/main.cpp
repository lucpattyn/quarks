#include <base64.hpp>
#include <filter.hpp>
#include <main.hpp>

#include <quarks.hpp>

int main(int argc, char ** argv) {
   
    std::vector<std::string> arguments(argv + 1, argv + argc);

    crow::SimpleApp app;

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
    
    auto route_cache_putjson_callback =
    [](const crow::request& req){
        auto x = crow::json::load(req.body);
        if (!x)
            return crow::response(400);
        //int sum = x["a"].i()+x["b"].i();
        
        std::string s = x["key"].s();
        crow::json::rvalue v = x["value"];
        
        
        //crow::json::wvalue w = std::move(x);
        //CROW_LOG_INFO << "w : " << crow::json::dump(w);
        
        
        std::string result = Quarks::Cache::_Instance.putJson(s, v);
        //std::string result = Quarks::Cache::cacheJson(x["a"].s(), x);
        
        std::ostringstream os;
        //os << result << sum;
        os << result;
        
        return crow::response{os.str()};
    };
    
    auto route_cache_getjson_callback =
    [](const crow::request& req)
    {
        const std::string& key = req.body;
        CROW_LOG_INFO << "wild-card : " << key;
        
        crow::json::wvalue out;
        Quarks::Cache::_Instance.getJson(key, out);
        
        return out;
        
    };
    
    auto route_cache_findjson_callback =
    [](const crow::request& req)
    {
        const std::string& wild = req.body;
        CROW_LOG_INFO << "wild-card : " << wild;
        
        std::vector<crow::json::wvalue> jsonResults;
        Quarks::Cache::_Instance.findJson(wild, jsonResults);
        
        crow::json::wvalue w;
        w["result"] = "";
        
        if(jsonResults.size()){
            //w["result"] = jsonResults[0].s();
            //CROW_LOG_INFO << "jsonResults[0] : "
            //   <<  crow::json::dump(jsonResults[0])
            
            w["result"] = std::move(jsonResults);
        }
        
        return w;
        
    };
    
    CROW_ROUTE(app, "/filter/gaussian")
        .methods("POST"_method)(route_filter_gaussian_callback);
    
    CROW_ROUTE(app, "/filter/adjust")
        .methods("POST"_method)(route_filter_adjust_callback);
    
    CROW_ROUTE(app, "/filter/clahe")
        .methods("POST"_method)(route_filter_clahe_callback);
    
    CROW_ROUTE(app, "/quarks/cache/putjson")
    .methods("POST"_method)(route_cache_putjson_callback);
  
    CROW_ROUTE(app, "/quarks/cache/getjson")
    .methods("GET"_method, "POST"_method)(route_cache_getjson_callback);
    
    CROW_ROUTE(app, "/quarks/cache/findjson")
    .methods("GET"_method, "POST"_method)(route_cache_findjson_callback);
    
    
    auto& v = Quarks::Matrix::_Instance; // we will work with the matrix data struct
                                            // in later api calls
    
    
    
    std::cout << "running .." << std::endl;

    app.port(18080).run();

    return 0;
}
