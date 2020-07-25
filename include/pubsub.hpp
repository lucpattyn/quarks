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
	
	virtual void write(const char* data, size_t size, int totalreaders){
		EncodedMsg e(data, size);
		zmq::message_t message (e.size());
		memcpy (message.data (), e.data(), e.size());
		
		_producer->send(message);
		
		// sending to readers (this is dangerous because any reader may go down)
		for(int i = 0; i < totalreaders; i++){
			zmq::message_t message_readers (e.size());
			memcpy (message_readers.data (), e.data(), e.size());
			_producer->send(message_readers);
		}	
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

/*
// working examples ..

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

}*/

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
	
			char reqdata[1025];	// 1 byte extra for trailing 0
			size_t  reqsize = req.size() < 1024 ? req.size() : 1024;
			memcpy(reqdata, req.data(), reqsize);
			reqdata[reqsize] = 0;
				
			std::cout << "Broker received: " << reqdata << std::endl;
			
			//  Send reply back to client
			EncodedMsg rep(BROKER_OK, strlen(BROKER_OK)+1);			
			zmq::message_t reply (rep.size()); 
			memcpy (reply.data (), rep.data(), rep.size());

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
	
	// push pull
	zmq::socket_t consumer(context, ZMQ_PULL);
    consumer.connect(consumerUrl);
    
	zmq::socket_t producer(context, ZMQ_PUSH);
    producer.bind(producerUrl);	    
	writer.setProducer(producer);

	// broker
	zmq::socket_t broker (context, ZMQ_REQ);	
	
	bool brokerConnected = false;
	if(strcmp(brokerUrl, "!")){
		std::cout << "Writer connecting to broker .. " << std::endl;
	
		broker.connect (brokerUrl);				
		
		const char* r = "reportwriter";
		EncodedMsg presence(r, strlen(r)+1);		
		zmq::message_t report(presence.size()); 
		memcpy (report.data (), presence.data(), presence.size());	
			
		broker.send (report);	
				
		//  Get the reply.
		zmq::message_t res;
		broker.recv (&res);
		
		brokerConnected = true;
		
		DecodedMsg reportRep(res.data());
		writer.onRead(reportRep.data(), reportRep.size());

	}
		
	s_catch_signals ();	
	while(true){			
				
		zmq::message_t message; 
		
		try {       
		
			consumer.recv(&message);			
			DecodedMsg req(message.data());
			
			if(brokerConnected){
				EncodedMsg send(req.data(), req.size());						
				zmq::message_t request (send.size());
				memcpy (request.data (), send.data(), send.size());			
				std::cout << "Writer sending to broker .. " << send.data() + sizeof(size_t)<< std::endl;
				broker.send (request);	
				
				//  Get the reply.
				zmq::message_t reply;
				broker.recv (&reply);
				
				DecodedMsg rep(reply.data());
				writer.onRead(rep.data(), rep.size());

			}
									
			// artificial delay 
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


int runReader(const char* brokerUrl, const char* consumerUrl, Consumer& reader) {
	
	// Prepare our context and socket
	zmq::context_t context (1);
	
	// push pull
	zmq::socket_t consumer(context, ZMQ_PULL);
    
	bool consumerConnected = false;
	if(strcmp(consumerUrl, "")){
		std::cout << "Reader connecting to writer at: " << consumerUrl <<  std::endl;
	
		consumer.connect(consumerUrl);
    	consumerConnected = true;
	}
	
    // broker
	zmq::socket_t broker (context, ZMQ_REQ);	
	
	bool brokerConnected = false;
	if(strcmp(brokerUrl, "!")){
		std::cout << "Reader connecting to broker .. " << std::endl;
	
		broker.connect (brokerUrl);				
		
		const char* r = "reportreader";
		EncodedMsg presence(r, strlen(r)+1);		
		zmq::message_t report(presence.size()); 
		memcpy (report.data (), presence.data(), presence.size());	
			
		broker.send (report);	
				
		//  Get the reply.
		zmq::message_t res;
		broker.recv (&res);
		
		brokerConnected = true;
		
		DecodedMsg reportRep(res.data());
		reader.onRead(reportRep.data(), reportRep.size());
	}

	s_catch_signals ();	
	while(true){
		zmq::message_t message; 
		
		try {       
		
			if(consumerConnected){
				consumer.recv(&message);			
				DecodedMsg req(message.data());		
			
				reader.onRead(req.data(), req.size());
			}							
			
			// artificial delay 
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


#endif

