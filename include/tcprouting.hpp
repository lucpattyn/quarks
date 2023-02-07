#ifndef _TCPROUTING_H_
#define _TCPROUTING_H_

#include <routing.hpp>

class TCPRouting : public Routing {

	public:
	static std::string DispatchRequest(const char* data);
		
};


#endif 
