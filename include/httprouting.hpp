#ifndef _HTTP_ROUTES_H_
#define _HTTP_ROUTES_H_

#include <crow.h>

////////////////////////////////UNDEFED INTENTIONALLY//////////////////////////////////////

#ifdef _USE_RAPIDAPI

#include <curl/curl.h>


static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

#endif
////////////////////////////////////////////////////////////////////////////////////////

struct CrowMiddleware
{
    std::string message;
	

    CrowMiddleware() : message(""), before_handler(nullptr), after_handler(nullptr)
    {
    	
    }

	std::string getMessage()
	{
		return message;
	}
    void setMessage(std::string newMsg)
    {
        message = newMsg;
    }

    struct context
    {
    };

    void before_handle(crow::request& req, crow::response& res, context& ctx)
    {
        CROW_LOG_INFO << " BEFORE - HANDLE: " << message;            
		
		if(before_handler)
			before_handler(req, res, ctx);
	}
	

    void after_handle(crow::request& req, crow::response& res, context& ctx)
    {
    	if(after_handler)
    		after_handler(req, res, ctx);
        
		CROW_LOG_INFO << " AFTER - HANDLE: " << message;
    }
    
    std::function<void(crow::request& req, crow::response& res, context&)> before_handler;
	std::function<void(crow::request& req, crow::response& res, context&)> after_handler;
};


static void handle_runtime_error(const std::runtime_error& error, crow::json::wvalue& out,
                                 std::vector<crow::json::wvalue>& jsonResults) {
	std::string errs = R"([{"error" : "runtime parsing error"},)";

	crow::json::wvalue err;
	err["reason"] = error.what();
	out["error"] = std::move(err);
		
	out["result"] = std::move(jsonResults);
	
}

static void handle_runtime_error(const std::runtime_error& error, std::string& out,
                                 std::string& results) {
	std::string errs = R"([{"error" : "runtime parsing error"},)";

	crow::json::wvalue w;

	crow::json::wvalue err;
	err["reason"] = error.what();
	w["error"] = std::move(err);
		
	w["result"] = results;
	
	out = crow::json::dump(w);
}

struct QueryParams {
	std::string wild;
	std::string skip;
	std::string limit;

	static QueryParams Parse(const crow::request& req) {
		QueryParams q;
		
		auto k = req.url_params.get("keys");
		q.wild = (k == nullptr || !strlen(k) ? "" : k);

		auto s = req.url_params.get("skip");
		q.skip = (s == nullptr || !strlen(s) ? "0" : s);

		auto l = req.url_params.get("limit");
		q.limit = (l == nullptr || !strlen(l)? "-1" : l);

		//CROW_LOG_INFO << "qp >> wild: " << q.wild << ", skip: " << q.skip << ", limit: " << q.limit;

		return q;
	}

};

void BuildHttpRoutes(crow::App<CrowMiddleware>& app){

	auto route_ping_callback = 
	[](const crow::request& /*req*/) {
		return "pong";  
	};

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


///////////////////////////////////////////
/////////// core functionalities //////////
///////////////////////////////////////////
	auto post = [](std::string body, std::string& out) {

		//CROW_LOG_INFO << "post: " << body;

		/*auto res = crow::response{os.str()};
		 res.add_header("Access-Control-Allow-Origin", "*");
		 res.add_header("Access-Control-Allow-Methods", "GET, POST, PATCH, PUT, DELETE, OPTIONS");
		 res.add_header("Access-Control-Allow-Headers", "Origin, Content-Type, X-Auth-Token");


		 return res;*/


		bool success = Quarks::Core::_Instance.post(body, out);

		return success;


	};

	auto put = [](std::string body, std::string& out) {
		//CROW_LOG_INFO << "put: " << body;

		bool success = Quarks::Core::_Instance.put(body, out);


		return success;


	};

	auto putatom = [](std::string body, std::string& out) {
		//CROW_LOG_INFO << body;

		bool success = Quarks::Core::_Instance.putAtom(body, out);

		return success;

	};

	auto removeatom = [](std::string body, std::string& out) {
		//CROW_LOG_INFO << body;

		bool success = Quarks::Core::_Instance.removeAtom(body, out);

		return success;

	};

	auto atom = [](std::string body, std::string& out) {
		//CROW_LOG_INFO << body;

		bool success = Quarks::Core::_Instance.atom(body, out);

		return success;

	};


	auto route_core_postjson_callback =
	[post](const crow::request& req) {

		std::string out;
		post(req.body, out);

		return out;
	};


	auto route_core_put_callback =
	[put](const crow::request& req) {

		std::string out;

		std::string body = req.body;

		auto x = req.url_params.get("body");
		if(x != nullptr) {
			body = x;
		}

		put(body, out);

		return out;

	};

	auto route_core_putatom_callback =
	[putatom](const crow::request& req) {

		std::string out;

		std::string body = req.body;

		auto x = req.url_params.get("body");
		if(x != nullptr) {
			body = x;
		}

		putatom(body, out);

		return out;

	};

	auto route_core_existkey_callback =
	[](const crow::request& req) {
		std::string out = "";
		//CROW_LOG_INFO <<"get_key:";
		//CROW_LOG_INFO<<req.url_params;

		try {

			auto x = req.url_params.get("key");
			std::string key = (x == nullptr ? "" : x);

			if(key.size() > 0) {
				Quarks::Core::_Instance.exists(key, out);
			} else {
				out = "{\"error\": \"parameter 'key' missing\"}";
			}


		} catch (const std::runtime_error& error) {
			out = R"({"error":"parsing error"})";
		}

		return out;

	};

	auto route_core_get_callback =
	[](const crow::request& req) {
		std::string out = "";
		//CROW_LOG_INFO <<"get_key:";
		//CROW_LOG_INFO<<req.url_params;

		try {

			auto x = req.url_params.get("key");
			std::string key = (x == nullptr ? "" : x);

			if(key.size() > 0) {
				Quarks::Core::_Instance.get(key, out);
			} else {
				out = "{\"error\": \"parameter 'key' missing\"}";
			}


		} catch (const std::runtime_error& error) {
			out = R"({"error":"parsing error"})";
		}

		return out;

	};

	auto route_core_getall_callback =
	[](const crow::request& req) {
		crow::json::wvalue out;

		std::vector<crow::json::wvalue> jsonResults;

		try {
			auto q = QueryParams::Parse(req);

			//CROW_LOG_INFO << "wild: " << q.wild << ", skip: " << q.skip << ", limit: " << q.limit;

			bool ret = false;
			if(q.wild.size() > 0) {
				//Quarks::Core::_Instance.iterJson(wild, jsonResults);
				ret = Quarks::Core::_Instance.getAll(q.wild, jsonResults, std::stoi(q.skip), std::stoi(q.limit));

				out["result"] = std::move(jsonResults);
				if(!ret) {
					out["error"] = R"({"error" : "query returned error"})";
				}

			} else {
				out["error"] = "{\"error\":\"parameter 'keys' missing\"}";

			}

		} catch (const std::runtime_error& error) {
			handle_runtime_error(error, out, jsonResults);
		}

		return out;

	};

	auto route_core_getsorted_callback =
	[](const crow::request& req) {
		crow::json::wvalue out;

		std::vector<crow::json::wvalue> jsonResults;

		try {

			auto q =  QueryParams::Parse(req);

			auto sb = req.url_params.get("sortby");
			std::string sortby = (sb == nullptr || !strlen(sb)) ? "" : sb;

			auto f = req.url_params.get("filter");
			std::string filterBy = (f == nullptr || !strlen(f)) ? "" : f;

			auto des = req.url_params.get("des");
			std::string descending = (des == nullptr || !strlen(des))  ? "false" : des;

			//CROW_LOG_INFO << "wild: " << q.wild << ", sortby: " << sortby << ", des: " << descending
			//    << ", skip: " << q.skip << ", limit: " << q.limit;

			bool ret = false;
			if(q.wild.size() > 0) {
				bool ascending = (descending.compare("true") == 0) ? false : true;

				ret = Quarks::Core::_Instance.getSorted(q.wild, sortby, ascending, jsonResults, std::stoi(q.skip), std::stoi(q.limit), filterBy);

				out["result"] = std::move(jsonResults);
				if(!ret) {
					out["error"] = R"({"error" : "query returned error"})";
				}

			} else {
				out["error"] = "{\"error\":\"parameter 'keys' missing\"}";

			}

		} catch (const std::runtime_error& error) {
			handle_runtime_error(error, out, jsonResults);
		}

		return out;

	};

	/*auto route_core_getkeys_callback =
	[](const crow::request& req) {
		crow::json::wvalue out;

		std::vector<crow::json::wvalue> jsonResults;

		try {
			auto q = QueryParams::Parse(req);

			//CROW_LOG_INFO << "wild: " << q.wild << " (size: " << q.wild.size() << "), skip: " << q.skip << ", /limit: " << q.limit;

			bool ret = false;
			if(q.wild.size() > 0) {
				auto rev = req.url_params.get("reverse");
				std::string reverse = (rev == nullptr || !strlen(rev))  ? "false" : rev;

				if(!reverse.compare("true")) {
					ret = Quarks::Core::_Instance.getKeysReversed(q.wild, jsonResults,
					        std::stoi(q.skip), std::stoi(q.limit));
				} else {
					ret = Quarks::Core::_Instance.getKeys(q.wild, jsonResults,
					                                      std::stoi(q.skip), std::stoi(q.limit));
				}

				out["result"] = std::move(jsonResults);
				if(!ret) {
					out["error"] = R"({"error" : "query returned error"})";
				}

			} else {
				out["error"] = "{\"error\":\"parameter 'keys' missing\"}";

			}

		} catch (const std::runtime_error& error) {
			handle_runtime_error(error, out, jsonResults);
		}

		std::ostringstream os;
		os << crow::json::dump(out);

		auto res = crow::response{os.str()};
	    res.add_header("Access-Control-Allow-Origin", "*");
	    //res.add_header("Access-Control-Allow-Methods", "GET, POST, PATCH, PUT, DELETE, OPTIONS");
	    //res.add_header("Access-Control-Allow-Headers", "Origin, Content-Type, X-Auth-Token");

		return res;

	};*/

	auto route_core_getkeys_callback =
	[](const crow::request& req) {
		std::string out;
		
		try {
			auto q = QueryParams::Parse(req);

			//CROW_LOG_INFO << "wild: " << q.wild << " (size: " << q.wild.size() << "), skip: " << q.skip << ", /limit: " << q.limit;
			//CROW_LOG_INFO << "wild: " << q.wild << " (size: " << q.wild.size() << "), q.skip: " << q.skip << ", /limit: " << q.limit;

			bool ret = false;
			if(q.wild.size() > 0) {
				auto rev = req.url_params.get("reverse");
				std::string reverse = (rev == nullptr || !strlen(rev))  ? "false" : rev;				
				if(!reverse.compare("true")) {
					ret = Quarks::Core::_Instance.getKeysReversed(q.wild, out,
					        									std::stoi(q.skip), std::stoi(q.limit));
				} else {
					ret = Quarks::Core::_Instance.getKeys(q.wild, out, std::stoi(q.skip), std::stoi(q.limit));
				}

				if(!ret){
					out[out.size() - 1] = ',';
					out += std::string(R"("error" : "wildcard or db error"})");
				}
			} 

		} catch (const std::runtime_error& error) {
			handle_runtime_error(error, out, out);
		}

		std::ostringstream os;
		os << out;
		
		auto res = crow::response{os.str()};
	    res.add_header("Access-Control-Allow-Origin", "*");
	    //res.add_header("Access-Control-Allow-Methods", "GET, POST, PATCH, PUT, DELETE, OPTIONS");
	    //res.add_header("Access-Control-Allow-Headers", "Origin, Content-Type, X-Auth-Token");

		return res;

	};
	
	
	auto route_core_getkeysmulti_callback =
	[](const crow::request& req) {
		crow::json::wvalue out;

		std::vector<crow::json::wvalue> jsonResults;

		try {
			std::string body = req.body;
			auto p = req.url_params.get("body");
			if(p != nullptr) {
				body = p;
			}
			auto q = QueryParams::Parse(req);
			
			if(body.size() > 0) {
				
				bool ret = Quarks::Core::_Instance.getKeysMulti(body, jsonResults,
					                                      std::stoi(q.skip), std::stoi(q.limit));
				out["result"] = std::move(jsonResults);
				if(!ret) {
					out["error"] = R"({"error" : "query returned error"})";
				}

			} else {
				out["error"] = R"({"error":"parameter 'body' missing or has no content"})";

			}

		} catch (const std::runtime_error& error) {
			handle_runtime_error(error, out, jsonResults);
		}

		std::ostringstream os;
		os << crow::json::dump(out);

		auto res = crow::response{os.str()};
	    res.add_header("Access-Control-Allow-Origin", "*");
	    //res.add_header("Access-Control-Allow-Methods", "GET, POST, PATCH, PUT, DELETE, OPTIONS");
	    //res.add_header("Access-Control-Allow-Headers", "Origin, Content-Type, X-Auth-Token");

		return res;

	};

	auto route_core_getcount_callback =
	[](const crow::request& req) {
		std::string out;
		long count = 0;

		try {

			auto q = QueryParams::Parse(req);

			bool ret = false;
			if(q.wild.size() > 0) {
				//Quarks::Core::_Instance.iterJson(wild, jsonResults);
				ret = Quarks::Core::_Instance.getCount(q.wild, count, std::stoi(q.skip), std::stoi(q.limit));
				out = std::to_string(count);

			} else {
				out = "{\"error\": \"parameter 'key' missing\"}";
			}

			if(!ret) {
				out = R"({"error" : "counting error"})";
			}

		} catch (const std::runtime_error& error) {
			out = error.what();
		}

		return out;

	};


	auto route_core_iter_callback =
	[](const crow::request& req) {
		crow::json::wvalue out;
		std::vector<crow::json::wvalue> jsonResults;
		try {
			auto q = QueryParams::Parse(req);
			
			auto rev = req.url_params.get("reverse");
				std::string reverse = (rev == nullptr || !strlen(rev))  ? "false" : rev;

			bool ret = true;
			if(!reverse.compare("true")) {
				ret = Quarks::Core::_Instance.iterReversed(jsonResults, std::stoi(q.skip), std::stoi(q.limit));
			
			} else{
				ret = Quarks::Core::_Instance.iter(jsonResults, std::stoi(q.skip), std::stoi(q.limit));
			
			}
			
			out["result"] = std::move(jsonResults);
			if(!ret) {
				out["error"] = R"({"error" : "query returned error"})";
			}
			
		} catch (const std::runtime_error& error) {
			handle_runtime_error(error, out, jsonResults);
		}

		std::ostringstream os;
		os << crow::json::dump(out);

		auto res = crow::response{os.str()};
	    res.add_header("Access-Control-Allow-Origin", "*");
	    //res.add_header("Access-Control-Allow-Methods", "GET, POST, PATCH, PUT, DELETE, OPTIONS");
	    //res.add_header("Access-Control-Allow-Headers", "Origin, Content-Type, X-Auth-Token");

		return res;

	};

	auto route_core_removekey_callback =
	[](const crow::request& req) {
		std::string out = "";
		//CROW_LOG_INFO <<"get_key:";
		//CROW_LOG_INFO<<req.url_params;

		try {

			auto x = req.url_params.get("key");
			std::string key = (x == nullptr ? "" : x);

			if(key.size() > 0) {
				Quarks::Core::_Instance.remove(key, out);

			} else {
				out = "{\"error\": \"parameter 'key' missing\"}";
			}


		} catch (const std::runtime_error& error) {
			out = error.what();
		}

		return out;

	};

	auto route_core_removekeys_callback =
	[](const crow::request& req) {
		crow::json::wvalue out;

		try {
			auto q = QueryParams::Parse(req);

			//CROW_LOG_INFO << "wild: " << q.wild << ", skip: " << q.skip << ", limit: " << q.limit;

			if(q.wild.size() > 0) {
				//Quarks::Core::_Instance.iterJson(wild, jsonResults);
				int removeCount = Quarks::Core::_Instance.removeAll(q.wild, std::stoi(q.skip), std::stoi(q.limit));
				out["result"] = removeCount;

			} else {
				out["error"] = "{\"error\":\"parameter 'keys' missing\"}";
			}

		} catch (const std::runtime_error& error) {
			out["error"] = error.what();
		}

		return out;

	};

	auto route_core_removeatom_callback =
	[removeatom](const crow::request& req) {

		std::string out;

		std::string body = req.body;

		auto x = req.url_params.get("body");
		if(x != nullptr) {
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
	[put](const crow::request& req) {

		//crow::json::wvalue out;
		//putjson(req.body, out);

		std::string out;
		put(req.body, out);
		
		std::ostringstream os;
		os << out;

		auto res = crow::response{os.str()};
	    res.add_header("Access-Control-Allow-Origin", "*");
	    //res.add_header("Access-Control-Allow-Methods", "GET, POST, PATCH, PUT, DELETE, OPTIONS");
	    //res.add_header("Access-Control-Allow-Headers", "Origin, Content-Type, X-Auth-Token");

		return res;

		//return out;

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


	auto route_core_make_callback =
	[](const crow::request& req) {
		crow::json::wvalue out;
		try {
			std::string body = req.body;
			auto p = req.url_params.get("body");
			if(p != nullptr) {
				body = p;
			}
			Quarks::Core::_Instance.makePair(body, out);

		} catch (const std::runtime_error& error) {
			out["error"] = "runtime error";
		}

		return out;

	};

	auto route_core_getjson_callback =
	[](const crow::request& req) {
		crow::json::wvalue out;
		auto x = crow::json::load(req.body);
		if (!x) {
			out["error"] = "invalid parameters";
			return out;
		}

		try {
			std::string key = x["key"].s();
			Quarks::Core::_Instance.getJson(key, out);

		} catch (const std::runtime_error& error) {
			out["error"] = "parameter 'key' missing";
		}

		return out;

	};

	auto route_core_getlist_callback =
	[](const crow::request& req) {
		//crow::json::wvalue w;
		//w["result"] = "";

		std::string out;
		
		std::string body = req.body;
		auto p = req.url_params.get("body");
		if(p != nullptr) {
			body = p;
		}

		auto x = crow::json::load(body);
		if (!x) {
			//w["error"] = "invalid parameters";
			//return w;
			out = R"({"result":[],"error":"invalid json body"})";
			return out;
		}

		auto q = QueryParams::Parse(req);

		//std::vector<crow::json::wvalue> jsonResults;
		//bool ret = Quarks::Core::_Instance.getList(x, jsonResults, std::stoi(q.skip), std::stoi(q.limit));
		Quarks::Core::_Instance.getList(x, out, std::stoi(q.skip), std::stoi(q.limit));
		
			//w["result"] = std::move(jsonResults);
		
		//if(!ret) {
			//w["error"] = "runtime error";
		//}

		//return w;
		return out;

	};
	
	auto route_core_getitems_callback =
	[](const crow::request& req) {
		crow::json::wvalue w;
		
		std::string body = req.body;
		auto p = req.url_params.get("body");
		if(p != nullptr) {
			body = p;
		}

		auto x = crow::json::load(body);
		if (!x) {
			w["error"] = "invalid parameters";
			return w;
		}

		auto q = QueryParams::Parse(req);

		bool ret = Quarks::Core::_Instance.getItems(x, w, std::stoi(q.skip), std::stoi(q.limit));

		//if(jsonResults.size()) {
			//w["result"] = jsonResults[0].s();
			//CROW_LOG_INFO << "jsonResults[0] : "
			//   <<  crow::json::dump(jsonResults[0])
		if(!ret) {
			w["error"] = "One or more keys did not retrieve the corresponding item";
		}

		return w;

	};
	
	auto route_core_getjoinedmap_callback =
	[](const crow::request& req) {
		crow::json::wvalue w;
		w["result"] = "";

		std::string body = req.body;
		auto p = req.url_params.get("body");
		if(p != nullptr) {
			body = p;
		}

		auto x = crow::json::load(body);
		if (!x) {
			w["error"] = "invalid parameters";
			return w;
		}

		auto q = QueryParams::Parse(req);

		std::vector<crow::json::wvalue> jsonResults;
		bool ret = Quarks::Core::_Instance.getJoinedMap(x, jsonResults, std::stoi(q.skip), std::stoi(q.limit));

		//if(jsonResults.size()) {
			//w["result"] = jsonResults[0].s();
			//CROW_LOG_INFO << "jsonResults[0] : "
			//   <<  crow::json::dump(jsonResults[0])

			w["result"] = std::move(jsonResults);
		//}

		if(!ret) {
			w["error"] = "runtime error";
		}

		return w;

	};
	
	auto route_core_getkeysafter_callback =
	[](const crow::request& req) {
		crow::json::wvalue w;
		w["result"] = "";

		std::string body = req.body;
		auto p = req.url_params.get("body");
		if(p != nullptr) {
			body = p;
		}

		auto x = crow::json::load(body);
		if (!x) {
			w["error"] = "invalid parameters";
			return w;
		}

		auto q = QueryParams::Parse(req);

		std::vector<crow::json::wvalue> jsonResults;
		bool ret = Quarks::Core::_Instance.getKeysAfter(x, jsonResults, std::stoi(q.skip), std::stoi(q.limit));

		w["result"] = std::move(jsonResults);

		if(!ret) {
			w["error"] = "runtime error";
		}

		return w;

	};

	auto route_core_getkeyslast_callback =
	[](const crow::request& req) {
		crow::json::wvalue w;
		w["result"] = "";

		std::string body = req.body;
		auto p = req.url_params.get("body");
		if(p != nullptr) {
			body = p;
		}

		auto x = crow::json::load(body);
		if (!x) {
			w["error"] = "invalid parameters";
			return w;
		}

		std::vector<crow::json::wvalue> jsonResults;
		bool ret = Quarks::Core::_Instance.getKeysLast(x, jsonResults);

		w["result"] = std::move(jsonResults);

		if(!ret) {
			w["error"] = "runtime error";
		}

		return w;

	};

	auto route_core_incr_callback =
	[](const crow::request& req) {
		std::string out;

		std::string body = req.body;
		auto p = req.url_params.get("body");
		if(p != nullptr) {
			body = p;
		}
		CROW_LOG_INFO << "incr request body : " << body;


		auto x = crow::json::load(body);
		if (!x) {
			out = "invalid Json Body";
			return out;
		}

		std::string key = x["key"].s();
		int stepBy  = x["step"].i();
		//CROW_LOG_INFO << "incr request key, value : " << key << ", " << stepBy ;

		Quarks::Core::_Instance.increment(key, stepBy, out);

		return out;

	};
	
	auto route_core_incrvalue_callback =
	[](const crow::request& req) {
		std::string out;

		std::string body = req.body;
		auto p = req.url_params.get("body");
		if(p != nullptr) {
			body = p;
		}
		CROW_LOG_INFO << "incr value request body : " << body;

		Quarks::Core::_Instance.incrementValue(body, out);

		return out;

	};

	auto route_core_searchjson_callback =
	[](const crow::request& req) {

		crow::json::wvalue w;
		w["result"] = "";

		auto x = crow::json::load(req.body);
		if (!x) {
			w["error"] = "invalid parameters";
			return w;
		}

		auto q = QueryParams::Parse(req);

		std::vector<crow::json::wvalue> jsonResults;
		Quarks::Core::_Instance.searchJson(x, jsonResults, std::stoi(q.skip), std::stoi(q.limit));

		if(jsonResults.size()) {
			//w["result"] = jsonResults[0].s();
			//CROW_LOG_INFO << "jsonResults[0] : "
			//   <<  crow::json::dump(jsonResults[0])

			w["result"] = std::move(jsonResults);
		}

		return w;

	};

	auto route_core_atom_callback =
	[atom](const crow::request& req) {

		std::string out;

		std::string body = req.body;

		auto x = req.url_params.get("body");
		if(x != nullptr) {
			body = x;
		}

		atom(body, out);

		return out;

	};
	
	auto route_core_geoput_callback =
	[](const crow::request& req) {

		std::string body = req.body;
		auto x = req.url_params.get("body");
		if(x != nullptr) {
			body = x;
		}

		std::string out;
		Quarks::Core::_Instance.geoput(body, out);

		return out;

	};
	
	auto route_core_geonear_callback =
	[](const crow::request& req) {

		std::string body = req.body;
		auto x = req.url_params.get("body");
		if(x != nullptr) {
			body = x;
		}
		
		auto q = QueryParams::Parse(req);
	
		crow::json::wvalue out;
		out["result"] = "";

		std::vector<crow::json::wvalue> jsonResults;
		bool ret = Quarks::Core::_Instance.geonear(body, out, jsonResults, std::stoi(q.skip), std::stoi(q.limit));

		if(jsonResults.size()) {
			//w["result"] = jsonResults[0].s();
			//CROW_LOG_INFO << "jsonResults[0] : "
			//   <<  crow::json::dump(jsonResults[0])

			out["result"] = std::move(jsonResults);
		}
		
		if(!ret){
			out["error"] = "geonear runtime error occured";
		}

		return out;

	};
	
	auto route_core_backup_callback =
	[](const crow::request& req) {

		std::string out = R"({"result":false})";

		std::string body = req.body;

		auto x = req.url_params.get("body");
		if(x != nullptr) {
			body = x;
		}
		
		auto p = crow::json::load(body);
		if (!p) {
			out = R"({"result":false, "error":"invalid parameters"})";
			return out;
		}

		std::string path = "";
		if(p.has("path")){
			path = p["path"].s();			
		}
	
		if(Quarks::Core::_Instance.backup(path)){
			out = R"({"result":true})";
		}
		
		return out;

	};

	auto route_core_filetransfer_callback=
	[](const crow::request& req) {
		//bool ret = Quarks::Core::_Instance.fileTransfer("filetransfer", "transfer", "sendfile", "remotedes");
		FILE* fp = fopen("transferred.data", "wb");
		if(fp == NULL) return R"({"error":"not transferred"})";

		bool ret = fwrite(req.body.c_str(), sizeof(char), req.body.size(), fp) > 0;

		fclose(fp);

		if(ret) {
			return R"({"result":"transferred"})";
		} 
			
		return R"({"error":"not transferred"})";

	};

	auto route_core_opentcpsocket_callback=
	[](/*const crow::request& req*/) {
		bool ret = Quarks::Core::_Instance.openTCPSocketClient();

		if(ret) {
			return R"({"result":"opened socket"})";
		} 

		return R"({"error":"socket closed"})";
	};
	
	auto route_core_log_callback =
	[](const crow::request& req) {

		std::string out = R"({"result":false})";

		std::string url = req.body;
		auto x = req.url_params.get("url");
		if(x != nullptr) {
			url = x;
		}
				
		if(Quarks::Core::_Instance.setLogger(url.c_str())){
			
			// for now always returns true, later we can health check of the url
			out = R"({"result":true})";
		}
		
		return out;

	};

	auto route_core_test_callback =
	[](const crow::request& req) {

		std::string out = R"({"result":false})";

		std::string timeout = "1";
		auto x = req.url_params.get("timeout");
		if(x != nullptr) {
			timeout = x;
		}
				
		int seconds = std::stoi(timeout);
		
		sleep(seconds); 
		std::cout << "waited for " << seconds << " seconds" << std::endl;		
				
		out = R"({"result":true})";
				
		return out;

	};

	// core ends //
	//////////////

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
	[](const crow::request& /*req*/) {
		
#ifdef _USE_RAPIDAPI

		const char* q = req.url_params.get("msg");
		std::string params;
		//uriDecode(q, params);

		//CROW_LOG_INFO << "uri params:" << params << q;

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

		auto res = crow::response {os.str()};
		res.add_header("Access-Control-Allow-Origin", "*");
		res.add_header("Access-Control-Allow-Methods", "GET, POST, PATCH, PUT, DELETE, OPTIONS");
		res.add_header("Access-Control-Allow-Headers", "Origin, Content-Type, X-Auth-Token");

		return res;
	};

	// core routing .. 
	
	CROW_ROUTE(app, "/put")
	(route_core_put_callback);

	CROW_ROUTE(app, "/exists")
	(route_core_existkey_callback);

	CROW_ROUTE(app, "/get")
	(route_core_get_callback);

	CROW_ROUTE(app, "/getall")
	(route_core_getall_callback);

	CROW_ROUTE(app, "/getsorted")
	(route_core_getsorted_callback);

	CROW_ROUTE(app, "/getkeys")
	(route_core_getkeys_callback);

	CROW_ROUTE(app, "/getkeysmulti")
	(route_core_getkeysmulti_callback);

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

	CROW_ROUTE(app, "/make")
	.methods("GET"_method, "POST"_method)(route_core_make_callback);

	CROW_ROUTE(app, "/getjson")
	.methods("GET"_method, "POST"_method)(route_core_getjson_callback);

	CROW_ROUTE(app, "/getlist")
	.methods("GET"_method, "POST"_method)(route_core_getlist_callback);
	
	CROW_ROUTE(app, "/getitems")
	.methods("GET"_method, "POST"_method)(route_core_getitems_callback);
	
	CROW_ROUTE(app, "/getjoinedmap")
	.methods("GET"_method, "POST"_method)(route_core_getjoinedmap_callback);
	
	CROW_ROUTE(app, "/getkeysafter")
	.methods("GET"_method, "POST"_method)(route_core_getkeysafter_callback);
	
	CROW_ROUTE(app, "/getkeyslast")
	.methods("GET"_method, "POST"_method)(route_core_getkeyslast_callback);

	CROW_ROUTE(app, "/incr")
	.methods("GET"_method, "POST"_method)(route_core_incr_callback);

	CROW_ROUTE(app, "/incrval")
	.methods("GET"_method, "POST"_method)(route_core_incrvalue_callback);

	CROW_ROUTE(app, "/searchjson")
	.methods("GET"_method, "POST"_method)(route_core_searchjson_callback);
	
	// lat long stuff
	CROW_ROUTE(app, "/geo/put")
	.methods("GET"_method, "POST"_method)(route_core_geoput_callback);
	
	CROW_ROUTE(app, "/geo/near")
	.methods("GET"_method, "POST"_method)(route_core_geonear_callback);
	
	// backup and restore
	CROW_ROUTE(app, "/backup")
	.methods("GET"_method, "POST"_method)(route_core_backup_callback);

	// experimental
	CROW_ROUTE(app, "/filetransfer")
	.methods("GET"_method, "POST"_method)(route_core_filetransfer_callback);

	CROW_ROUTE(app, "/opentcpsocket")
	.methods("GET"_method, "POST"_method)(route_core_opentcpsocket_callback);
	
	CROW_ROUTE(app, "/log")
	.methods("GET"_method, "POST"_method)(route_core_log_callback);
	
	CROW_ROUTE(app, "/test")
	.methods("GET"_method, "POST"_method)(route_core_test_callback);
	// 
	
	//auto& v = Quarks::Matrix::_Instance; // we will work with the matrix data struct
	// in later api calls
	
	// html serving
	auto resourceLoader = [](crow::mustache::context& x, std::string filename,
	const char* base = nullptr) {
		char name[256];
		gethostname(name, 256);
		x["servername"] = name;

		crow::mustache::set_base(".");
		if(base == nullptr) {
			crow::mustache::set_base("templates");
		} else {
			crow::mustache::set_base(base);
		}

		return crow::mustache::load(filename);
	};

	// generic file serving from templates

	auto readFile = [](std::string name) {

		std::ifstream f(name.c_str(), std::ifstream::in|std::ifstream::binary|std::ifstream::ate);
		if(!f.is_open()) {
			return std::string("");
		};
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

	auto defaultPageLoader = [&resourceLoader, &readFile/*, &uriDecode*/](const crow::request& req, std::string defaultPage) {

		std::string result;

		const char* q = req.url_params.get("q");

		//bool useMustache = false;
		bool stylesheet = false;
		bool javascript = false;

		if(q == nullptr) {
			crow::mustache::context x;
			auto page = resourceLoader(x, defaultPage);
			result = page.render(x);

		} else {
			std::string asset;
			asset = q;
			//uriDecode(q, asset);


			if(asset.size() > 3) {
				size_t js = asset.size() - 3;
				//CROW_LOG_INFO << asset.substr(css);
				if(!asset.substr(js).compare(".js")) {
					javascript = true;
				}

			}

			if(asset.size() > 4) {
				size_t css = asset.size() - 4;
				//CROW_LOG_INFO << asset.substr(css);
				if(!asset.substr(css).compare(".css")) {
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

		auto res = crow::response {os.str()};

		if(javascript) {
			res.add_header("Content-type", "application/javascript");

		} else if(stylesheet) {
			//CROW_LOG_INFO << "stylesheet requested";
			res.add_header("Content-type", "text/css");
		} else {
			res.set_header("Content-Type", "text/html; charset=UTF-8");
		}

		return res;

	};

	CROW_ROUTE(app, "/")
	([&defaultPageLoader](const crow::request& req) {
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

	/*CROW_ROUTE(app, "/console")
	([&defaultPageLoader](const crow::request& req) {
		auto res = defaultPageLoader(req, "console/index.html");
		return res;
	});*/
	
	CROW_ROUTE(app, "/console")
	([&defaultPageLoader](const crow::request& req) {
		auto res = defaultPageLoader(req, "console.html");
		return res;
	});

	CROW_ROUTE(app, "/home")
	([&defaultPageLoader](const crow::request& req) {
		auto res = defaultPageLoader(req, "home.html");
		return res;
	});

	CROW_ROUTE(app, "/chat")
	([&defaultPageLoader](const crow::request& req) {
		auto res = defaultPageLoader(req, "chat.html");
		return res;
	});

	CROW_ROUTE(app, "/feed")
	([&readFile](const crow::request& /*req*/) {
		auto result = readFile("templates/feed.html");
		std::ostringstream os;
		os << result;

		auto res = crow::response {os.str()};

		return res;
	});
	
	CROW_ROUTE(app, "/wrenfeed")
	([&readFile](const crow::request& /*req*/) {
		auto result = readFile("wren/feed.html");
		std::ostringstream os;
		os << result;

		auto res = crow::response {os.str()};

		return res;
	});
	
	CROW_ROUTE(app, "/readme")
	([&readFile](const crow::request& /*req*/) {
		auto result = readFile("templates/readme.html");
		std::ostringstream os;
		os << result;

		auto res = crow::response {os.str()};

		return res;
	});

	CROW_ROUTE(app, "/home/<int>")
	([&resourceLoader](int resId) {
		crow::mustache::context x;
		auto page = resourceLoader(x, std::to_string(resId) + ".html");
		return page.render(x);
	});

	// js serving
	CROW_ROUTE(app, "/js/<int>")
	([&resourceLoader](int resId) {
		crow::mustache::context x;
		auto page = resourceLoader(x, std::to_string(resId) + ".js");
		return page.render(x);
	});

	// css serving
	CROW_ROUTE(app, "/css/<int>")
	([&resourceLoader](int resId) {
		crow::mustache::context x;
		auto page = resourceLoader(x, std::to_string(resId) + ".css");
		return page.render(x);
	});

	CROW_ROUTE(app, "/ping")
	(route_ping_callback);

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
	
// end AI calls

}


#endif 
