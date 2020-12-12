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
// Echoes back all received WebSocket messages
void do_session(tcp::socket& client_socket)
{
    try
    {
    	boost::system::error_code error;
    	
    	boost::asio::streambuf read_buffer;
	  		
    	bool quit = false;    	
    	while(!quit){
    		
			// Read from client.
	  		/*std::size_t bytes_transferred = 4;
			bytes_transferred = boost::asio::read(client_socket, read_buffer,
	      		boost::asio::transfer_exactly(bytes_transferred));
	  			  		 
	  		std::string data = make_string(read_buffer);
	  		std::cout << "Read: " <<  data << std::endl;
	  		read_buffer.consume(bytes_transferred); // Remove data that was read.*/
	  		
	  		char recv_str[1024] = {};
	  		/*for(int i=0; i<1024; i++){
	  			recv_str[i] = 0;	
			}*/
            client_socket.receive(boost::asio::buffer(recv_str));
            
            std::string data = recv_str;
	  		std::cout << "Read: " <<  data << std::endl;
	  		
	  		std::string message = std::string("ack") + data[data.size() - 1];
	  		client_socket.send(boost::asio::buffer(message));
	  	 	//boost::asio::write( client_socket, boost::asio::buffer(message), error);
	  	 	//std::cout << "Write: " <<  message << std::endl;
	  	 	
	  	 	//sleep(10);
	  	 	if(!data.compare("quit")){
	  	 		quit = true;
			}
	  
		}
		
		std::cout << "disconnecting from socket .." << std::endl;
        		
    }
    catch(boost::system::system_error const& se)
    {
        // This indicates that the session was closed
        std::cerr << "Error: " << se.code().message() << std::endl;
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

//------------------------------------------------------------------------------


int tcpServerStart(const char* tcpUrl)
{
    try
    {
		std::vector<std::string> tokens;
		tokenize_string(tcpUrl, ':', tokens);
		
		int port = 18070;
		if(tokens.size() > 1){
			port = std::stoi(tokens[1]);
		}
	
        boost::asio::io_service io_service;  
  
		//listen for new connection  
      	tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), port ));  
      	std::cout << "TCP server started at port: " << port <<  std::endl;
  
      	
		for(;;)
        {
            // This will receive the new connection
            tcp::socket socket(io_service);

            // Block until we get a connection
            acceptor.accept(socket);

            // Launch the session, transferring ownership of the socket
            std::thread{std::bind(
                &do_session,
                std::move(socket))}.detach();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}


int tcpClientStart(const char* tcpUrl) {
	
	std::vector<std::string> tokens;
	tokenize_string(tcpUrl, ':', tokens);
	
	std::string address = "127.0.0.1";
	int port = 18070;
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

	int n = 0;
	while(true){
		n++;
		
		const std::string msg = std::string("msg") + std::to_string(n);
		boost::system::error_code error;
	
		//boost::asio::write( socket, boost::asio::buffer(msg), error );
		socket.send(boost::asio::buffer(msg));
		
		if( !error ) {
		   //std::cout << "Client sent " << msg << std::endl;
		}
		else {
		   //std::cout << "send failed: " << error.message() << std::endl;
		}
		// getting response from server
		//boost::asio::streambuf receive_buffer;
		///boost::asio::read(socket, receive_buffer, boost::asio::transfer_all(), error);
		char recv_str[1024] = {};
  		/*for(int i=0; i<1024; i++){
  			recv_str[i] = 0;	
		}*/
        socket.receive(boost::asio::buffer(recv_str));
            
		if( error && error != boost::asio::error::eof ) {
		    //std::cout << "receive failed: " << error.message() << std::endl;
		}
		else {
		    //const char* data = boost::asio::buffer_cast<const char*>(receive_buffer.data());
		   const char* data = recv_str;
		   std::cout << "client received: " << data << std::endl;
		}
		
	}
	    
    //std::string quitMsg = "quit";
    //boost::asio::write(socket, boost::asio::buffer(quitMsg));
    
    return 0;
}


// unused
int tcpServerStartAsync(const char* tcpUrl){
	try
    {
    	std::vector<std::string> tokens;
    	tokenize_string(tcpUrl, ':', tokens);
		
		int port = 18070;
		if(tokens.size() > 1){
			port = std::stoi(tokens[1]);
		}
		
    	boost::asio::io_service io_service;  
    	TCPServer server(io_service, port);
    	io_service.run();
    	
    }
  	catch(std::exception& e)
    {
    	std::cerr << e.what() << std::endl;
    }
    
    return 0;
}

