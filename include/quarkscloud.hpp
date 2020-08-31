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

namespace QuarksCloud {

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
	
	enum ReadType{
		unknonwn = 0,
		broker,
		save,
		publish
	};
	
	class Interface {
	public:
		void setProducer(zmq::socket_t& producer){
			_producer = &producer;	
		}
		
		virtual void onRead(int type, void* data, size_t size) = 0;
		
		virtual void write(const char* data, size_t size){
			EncodedMsg e(data, size);
			zmq::message_t message (e.size());
			memcpy (message.data (), e.data(), e.size());
			
			if(_producer){
				std::cout << "writing:" << data << std::endl;
				_producer->send(message);
			}
			
		}
		
		Interface(){
			_producer = nullptr;
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
	
	// Broker capbilities:
	// 1. request responsder[with:socketUrl] -> publish [with:publisherUrl]		<-- subscribe reader[with:subscriberUrl]
	// 2. sink receiver[sinkUrl] -> publish [publisherUrl]						<-- subscribe reader[with:subscriberUrl]		  
	// 3. publisher
	//
	int runBroker(const char* socketUrl, const char* publisherUrl,  const char* sinkUrl) {
		//  Prepare our context and socket	
		zmq::context_t context (1);
		
		// rep req
		zmq::socket_t socket (context, ZMQ_REP);
		if(strcmp(socketUrl, "!")){			
			socket.bind (socketUrl);
		}
		
		// sink
		zmq::socket_t receiver(context, ZMQ_PULL);		
		bool sinkMode = false;
	    if(strcmp(sinkUrl, "")){			
			receiver.bind(sinkUrl);
			sinkMode = true;
		}	    
		
		// pub sub
		zmq::socket_t publisher (context, ZMQ_PUB);
	    publisher.bind(publisherUrl); // publisher url is mandatory because broker's main task is to publish
		
		std::cout << "Broker started.. socket url: " << socketUrl << ", publisher url: " 
						<< publisherUrl << ", sinkUrl:" << sinkUrl << std::endl;
		
		s_catch_signals ();
			
		while (true) {
	
			zmq::message_t request;
	
			try {
	
				if(!sinkMode){ 
					//  Wait for next request from client
					socket.recv (&request);			
					DecodedMsg req(request.data());
			
					char reqdata[1025];	// 1 byte extra for trailing 0
					size_t  reqsize = req.size() < 1024 ? req.size() : 1024;
					memcpy(reqdata, req.data(), reqsize);
					reqdata[reqsize] = 0;
						
					std::cout << "Broker received request: " << reqdata << std::endl;
					
					EncodedMsg send(req.data(), req.size());						
					zmq::message_t publishermsg (send.size());
					memcpy (publishermsg.data (), send.data(), send.size());			
					std::cout << "Broker publishing to readers.. " << send.data() + sizeof(size_t) << std::endl;
					publisher.send(publishermsg);
					
					//  Send reply back to client
					EncodedMsg rep(BROKER_OK, strlen(BROKER_OK)+1);			
					zmq::message_t reply (rep.size()); 
					memcpy (reply.data (), rep.data(), rep.size());
		
					socket.send (reply);

				} else {
									
					receiver.recv(&request);			
					DecodedMsg req(request.data());
					
					char reqdata[1025];	// 1 byte extra for trailing 0
					size_t  reqsize = req.size() < 1024 ? req.size() : 1024;
					memcpy(reqdata, req.data(), reqsize);
					reqdata[reqsize] = 0;
						
					std::cout << "Broker received through sink: " << reqdata << std::endl;
					
					EncodedMsg send(req.data(), req.size());						
					zmq::message_t publishermsg (send.size());
					memcpy (publishermsg.data (), send.data(), send.size());			
					std::cout << "Broker publishing to readers.. " << send.data() + sizeof(size_t) << std::endl;
					publisher.send(publishermsg);
				
				}
									
				//  Artificial delay
				sleep(1); //  Give 0MQ time to deliver
			
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
	
	//
	// Writer capbilities:
	// 1. self produce->self consume->send[with:brokerUrl]								 	--> request broker[sockerUrl]
	// 2. (brokerUrl = "!") self produce->self consume->sink[with:sinkUrl]					<-- pull broker[sinkUrl]
	// 3. (brokerUrl = ".") self produce->self consume->push[with:sinkUrl]->next consumer	<-- pull writer[consumerUrl] 
	// 4. (brokerUrl = "+") consume previous producer->push[with:sinkUrl]->next consumer	<-- pull writer[consumerUrl] 
	// 5. (brokerUrl = "}") last in chain - consume previous producer
	int runWriter (const char* brokerUrl, const char* producerUrl, const char* consumerUrl, const char* sinkUrl, Interface& writer) {
	
		// Prepare our context and socket
		zmq::context_t context (1);
		
		// push pull
		zmq::socket_t producer(context, ZMQ_PUSH);		
		if(strcmp(producerUrl, "!")){
			producer.bind(producerUrl);	    
			writer.setProducer(producer);	
		}
	    
		zmq::socket_t consumer(context, ZMQ_PULL);
		bool consumerConnected = false;
		if(strcmp(consumerUrl, "!")){ // if specifically stopped by supplying a '!' then don't connect, otherwise connect
			consumer.connect(consumerUrl);
			consumerConnected = true;
			
			std::cout << "Consumer connected: " << consumerUrl << std::endl;
		}
		
		// writer chain (experimental - probably shouldn't be used)
		bool writerChain = false;
		bool firstInChain = false;
		bool lastInChain = false;
		zmq::socket_t chainedWriter(context, ZMQ_PUSH);
	    if(!strcmp(brokerUrl, "-")){
			firstInChain = true;
			writerChain = true;			
			chainedWriter.bind(sinkUrl);		
			std::cout << "Writer connected to head of chain: " << sinkUrl << std::endl;
			sinkUrl = "";
			
		}else if(!strcmp(brokerUrl, "+")) {
			writerChain = true;
			chainedWriter.bind(sinkUrl);	
			std::cout << "Writer connected to chain: " << sinkUrl << std::endl;
			sinkUrl = "";
			
		}else if(!strcmp(brokerUrl, ".")){
			lastInChain = true;
			writerChain = true;
			std::cout << "Writer connected to end of chain: " << sinkUrl << std::endl;
			
		}
			
		// sink
		bool sinkConnected = false;
		zmq::socket_t sender(context, ZMQ_PUSH);
	    if(strcmp(sinkUrl, "")){
			sender.connect(sinkUrl);	    
			sinkConnected = true;
		
			std::cout << "Writer connected to sink: " << sinkUrl << std::endl;		
		}	
		
		// broker
		bool brokerConnected = false;
		zmq::socket_t broker (context, ZMQ_REQ);		
		if(!writerChain && strcmp(brokerUrl, "!")){
			
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
			writer.onRead(QuarksCloud::broker, reportRep.data(), reportRep.size());
			
			std::cout << "Writer connected to broker: " << brokerUrl  << std::endl;
		
	
		}
		
		std::cout << "Writer started -> broker url: " << brokerUrl << ", producer url: " << producerUrl 
						<< ", consumer url: " << consumerUrl << ", sink url: " << sinkUrl << std::endl;
				
		s_catch_signals ();	
		while(true){			
					
			zmq::message_t message; 
			
			try {       
			
				if(consumerConnected){
					consumer.recv(&message);			
					DecodedMsg req(message.data());
					
					// if connected to a broker then delegate, otherwise dispatch to sink or the next consumer
					if(brokerConnected){
						EncodedMsg send(req.data(), req.size());						
						zmq::message_t request (send.size());
						memcpy (request.data (), send.data(), send.size());			
						std::cout << "Writer sending to broker .. " << send.data() + sizeof(size_t) << std::endl;
						broker.send (request);	
						
						//  Get the reply.
						zmq::message_t reply;
						broker.recv (&reply);
						
						DecodedMsg rep(reply.data());
						writer.onRead(QuarksCloud::broker, rep.data(), rep.size());
	
					}else {
						
						if(writerChain){ // push to next consumer through new producer
							
							if(!firstInChain){
								writer.onRead(QuarksCloud::save, req.data(), req.size());
							}
													
							if(!lastInChain){
								EncodedMsg e(req.data(), req.size());
								zmq::message_t writermessage (e.size());
								memcpy (writermessage.data (), e.data(), e.size());	
													
								std::cout << "Writer producing for next consumer .. ";							
								chainedWriter.send(writermessage);
							
							}
						}
											
						if(sinkConnected){
							std::cout << "Writer disptaching to sink .. ";
								
							EncodedMsg e(req.data(), req.size());
							zmq::message_t sinkmessage (e.size());
							memcpy (sinkmessage.data (), e.data(), e.size());
							
							sender.send(sinkmessage);
						}
						
					}
					
				}else{
					sleep(100);
					std::cout << "Writer waking up after 100ms !! " << std::endl;
				}
			
						
				// artificial delay 
				sleep(1); //  Give 0MQ time to deliver
				
				
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
	
	// Reader capabilities:
	// subscribe[subscriberUrl] -->	<-- publish broker[publisherUrl]
	//
	int runReader(const char* subscriberUrl, Interface& reader) {
		
		// Prepare our context and socket
		zmq::context_t context (1);
		
		// pub sub
		
	    // broker to subscribe to
		zmq::socket_t subscriber (context, ZMQ_SUB);   
		
		bool brokerConnected = false;
		if(strcmp(subscriberUrl, "!")){
			std::cout << "Reader connecting to broker .. " << std::endl;
		
		 	subscriber.connect(subscriberUrl);	
		 	brokerConnected = true;
		 		
			subscriber.setsockopt(ZMQ_SUBSCRIBE, "", 0);
					
		}
	
		std::cout << "Reader started -> subscriber url: " << subscriberUrl << std::endl;
		
		s_catch_signals ();	
		while(true){
			zmq::message_t message; 
			
			try {       
			
				if(brokerConnected){
					subscriber.recv(&message);			
					DecodedMsg msg(message.data());		
				
					reader.onRead(QuarksCloud::save, msg.data(), msg.size());
					
				}						
				
				// artificial delay 
				sleep(1); //  Give 0MQ time to deliver
				
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

}

#endif

