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

#include <quarkstcp.hpp>
#include <boost/asio.hpp>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

using namespace boost::asio;  
using ip::tcp;  

static int tcp_port = 18071;
static bool tcp_server_running = true;

static int tcp_s_interrupted = 0;
void tcp_s_signal_handler (int /*signal_value*/) {
	tcp_s_interrupted = 1;	
	std::cerr << "Interrupt received while running TCP Server .." << std::endl;
	
	tcpServerQuit();
	
}

void tcp_s_catch_signals (void) {
	struct sigaction action;
	action.sa_handler = tcp_s_signal_handler;
	action.sa_flags = 0;
	sigemptyset (&action.sa_mask);
	sigaction (SIGINT, &action, NULL);
	sigaction (SIGTERM, &action, NULL);
	
}
 
//------------------------------------------------------------------------------

std::string make_string(boost::asio::streambuf& streambuf)
{
  return {boost::asio::buffers_begin(streambuf.data()), 
          boost::asio::buffers_end(streambuf.data())};
}

void tokenize_string(const char* input, char delim, std::vector<std::string>& tokens){
	std::istringstream ss(input);
	
	std::string token;
	while(std::getline(ss, token, delim)) {
    	tokens.push_back(token);
	}

}

//------------------------------------------------------------------------------

int tcpServerStart(const char* tcpUrl)
{
    try
    {
		std::vector<std::string> tokens;
		tokenize_string(tcpUrl, ':', tokens);
		
		int port = tcp_port;
		if(tokens.size() > 1){
			port = std::stoi(tokens[1]);
			tcp_port = port;
		}
		
		//tcp_s_catch_signals();
		
    	boost::asio::io_service io_service;  
    	TCPServer server(io_service, port);
    	
    	try{
    		tcp_server_running = true;
    		io_service.run();
    		
		}catch (const std::runtime_error& e){
			
			tcp_server_running = false;
			
			//io_service.stop();
			std::cerr << "TCP Server runtime error: " << e.what() << std::endl;
			boost::optional<boost::asio::io_service::work> work{io_service};
			io_service.post([&]() {
            	work.reset(); // let io_service run out of work
        	});
		} 
      	
    }
    catch (const std::exception& e)
    {
    	tcp_server_running = false;
    	
        std::cerr << "TCP Server Exception. Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    std::cout << ".......TCP: SERVER EXITING FOR REAL ......." << std::endl;
    
    return 0;
}


int tcpClientStart(const char* tcpUrl) {
	
	std::vector<std::string> tokens;
	tokenize_string(tcpUrl, ':', tokens);
	
	std::string address = "127.0.0.1";
	int port = tcp_port;
	if(tokens.size() > 1){
		address = tokens[0];
		port = std::stoi(tokens[1]);
	}
	
    boost::asio::io_service io_service;
	//socket creation
    tcp::socket socket(io_service);
	//connection
    socket.connect( tcp::endpoint( boost::asio::ip::address::from_string(address), port ));
    
    //boost::asio::socket_base::send_buffer_size option(8192);
	//socket.set_option(option);
    
    std::cout << "TCP client started at address: " << address  <<" port: " << port <<  std::endl;

	boost::system::error_code error;
	
	boost::asio::streambuf receive_buffer;
	
	char buf[1024];

	//boost::asio::read(socket, receive_buffer, boost::asio::transfer_all(), error);	
	for(int i = 0; i < 2; i++){
		const std::string msg = "Hello " + std::to_string(i) + " from Client!\n";
		boost::asio::write( socket, boost::asio::buffer(msg), error );
	
		size_t len = socket.read_some(boost::asio::buffer(buf), error);
		if( error && error != boost::asio::error::eof ) {
			std::cout << "client failed to receive: " << error.message() << std::endl;
		}
		else {
			const char* data = boost::asio::buffer_cast<const char*>(receive_buffer.data());
			std::cout << "client received: " << buf << std::endl;
		}
	}
	
    std::string quitMsg = "quit";
    boost::asio::write( socket, boost::asio::buffer(quitMsg), error );
	
    std::cout << "TCP Client Exiting" << std::endl;
    //boost::asio::read(socket, receive_buffer, boost::asio::transfer_all(), error);
    
    return 0;
}


int tcpServerQuit() {
	if(!tcp_server_running){
		return 0;
	}
	
	std::string address = "127.0.0.1";
	int port = tcp_port;
	
    boost::asio::io_service io_service;
	//socket creation
    tcp::socket socket(io_service);
	//connection
    socket.connect( tcp::endpoint( boost::asio::ip::address::from_string(address), port ));
    
    std::cout << "TCP client started to instigate quit at : " << address  <<" port: " << port <<  std::endl;
	
    std::string quitMsg = "quit";
    boost::asio::write(socket, boost::asio::buffer(quitMsg));
    
    return 0;
}


