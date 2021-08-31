//
// Copyright (c) 2016-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: WebSocket server, synchronous
// File: https://www.boost.org/doc/libs/1_66_0/libs/beast/example/websocket/server/sync/websocket_server_sync.cpp
//
//------------------------------------------------------------------------------

#include <tcprouting.hpp>
#include <quarks.hpp>

#define TCP_ROUTE(ROUTER, URL) (ROUTER[URL]) = TCP_CALLBACK
#define TCP_CALLBACK(c) c

#define TCP_REQ_UNUSED(u) u.query.size()

static void handle_runtime_error(const std::runtime_error& error, crow::json::wvalue& out,
	                                 std::vector<crow::json::wvalue>& jsonResults) {
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

//------------------------------------------------------------------------------

/*std::map< std::string , 
		std::function<std::string(TCP::Request& request)> > 
	TCP::Router::routingTableString;

std::map< std::string , 
		std::function<crow::json::wvalue&(TCP::Request& request)> >
	TCP::Router::routingTableJson;*/

TCP::Router& TCP::DefaultRouter(){
	static TCP::Router defaultRouter;
	return defaultRouter;
}

// Route mapping starts ...
	
void TCP::BuildRoutes(TCP::Router& router){

	auto route_ping_callback = 
	[](const TCP::Request& /*req*/) {
		return "pong";  
	};

#ifdef _USE_PLUGINS

	auto route_filter_gaussian_callback =
	[](const TCP::Request &req) {
		std::string res = Core::Filter::gaussian(req.body);
		return res;
	};


	auto route_filter_adjust_callback =
	[](const TCP::Request &req) {
		std::string res = Core::Filter::adjust(req.body);
		return res;
	};

	auto route_filter_clahe_callback =
	[](const TCP::Request &req) {
		std::string res = Core::Filter::clahe(req.body);
		return res;
	};

#endif


///////////////////////////////////////////
/////////// core functionalities //////////
///////////////////////////////////////////
	auto post = [](std::string body, std::string& out) {
		
		bool success = Quarks::Core::_Instance.post(body, out);
		return success;
	};

	auto put = [](std::string body, std::string& out) {
		
		bool success = Quarks::Core::_Instance.put(body, out);
		return success;
	};

	auto putatom = [](std::string body, std::string& out) {

		bool success = Quarks::Core::_Instance.putAtom(body, out);
		return success;
	};

	auto removeatom = [](std::string body, std::string& out) {

		bool success = Quarks::Core::_Instance.removeAtom(body, out);
		return success;
	};

	auto atom = [](std::string body, std::string& out) {

		bool success = Quarks::Core::_Instance.atom(body, out);
		return success;
	};


	auto route_core_postjson_callback =
	[post](const TCP::Request& req) {

		std::string out;
		post(req.body, out);

		return out;
	};


	auto route_core_put_callback =
	[put](const TCP::Request& req) {

		std::string out;
		put(req.body, out);

		return out;

	};

	auto route_core_putatom_callback =
	[putatom](const TCP::Request& req) {
		
		std::string out;
		putatom(req.body, out);

		return out;

	};

	auto route_core_existkey_callback =
	[](const TCP::Request& req) {
		
		std::string out = "";
		Quarks::Core::_Instance.exists(req.body, out);
		
		return out;

	};

	auto route_core_get_callback =
	[](const TCP::Request& req) {
		std::string out = "";
		
		try {

			Quarks::Core::_Instance.get(req.get("key"), out);
			
		} catch (const std::runtime_error& error) {
			out = R"({"error":"parsing error"})";
		}

		return out;

	};

	auto route_core_getall_callback =
	[](const TCP::Request& req) {
		crow::json::wvalue out;

		std::vector<crow::json::wvalue> jsonResults;
		try {
			bool ret = Quarks::Core::_Instance.getAll(req.wild(), jsonResults, req.skip, req.limit);
			out["result"] = std::move(jsonResults);
			if(!ret) {
				out["error"] = R"({"error" : "query returned error"})";
			}

			
		} catch (const std::runtime_error& error) {
			handle_runtime_error(error, out, jsonResults);
		}

		return out;

	};

	auto route_core_getsorted_callback =
	[](const TCP::Request& req) {
		crow::json::wvalue out;

		std::vector<crow::json::wvalue> jsonResults;

		try {
			auto sortby = req.get("sortby");
			auto filterBy = req.get("filter");
			auto descending = req.get("des");
			
			//CROW_LOG_INFO << "wild: " << q.wild << ", sortby: " << sortby << ", des: " << descending
			//    << ", q.skip: " << q.skip << ", q.limit: " << q.limit;

			bool ret = false;
			if(req.wild().size() > 0) {
				bool ascending = (descending.compare("true") == 0) ? false : true;

				ret = Quarks::Core::_Instance.getSorted(
											req.wild(), sortby, 
											ascending, jsonResults, 
											req.skip, req.limit, filterBy);

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

	auto route_core_getkeys_callback =
	[](const TCP::Request& req) {
		std::string out;
		
		try {
			auto& q = req;
			//CROW_LOG_INFO << "wild: " << q.wild << " (size: " << q.wild.size() << "), q.skip: " << q.skip << ", /limit: " << q.limit;

			bool ret = false;
			if(q.wild().size() > 0) {
				auto reverse = req.reverse;
				if(reverse){
					ret = Quarks::Core::_Instance.getKeysReversed(q.wild(), out,
					        									q.skip, q.limit);
				} else {
					ret = Quarks::Core::_Instance.getKeys(q.wild(), out, q.skip, q.limit);
				}

				if(!ret){
					out[out.size() - 1] = ',';
					out += std::string(R"("error" : "wildcard or db error"})");
				}

			} 

		} catch (const std::runtime_error& error) {
			handle_runtime_error(error, out, out);
		}

		return out;

	};

	auto route_core_getkeysmulti_callback =
	[](const TCP::Request& req) {
		crow::json::wvalue out;

		std::vector<crow::json::wvalue> jsonResults;

		try {
			auto& q = req;
			std::string body = req.body;
						
			if(body.size() > 0) {
			
				bool ret = Quarks::Core::_Instance.getKeysMulti(body, jsonResults,
					                                      q.skip, q.limit);
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

		return out;

	};

	auto route_core_getcount_callback =
	[](const TCP::Request& req) {
		std::string out;
		long count = 0;

		try {

			auto& q = req;

			bool ret = false;
			if(q.wild().size() > 0) {
				//Quarks::Core::_Instance.iterJson(wild, jsonResults);
				ret = Quarks::Core::_Instance.getCount(q.wild(), count, q.skip, q.limit);
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
	[](const TCP::Request& req) {
		crow::json::wvalue out;
		std::vector<crow::json::wvalue> jsonResults;
		try {
			auto& q = req;
			bool ret = Quarks::Core::_Instance.iter(jsonResults, q.skip, q.limit);
			
			out["result"] = std::move(jsonResults);
			if(!ret) {
				out["error"] = R"({"error" : "query returned error"})";
			}
			
		} catch (const std::runtime_error& error) {
			handle_runtime_error(error, out, jsonResults);
		}

		return out;

	};

	auto route_core_removekey_callback =
	[](const TCP::Request& req) {
		std::string out = "";
		//CROW_LOG_INFO <<"get_key:";
		//CROW_LOG_INFO<<req.url_params;

		try {

			auto key = req.get("key");		
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
	[](const TCP::Request& req) {
		crow::json::wvalue out;

		try {
			auto& q = req;

			//CROW_LOG_INFO << "wild: " << q.wild << ", q.skip: " << q.skip << ", q.limit: " << q.limit;

			if(q.wild().size() > 0) {
				//Quarks::Core::_Instance.iterJson(wild, jsonResults);
				int removeCount = Quarks::Core::_Instance.removeAll(q.wild(), q.skip, q.limit);
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
	[removeatom](const TCP::Request& req) {

		std::string out;

		std::string body = req.body;
		removeatom(body, out);

		return out;

	};

	
	auto route_core_putjson_callback =
	[put](const TCP::Request& req) {

		std::string out;
		put(req.body, out);
		
		return out;
		
	};


	auto route_core_make_callback =
	[](const TCP::Request& req) {
		crow::json::wvalue out;
		try {
			std::string body = req.body;
			Quarks::Core::_Instance.makePair(body, out);

		} catch (const std::runtime_error& error) {
			out["error"] = "runtime error";
		}

		return out;

	};

	auto route_core_getjson_callback =
	[](const TCP::Request& req) {
		
		crow::json::wvalue out;
					
		auto x = req.bodyJson();
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
	[](const TCP::Request& req) {
		
		std::string out;
		auto x = req.bodyJson();
		if (!x) {
			out = R"({"result":[],"error":"invalid json body"})";
			return out;
		}

		auto& q = req;

		Quarks::Core::_Instance.getList(x, out, q.skip, q.limit);

		return out;

	};
	
	auto route_core_getitems_callback =
	[](const TCP::Request& req) 
	{
		crow::json::wvalue w;
		w["result"] = "";

		auto x = req.bodyJson();
		if (!x) {
			w["error"] = "invalid parameters";
			return w;
		}

		auto& q = req;

		bool ret = Quarks::Core::_Instance.getItems(x, w, q.skip, q.limit);
		if(!ret) {
			w["error"] = "One or more keys did not retrieve the corresponding item";
		}

		return w;

	};
	
	auto route_core_getjoinedmap_callback =
	[](const TCP::Request& req) {
		
		crow::json::wvalue w;
		w["result"] = "";

		auto x = req.bodyJson();
		if (!x) {
			w["error"] = "invalid parameters";
			return w;
		}

		auto& q = req;

		std::vector<crow::json::wvalue> jsonResults;
		bool ret = Quarks::Core::_Instance.getJoinedMap(x, jsonResults, q.skip, q.limit);

		w["result"] = std::move(jsonResults);
		if(!ret) {
			w["error"] = "runtime error";
		}

		return w;

	};
	
	auto route_core_getkeysafter_callback =
	[](const TCP::Request& req) {
		
		crow::json::wvalue w;
		w["result"] = "";

		auto x = req.bodyJson();
		if (!x) {
			w["error"] = "invalid parameters";
			return w;
		}

		auto& q = req;

		std::vector<crow::json::wvalue> jsonResults;
		bool ret = Quarks::Core::_Instance.getKeysAfter(x, jsonResults, q.skip, q.limit);

		w["result"] = std::move(jsonResults);
		if(!ret) {
			w["error"] = "runtime error";
		}

		return w;

	};

	auto route_core_getkeyslast_callback =
	[](const TCP::Request& req) {
		
		crow::json::wvalue w;
		w["result"] = "";

		auto x = req.bodyJson();
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
	[](const TCP::Request& req) {
		
		std::string out;
		
		auto x = req.bodyJson();
		if (!x) {
			out = R"({"error":"invalid parameters"})";
			return out;
		}
				
		std::string key = x["key"].s();
		int stepBy  = x["step"].i();
		Quarks::Core::_Instance.increment(key, stepBy, out);

		return out;

	};
	
	auto route_core_incrvalue_callback =
	[](const TCP::Request& req) {
		std::string out;

		std::string body = req.body;
		Quarks::Core::_Instance.incrementValue(body, out);

		return out;

	};

	auto route_core_searchjson_callback =
	[](const TCP::Request& req) {

		crow::json::wvalue w;
		w["result"] = "";

		auto x = req.bodyJson();
		if (!x) {
			w["error"] = "invalid parameters";
			return w;
		}

		auto& q = req;
		
		std::vector<crow::json::wvalue> jsonResults;
		Quarks::Core::_Instance.searchJson(x, jsonResults, q.skip, q.limit);

		if(jsonResults.size()) {
			w["result"] = std::move(jsonResults);
		}

		return w;

	};

	auto route_core_atom_callback =
	[atom](const TCP::Request& req) {

		std::string out;

		std::string body = req.body;
		atom(body, out);

		return out;

	};
	
	auto route_core_backup_callback =
	[](const TCP::Request& req) {

		std::string out = R"({"result":false})";

		auto p = req.bodyJson();
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
	[](const TCP::Request& req) {
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
	[](const TCP::Request& req) {
		TCP_REQ_UNUSED(req);
		
		bool ret = Quarks::Core::_Instance.openTCPSocketClient();
		if(ret) {
			return R"({"result":"opened socket"})";
		} 
		
		return R"({"error":"socket closed"})";
	};
	
	auto route_core_log_callback =
	[](const TCP::Request& req) {

		std::string out = R"({"result":false})";

		std::string url = req.body;		
		if(Quarks::Core::_Instance.setLogger(url.c_str())){
			out = R"({"result":true})";
		}
		
		return out;

	};

	auto route_core_test_callback =
	[](const TCP::Request& req) {

		std::string out = R"({"result":false})";

		std::string timeout = req.get("timeout");
				
		int seconds = std::stoi(timeout);
		
		sleep(seconds); 
		std::cout << "waited for " << seconds << " seconds" << std::endl;		
				
		out = R"({"result":true})";
				
		return out;

	};

	
	auto route_ai_callback =
	[](const TCP::Request& /*req*/) {
		std::string readBuffer  = "Feature not available";		
		return readBuffer;
	};

	/*TCP_ROUTE(router, "/getall")
	(route_core_getall_callback);*/

	// core routing .. 
	
	TCP_ROUTE(router, "/put")
	(route_core_put_callback);

	TCP_ROUTE(router, "/exists")
	(route_core_existkey_callback);

	TCP_ROUTE(router, "/get")
	(route_core_get_callback);

	TCP_ROUTE(router, "/getall")
	(route_core_getall_callback);

	TCP_ROUTE(router, "/getsorted")
	(route_core_getsorted_callback);

	TCP_ROUTE(router, "/getkeys")
	(route_core_getkeys_callback);

	TCP_ROUTE(router, "/getkeysmulti")
	(route_core_getkeysmulti_callback);

	TCP_ROUTE(router, "/getcount")
	(route_core_getcount_callback);

	TCP_ROUTE(router, "/iter")
	(route_core_iter_callback);

	TCP_ROUTE(router, "/remove")
	(route_core_removekey_callback);

	TCP_ROUTE(router, "/removeall")
	(route_core_removekeys_callback);

	TCP_ROUTE(router, "/remove/atom")
	(route_core_removeatom_callback);

	TCP_ROUTE(router, "/putjson")
	(route_core_putjson_callback);

	TCP_ROUTE(router, "/postjson")
	(route_core_postjson_callback);

	TCP_ROUTE(router, "/put/atom")
	(route_core_putatom_callback);

	TCP_ROUTE(router, "/atom")
	(route_core_atom_callback);

	TCP_ROUTE(router, "/make")
	(route_core_make_callback);

	TCP_ROUTE(router, "/getjson")
	(route_core_getjson_callback);

	TCP_ROUTE(router, "/getlist")
	(route_core_getlist_callback);
	
	TCP_ROUTE(router, "/getitems")
	(route_core_getitems_callback);
	
	TCP_ROUTE(router, "/getjoinedmap")
	(route_core_getjoinedmap_callback);
	
	TCP_ROUTE(router, "/getkeysafter")
	(route_core_getkeysafter_callback);
	
	TCP_ROUTE(router, "/getkeyslast")
	(route_core_getkeyslast_callback);

	TCP_ROUTE(router, "/incr")
	(route_core_incr_callback);

	TCP_ROUTE(router, "/incrval")
	(route_core_incrvalue_callback);

	TCP_ROUTE(router, "/searchjson")
	(route_core_searchjson_callback);
	
	// backup and restore
	TCP_ROUTE(router, "/backup")
	(route_core_backup_callback);

	// experimental
	TCP_ROUTE(router, "/filetransfer")
	(route_core_filetransfer_callback);

	TCP_ROUTE(router, "/opentcpsocket")
	(route_core_opentcpsocket_callback);
	
	TCP_ROUTE(router, "/log")
	(route_core_log_callback);
	
	TCP_ROUTE(router, "/test")
	(route_core_test_callback);
	// 
		
	TCP_ROUTE(router, "/ping")
	(route_ping_callback);

#ifdef _USE_PLUGINS
	TCP_ROUTE(router, "/filter/gaussian")
	(route_filter_gaussian_callback);

	TCP_ROUTE(router, "/filter/adjust")
	(route_filter_adjust_callback);

	TCP_ROUTE(router, "/filter/clahe")
	(route_filter_clahe_callback);
#endif

// AI calls

	TCP_ROUTE(router, "/ai")
	(route_ai_callback);
	
// end AI calls

}

// Route mapping ends ____

//
std::string TCP::DispatchRequest(const char* data){
	return TCP::DefaultRouter().dispatch(data);
}

//

