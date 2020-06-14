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

static int s_interrupted = 0;
static void s_signal_handler (int /*signal_value*/)
{
    s_interrupted = 1;
}

static void s_catch_signals (void)
{
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
	socket.bind ("tcp://*:5555");

	s_catch_signals ();
	while (true) {
		
		zmq::message_t request;

		try{

			//  Wait for next request from client
			socket.recv (&request);
			std::cout << "Received Hello" << std::endl;

			//  Do some 'work'
			sleep(1);

			//  Send reply back to client
			zmq::message_t reply (5);
			memcpy (reply.data (), "World", 5);
			socket.send (reply);
		
		}catch(zmq::error_t& e) {
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

int runSubscriber ()
{
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

#endif

