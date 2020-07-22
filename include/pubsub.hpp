#if !defined(PUBSUB__INCLUDED_)
#define PUBSUB__INCLUDED_

#include <zmq.hpp>
#include <string>
#include <iostream>
#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>

#define sleep(n)    Sleep(n)
#endif

#define BROKER_OK "brokerok"

struct RelayMsg{
	size_t sz;	
	char* buffer;
	
	char* data(){
		return buffer;
	}
	
	size_t size(){
		return sz;
	}
		
	RelayMsg(){
		buffer = nullptr;
	}

	~RelayMsg(){
		delete [] buffer;
	}
	
};

struct EncodedMsg : RelayMsg{
	
	void encode(const char* msg, size_t s){
		size_t offset = sizeof(size_t);
		sz = offset + s;
		
		delete [] buffer;
		
		buffer = new char [sz];
		memcpy(buffer, &s, offset);
		memcpy(buffer + offset, msg, s);
		
		std::cout << "EncodedMsg size: " << size() << " data: " << (char*)(data() + offset)<< std::endl;
	
	}	
	
	EncodedMsg(const char* msg, size_t s){
		encode(msg, s);
	}
	
};

struct DecodedMsg : RelayMsg{
		
	void decode(void* m){	
		sz = *((size_t*) m);		
		char* msg = (char*) m + sizeof(size_t);
		
		delete [] buffer;
		
		buffer = new char [sz];
		memcpy(buffer, msg, sz);
		buffer[sz] = 0; // need to remove
		
		std::cout << "DecodedMsg size: " << size() << " data: " << data() << std::endl;	
	}

	DecodedMsg(void* m){
		decode(m);
	}
	
};

class Consumer {
public:
	virtual void onRead(void* data, size_t size) = 0;

};

class Producer {
public:
	void setProducer(zmq::socket_t& producer){
		_producer = &producer;	
	}
	
	virtual void onRead(void* data, size_t size) = 0;
	
	virtual void write(const char* data, size_t size){
		EncodedMsg e(data, size);
		zmq::message_t message (e.size());
		memcpy (message.data (), e.data(), e.size());
		
		_producer->send(message);
	}

	zmq::socket_t* _producer;
	   
};


static int s_interrupted = 0;
static void s_signal_handler (int /*signal_value*/) {
	s_interrupted = 1;
}

static void s_catch_signals (void) {
	struct sigaction action;
	action.sa_handler = s_signal_handler;
	action.sa_flags = 0;
	sigemptyset (&action.sa_mask);
	sigaction (SIGINT, &action, NULL);
	sigaction (SIGTERM, &action, NULL);
}

int runPublisher () {
	//  Prepare our context and socket

	zmq::context_t context (1);
	zmq::socket_t socket (context, ZMQ_REP);
	socket.bind ("tcp://*:5556");

	s_catch_signals ();
	while (true) {

		zmq::message_t request;

		try {

			//  Wait for next request from client
			socket.recv (&request);
			std::cout << "Received Hello" << std::endl;

			//  Do some 'work'
			sleep(1);

			//  Send reply back to client
			zmq::message_t reply (5);
			memcpy (reply.data (), "World", 5);
			socket.send (reply);

		} catch(zmq::error_t& e) {
			std::cout << "W: interrupt received, proceeding…" << std::endl;
		}
		if (s_interrupted) {
			std::cout << "W: interrupt received, killing server…" << std::endl;
			break;
		}

	}

	std::cout << "W: interrupt received, exiting…" << std::endl;

	return 0;
}


int runSubscriber () {
	//  Prepare our context and socket
	zmq::context_t context (1);
	zmq::socket_t socket (context, ZMQ_REQ);

	std::cout << "Connecting to hello world server…" << std::endl;
	socket.connect ("tcp://localhost:5555");

	//  Do 10 requests, waiting each time for a response
	for (int request_nbr = 0; request_nbr != 10; request_nbr++) {
		zmq::message_t request (5);
		memcpy (request.data (), "Hello", 5);
		std::cout << "Sending Hello " << request_nbr << "…" << std::endl;
		socket.send (request);

		//  Get the reply.
		zmq::message_t reply;
		socket.recv (&reply);
		std::cout << "Received World " << request_nbr << std::endl;
	}

	return 0;

}

int runBroker(const char* tcpBindUrl) {
	//  Prepare our context and socket

	zmq::context_t context (1);
	zmq::socket_t socket (context, ZMQ_REP);
	socket.bind (tcpBindUrl);
	
	std::cout << "Broker started.." << std::endl;

	s_catch_signals ();
	while (true) {

		zmq::message_t request;

		try {

			//  Wait for next request from client
			socket.recv (&request);			
			DecodedMsg req(request.data());
	
			char reqdata[256];	// prone to buffer overflow
			memcpy(reqdata, req.data(), req.size());
			reqdata[req.size()] = 0;
				
			std::cout << "Broker received: " << reqdata << std::endl;
			
			//  Send reply back to client
			EncodedMsg rep(BROKER_OK, strlen(BROKER_OK) + 1);			
			zmq::message_t reply (rep.size());
			memcpy (reply.data (), rep.data(), rep.size());
			/*size_t sz = 8;
			zmq::message_t reply (sz);			
			memcpy(reqdata, &sz, sizeof(sz));
			memcpy(reqdata + sizeof(size_t), BROKER_OK, sz);
			memcpy (reply.data (), reqdata, sizeof(sz) + sz);*/
			socket.send (reply);
				
			//  Artificial delay
			sleep(1);
		
		} catch(zmq::error_t& e) {
			std::cout << "W: interrupt received, proceeding…" << std::endl;
		}
		if (s_interrupted) {
			std::cout << "W: interrupt received, killing server…" << std::endl;
			break;
		}

	}

	std::cout << "W: interrupt received, exiting…" << std::endl;

	return 0;
}

int runWriter (const char* brokerUrl, const char* producerUrl, const char* consumerUrl, Producer& writer) {

	// Prepare our context and socket
	zmq::context_t context (1);
	
	zmq::socket_t broker (context, ZMQ_REQ);	
	std::cout << "Writer connecting to broker .. " << std::endl;
	
	broker.connect (brokerUrl);
			
	// push pull
	zmq::socket_t consumer(context, ZMQ_PULL);
    consumer.connect(consumerUrl);
    
	zmq::socket_t producer(context, ZMQ_PUSH);
    producer.bind(producerUrl);
		    
	writer.setProducer(producer);
			
	const char* p = "WriterOnline";
	EncodedMsg presence(p, strlen(p));	
		
	zmq::message_t request(presence.size());
	memcpy (request.data (), presence.data(), presence.size());		
	//memcpy (request.data (), presence.buffer(), presence.bufferSize());		
	broker.send (request);	
			
	//  Get the reply.
	zmq::message_t reply;
	broker.recv (&reply);
	
	DecodedMsg rep(reply.data());
	writer.onRead(rep.data(), rep.size());
		
	s_catch_signals ();	
	while(true){			
				
		zmq::message_t message; 
		
		try {       
		
			consumer.recv(&message);			
			DecodedMsg req(message.data());			
						
			zmq::message_t request (5);
			memcpy (request.data (), "putme", 5);			
			std::cout << "Writer sending to broker .. " << std::endl;
			broker.send (request);	
			
			//  Get the reply.
			zmq::message_t reply;
			broker.recv (&reply);
			
			//RelayMsg rep(reply.data());					
			//writer.onRead(rep.data(), rep.size());
			
			// artificail delay 
			sleep(1);
			
			
		} catch(zmq::error_t& e) {
			std::cout << "W: interrupt received, proceeding…" << std::endl;
		}
		if (s_interrupted) {
			std::cout << "W: interrupt received, killing server…" << std::endl;
			break;
		}
	}		
				

	return 0;
}


int runReader(const char* tcpUrl, Consumer& reader) {
	//  Prepare our context and socket
	zmq::context_t context (1);
	zmq::socket_t socket (context, ZMQ_REQ);

	std::cout << "Reader connecting to broker .. " << std::endl;
	socket.connect (tcpUrl);

	while(true){
		zmq::message_t request (5);
		memcpy (request.data (), "readm", 5);
		std::cout << "Sending readm .. " << std::endl;
		socket.send (request);
			
		//  Get the reply.
		zmq::message_t reply;
		socket.recv (&reply);
	
		reader.onRead(reply.data(), 0);
		
		sleep(1);
	}

	return 0;
}


#endif

