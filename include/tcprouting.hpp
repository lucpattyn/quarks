#ifndef _TCPROUTES_H_
#define _TCPROUTES_H_

#include <crow/json.h>
#include <map>
#include <chrono>  // for high_resolution_clock

namespace TCP{

	struct Request {
		std::string query ="";
		std::string body = "";
		int skip = 0;
		int limit = -1;
		bool reverse = false;
		crow::json::rvalue req;
		
		Request(const std::string request) {
			req = crow::json::load(request);
			if(req){
				if(req.has("query")){
					query = req["query"].s();
				}				
				if(req.has("body")){
					if(! (req["body"].size())) {
						body = req["body"].s();
					}
				}				
				if(req.has("skip")){
					skip = req["skip"].i();
				}
				if(req.has("limit")){
					limit = req["limit"].i();
				}
				if(req.has("reverse")){
					reverse = req["reverse"].b();
				}
			}
		}
		
		std::string wild() const{
			return get("keys");
		}
		
		std::string get(std::string attrib) const{
			std::string res = "";
			if(req.has(attrib)){
				res = req[attrib].s();
			}
			
			return res;
		}
		
		crow::json::rvalue bodyJson() const{
			if(req.has("body")){
				return req["body"];
			}
			
			return {};
			
		}
	
	};
	
	class Router{
		struct RouteMap{
			std::string _key;
			Router& _router;
			
			RouteMap(std::string k, Router& r) : _key(k), _router(r){}
			
			void operator=(std::function<std::string(Request& request)> callback){
				_router.addRoute(_key, callback);
			}
			void operator=(std::function<crow::json::wvalue(Request& request)> callback){
				_router.addRoute(_key, callback);
			}
		};
		
		public:
		
		Router(){
			
		}
		
		void addRoute(std::string id, std::function<std::string(Request& request)> callback){
			routingTableString[id]  = callback;	
		}
		void addRoute(std::string id, std::function<crow::json::wvalue(Request& request)> callback){
			routingTableJson[id] = callback;
		}
				
		RouteMap operator[](std::string key){
			return RouteMap(key, *this); 
		}
		
		std::string dispatch(std::string requestStr){
			
			try{
				// Record start time
				auto start = std::chrono::high_resolution_clock::now();
				
				Request req(requestStr);				
				
				std::string result = "";
				auto dispatcherString = routingTableString[req.query];
				if(dispatcherString){
					result = dispatcherString(req);
				}		
				
				if(!result.size()){
					auto dispatcherJson = routingTableJson[req.query];
					if(dispatcherJson){
						result = crow::json::dump(dispatcherJson(req));
					}
				}		
								
				// Record end time
				auto finish = std::chrono::high_resolution_clock::now();
				
				std::chrono::duration<double> elapsed = finish - start;				
				std::cout << req.query << ":"  << elapsed.count() * 1000 << " ms" << std::endl;

				return result;
			
			}catch (const std::runtime_error& error) {
				return R"({"error":"dispatch error"})";
			};
			
			return R"({"error":"Invalid Request"})";
		}
		
		private:
		std::map< std::string , 
			std::function<std::string(TCP::Request& request)> > routingTableString;
		std::map< std::string , 
			std::function<crow::json::wvalue(TCP::Request& request)> > routingTableJson;
	};
	

	Router& DefaultRouter();
	void BuildRoutes(TCP::Router& router);
	std::string DispatchRequest(const char* data);
		
}


#endif 
