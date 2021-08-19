#include <base64.hpp>
#include <fstream>

#ifdef _USE_PLUGINS
#include <filter.hpp>
#endif

#include <main.hpp>

#include <quarks.hpp>
#include <v8engine.hpp>

#include <qsocket.hpp>

#include <quarkstcp.hpp>
#include <quarkstaskqueue.hpp>

#include <routes.hpp>

// LUC: 20210717 : ugliest of hacks
int crow::detail::dumb_timer_queue::tick = 5;

int main(int argc, char ** argv) {

	crow::SimpleApp app;
	
	Quarks::Core::_Instance.setEnvironment(argc, argv);

	/*v8::V8::InitializeICUDefaultLocation(argv[0]);
	v8::V8::InitializeExternalStartupData(argv[0]);
	std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
	v8::V8::InitializePlatform(platform.get());
	v8::V8::Initialize();*/

#ifdef _V8_LATEST
	v8EngineInitializeInMain(argc, argv);
#endif
	  
	routeHttpRequests(app);
	              
	// websockets
	QSocket::Interceptor& interceptor = Quarks::Core::_Instance.shouldHookSocket()
	                                    ? Quarks::SocketInterceptor::getInstance(Quarks::Core::_Instance)
	                                    : QSocket::DefaultInterceptor();

	QSocket qsock(CROW_ROUTE(app, "/ws"), interceptor);

	//QSocket qsockFileUploader(CROW_ROUTE(app, "/ws/files/upload"), QSocket::FileInterceptor("upload"));
	// end websockets


	std::cout << "running main .." << std::endl;
	
	if(Quarks::Core::_Instance.isTcpServer()){	
		std::thread ts([]() {
			tcpServerStart(Quarks::Core::_Instance.getTcpUrl());				
		});
		
		ts.detach();
	}
	
	if(Quarks::Core::_Instance.isTcpClient()){
		std::thread tc([]() {
			tcpClientStart(Quarks::Core::_Instance.getTcpUrl());		
		});
		
		tc.detach();
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
	

#ifdef _V8_LATEST
	v8EngineShutdownInMain();
#endif

	Quarks::Core::_Instance.shutDown();

	exit(-1);

	return 0;
}
