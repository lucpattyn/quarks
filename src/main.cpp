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
std::string vm_getheader(std::string ctx, std::string key){
	std::string headerValue = "not found";
	
	crow::request* req = vm_reqctxs[ctx];
	if(req){
		headerValue = req->get_header_value(key);
	}
	
	return headerValue;
}

std::string vm_request(std::string body){
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

std::string vm_envrequest(std::string){
	return "Quarks ENV Request";
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
	
	wrenpp::VM vm;
  	vm.beginModule( "quarks" )
    	.beginClass( "QuarksAPI" )
    		.bindFunction< decltype(&vm_getheader), &vm_getheader >( true, "getheader(_,_)" )
      		.bindFunction< decltype(&vm_request), &vm_request >( true, "request(_)" )
      		.bindFunction< decltype(&vm_get), &vm_get >( true, "get(_)" )
      		.bindFunction< decltype(&vm_getkeys), &vm_getkeys >( true, "getkeys(_,_,_)" )
      		.bindFunction< decltype(&vm_put), &vm_put >( true, "put(_)" )
			.bindFunction< decltype(&vm_incrval), &vm_incrval >( true, "incrval(_)" )
    	.endClass()
		.beginClass( "QuarksEnv" )
      			.bindFunction< decltype(&vm_envrequest), &vm_envrequest >( true, "request(_)" )
      	.endClass()
  	
	.endModule();

  	vm.executeModule("app");
  	
  	app.get_middleware<CrowMiddleware>().before_handler = [&vm](crow::request& req, 
	  	crow::response& res, CrowMiddleware::context& ctx){
			wrenpp::Method handleRequest = vm.method( "app", "App", "handleRequest(_,_,_,_)" );
			std::ostringstream os;
			os << req.url_params;
			
			intptr_t reqptr = (intptr_t)(&req);
			std::stringstream ss;
			ss << std::hex << reqptr;
			std::string reqctx = ss.str();
			vm_reqctxs[reqctx] = &req;
			std::string headerValue =  vm_reqctxs[reqctx]->get_header_value("Connection");
			if(!headerValue.empty()){
				CROW_LOG_INFO << "before_handle get header: " << headerValue;
			}
			wrenpp::Value code = handleRequest(reqctx, req.url, os.str(), req.body);
			std::string result = code.as<const char*>();
			CROW_LOG_INFO << "before_handle return result: " << result;
			if(result.compare("")){
				res.body = result;
				res.code = 200;
				
				res.end();
			} 
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
