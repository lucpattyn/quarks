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


std::string vm_request(std::string){
	return "Quarks API Request";
}

std::string vm_getinfo(std::string){
	return "Quarks ENV Request";
}
// LUC: 20210717 : ugliest of hacks
int crow::detail::dumb_timer_queue::tick = 5;

int main(int argc, char ** argv) {
	
	wrenpp::VM vm;
  	vm.beginModule( "quarks" )
    	.beginClass( "QuarksAPI" )
      		.bindFunction< decltype(&vm_request), &vm_request >( true, "request(_)" )
      		//.bindFunction< decltype(&tan), &tan >( true, "tan(_)" )
      		//.bindFunction< decltype(&exp), &exp >( true, "exp(_)" )
    		.endClass()
			.beginClass( "QuarksEnv" )
      			.bindFunction< decltype(&vm_getinfo), &vm_getinfo >( true, "request(_)" )
      		.endClass()
  	
	.endModule();

  	vm.executeModule("app");
	
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
		res.code = 204;
		
		std::string msg = app.get_middleware<CrowMiddleware>().getMessage();
		if(msg.compare("sse")){
				app.get_middleware<CrowMiddleware>().setMessage("sse");
			
				auto body = req.body;
				std::cout << "SSE : " << body;
				
				res.add_header("Access-Control-Allow-Origin", "*");
				res.add_header("Content-Type", "text/event-stream");
				res.add_header("Cache-Control", "no-cache");
				res.add_header("Keep-Alive", "timeout=10, max=1000");
				res.write("\ndata:\n");
 		}				
			
		
		return res;
	});
	
	std::cout << "Quarks launching .." << std::endl;

	std::vector<std::thread> services;
	
	if(Quarks::Core::_Instance.isTcpServer()){
		
		TCP::BuildRoutes(TCP::DefaultRouter());
				
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
