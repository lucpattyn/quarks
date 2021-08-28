#ifndef _QUARKSTCP_H_
#define _QUARKSTCP_H_

// src: https://www.codeproject.com/Articles/1264257/Socket-Programming-in-Cplusplus-using-boost-asio-T

//importing libraries
#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/optional.hpp>

#include <tcprouting.hpp>

using namespace boost::asio;
using ip::tcp;

void tcp_s_signal_handler (int /*signal_value*/);
void tcp_s_catch_signals (void);

class tcp_con_handler : public boost::enable_shared_from_this<tcp_con_handler>
{
private:
	tcp::socket sock;
	std::string message="";
	enum { max_length = 1024*1024 };
	char data[max_length];
	void reset_data(){
		data[0] = 0;
		data[1] = 0;
	}

public:
	bool notifyStop = false;
   
	typedef boost::shared_ptr<tcp_con_handler> pointer;
	tcp_con_handler(boost::asio::io_service& io_service): sock(io_service){}
	// creating the pointer
	static pointer create(boost::asio::io_service& io_service){
		return pointer(new tcp_con_handler(io_service));
	}
	
	//socket creation
	tcp::socket& socket(){
		return sock;
	}
	
	void listen(){
		try{
			reset_data();
			sock.async_read_some(
	        	boost::asio::buffer(data, max_length),
	        	boost::bind(&tcp_con_handler::handle_read,
	                    shared_from_this(),
	                    boost::asio::placeholders::error,
	                    boost::asio::placeholders::bytes_transferred));
	                    
		}catch (std::exception& e) {
        	// do some error handling depending in context
    		std::cout << "exception occured during listening ...";
		}
    }
		
  	void handle_read(const boost::system::error_code& err, size_t bytes_transferred){
    	if (!err) {
    		//std::cout << "Server received: " << data << std::endl;
    		data[bytes_transferred] = 0;
    		//message = std::string("Acknowledging ") + data;
    		
         	if(!strcmp(data, "quit")){
         		std::cout << "Quit received ... " << std::endl;
         		message = "quit";
         		notifyStop = true;
				 	
		 	} else {
		 		message = TCP::DispatchRequest(data);
			}
		 		
 			sock.async_write_some(
    			boost::asio::buffer(message, max_length),
   				boost::bind(&tcp_con_handler::handle_write,
              		shared_from_this(),
              		boost::asio::placeholders::error,
              		boost::asio::placeholders::bytes_transferred));
 		
 			if(!notifyStop){
				reset_data();
				sock.async_read_some(
    				boost::asio::buffer(data, max_length),
    				boost::bind(&tcp_con_handler::handle_read,
                	shared_from_this(),
                	boost::asio::placeholders::error,
                	boost::asio::placeholders::bytes_transferred));
            
                	
        		std::cout << "async_read_some inside handle_read" << std::endl;
			}
		       
    	} else {
         	std::cerr << "TCP: handle_read error: " << err.message() << " (possible connection close)" << std::endl;
         	sock.close();
    	}
	}
  
	void handle_write(const boost::system::error_code& err, size_t bytes_transferred){
	    if (!err) {
	       	//std::cout << "Server sent "<< message <<  std::endl;
	    } else {
	       	std::cerr << "TCP: handle_write error: " << err.message() << " (possible connection close)" << std::endl;
	       	sock.close();
		}
		
		if(notifyStop){
			//stop_assoc_io_service();
			//throw std::exception();
			throw std::runtime_error("TCP: Server forced to quit!!");
		}
	}
	
	void stop_assoc_io_service(){
		boost::asio::io_service& io_service = socket().get_io_service();
		boost::optional<boost::asio::io_service::work> work{io_service};
		io_service.post([&]() {
            work.reset(); // let io_service run out of work
            
        });
		//io_service.stop();
		//throw std::runtime_error("TCP: Server forced to quit!!");
	}
};

class TCPServer 
{
private:
   tcp::acceptor acceptor_;
   boost::asio::io_service& io_service_;
   
	void start_accept(){
	    // socket
	    tcp_con_handler::pointer connection = tcp_con_handler::create(acceptor_.get_io_service());

    	// asynchronous accept operation and wait for a new connection.
     	acceptor_.async_accept(connection->socket(),
        	boost::bind(&TCPServer::handle_accept, this, connection,
        	boost::asio::placeholders::error));
        
        
        std::cout << "TCP: start_accept" << std::endl;
	}

public:
	//constructor for accepting connection from client
	TCPServer(boost::asio::io_service& io_service, unsigned short port): io_service_(io_service), 
				acceptor_(io_service, tcp::endpoint(tcp::v4(), port)){
		start_accept();
  	}
  
  	void handle_accept(tcp_con_handler::pointer connection, const boost::system::error_code& err){
		std::cout << "TCP: handle_accept starting ..." << std::endl;
		if (!err) {
      		connection->listen();
   		}
   		start_accept();
   		
		std::cout << "TCP: handle_accept exiting ..." << std::endl;
  	}
  	
  	void stop(){
  		boost::asio::io_service& io_service = io_service_;
		io_service.stop();	
	}
};

struct TCPCommand{
	std::string command;
	std::string message;
};


int tcpServerStart(const char* tcpUrl);
int tcpClientStart(const char* tcpUrl);
int tcpServerQuit();

	

#endif
