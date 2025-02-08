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

#ifdef _USE_WREN
#include <wrenengine.hpp>
#endif

#include <qsearch.hpp>

// LUC: 20210717 : ugliest of hacks
int crow::detail::dumb_timer_queue::tick = 5;

int main(int argc, char ** argv) {
	
	crow::App<CrowMiddleware> app;
		
	Quarks::Core::_Instance.setEnvironment(argc, argv);

	CROW_LOG_INFO << "Building Routes and Setting up Search Engine ..";
	
	QSearch s;
	s.BuildHttpRoutes(&app);
	s.TestRun();
	
	HttpRouter::BuildHttpRoutes(app);
	
	Routing::BuildRoutes(Routing::DefaultRouter()); // for VM and TCP Routing
	
	/////////////////////////scripting initialize///////////////////////////////////////////////////
		
#ifdef _V8_LATEST
	v8EngineInitializeInMain(argc, argv);
#endif

#ifdef _USE_WREN
	CROW_LOG_INFO << "Initializing Wren Engine ..";
	wrenEngineInitialize(app);
#endif
	
	////////////////////////////////////web sockets //////////////////////////////////////////////////////
	              
	// 
	QSocket::Interceptor& interceptor = Quarks::Core::_Instance.shouldHookSocket()
	                                    ? Quarks::SocketInterceptor::getInstance(Quarks::Core::_Instance)
	                                    : QSocket::DefaultInterceptor();

	QSocket qsock(CROW_ROUTE(app, "/ws"), interceptor);

	//QSocket qsockFileUploader(CROW_ROUTE(app, "/ws/files/upload"), QSocket::FileInterceptor("upload"));
	
	/////////////////////////////////// end websockets ///////////////////////////////////////////////////

	// // SSE 
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
	
	//
	
	CROW_LOG_INFO << "Quarks launching ..";

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
		CROW_LOG_INFO << "Quarks connection timeout set to " << Quarks::Core::_Instance.getTimeout() << " seconds ..";
	}
	
	bool isANode = Quarks::Core::_Instance.isANode();
	
	//if(isANode){ <-- warning: cannot put thread t inside a scope since it gets destroyed on scope exit
	std::thread t([&app, &isANode]() {
		if(isANode){
			CROW_LOG_INFO << "Quarks running as a NODE (broker/reader/writer) ..";
    		app.port(Quarks::Core::_Instance.getPort()).multithreaded().run();
		}		
			
	});
	//}
	Quarks::Core::_Instance.run();
		
	if(!isANode){
		app.port(Quarks::Core::_Instance.getPort()).multithreaded().run();
	}
	
	CROW_LOG_INFO << "^ Http Server has exited.. !!";
	
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

	CROW_LOG_INFO << "-----Cleaning up search engine!!--------";
	s.cleanup();
	
	Quarks::Core::_Instance.shutDown();

	CROW_LOG_INFO << "<<<<<<<<<<<>>>>>>>>>>>\nQuarks Has Shut Down!!";
	
	exit(-1);

	return 0;
	
}
