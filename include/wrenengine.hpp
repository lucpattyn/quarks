#ifndef _WREN_ENGINE_H_
#define _WREN_ENGINE_H_

#include <Wren++.hpp>

#include <plugins.hpp>

std::map<std::string, std::string>vm_routes; // global declaration

std::string vm_setroute(std::string url, std::string handler){
	vm_routes[url] = handler; // since set in main thread don't have to worry about thread safety
	return handler;
}

bool vm_findroute(std::string url, std::string& out_handler){
	bool ret = (vm_routes.find(url) != vm_routes.end());
	if(ret){
		out_handler = vm_routes.at(url); // at used for thread safety
	}
	
	return ret;
}

struct ScriptEngine : wrenpp::VM{
	ScriptEngine():context(nullptr){
	}
	
	crow::request* context;
};
std::vector<ScriptEngine> VMs; // global declaration
std::mutex vmMutex;

ScriptEngine& getCurrentVM(){
	
	static unsigned int assignedIndex = 0;
	static std::map<std::thread::id, int> vmIndex;
	std::thread::id this_id = std::this_thread::get_id();
	//CROW_LOG_INFO << "thread " << this_id << " at work ...";
	
	ScriptEngine* vm = nullptr;
	
	vmMutex.lock();	
	size_t vmSize = VMs.size();
	if(vmIndex.find(this_id) != vmIndex.end()){
		vm = &(VMs[vmIndex[this_id]]);

	} else{
		if(assignedIndex >= vmSize){
			assignedIndex = 0;
		}		
		
		vmIndex[this_id] = assignedIndex;
		vm = &VMs[assignedIndex];		
		assignedIndex++;
	}
	vmMutex.unlock();
	
	return *vm;
}

crow::request* vmGetCurrentReqContext(){
	return getCurrentVM().context;
}

ScriptEngine& vmSetCurrentReqContext(crow::request* ctx){
	ScriptEngine& vm = getCurrentVM();
	vm.context = ctx;
	
	return vm;
}

std::string vm_getheader(std::string key){
	std::string headerValue = "not found";
	
	crow::request* req = vmGetCurrentReqContext();
	if(req){
		headerValue = req->get_header_value(key);
	}
	
	return headerValue;
}

std::string vm_getparam(std::string key){
	std::string paramValue = "not found";
	
	crow::request* req = vmGetCurrentReqContext();
	if(req){
		paramValue = req->url_params.get(key);
	}
	
	return paramValue;
}


std::string vm_dispatch(std::string body){
	return Routing::DispatchRequest(body.c_str());
}

std::string vm_get(std::string key){
	std::string out;
	if(key.size() > 0) {
		Quarks::Core::_Instance.get(key, out);
	} else {
		out = "{\"error\": \"parameter 'key' missing\"}";
	}
	
	return out;
}

std::string vm_getkeys(std::string wild, int skip, int limit){
	std::string out;
	Quarks::Core::_Instance.getKeys(wild, out, skip, limit);	
	
	return out;
}

std::string vm_getkeys_r(std::string wild, int skip, int limit){
	std::string out;
	Quarks::Core::_Instance.getKeysReversed(wild, out, skip, limit);	
	
	return out;
}

std::string vm_put(std::string body){
	std::string out;
	Quarks::Core::_Instance.put(body, out);
	
	return out;
}

std::string vm_incrval(std::string body){
	std::string out;
	Quarks::Core::_Instance.incrementValue(body, out);
	
	return out;
}

std::string vm_envget(std::string){
	return "Quarks ENV Request Not Implemented Yet!";
}

void wrenEngineInitialize(crow::App<CrowMiddleware>& app){
	unsigned int concurrency = std::thread::hardware_concurrency();
	if(concurrency == 0){
		concurrency = 1;
    }
	for(unsigned int t = 0; t < concurrency; t++ ){
		ScriptEngine vm;
	  	vm.beginModule( "wren/scripts/modules/quarks" )
	    	.beginClass( "QuarksAPI" )
	      		.bindFunction< decltype(&vm_dispatch), &vm_dispatch >( true, "dispatch(_)" )
	      		.bindFunction< decltype(&vm_get), &vm_get >( true, "get(_)" )
	      		.bindFunction< decltype(&vm_getkeys), &vm_getkeys >( true, "getkeys(_,_,_)" )
				.bindFunction< decltype(&vm_getkeys_r), &vm_getkeys_r >( true, "getkeysr(_,_,_)" )
	      		.bindFunction< decltype(&vm_put), &vm_put >( true, "put(_)" )
				.bindFunction< decltype(&vm_incrval), &vm_incrval >( true, "incrval(_)" )
	    	.endClass()
			.beginClass( "QuarksEnv" )
	      			.bindFunction< decltype(&vm_envget), &vm_envget >( true, "get(_)" )
	      			.bindFunction< decltype(&plugins::loadPlugin), & plugins::loadPlugin >( true, "loadplugin(_)" )
	      			.bindFunction< decltype(&plugins::unloadPlugin), & plugins::unloadPlugin >( true, "unloadplugin(_)" )
					.bindFunction< decltype(&plugins::executePluginFn), & plugins::executePluginFn >( true, "callplugin(_,_,_)" )
	      	.endClass()
	  		.beginClass( "Request" )
	  			.bindFunction< decltype(&vm_setroute), &vm_setroute >( true, "on(_,_)" )
	  			.bindFunction< decltype(&vm_getheader), &vm_getheader >( true, "getheader(_)" )
				.bindFunction< decltype(&vm_getparam), &vm_getparam >( true, "getparam(_)" )
	    	.endClass()
		.endModule();
	
	  	vm.executeModule("wren/scripts/app");
	  	
	  	VMs.push_back(std::move(vm));
   }
  
  	app.get_middleware<CrowMiddleware>().before_handler = [](crow::request& req, 
	  	crow::response& res, CrowMiddleware::context& ctx){
			
			ScriptEngine& vm = vmSetCurrentReqContext(&req);
			 
			wrenpp::Method handleRequest = vm.method( "wren/scripts/app", "App", "handleRequest(_,_,_)" );
			std::ostringstream params;
			params << req.url_params;
				
			try{
				wrenpp::Value code = handleRequest(req.url, params.str(), req.body);
				std::string result = code.as<const char*>();
				//CROW_LOG_INFO << "before_handle return result: " << result;
				if(result.compare("")){
					auto jsonResult = crow::json::load(result);
					res.body = jsonResult["result"].s();
					res.code = (int)jsonResult["code"].d();
				
					res.end();
				}
				
			} catch (std::exception& e){
				CROW_LOG_INFO << "before_handler exception occured with error:\n" << e.what();
			}
						 
			std::string handler;
			if(vm_findroute(req.url, handler)){
				CROW_LOG_INFO << "vm_findroute: " << handler;
				wrenpp::Method requestHandler = vm.method( "wren/scripts/app", "App", handler );
				wrenpp::Value code = requestHandler(params.str(), req.body);
				std::string handlerResult = code.as<const char*>();
				auto jsonResult = crow::json::load(handlerResult);
				res.body = jsonResult["result"].s();
				res.code = (int)jsonResult["code"].d();
				
				res.end();
				
			}
		
	};
}

#endif 
