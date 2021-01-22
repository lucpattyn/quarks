#ifndef _WSSSERVER_H_
#define _WSSSERVER_H_

// src: https://www.codeproject.com/Articles/1264257/Socket-Programming-in-Cplusplus-using-boost-asio-T

//importing libraries
#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>


using namespace boost::asio;
using ip::tcp;

class tcp_con_handler : public boost::enable_shared_from_this<tcp_con_handler>
{
private:
  tcp::socket sock;
  std::string message="Hello From Server!";
  enum { max_length = 1024 };
  char data[max_length];

public:
  typedef boost::shared_ptr<tcp_con_handler> pointer;
  tcp_con_handler(boost::asio::io_service& io_service): sock(io_service){}
// creating the pointer
  static pointer create(boost::asio::io_service& io_service)
  {
    return pointer(new tcp_con_handler(io_service));
  }
//socket creation
  tcp::socket& socket()
  {
    return sock;
  }

  void start()
  {
    sock.async_read_some(
        boost::asio::buffer(data, max_length),
        boost::bind(&tcp_con_handler::handle_read,
                    shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
  
    /*sock.async_write_some(
        boost::asio::buffer(message, max_length),
        boost::bind(&tcp_con_handler::handle_write,
                  shared_from_this(),
                  boost::asio::placeholders::error,
                  boost::asio::placeholders::bytes_transferred));*/
  }
  
  void stop(boost::asio::io_service& io_service)
  {
  	//boost::optional<boost::asio::io_service::work> work =    boost::emplace(boost::ref(io_service));
	//work = boost::none;
  	io_service.stop();
  }

  void handle_read(const boost::system::error_code& err, size_t bytes_transferred)
  {
    if (!err) {
         std::cout << data << std::endl;
        
    } else {
         std::cerr << "error: " << err.message() << std::endl;
         sock.close();
    }
  }
  void handle_write(const boost::system::error_code& err, size_t bytes_transferred)
  {
    if (!err) {
       std::cout << "Server sent Hello message!"<< std::endl;
    } else {
       std::cerr << "error: " << err.message() << std::endl;
       sock.close();
    }
  }
};

class TCPServer 
{
private:
   tcp::acceptor acceptor_;
   void start_accept()
   {
    // socket
     tcp_con_handler::pointer connection = tcp_con_handler::create(acceptor_.get_io_service());

    // asynchronous accept operation and wait for a new connection.
     acceptor_.async_accept(connection->socket(),
        boost::bind(&TCPServer::handle_accept, this, connection,
        boost::asio::placeholders::error));
  }
public:
//constructor for accepting connection from client
  TCPServer(boost::asio::io_service& io_service, unsigned short port): acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
  {
     start_accept();
  }
  void handle_accept(tcp_con_handler::pointer connection, const boost::system::error_code& err)
  {
    if (!err) {
      connection->start();
    }
    start_accept();
  }
};

struct TCPCommand{
	std::string command;
	std::string message;
};


int tcpServerStart(const char* tcpUrl);

int tcpClientStart(const char* tcpUrl);


#endif
