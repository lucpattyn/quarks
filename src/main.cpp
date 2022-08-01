#include <base64.hpp>
#include <fstream>

#ifdef _USE_PLUGINS
 #include <filter.hpp>
#endif

#include <main.hpp>
#include <quarks.hpp>
#include <v8engine.hpp>
#include <qsocket.hpp>

#include <httprouting.hpp>
#include <tcprouting.hpp>

#include <quarkstcp.hpp>
#include <quarkstaskqueue.hpp>

#include <Wren++.hpp>


std::map<std::string, crow::request*> vm_reqctxs;
std::map<std::string, std::string>vm_routes;

std::string vm_setroute(std::string url, std::string handler){
	vm_routes[url] = handler;		
	return handler;
}

bool vm_findroute(std::string url, std::string& out_handler){
	bool ret = (vm_routes.find(url) != vm_routes.end());
	if(ret){
		out_handler = vm_routes[url];
	}
	
	return ret;
}

std::string vm_getheader(std::string ctx, std::string key){
	std::string headerValue = "not found";
	
	crow::request* req = vm_reqctxs[ctx];
	if(req){
		headerValue = req->get_header_value(key);
	}
	
	return headerValue;
}

std::string vm_getparam(std::string ctx, std::string key){
	std::string paramValue = "not found";
	
	crow::request* req = vm_reqctxs[ctx];
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
	return "Quarks ENV Request";
}


wrenpp::VM& getCurrentVM(std::vector<wrenpp::VM>& VMs){
	size_t vmSize = VMs.size();
	
	std::thread::id this_id = std::this_thread::get_id();
	CROW_LOG_INFO << "thread " << this_id << " at work ...";
	
	static int assignedIndex = -1;
	static std::map<std::thread::id, int> vmIndex;
	if(vmIndex.find(this_id) != vmIndex.end()){
		return VMs[vmIndex[this_id]];

	} else{
		assignedIndex++;
		if(assignedIndex >= vmSize){
			assignedIndex = 0;
		}		
		
		vmIndex[this_id] = assignedIndex;
	}
	
	CROW_LOG_INFO << "VM Index: " << assignedIndex;
	
	return VMs[assignedIndex];	
}


// LUC: 20210717 : ugliest of hacks
int crow::detail::dumb_timer_queue::tick = 5;

int main(int argc, char ** argv) {
	
	//crow::SimpleApp app;
	crow::App<CrowMiddleware> app;
		
	Quarks::Core::_Instance.setEnvironment(argc, argv);

	/*v8::V8::InitializeICUDefaultLocation(argv[0]);
	v8::V8::InitializeExternalStartupData(argv[0]);
	std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
	v8::V8::InitializePlatform(platform.get());
	v8::V8::Initialize();*/

#ifdef _V8_LATEST
	v8EngineInitializeInMain(argc, argv);
#endif
	
	BuildHttpRoutes(app);
	
	Routing::BuildRoutes(Routing::DefaultRouter()); // for VM and TCP Routing
	
	unsigned int concurrency = std::thread::hardware_concurrency();
	if(concurrency == 0){
		concurrency = 1;
	}
	
	std::vector<wrenpp::VM> VMs;
	
	for(unsigned int t = 0; t < concurrency; t++ ){
		wrenpp::VM vm;
	  	vm.beginModule( "wren/scripts/modules/quarks" )
	    	.beginClass( "QuarksAPI" )
	    		.bindFunction< decltype(&vm_getheader), &vm_getheader >( true, "getheader(_,_)" )
				.bindFunction< decltype(&vm_getparam), &vm_getparam >( true, "getparam(_,_)" )
	      		.bindFunction< decltype(&vm_dispatch), &vm_dispatch >( true, "dispatch(_)" )
	      		.bindFunction< decltype(&vm_get), &vm_get >( true, "get(_)" )
	      		.bindFunction< decltype(&vm_getkeys), &vm_getkeys >( true, "getkeys(_,_,_)" )
				.bindFunction< decltype(&vm_getkeys_r), &vm_getkeys_r >( true, "getkeysr(_,_,_)" )
	      		.bindFunction< decltype(&vm_put), &vm_put >( true, "put(_)" )
				.bindFunction< decltype(&vm_incrval), &vm_incrval >( true, "incrval(_)" )
	    	.endClass()
			.beginClass( "QuarksEnv" )
	      			.bindFunction< decltype(&vm_envget), &vm_envget >( true, "get(_)" )
	      	.endClass()
	  		.beginClass( "Request" )
	  			.bindFunction< decltype(&vm_setroute), &vm_setroute >( true, "on(_,_)" )
	    	.endClass()
		.endModule();
	
	  	vm.executeModule("wren/scripts/app");
	  	
	  	VMs.push_back(std::move(vm));
   }
  
  	app.get_middleware<CrowMiddleware>().before_handler = [&VMs](crow::request& req, 
	  	crow::response& res, CrowMiddleware::context& ctx){
			wrenpp::VM& vm = getCurrentVM(VMs);
			 
			wrenpp::Method handleRequest = vm.method( "wren/scripts/app", "App", "handleRequest(_,_,_,_)" );
			std::ostringstream os;
			os << req.url_params;
			
			intptr_t reqptr = (intptr_t)(&req);
			std::stringstream ss;
			ss << std::hex << reqptr;
			std::string reqctx = ss.str();
			vm_reqctxs[reqctx] = &req;
			//std::string headerValue =  vm_reqctxs[reqctx]->get_header_value("Connection");
			//if(!headerValue.empty()){
			//	CROW_LOG_INFO << "before_handle get header: " << headerValue;
			//}
			//if(handleRequest != nullptr){
			try{
				wrenpp::Value code = handleRequest(reqctx, req.url, os.str(), req.body);
				std::string result = code.as<const char*>();
				CROW_LOG_INFO << "before_handle return result: " << result;
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
				wrenpp::Value code = requestHandler(reqctx, os.str(), req.body);
				std::string handlerResult = code.as<const char*>();
				auto jsonResult = crow::json::load(handlerResult);
				res.body = jsonResult["result"].s();
				res.code = (int)jsonResult["code"].d();
				
				res.end();
				
			}
		
			vm_reqctxs.erase(reqctx);	
			
	};
	              
	// websockets
	QSocket::Interceptor& interceptor = Quarks::Core::_Instance.shouldHookSocket()
	                                    ? Quarks::SocketInterceptor::getInstance(Quarks::Core::_Instance)
	                                    : QSocket::DefaultInterceptor();

	QSocket qsock(CROW_ROUTE(app, "/ws"), interceptor);

	//QSocket qsockFileUploader(CROW_ROUTE(app, "/ws/files/upload"), QSocket::FileInterceptor("upload"));
	// end websockets


	// SSE 
	CROW_ROUTE(app, "/sse")
	([&app](const crow::request &req) {
		
		crow::response res;
		res.add_header("Access-Control-Allow-Origin", "*");
		res.add_header("Content-Type", "text/event-stream");
		res.add_header("Cache-Control", "no-cache");
		res.add_header("Keep-Alive", "timeout=10, max=1000");
		res.write("\ndata:\n");
		
		return res;
	});
	
	std::cout << "Quarks launching .." << std::endl;

	std::vector<std::thread> services;
	
	if(Quarks::Core::_Instance.isTcpServer()){
		
		//TCPRouting::BuildRoutes(Routing::DefaultRouter());
				
		services.push_back(std::thread([]() {
			tcpServerStart(Quarks::Core::_Instance.getTcpUrl());				
		}));
	}
	if(Quarks::Core::_Instance.isTcpClient()){
		services.push_back(std::thread ([]() {
			tcpClientStart(Quarks::Core::_Instance.getTcpUrl());		
		}));		
	}
	
	/*std::thread tq([](){
		startTaskQueueService();	
	});
	tq.detach();*/
	
	// Timeout setting
	if(Quarks::Core::_Instance.getTimeout() > 0 ){
		//app.timeout(Quarks::Core::_Instance.getTimeout());
		// LUC: 20210717
		crow::detail::dumb_timer_queue::tick = Quarks::Core::_Instance.getTimeout();
		std::cout << "Quarks connection timeout set to " << Quarks::Core::_Instance.getTimeout() << " seconds .." << std::endl;
	}
	
	bool runAsServer = Quarks::Core::_Instance.isAServer();
	
	//if(runAsServer){ <-- warning: cannot put thread t inside a scope since it gets destroyed on scope exit
	std::thread t([&app, &runAsServer]() {
		if(runAsServer){
			std::cout << "Quarks running as server .." << std::endl;
    		app.port(Quarks::Core::_Instance.getPort()).multithreaded().run();
		}		
			
	});
	//}
	
	Quarks::Core::_Instance.run();
		
	if(!runAsServer){
		app.port(Quarks::Core::_Instance.getPort()).multithreaded().run();
	}
	
	std::cerr << "Http Server Exiting .. !";
	
	tcpServerQuit();
	
	for (std::thread &t: services) {
  		if (t.joinable()) {
    		t.join();
  		}		
	}

//#ifdef _USE_V8_PLUGIN

#ifdef _V8_LATEST
	v8EngineShutdownInMain();
#endif

//#endif

	Quarks::Core::_Instance.shutDown();

	std::cout << "Quarks Exiting!!" << std::endl;
	exit(-1);

	return 0;
}
