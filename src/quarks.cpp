#include <quarks.hpp>
#include <v8engine.hpp>
#include <HTTPRequest.hpp>

#include <qsorter.hpp>

#include <quarkscloud.hpp>

#include "rocksdb/db.h"
#include "rocksdb/utilities/checkpoint.h"

#include <iomanip>

#include <iostream>

#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.

#include <algorithm>

#include <shared_mutex>


using namespace boost::asio;
using ip::tcp;

using namespace Quarks;

// Mutex definition Area
std::mutex __put_mtx;
std::shared_mutex __readerwriter_mtx;

std::mutex __wildextract_mtx;
std::mutex __getkeysmulti_mtx;

std::mutex __critical_mtx;

std::mutex __cache_mtx;
//


Core Core::_Instance;
//Matrix Matrix::_Instance;

int wildcmp(const char *wild, const char *str) {
	// Written by Jack Handy - <A href="mailto:jakkhandy@hotmail.com">jakkhandy@hotmail.com</A>
	const char *cp = NULL, *mp = NULL;

	while ((*str) && (*wild != '*')) {
		if ((*wild != *str) && (*wild != '?')) {
			return 0;
		}
		wild++;
		str++;
	}

	while (*str) {
		if (*wild == '*') {
			if (!*++wild) {
				return 1;
			}
			mp = wild;
			cp = str+1;
		} else if ((*wild == *str) || (*wild == '?')) {
			wild++;
			str++;
		} else {
			wild = mp;
			str = cp++;
		}
	}

	while (*wild == '*') {
		wild++;
	}
	return !*wild;
}

/// Caching section ///

const bool _CACHE_COUNTONLY = true;
const bool _CACHE_REVERSED  = true;

bool _CacheEnabled = false;
std::map<std::string, std::string> _CacheMap;

bool _GetCached(std::string key, std::string& value){
	std::lock_guard<std::mutex> lock(__cache_mtx);
	value = _CacheMap[key];
	
	return true;
}

bool _PutCached(std::string key, std::string value){
	std::lock_guard<std::mutex> lock(__cache_mtx);
	_CacheMap[key] = value;
	
	return true;
}

int _GetCachedKeys(bool countOnly, bool reversed, std::string wild,
                   std::vector<crow::json::wvalue>& matchedResults,
                   int skip = 0, int limit = -1) {

	int ret = 0;
	
	std::size_t found = wild.find("*");
	if(found != std::string::npos && found == 0) {
		return 0;
	}
	std::string prefix = wild.substr(0, found);
	
	std::string firstAfterPrefix = prefix;
	++firstAfterPrefix[firstAfterPrefix.length() - 1];

	std::lock_guard<std::mutex> lock(__cache_mtx);
		
	auto prefixedBeginIt = _CacheMap.lower_bound(prefix);
	auto prefixedEndIt = _CacheMap.lower_bound(firstAfterPrefix);

	auto itBegin = reversed?prefixedEndIt : prefixedBeginIt;
	auto itEnd = reversed?prefixedBeginIt : prefixedEndIt; 
	
	int i  = -1;
	int count = (limit == -1) ? INT_MAX : limit;

	int lowerbound  = skip - 1;
	int upperbound = skip + count;
	
	if(reversed){
		itBegin--;
	}else{
		itEnd--;
	}
	
	// if first entry doesn't match return
	if(!wildcmp(wild.c_str(), itBegin->first.c_str())){
		return 0;
	}
	
	for (   ; itBegin != _CacheMap.end(); reversed?--itBegin : ++itBegin) {

		if(wildcmp(wild.c_str(), itBegin->first.c_str())) {

			i++;

			if(i > lowerbound && (i < upperbound || limit == -1)) {
				ret++;
				
				if(!countOnly){
					crow::json::wvalue w;

					try {
						auto x = crow::json::load(itBegin->second);
	
						if(!x) {
							w["value"] =  crow::json::load(std::string("[\"") +
							                               itBegin->second + std::string("\"]"));
	
						} else {
							w["value"] = x;
						}
	
					} catch (const std::runtime_error& error) {
						CROW_LOG_INFO << "Runtime Error: " << i << prefixedBeginIt->second;
	
						w["error"] =  itBegin->second;
	
						matchedResults.push_back(std::move(w));
	
					}
	
					w["key"] = itBegin->first;
	
					//CROW_LOG_INFO << "w key fwd: " << crow::json::dump(w) << " skip: "
					//<< skip << ", limit: " << limit;
	
					matchedResults.push_back(std::move(w));	
	
				}
				
			}

			if((i == upperbound) && (limit != -1)) {
				break;
			}
			
			if(itBegin == itEnd){
				break;
			}

		}
	}


	return ret;
}

int _GetCachedKeys(bool countOnly, bool reversed, std::string wild,
                   std::string& out,
                   int skip = 0, int limit = -1) {

	int ret = 0;
	
	std::size_t found = wild.find("*");
	if(found != std::string::npos && found == 0) {
		return 0;
	}
	std::string prefix = wild.substr(0, found);
	
	std::string firstAfterPrefix = prefix;
	++firstAfterPrefix[firstAfterPrefix.length() - 1];

	//std::lock_guard<std::mutex> lock(__cache_mtx);
		
	auto prefixedBeginIt = _CacheMap.lower_bound(prefix);
	auto prefixedEndIt = _CacheMap.lower_bound(firstAfterPrefix);

	auto itBegin = reversed?prefixedEndIt : prefixedBeginIt;
	auto itEnd = reversed?prefixedBeginIt : prefixedEndIt; 
	
	int i  = -1;
	int count = (limit == -1) ? INT_MAX : limit;

	int lowerbound  = skip - 1;
	int upperbound = skip + count;
	
	if(reversed){
		itBegin--;
	}else{
		itEnd--;
	}
	
	// if first entry doesn't match return
	if(!wildcmp(wild.c_str(), itBegin->first.c_str())){
		out = R"({"result":[]})";
		return 0;
	}
	
	out = R"({"result":[)";
	for (   ; itBegin != _CacheMap.end(); reversed?--itBegin : ++itBegin) {
		
		if(wildcmp(wild.c_str(), itBegin->first.c_str())) {
			i++;
			if(i > lowerbound && (i < upperbound || limit == -1)) {
				ret++;
				if(!countOnly){
					out += std::string(R"({"key":")") +  itBegin->first + std::string(R"(","value":)");
					out += itBegin->second + R"(},)";
				}		
			}

			if((i == upperbound) && (limit != -1)) {
				break;
			}
			
			if(itBegin == itEnd){
				break;
			}
		}	
		
	}
	out[out.size()-1] = ']';
	out = out + '}';

	return ret;
}

/// end of Caching section ///

rocksdb::DB* db = nullptr;
rocksdb::Status dbStatus;

void openDB(std::string schemaname) {
	rocksdb::Options options;
	options.create_if_missing = true;
	dbStatus = rocksdb::DB::Open(options, schemaname, &db);

	CROW_LOG_INFO << "opening db: " << schemaname;

	if(_CacheEnabled){
		rocksdb::ReadOptions ro;
		rocksdb::Iterator* it = db->NewIterator(ro);
		
		int cacheCount = 0;
		for (it->SeekToFirst(); it->Valid(); it->Next()) {
			_PutCached(it->key().ToString(), it->value().ToString());
			cacheCount++;
		}
		
		CROW_LOG_INFO << "Cache enabled! Total entries cached: " << cacheCount;
		
		delete it;
	}
}

void closeDB(std::string schemaname) {

	CROW_LOG_INFO << "closing db: " << schemaname;

	// close the database
	delete db; // deleting causes segmentation fault, let the memory leak take over :(

}

void tokenize(std::string input, char delim, std::vector<std::string>& tokens){
	std::istringstream ss(input);
	
	std::string token;
	while(std::getline(ss, token, delim)) {
    	tokens.push_back(token);
	}
}


// Quarks Cloud 

void qcSave(void* data, size_t size){
	std::string jsonData = (char*) data;
	
	auto x = crow::json::load(jsonData);		
	if(!x){
		std::cout << "__Writer received invalid put request!" << std::endl;
		
	}else{
		std::string key = x["key"].s();
		std::string value = x["value"].s();
		std::string action = x["action"].s();
		
		std::cout << "__Writer action - " << action << " : " << key << " , " << value << std::endl;

		rocksdb::Slice keySlice = key;			
		
		// modify the database
		if (dbStatus.ok()) {
	
			rocksdb::Status status;
			if(!action.compare("put")){
				status = db->Put(rocksdb::WriteOptions(), keySlice, value);					
			}else if(!action.compare("remove")){
				status = db->Delete(rocksdb::WriteOptions(), key);
			}	
					
			if(!status.ok()) {
				std::cout << "__Writer could not operate on db!" << std::endl;
				
			}
		}
		
	}
}

class QCWriter : public QuarksCloud::Interface {
public:
	virtual void onRead(int type, void* data, size_t size){
		std::cout << "Writer received: " << (char*)data << std::endl;
		
		if(type == QuarksCloud::save){
			qcSave(data, size);
		}		
	
	}

};

class QCReader : public QuarksCloud::Interface {
public:
	virtual void onRead(int type, void* data, size_t size){
		std::cout << "Reader received: " << (char*)data << std::endl;

		if(type == QuarksCloud::save){
			qcSave(data, size);
		}		
		
	}	

};

static QCWriter __WriterNode;
static QCReader __ReaderNode;

static std::string producerUrl = "";
static std::string consumerUrl = "";

// end Quarks Cloud


Core::Core() : _portNumber(18080) , _timeout(0) {

}

Core::~Core() {

}

void Core::setEnvironment(int argc, char** argv) {

	std::vector<std::string> arguments(argv + 1, argv + argc);
	_argv = std::move(arguments);
	_argc = argc;

	std::string schemaname = "quarks_db";
	_hooksocket = true;

	bool schema = false;
	bool port = false;
	bool interceptsocket = false;
	
	bool brokerFlag = false;
	bool readerFlag = false;
	bool writerFlag = false;
	
	bool producerFlag = false;
	bool consumerFlag = false;
	
	bool sinkFlag = false;
	
	bool tcpServerFlag = false;
	bool tcpClientFlag = false;
	
	bool loggerFlag = false;
	
	bool timeoutFlag = false;
	
	_broker = _writer = _reader = false;
	_tcpServer = _tcpClient = false;
	_hasLogger = false;
	
	for(auto v : _argv) {
		CROW_LOG_INFO << " v = " << v << " ";
		if(schema) {
			schemaname = v;
			schema = false;
			
		} else if(port) {
			_portNumber = std::stoi(v);
			port = false;
			
		} else if(interceptsocket) {
			_hooksocket = false;
			interceptsocket = false;
			
		}else if(brokerFlag){
			_broker = true;
			_brokerBindUrl = v;
			brokerFlag = false;
			
		}else if(sinkFlag){
			_sinkUrl = v;
			sinkFlag = false;
			
		}else if(readerFlag){
			_reader = true;
			_brokerUrl = v;
			readerFlag = false;
			
		}else if(writerFlag){
			_writer = true;
			_brokerUrl = v;
			writerFlag = false;
			
		}else if(producerFlag){
			producerUrl = v;
			producerFlag = false;
			
		}else if(consumerFlag){
			consumerUrl = v;
			consumerFlag = false;
			
		}else if(tcpServerFlag){
			_tcpUrl = v;
			_tcpServer = true;
			tcpServerFlag = false;
			
		}else if(tcpClientFlag){
			_tcpUrl= v;
			_tcpClient = true;
			tcpClientFlag = false;
			
		}else if(loggerFlag){
			_loggerUrl= v;
			_hasLogger = true;
			loggerFlag = false;
			
		}else if(timeoutFlag){
			_timeout = std::stoi(v);
			timeoutFlag = false;
		}

		if(!v.compare("-db") || !v.compare("-store")) {
			schema = true;
		} else if(!v.compare("-port")) {
			port = true;
		} else if(!v.compare("-nohooksocket")) {
			interceptsocket = true;
		} else if(!v.compare("-broker")) {
			brokerFlag = true;
		} else if(!v.compare("-sink")) {
			sinkFlag = true;
		} else if(!v.compare("-reader")) {
			readerFlag = true;
		} else if(!v.compare("-writer")) {
			writerFlag = true;
		} else if(!v.compare("-producer") || (!v.compare("-publisher"))) {
			producerFlag = true;
		} else if(!v.compare("-consumer") || (!v.compare("-subscriber"))) {
			consumerFlag = true;
		} else if(!v.compare("-tcpserver")) {
			tcpServerFlag = true;
		} else if(!v.compare("-tcpclient")) {
			tcpClientFlag = true;
		} else if(!v.compare("-log")) {
			loggerFlag = true;
		} else if(!v.compare("-timeout")) {
			timeoutFlag = true;
		} else if(!v.compare("-cached")){
			_CacheEnabled = true;
		}
	}

	_dbPath = schemaname;
	openDB(schemaname);
	
	_argv.push_back(schemaname);

}


void Core::run() {
	
	if(_broker){	
		CROW_LOG_INFO << "broker starting .. ";
		try{
			// Broker is a request receiver from writer as well as a publisher for reader nodes
			// Broker can also act as sync for multiple consumers
			
			std::vector<std::string> tokens;
			tokenize(_brokerBindUrl, ':', tokens);
			
			
			auto portNumber = tokens.size() > 1 ? std::stoi(tokens[2]) : 5556;
			
			if(!producerUrl.compare("")){
				producerUrl = std::string("tcp://*:") + std::to_string(portNumber + 1); // being treated as a publisherUrl
			}
			if(!_sinkUrl.compare("x")){
				_sinkUrl = "tcp://*:5558";
			}
			QuarksCloud::runBroker(_brokerBindUrl.c_str(), producerUrl.c_str(), _sinkUrl.c_str());
		
		} catch(const std::runtime_error& error) {	
		
			CROW_LOG_INFO << "runtime error encountered: " << error.what();
		
		}
		
	}else{
		try {
			if(_writer){
				CROW_LOG_INFO << "writer starting .. ";
				
				// Writer node is by default a producer and a consumer of it's own message
				if(!producerUrl.compare("")){
					producerUrl = "tcp://*:5557";
				}			
				if(!consumerUrl.compare("")){
					consumerUrl = "tcp://localhost:5557";
				}
				if(!_sinkUrl.compare("x")){
					_sinkUrl = "tcp://*:5558";
										
				}else if(!_sinkUrl.compare("+")){
					 _sinkUrl = "tcp://localhost:5558";
				}		
				
				QuarksCloud::runWriter(_brokerUrl.c_str(), producerUrl.c_str(), consumerUrl.c_str(), _sinkUrl.c_str(), __WriterNode);
				
			}else if(_reader){
				CROW_LOG_INFO << "reader starting .. ";
				// Reader node is a subscriber to broker
				if(!_brokerUrl.compare("+")){
					_brokerUrl = "tcp://localhost:5556"; // being treated as a subscriberUrl
				}
				QuarksCloud::runReader(_brokerUrl.c_str(), __ReaderNode);
			}
			
			//runSubscriber();	
		} catch(const std::runtime_error& error) {	
		
			CROW_LOG_INFO << "runtime error encountered: " << error.what();
		
		}
	}	
	
	//runPublisher();
}

void Core::shutDown() {
	closeDB(_argv.back());
}

int Core::getPort() {
	return _portNumber;
}

int Core::getTimeout(){
	return _timeout;
}

bool Core::shouldHookSocket() {
	return _hooksocket;
}

std::string getCurrentTimestamp(){
	using namespace std::chrono;
	milliseconds ms = duration_cast< milliseconds >(
    	system_clock::now().time_since_epoch()
	);

	return std::to_string(ms.count());

}

bool getKeyValuePair(std::string key, std::string& value, std::string& out) {
	bool ret = false;

	if (dbStatus.ok()) {
		try {
			rocksdb::Status status = db->Get(rocksdb::ReadOptions(), key, &value);
			if(status.ok()){
				ret = true;
				out = R"({"result":true})";
				
			} else {
				out = R"({"result":false})";
			}

		} catch (const std::runtime_error& error) {
			CROW_LOG_INFO << "Runtime Error: " <<  key;
			out = R"({"error":"runtime error"})";

		}


	} else {
		out = R"({"error":"db status error"})";
	}

	return ret;
}


void httpPost(std::string url, std::string key, std::string value){
	try
	{
	    http::Request request(url.c_str());
	    // pass parameters as a map
	    //crow::json::wvalue w;
		//w["value"] = value;
		//w["key"] = key;
		
		//std::string data = crow::json::dump(w);
		std::string data = std::string("{") + R"("key":")" + key + R"(", "value":)" + value + std::string("}");    
		const http::Response response = request.send("POST", data, {
	        "Content-Type: application/json"
	    });
	    //const http::Response response = request.send("GET");
	    std::cout << "httpPost: " << std::string(response.body.begin(), response.body.end()) << std::endl; // print the result
	}	
	catch (const std::exception& e)
	{
	    std::cerr << "Request failed, error: " << e.what() << '\n';
	}
}

void httpGet(std::string url){
	try
	{
	    // you can pass http::InternetProtocol::V6 to Request to make an IPv6 request
	    http::Request request(url);
	
	    // send a get request
	    const http::Response response = request.send("GET");
	    std::cout << "httpGet: " << std::string(response.body.begin(), response.body.end()) << std::endl; // print the result
	}
	catch (const std::exception& e)
	{
	    std::cerr << "Request failed, error: " << e.what() << '\n';
	}
}

void putRequest(std::string key, std::string value){

	if(_CacheEnabled){
		//_CachedMap[key] = value;
		_PutCached(key, value);
	}
			
	if(Core::_Instance.isWriterNode()){
		//CROW_LOG_INFO << "publishing put ..";

		std::lock_guard<std::mutex> _(__put_mtx);
	
		crow::json::wvalue w;
		w["value"] = value;
		w["key"] = key;
		w["action"] = "put";
		
		std::string data = crow::json::dump(w);
		
		__WriterNode.write(data.c_str(), data.size()+1); // 1 added for a trailing zero
	}
	
	if(Core::_Instance.hasLogger()){
		std::string putUrl = std::string(Core::_Instance.getLoggerUrl()) + "/putjson";
		httpPost(putUrl, key, value);
	}
	
}

void removeRequest(std::string key){
	
	if(Core::_Instance.isWriterNode()){
		//CROW_LOG_INFO << "publishing remove ..";
	
		std::lock_guard<std::mutex> _(__put_mtx);

		crow::json::wvalue w;
		w["value"] = "";
		w["key"] = key;
		w["action"] = "remove";
		
		std::string data = crow::json::dump(w);
		
		__WriterNode.write(data.c_str(), data.size()+1); // 1 added for a trailing zero
	}

	if(Core::_Instance.hasLogger()){
		std::string removeUrl = std::string(Core::_Instance.getLoggerUrl()) + "/remove?key=" + key;
		httpGet(removeUrl);
	}


}

bool insertKeyValuePair(bool failIfExists, crow::json::rvalue& x, std::string& out) {

	std::string key = x["key"].s();
	//CROW_LOG_INFO << "key found :  " << key;

	std::string result = "true";
	std::string error = "";

	std::string readValue;
	bool foundKey = getKeyValuePair(key, readValue, out);
	if(failIfExists) {
		if(foundKey) {
			result = "false";
			error = ",\"error\":\"key already exists\"";

			out = std::string("{") + R"("result":)" + result + error + std::string("}");

			return false;
		}
	}

	crow::json::wvalue w;
	crow::json::wvalue putValue = x["value"];

	bool moveValueJson = true;
	if(foundKey) {
		crow::json::wvalue existingValues = crow::json::load(readValue);

		std::vector<std::string> putValueKeys = putValue.keys();

		if(putValueKeys.size() > 0) {
			moveValueJson = false;

			std::vector<std::string> existingKeys = existingValues.keys();

			for(auto e : existingKeys) {
				w[e] = std::move(existingValues[e]);
			}
			for(auto p : putValueKeys) {
				w[p] = std::move(putValue[p]);
			}

		}

	}

	if(moveValueJson) {
		w = std::move(putValue);
	}

	std::string writeValue = crow::json::dump(w);

	//CROW_LOG_INFO << "put body : " << "key : " << key << ", value >> " << writeValue << "\n";

	bool ret = false;

	rocksdb::Slice keySlice = key;

	// modify the database
	if (dbStatus.ok()) {
		ret = true;

		rocksdb::Status status = db->Put(rocksdb::WriteOptions(), keySlice, writeValue);
		if(!status.ok()) {
			//result = "{\"error\":\"data failed to save\"}";
			result = "false";
			error = ",\"error\":\"data failed to save\"";
			ret = false;
			
		}else{
			putRequest(key, writeValue);
			
		}

	} else {
		//result = "{\"error\":\"db status error\"}";
		result = "false";
		error = ",\"error\":\"db status error\"";
		ret = false;
	}

	//std::stringstream ss;
	//ss<< "{" << std::quoted("result") << ":" << std::quoted(key) << "}";
	if(ret) {
		out = std::string("{") + R"("result":)" + result + std::string("}");
	} else {
		out = std::string("{") + R"("result":)" + result + error + std::string("}");
	}

	return  ret;
}

bool Core::insert(bool failIfExists, std::string body, std::string& out) {
	auto x = crow::json::load(body);

	if (!x) {
		CROW_LOG_INFO << "invalid put body" << body;
		//out = "invalid put body";

		out = "{\"error\": \"Invalid put body\"}";

		return false;

	}

	return insertKeyValuePair(failIfExists, x, out);
}

bool Core::post(std::string body, std::string& out) {
	return insert(true, body, out);

}

bool Core::put(std::string body, std::string& out) {
	return insert(false, body, out);

}

bool Core::dump(std::string key, std::string value, std::string& out) {
	
	bool ret = false;
	
	out = R"({"result":false})";

	rocksdb::Slice keySlice = key;

	// modify the database
	if (dbStatus.ok()) {

		rocksdb::Status status = db->Put(rocksdb::WriteOptions(), keySlice, value);

		if(status.ok()) {
			out = R"({"result":true})";
			
			putRequest(key, value);
			
			ret = true;
		}
	}

	return ret;
}

bool Core::putPair(crow::json::rvalue& pair,  std::string& out) {

	return insertKeyValuePair(false, pair, out);

}

bool Core::putAtom(crow::json::rvalue& x, std::string& out) {
	out = std::string("{") + R"("result":false)" + std::string("}");

	bool ret = true;

	for(auto& v : x) {
		//CROW_LOG_INFO << ".. put: " << crow::json::dump(v);
		ret &= insertKeyValuePair(false, v, out);
		if(!ret) {
			break;
		}
	}

	return ret;
}

bool Core::putAtom(std::string body, std::string& out) {
	auto x = crow::json::load(body);
	//CROW_LOG_INFO << "put body" << body;
	if (!x) {
		CROW_LOG_INFO << "invalid put body" << body;
		//out = "invalid put body";

		out = "{\"error\": \"Invalid put body\"}";

		return false;

	}

	return putAtom(x, out);

}


bool Core::exists(std::string key, std::string& out) {
	std::string value;
	return getKeyValuePair(key, value, out);
}

bool Core::get(std::string key, std::string& value) {
	
	if(_CacheEnabled){
		return _GetCached(key, value);
	}

	bool ret = false;

	if (dbStatus.ok()) {
		rocksdb::Status status = db->Get(rocksdb::ReadOptions(), key, &value);

		if(status.ok()) {
			ret = true;
		}
	}

	return ret;

}

bool Core::getAll(std::string wild,
                  std::vector<crow::json::wvalue>& matchedResults,
                  int skip /*= 0*/, int limit /*= -1*/) {

	bool ret = true;
	if (dbStatus.ok()) {

		std::size_t found = wild.find("*");
		if(found != std::string::npos && found == 0) {
			return false;
		}
		// create new iterator
		rocksdb::ReadOptions ro;
		rocksdb::Iterator* it = db->NewIterator(ro);

		std::string pre = wild.substr(0, found);

		rocksdb::Slice prefix(pre);

		//rocksdb::Slice prefixPrint = prefix;
		//CROW_LOG_INFO << "prefix : " << prefixPrint.ToString();


		int i  = -1;
		int count = (limit == -1) ? INT_MAX : limit;

		int lowerbound  = skip - 1;
		int upperbound = skip + count;

		for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {

			if(wildcmp(wild.c_str(), it->key().ToString().c_str())) {
				crow::json::wvalue w;

				try {

					i++;

					if(i > lowerbound && (i < upperbound || limit == -1)) {
						auto x = crow::json::load(it->value().ToString());

						if(!x) {
							w =  crow::json::load(std::string("[\"") +
							                      it->value().ToString() + std::string("\"]"));

						} else {
							w = x;
						}

						//CROW_LOG_INFO << "w fwd: " << crow::json::dump(w) << " skip: "
						//<< skip << ", limit: " << limit;

						matchedResults.push_back(std::move(w));

					}

					if((i == upperbound) && (limit != -1)) {
						break;
					}

				} catch (const std::runtime_error& error) {
					CROW_LOG_INFO << "Runtime Error: " << i << it->value().ToString();

					w["error"] =  it->value().ToString();

					matchedResults.push_back(std::move(w));

					ret = false;
				}

			}
		}

		// do something after loop

		delete it;

	}

	return ret && dbStatus.ok();
}

// Needs serious review
bool Core::getSorted(std::string wild, std::string sortby, bool ascending,
                     std::vector<crow::json::wvalue>& matchedResults,
                     int skip /*= 0*/, int limit /*= -1*/, std::string filter /*= ""*/) {

	bool ret = true;

	if (dbStatus.ok()) {

		std::size_t found = wild.find("*");
		if(found != std::string::npos && found == 0) {
			return false;
		}

		// create new iterator
		rocksdb::ReadOptions ro;
		rocksdb::Iterator* it = db->NewIterator(ro);

		std::string pre = wild.substr(0, found);

		rocksdb::Slice prefix(pre);

		//rocksdb::Slice prefixPrint = prefix;
		//CROW_LOG_INFO << "prefix : " << prefixPrint.ToString();


		int i  = -1;
		int count = (limit == -1) ? INT_MAX : limit;

		int lowerbound  = skip - 1;
		int upperbound = skip + count;

		//bool isNumber = false;
		bool isSortable = sortby.size() > 0;


		std::vector<crow::json::rvalue> allResults;


		auto filterBy = crow::json::load(filter);
		bool applyFilter = !filterBy ? false : true;

		for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {
			std::string key = it->key().ToString().c_str();
			if(wildcmp(wild.c_str(), key.c_str())) {

				try {
					std::string value = it->value().ToString();

					std::string keyVal = + R"({"value":)";

					bool addQuote = false;
					if(value.size() > 0 && value[0] != '{') {
						if(value[0] != '"') {
							keyVal += '"';
							addQuote = true;
						}
					}
					keyVal += value;
					if(addQuote) {
						keyVal += '"';
					}
					keyVal += std::string(",") + R"("key":")";
					keyVal +=  key + R"("})";

					//CROW_LOG_INFO << "get sorted keyVal: " << keyVal << ". " << i;
					auto x = crow::json::load(keyVal);

					if(!x) {
						isSortable = false;
					} else {

						try {
							if(x.t() == crow::json::type::Object) {
								if(filter.size() > 0) {
									CROW_LOG_INFO << "apply filter: " << applyFilter << ". " << crow::json::dump(filterBy);
								}
								if(!applyFilter || QSorter::JsonComparer().validate(x["value"], filterBy)) {
									i++;
									CROW_LOG_INFO << "obj to sort: " << i << ". " << crow::json::dump(x);
									allResults.push_back(std::move(x));
								}

							}

						} catch (const std::runtime_error& error) {
							CROW_LOG_INFO << "Runtime Sort Error 1: " << error.what();
							isSortable = false;

							ret = false;

						};

					}

				} catch (const std::runtime_error& error) {
					CROW_LOG_INFO << "Runtime Sort Error 2: " << error.what() << " , " << i << " . " << it->value().ToString();
					//w["error"] =  it->value().ToString();
					isSortable = false;

					//matchedResults.push_back(std::move(w));

					ret = false;
				}

			}
		}

		// do something after loop

		if(isSortable) {
			//CROW_LOG_INFO << "sorter: " << sortby;
			//Sorter::JsonComparer c(sortby, ascending);
			try {
				std::sort(allResults.begin(), allResults.end(), QSorter::JsonComparer(sortby));

			} catch (const std::runtime_error& error) {
				CROW_LOG_INFO << "Runtime Sort Error 3: Not Sortable";
				ret = false;
			}

		}


		i = -1;
		if(!ascending) {
			for(auto& x : QSorter::backwards< std::vector<crow::json::rvalue> >(allResults)) {
				i++;
				if(i > lowerbound && (i < upperbound || limit == -1)) {
					try {
						crow::json::wvalue w;
						w["value"] = std::move(x["value"]);
						w["key"] = std::move(x["key"]);
						//CROW_LOG_INFO << "sorted object: " << i << ". " << crow::json::dump(w);
						matchedResults.push_back(std::move(w));

					} catch (const std::runtime_error& error) {
						CROW_LOG_INFO << "Runtime Sort Error 4.1: " << error.what() << " , " << i;
					}
				}

				if((i == upperbound) && (limit != -1)) {
					break;
				}
			}
		} else {
			for(auto& x : allResults) {
				i++;
				if(i > lowerbound && (i < upperbound || limit == -1)) {
					try {
						crow::json::wvalue w;
						w["value"] = std::move(x["value"]);
						w["key"] = std::move(x["key"]);

						//CROW_LOG_INFO << "sorted object: " << i << ". " << crow::json::dump(w);
						matchedResults.push_back(std::move(w));

					} catch (const std::runtime_error& error) {
						CROW_LOG_INFO << "Runtime Sort Error 4.2: " << error.what() << " , " << i;
					}
				}

				if((i == upperbound) && (limit != -1)) {
					break;
				}
			}

		}


		delete it;

	}

	return ret && dbStatus.ok();
}

bool Core::getKeys(std::string wild,
                   std::vector<crow::json::wvalue>& matchedResults,
                   int skip /*= 0*/, int limit /*= -1*/) {

	if(_CacheEnabled){
		_GetCachedKeys(!_CACHE_COUNTONLY, !_CACHE_REVERSED, wild, matchedResults, skip, limit);
		
		return true;
	}
	
	bool ret = true;
	if (dbStatus.ok()) {

		std::size_t found = wild.find("*");
		if(found != std::string::npos && found == 0) {
			return false;
		}

		// create new iterator
		rocksdb::ReadOptions ro;
		rocksdb::Iterator* it = db->NewIterator(ro);

		std::string pre = wild.substr(0, found);

		rocksdb::Slice prefix(pre);

		//rocksdb::Slice prefixPrint = prefix;
		//CROW_LOG_INFO << "prefix : " << prefixPrint.ToString();


		int i  = -1;
		int count = (limit == -1) ? INT_MAX : limit;

		int lowerbound  = skip - 1;
		int upperbound = skip + count;

		for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {

			if(wildcmp(wild.c_str(), it->key().ToString().c_str())) {

				i++;

				if(i > lowerbound && (i < upperbound || limit == -1)) {
					crow::json::wvalue w;

					try {
						auto x = crow::json::load(it->value().ToString());

						if(!x) {
							w["value"] =  crow::json::load(std::string("[\"") +
							                               it->value().ToString() + std::string("\"]"));

						} else {
							w["value"] = x;
						}

					} catch (const std::runtime_error& error) {
						CROW_LOG_INFO << "Runtime Error: " << i << it->value().ToString();

						w["error"] =  it->value().ToString();

						matchedResults.push_back(std::move(w));

						ret = false;
					}

					w["key"] = it->key().ToString();

					//CROW_LOG_INFO << "w key fwd: " << crow::json::dump(w) << " skip: "
					//<< skip << ", limit: " << limit;

					matchedResults.push_back(std::move(w));

				}

				if((i == upperbound) && (limit != -1)) {
					break;
				}

			}
		}

		// do something after loop

		delete it;

	}

	return ret && dbStatus.ok();
}

bool Core::getKeys(std::string wild,
                   std::string& out,
                   int skip /*= 0*/, int limit /*= -1*/) {

	if(_CacheEnabled){
		_GetCachedKeys(!_CACHE_COUNTONLY, !_CACHE_REVERSED, wild, out, skip, limit);
		
		return true;
	}
	
	bool ret = true;
	if (dbStatus.ok()) {

		std::size_t found = wild.find("*");
		if(found != std::string::npos && found == 0) {
			return false;
		}

		// create new iterator
		rocksdb::ReadOptions ro;
		rocksdb::Iterator* it = db->NewIterator(ro);

		std::string pre = wild.substr(0, found);

		rocksdb::Slice prefix(pre);

		//rocksdb::Slice prefixPrint = prefix;
		//CROW_LOG_INFO << "prefix : " << prefixPrint.ToString();


		int i  = -1;
		int count = (limit == -1) ? INT_MAX : limit;

		int lowerbound  = skip - 1;
		int upperbound = skip + count;

		out = R"({"result":[)";
		for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {

			if(wildcmp(wild.c_str(), it->key().ToString().c_str())) {

				i++;

				if(i > lowerbound && (i < upperbound || limit == -1)) {
					out += std::string(R"({"key":")") +  it->key().ToString() + std::string(R"(","value":)");
					out += it->value().ToString() + R"(},)";
				}

				if((i == upperbound) && (limit != -1)) {
					break;
				}

			}
		}
		
		if(i > -1){
			out[out.size()-1] = ']';
			out = out + '}';
			
		} else{
			out = R"({"result":[]})";
		}
		

		// do something after loop

		delete it;

	}

	return ret && dbStatus.ok();
}

// Needs review and improvement - now kept for example of sorting
//bool legacyGetKeysReversed(std::string wild,
//                           std::vector<crow::json::wvalue>& matchedResults,
//                           int skip /*= 0*/, int limit /*= -1*/) {

/*	if(_CacheEnabled){
		_GetCachedKeys(!_CACHE_COUNTONLY, _CACHE_REVERSED, wild, matchedResults, skip, limit);
		
		return true;
	}
	
	bool ret = true;
	if (dbStatus.ok()) {

		std::size_t found = wild.find("*");
		if(found != std::string::npos && found == 0) {
			return false;
		}

		// create new iterator
		rocksdb::ReadOptions ro;
		rocksdb::Iterator* it = db->NewIterator(ro);

		std::string pre = wild.substr(0, found);

		rocksdb::Slice prefix(pre);

		//rocksdb::Slice prefixPrint = prefix;
		//CROW_LOG_INFO << "prefix : " << prefixPrint.ToString();


		int i  = -1;
		int count = (limit == -1) ? INT_MAX : limit;

		int lowerbound  = skip - 1;
		int upperbound = skip + count;

		std::vector<crow::json::wvalue> allResults;

		for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {

			if(wildcmp(wild.c_str(), it->key().ToString().c_str())) {

				crow::json::wvalue w;
				try {
					auto x = crow::json::load(it->value().ToString());

					if(!x) {
						w["value"] =  crow::json::load(std::string("[\"") +
						                               it->value().ToString() + std::string("\"]"));

					} else {
						w["value"] = x;
					}

				} catch (const std::runtime_error& error) {
					CROW_LOG_INFO << "Runtime Error: " << i << it->value().ToString();

					w["error"] =  it->value().ToString();

					allResults.push_back(std::move(w));

					ret = false;
				}

				w["key"] = it->key().ToString();

				//CROW_LOG_INFO << "w key fwd: " << crow::json::dump(w) << " skip: "
				//<< skip << ", limit: " << limit;

				allResults.push_back(std::move(w));


			}
		}

		for(auto& x : QSorter::backwards< std::vector<crow::json::wvalue> >(allResults)) {
			i++;
			if(i > lowerbound && (i < upperbound || limit == -1)) {
				try {
					//crow::json::wvalue wb = x;
					//CROW_LOG_INFO << "sorted object: " << i << ". " << crow::json::dump(w);
					matchedResults.push_back(std::move(x));

				} catch (const std::runtime_error& error) {
					CROW_LOG_INFO << "Runtime Sort Error 4.1: " << error.what() << " , " << i;
				}
			}

			if((i == upperbound) && (limit != -1)) {
				break;
			}
		}


		// do something after loop

		delete it;

	}

	return ret && dbStatus.ok();*/
//}

bool Core::getKeysReversed(std::string wild,
                           std::vector<crow::json::wvalue>& matchedResults,
                           int skip /*= 0*/, int limit /*= -1*/){
	if(_CacheEnabled){
		_GetCachedKeys(!_CACHE_COUNTONLY, _CACHE_REVERSED, wild, matchedResults, skip, limit);
		
		return true;
	}
	
	bool ret = true;
	if (dbStatus.ok()) {

		std::size_t found = wild.find("*");
		if(found != std::string::npos && found == 0) {
			return false;
		}

		// create new iterator
		rocksdb::ReadOptions ro;
		rocksdb::Iterator* it = db->NewIterator(ro);

		std::string pre = wild.substr(0, found);
	
		std::string firstAfterPrefix = pre;
		++firstAfterPrefix[firstAfterPrefix.length() - 1];
	
		rocksdb::Slice prefix(pre);
		rocksdb::Slice prefixRev(firstAfterPrefix);

		//rocksdb::Slice prefixPrint = prefixRev;
		//CROW_LOG_INFO << "prefix : " << prefixPrint.ToString();

		int i  = -1;
		int count = (limit == -1) ? INT_MAX : limit;

		int lowerbound  = skip - 1;
		int upperbound = skip + count;

		it->Seek(prefixRev);
		it->Prev();
		
		for ( ;it->Valid() && it->key().starts_with(prefix); it->Prev()) {

			if(wildcmp(wild.c_str(), it->key().ToString().c_str())) {

				i++;

				if(i > lowerbound && (i < upperbound || limit == -1)) {
					crow::json::wvalue w;

					try {
						auto x = crow::json::load(it->value().ToString());

						if(!x) {
							w["value"] =  crow::json::load(std::string("[\"") +
							                               it->value().ToString() + std::string("\"]"));

						} else {
							w["value"] = x;
						}

					} catch (const std::runtime_error& error) {
						CROW_LOG_INFO << "Runtime Error: " << i << it->value().ToString();

						w["error"] =  it->value().ToString();

						matchedResults.push_back(std::move(w));

						ret = false;
					}

					w["key"] = it->key().ToString();

					//CROW_LOG_INFO << "w key fwd: " << crow::json::dump(w) << " skip: "
					//<< skip << ", limit: " << limit;

					matchedResults.push_back(std::move(w));

				}

				if((i == upperbound) && (limit != -1)) {
					break;
				}

			}
		}

		// do something after loop

		delete it;

	}

	return ret && dbStatus.ok();
}

bool Core::getKeysReversed(std::string wild,
                           std::string& out,
                           int skip /*= 0*/, int limit /*= -1*/){
	if(_CacheEnabled){
		_GetCachedKeys(!_CACHE_COUNTONLY, _CACHE_REVERSED, wild, out, skip, limit);		
		return true;
	}
	
	bool ret = true;
	if (dbStatus.ok()) {

		std::size_t found = wild.find("*");
		if(found != std::string::npos && found == 0) {
			return false;
		}

		// create new iterator
		rocksdb::ReadOptions ro;
		rocksdb::Iterator* it = db->NewIterator(ro);

		std::string pre = wild.substr(0, found);
	
		std::string firstAfterPrefix = pre;
		++firstAfterPrefix[firstAfterPrefix.length() - 1];
	
		rocksdb::Slice prefix(pre);
		rocksdb::Slice prefixRev(firstAfterPrefix);

		//rocksdb::Slice prefixPrint = prefixRev;
		//CROW_LOG_INFO << "prefix : " << prefixPrint.ToString();

		int i  = -1;
		int count = (limit == -1) ? INT_MAX : limit;

		int lowerbound  = skip - 1;
		int upperbound = skip + count;

		it->Seek(prefixRev);
		it->Prev();
		
		out = R"({"result":[)";
		for ( ;it->Valid() && it->key().starts_with(prefix); it->Prev()) {

			if(wildcmp(wild.c_str(), it->key().ToString().c_str())) {

				i++;

				if(i > lowerbound && (i < upperbound || limit == -1)) {
					out += std::string(R"({"key":")") +  it->key().ToString() + std::string(R"(","value":)");
					out += it->value().ToString() + R"(},)";
				}

				if((i == upperbound) && (limit != -1)) {
					break;
				}
			}
		}

		// do something after loop
		if(i > -1){
			out[out.size()-1] = ']';
			out = out + '}';
			
		} else{
			out = R"({"result":[]})";
		}
		
		delete it;

	}

	return ret && dbStatus.ok();
}

// experimental thread codes /////////
bool getKeysInThread(std::string wild,
					std::vector<crow::json::wvalue>& matchedResults,
		            int skip, int limit, std::atomic<int>& atomic_index, std::atomic<bool>& exit){
	bool ret = true;
	if (dbStatus.ok()) {

		std::size_t found = wild.find("*");
		if(found != std::string::npos && found == 0) {
			return false;
		}

		// create new iterator
		rocksdb::ReadOptions ro;
		rocksdb::Iterator* it = db->NewIterator(ro);

		std::string pre = wild.substr(0, found);

		rocksdb::Slice prefix(pre);

		//rocksdb::Slice prefixPrint = prefix;
		//CROW_LOG_INFO << "prefix : " << prefixPrint.ToString();
		int count = (limit == -1) ? INT_MAX : limit;

		int lowerbound  = skip - 1;
		int upperbound = skip + count;

		for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {
			if(exit){
				break;
			}

			if(wildcmp(wild.c_str(), it->key().ToString().c_str())) {

				const int i = ++atomic_index;
				
				if(i > lowerbound && (i < upperbound || limit == -1)) {
					crow::json::wvalue w;

					try {
						auto x = crow::json::load(it->value().ToString());

						if(!x) {
							w["value"] =  crow::json::load(std::string("[\"") +
							                               it->value().ToString() + std::string("\"]"));

						} else {
							w["value"] = x;
						}

					} catch (const std::runtime_error& error) {
						CROW_LOG_INFO << "Runtime Error: " << i << it->value().ToString();

						w["error"] =  it->value().ToString();

						std::lock_guard<std::mutex> lock(__getkeysmulti_mtx);
						matchedResults.push_back(std::move(w));

						ret = false;
					}

					w["key"] = it->key().ToString();

					//CROW_LOG_INFO << "w key fwd: " << crow::json::dump(w) << " skip: "
					//<< skip << ", limit: " << limit;
					std::lock_guard<std::mutex> lock(__getkeysmulti_mtx);
					matchedResults.push_back(std::move(w));

				}

				if((i == upperbound) && (limit != -1)) {
					exit = true;
					break;
				}

			}
		}
		// do something after loop

		delete it;

	}

	return ret && dbStatus.ok();
			            	
}

bool Core::getKeysMulti(std::string wilds,
			             std::vector<crow::json::wvalue>& matchedResults,
			             int skip /*= 0*/, int limit /*= -1*/){
	
	std::atomic<bool> ret(true);
		
	try{	
		auto x = crow::json::load(wilds);
		std::atomic<int> wildSize(x.size());
  		std::atomic<int> wildIndex(-1); 
  		
  		const unsigned int numWorkers = std::thread::hardware_concurrency();
		std::vector<std::thread> workers;
  		
  		std::atomic<int> atomic_index(-1);
  		std::atomic<bool> exit(false);
  		
  		for (unsigned int j = 0; j < numWorkers; ++j) {
      		workers.push_back(std::thread([&]{
      			
      			while(1){
      				const int wildIndx = ++wildIndex;
      				if(wildIndx < wildSize){
						try{
	      					std::string wild = "";
							{
								std::lock_guard<std::mutex> lock(__wildextract_mtx);
								auto& r = x[wildIndx];
								wild = r.s();
							}
							
	      					ret = ret && getKeysInThread(wild, matchedResults, skip, limit, atomic_index, exit);
	      					
						} catch (const std::runtime_error& error) {
							CROW_LOG_INFO << "Runtime Error: getKeysMulti, wild index " << wildIndx;
		
							ret = false;
						}
					} else{
						break;
					}
				}

			}));
    	}
				
    	for (std::thread &t: workers) {
      		if (t.joinable()) {
        		t.join();
      		}		
    	}
		
	} catch (const std::runtime_error& error) {
		CROW_LOG_INFO << "Runtime Error: " << wilds ;
	
		ret = false;
	}
	
	return ret;
}

//bool getCountThreaded(std::string wild,
//                    long& out,
//                    int skip /*= 0*/, int limit /*= -1*/){
//    
//    bool ret = true;
//    if (!dbStatus.ok()){
//		return false;
//	}
//	out = 0;
//	
//	std::size_t found = wild.find("*");
//	if(found != std::string::npos && found == 0) {
//		return false;
//	}
//
//	static std::string lastWild = "";
//	static std::vector<std::string> iterKeys;
//	
//	std::string pre = wild.substr(0, found);
//	std::string preRev = pre;
//	preRev[preRev.size() - 1] = pre[pre.size() - 1] + 1;
//
//	rocksdb::Slice prefix(pre);
//	rocksdb::Slice prefixRev(preRev);
//
//	//rocksdb::Slice prefixPrint = prefix;
//	//CROW_LOG_INFO << "prefix : " << prefixPrint.ToString();
//
//
//	int i  = -1;
//	int count = (limit == -1) ? INT_MAX : limit;
//
//	int lowerbound  = skip - 1;
//	int upperbound = skip + count;
//
//	
//	std::atomic<int> totalCount(0);
//	
//	std::string iterKey1 = "";		
//	std::string iterKey2 = "";
//		
//	int count1 = 0, count2 = 0;
//	
//	if(lastWild.compare(wild)){
//		iterKeys.clear();
//		lastWild = wild;
//	}else{
//		
//		std::vector<std::thread> v(8); 
//  		
//  		int size = iterKeys.size()/8;
//  		std::atomic<int> tc(0);
//		for(auto& t: v){
//			t = std::thread([&]{ 
//					std::lock_guard<std::mutex> lock(__critical_mtx);  	
//					
//					std::cout << "tc: " << tc << " size: " << (tc * size  + size) << " real size: " << size;
//					for(int n = tc * size; n < (tc * size  + size); ++n){						
//						if(wildcmp(wild.c_str(), iterKeys[n].c_str())){
//							totalCount++;
//						}
//					}	
//					
//				tc++;				  
//			});
//	
//		}
//		
//		for(auto& t: v){
//			t.join();
//		}
//		
//		out = totalCount;
//		CROW_LOG_INFO << "returning from cache : " << totalCount;
//		
//		return true;
//	}
//	
//	std::thread iter1([&]{ 	
//		//std::lock_guard<std::shared_mutex> writerLock(__readerwriter_mtx);			
//		
//		// create new iterator
//		rocksdb::ReadOptions ro;
//		rocksdb::Iterator* it = db->NewIterator(ro);
//		
//		const char* wildstr = wild.c_str();
//		CROW_LOG_INFO << "prefix : " << pre;
//		
//		
//		for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {
//			iterKey1 = it->key().ToString().c_str();
//			
//			if(wildcmp(wildstr, iterKey1.c_str())){
//				totalCount++;
//				count1++;
//			
//				
//			}	
//			
//			std::lock_guard<std::mutex> lock(__critical_mtx);  	
//			iterKeys.push_back(iterKey1);		
//			
//			if(iterKey1.size() && iterKey2.size() && iterKey1.compare(iterKey2)  > 0){
//				std::cout << "breaking iter 1 .. "  << iterKey1.c_str() << ", " << iterKey2.c_str() << count1 << std::endl;
//				break;
//			}
//		}	
//		
//		ret = ret && it->status().ok();
//		delete it;
//	});	
//	 
//	std::thread iter2([&]{ 	
//		//std::lock_guard<std::shared_mutex> writerLock(__readerwriter_mtx);			
//		
//		// create new iterator
//		rocksdb::ReadOptions ro;
//		rocksdb::Iterator* it = db->NewIterator(ro);
//		
//		const char* wildstr = wild.c_str();
//		CROW_LOG_INFO << "prefixRev : " << preRev;
//		
//		it->Seek(prefixRev);
//		it->Prev();
//		
//		for ( ;it->Valid() && it->key().starts_with(prefix); it->Prev()) {
//			iterKey2 = it->key().ToString().c_str();
//			
//			if(wildcmp(wildstr, iterKey2.c_str())){
//				totalCount++;
//				count2++;
//				
//				
//			}
//		
//			std::lock_guard<std::mutex> lock(__critical_mtx);  
//			iterKeys.push_back(iterKey2);
//				
//			if(iterKey2.compare(iterKey1)  < 0){
//				break;
//			}
//		}	
//		
//		ret = ret && it->status().ok();
//		delete it;
//	});		
//	 
//
//	iter1.join();
//	iter2.join();
//
//	out = totalCount;
//	
//	std::cout << "\ncount1: " << count1 << " count2: "  << count2;
//	
//	return ret;
//}

/// end of experimental thread codes 

bool Core::getCount(std::string wild,
                    long& out,
                    int skip /*= 0*/, int limit /*= -1*/) {

	//return getCountThreaded(wild, out, skip, limit);
	if(_CacheEnabled){
		std::vector<crow::json::wvalue> matchedResults;
		out = _GetCachedKeys(_CACHE_COUNTONLY, !_CACHE_REVERSED, wild, matchedResults, skip, limit);
		return true;
	}
	
	bool ret = true;
	if (dbStatus.ok()) {
		out = 0;

		std::size_t found = wild.find("*");
		if(found != std::string::npos && found == 0) {
			return false;
		}

		// create new iterator
		rocksdb::ReadOptions ro;
		rocksdb::Iterator* it = db->NewIterator(ro);

		std::string pre = wild.substr(0, found);

		rocksdb::Slice prefix(pre);

		//rocksdb::Slice prefixPrint = prefix;
		//CROW_LOG_INFO << "prefix : " << prefixPrint.ToString();


		int i  = -1;
		int count = (limit == -1) ? INT_MAX : limit;

		int lowerbound  = skip - 1;
		int upperbound = skip + count;

		for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {

			if(wildcmp(wild.c_str(), it->key().ToString().c_str())) {

				i++;

				if(i > lowerbound && (i < upperbound || limit == -1)) {

					out++;
				}

				if((i == upperbound) && (limit != -1)) {
					break;
				}

			}
		}



		// do something after loop

		delete it;

	}

	return ret && dbStatus.ok();
}

bool Core::iter(std::vector<crow::json::wvalue>& matchedResults,
                	int skip /*= 0*/, int limit /*= -1*/) {

	bool ret = true;
	if (dbStatus.ok()) {

		//CROW_LOG_INFO << "iter1 starting .. " ;
		
		// create new iterator
		rocksdb::ReadOptions ro;
		rocksdb::Iterator* it = db->NewIterator(ro);

		int i  = -1;
		int count = (limit == -1) ? INT_MAX : limit;

		int lowerbound  = skip - 1;
		int upperbound = skip + count;

		for (it->SeekToFirst(); it->Valid(); it->Next()) {
			i++;
			//CROW_LOG_INFO << "iter " << i ;
		
			if(i > lowerbound && (i < upperbound || limit == -1)) {
				crow::json::wvalue w;

				try {
					auto x = crow::json::load(it->value().ToString());

					if(!x) {
						w["value"] =  crow::json::load(std::string("[\"") +
						                               it->value().ToString() + std::string("\"]"));

					} else {
						w["value"] = x;
					}

				} catch (const std::runtime_error& error) {
					CROW_LOG_INFO << "Runtime Error: " << i << it->value().ToString();

					w["error"] =  it->value().ToString();

					matchedResults.push_back(std::move(w));

					ret = false;
				}

				w["key"] = it->key().ToString();

				//CROW_LOG_INFO << "w key fwd: " << crow::json::dump(w) << " skip: "
				//<< skip << ", limit: " << limit;

				matchedResults.push_back(std::move(w));

			}

			if((i == upperbound) && (limit != -1)) {
				break;
			}

		}
		// do something after loop

		delete it;

	}

	return ret && dbStatus.ok();

}

bool Core::iterReversed(std::vector<crow::json::wvalue>& matchedResults,
                	int skip /*= 0*/, int limit /*= -1*/) {

	bool ret = true;
	if (dbStatus.ok()) {

		//CROW_LOG_INFO << "iter1 starting .. " ;
		
		// create new iterator
		rocksdb::ReadOptions ro;
		rocksdb::Iterator* it = db->NewIterator(ro);

		int i  = -1;
		int count = (limit == -1) ? INT_MAX : limit;

		int lowerbound  = skip - 1;
		int upperbound = skip + count;

		for (it->SeekToLast(); it->Valid(); it->Prev()) {
			i++;
			//CROW_LOG_INFO << "iter " << i ;
		
			if(i > lowerbound && (i < upperbound || limit == -1)) {
				crow::json::wvalue w;

				try {
					auto x = crow::json::load(it->value().ToString());

					if(!x) {
						w["value"] =  crow::json::load(std::string("[\"") +
						                               it->value().ToString() + std::string("\"]"));

					} else {
						w["value"] = x;
					}

				} catch (const std::runtime_error& error) {
					CROW_LOG_INFO << "Runtime Error: " << i << it->value().ToString();

					w["error"] =  it->value().ToString();

					matchedResults.push_back(std::move(w));

					ret = false;
				}

				w["key"] = it->key().ToString();

				//CROW_LOG_INFO << "w key fwd: " << crow::json::dump(w) << " skip: "
				//<< skip << ", limit: " << limit;

				matchedResults.push_back(std::move(w));

			}

			if((i == upperbound) && (limit != -1)) {
				break;
			}

		}
		// do something after loop

		delete it;

	}

	return ret && dbStatus.ok();

}

bool removeIterating(std::string key, std::string& out) {
	bool ret = false;

	// Delete value
	if (dbStatus.ok()) {
		rocksdb::ReadOptions ro;
		rocksdb::Iterator* it = db->NewIterator(ro);

		try {
			rocksdb::Slice prefix(key);
			//rocksdb::Slice prefixPrint = prefix;
			//CROW_LOG_INFO << "prefix : " << prefixPrint.ToString();

			it->Seek(prefix);
			if(it->Valid() && !key.compare(it->key().ToString())) {

				rocksdb::Status status = db->Delete(rocksdb::WriteOptions(), key);

				if(status.ok()) {
					ret = true;
					removeRequest(key);
					
					out = R"({"result":1})";

				} else {
					out = R"({"result":-1})";
				}

			} else {
				ret = true;
				out = R"({"result":0})";
			}

		} catch (const std::runtime_error& error) {
			CROW_LOG_INFO << "Runtime Error: " <<  it->value().ToString();

			out = R"({"result":-1})";

		}

		delete it;

	} else {
		out = R"({"result":-1})";
	}

	return ret;

}

bool Core::remove(std::string key, std::string& out) {
	bool ret = false;

	// Delete value
	if (dbStatus.ok()) {

		try {
			std::string value;
			
			rocksdb::Status status = db->Get(rocksdb::ReadOptions(), key, &value);
			if(status.ok()){
				rocksdb::Status delstatus = db->Delete(rocksdb::WriteOptions(), key);				
				if(delstatus.ok()) {
					ret = true;
					removeRequest(key);
					
					out = R"({"result":1})";
	
				} else {
					out = R"({"result":0})";
				}
				
			}else {
					out = R"({"result":0})";
			}			
				

		} catch (const std::runtime_error& error) {
			CROW_LOG_INFO << "Runtime Error: Remove " <<  key << " ..";

			out = R"({"result":-1})";

		}


	} else {
		out = R"({"result":-1})";
	}

	return ret;

}

int Core::removeAll(std::string wild,  int skip /*= 0*/, int limit /*= -1*/) {
	int out = 0;
	if (dbStatus.ok()) {
		std::size_t found = wild.find("*");
		if(found != std::string::npos && found == 0) {
			return out;
		}

		// create new iterator
		rocksdb::ReadOptions ro;

		std::string pre = wild.substr(0, found);

		rocksdb::Slice prefix(pre);

		//rocksdb::Slice prefixPrint = prefix;
		//CROW_LOG_INFO << "prefix : " << prefixPrint.ToString();

		int i  = -1;
		int count = (limit == -1) ? INT_MAX : limit;

		int lowerbound  = skip - 1;
		int upperbound = skip + count;

		rocksdb::Iterator* it = db->NewIterator(ro);

		try {

			rocksdb::WriteBatch batch;
			for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {

				if(wildcmp(wild.c_str(), it->key().ToString().c_str())) {
					i++;

					if(i > lowerbound && (i < upperbound || limit == -1)) {
						CROW_LOG_INFO << "w batch del: " << it->key().ToString();
						batch.Delete(it->key());
						// needs improvement
						std::string strKey = it->key().ToString();
						removeRequest(strKey);
						
						out++;

					}

					if((i == upperbound) && (limit != -1)) {
						break;
					}

				}

			}

			// do something after loop
			delete it;

			rocksdb::Status s = db->Write(rocksdb::WriteOptions(), &batch);
			
			if(!s.ok()){
				CROW_LOG_INFO << "Batch Delete Error.. ";
			}

		} catch (const std::runtime_error& error) {
			CROW_LOG_INFO << "Runtime Error: " << out << " " << it->value().ToString();

			delete it;

		}

		return out;

	}

	return out;
}

bool Core::removeAtom(crow::json::rvalue& x, std::string& out) {

	bool ret = true;

	int i = 0;
	for(auto& v : x) {
		ret &= remove(v.s(), out);
		if(!ret) {
			break;
		}

		i++;
	}
	if(ret) {
		out = std::string("{") + R"("result":)" + std::to_string((int) i) + std::string("}");

	} else {
		std::string error = ",\"error\":\"could not remove all keys\"";
		out = std::string("{") + R"("result":)" + std::to_string(i) + error + std::string("}");
	}

	return ret;

}

bool Core::removeAtom(std::string body, std::string& out) {
	auto x = crow::json::load(body);

	if (!x) {
		CROW_LOG_INFO << "invalid put body" << body;
		//out = "invalid put body";

		out = "{\"error\": \"Invalid put body\"}";

		return false;

	}

	return removeAtom(x, out);

}


bool Core::makePair(std::string body, crow::json::wvalue& out) {

	auto x = crow::json::load(body);

	if (!x) {
		CROW_LOG_INFO << "invalid make body" << body;
		//out = "invalid put body";

		out = "{\"error\": \"Invalid make body\"}";

		return false;

	}


	std::string key = "";
	std::string writeValue = "";

	std::string tmpkey = "";

	if(x.has("key")) {
		key = x["key"].s();
	} else {
		boost::uuids::uuid uuid = boost::uuids::random_generator()();
		std::cout << uuid << std::endl;
		key = boost::lexical_cast<std::string>(uuid);

	}
	tmpkey = key;

	if(x.has("prefix")) {
		std::string prefix = x["prefix"].s();

		key = prefix + tmpkey;
	}
	if(x.has("value")) {
		writeValue = x["value"].s();
	}

	out["value"] = writeValue;
	out["key"] = key;

	CROW_LOG_INFO << "put body : " << "key : " << key << ", value >> " << writeValue << "\n";


	bool save = false;
	if(x.has("save")) {
		save = x["save"].b();
	}

	bool ret = !save;

	if(save) {
		rocksdb::Slice keySlice = key;

		std::string error;
		// modify the database
		if (dbStatus.ok()) {
			ret = true;

			rocksdb::Status status = db->Put(rocksdb::WriteOptions(), keySlice, writeValue);
			if(!status.ok()) {
				error = "data failed to save";
				ret = false;
			}else{
				putRequest(key, writeValue);
			}

		} else {
			//result = "{\"error\":\"db status error\"}";
			error = "db status error";
			ret = false;
		}

		//std::stringstream ss;
		//ss<< "{" << std::quoted("result") << ":" << std::quoted(key) << "}";
		if(!ret) {
			out["error"] = error;
		}
	}


	return ret;

}


bool Core::getJson(std::string key, crow::json::wvalue& out) {

	bool ret = false;

	/*std::map<std::string, crow::json::rvalue>::iterator it = _Core.find(key);
	 if (it != _Core.end()){
	 out = it->second;
	 ret = true;
	 }*/
	if (dbStatus.ok()) {
		std::string value;
		rocksdb::Status status = db->Get(rocksdb::ReadOptions(), key, &value);

		if(status.ok()) {
			out =  crow::json::load(value);
			ret = true;
		}
	}

	return ret;

}

// to be discarded later
bool prefixIter(rocksdb::Iterator*& it, std::string wild,
                std::vector<crow::json::wvalue>& matchedResults,
                int skip /*= 0*/, int limit /*= -1*/) {

	std::size_t found = wild.find("*");
	if(found != std::string::npos && found == 0) {
		return false;
	}

	std::string pre = wild.substr(0, found);

	rocksdb::Slice prefix(pre);

	//rocksdb::Slice prefixPrint = prefix;
	//CROW_LOG_INFO << "prefix : " << prefixPrint.ToString();



	// Go Backwards
	//it->Seek(prefix);

	//CROW_LOG_INFO << "key : " <<  it->key().ToString();

	/*for ( ; it->Valid() && it->key().starts_with(prefix); it->Prev()) {
	 if(wildcmp(wild.c_str(), it->key().ToString().c_str())){
	 crow::json::wvalue w;
	 auto x = crow::json::load(it->value().ToString());

	 if(!x){
	 w =  crow::json::load(std::string("[\"") +
	 it->value().ToString() + std::string("\"]"));

	 }else{
	 w = x;
	 }

	 CROW_LOG_INFO << "w backwd : " << crow::json::dump(w);

	 matchedResults.push_back(std::move(w));

	 }
	 // do something
	 }*/

	// Go Forward
	//it->Seek(prefix); // already processed so need to start from next
	int i  = -1;
	int count = (limit == -1) ? INT_MAX : limit;

	int lowerbound  = skip - 1;
	int upperbound = skip + count;

	for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {

		if(wildcmp(wild.c_str(), it->key().ToString().c_str())) {
			crow::json::wvalue w;
			auto x = crow::json::load(it->value().ToString());

			if(!x) {
				w =  crow::json::load(std::string("[\"") +
				                      it->value().ToString() + std::string("\"]"));

			} else {
				w = x;
			}

			i++;
			if(i > lowerbound && (i < upperbound || limit == -1)) {
				//CROW_LOG_INFO << "w fwd: " << crow::json::dump(w) << " skip: "
				//<< skip << ", limit: " << limit;
				matchedResults.push_back(std::move(w));

			}

			if((i == upperbound) && (limit != -1)) {
				break;
			}

		}

	}

	// do something after loop


	return true;

}

bool Core::getList(crow::json::rvalue& args,
                   std::vector<crow::json::wvalue>& matchedResults,
                   int skip /*= 0*/, int limit /*= -1*/) {


	bool ret = true;

	if (dbStatus.ok()) {

		try {
			int i  = -1;
			int count = (limit == -1) ? INT_MAX : limit;

			int lowerbound  = skip - 1;
			int upperbound = skip + count;

			crow::json::wvalue out;
			crow::json::wvalue w;

			// iterate all entries
			for (auto key : args) {
				bool valid = false;
		
				std::string s = key.s();
				if(getJson(s, out)) {
					w["value"] = std::move(out);
					w["key"] = s;
					
					valid = true;

				} //else {
					//w["error"] = i+1; // i is incremented later ..
				//}
				
				i++;
				if(i > lowerbound && (i < upperbound || limit == -1)) {
					if(valid){
						matchedResults.push_back(std::move(w));
					}					
				}

			}

		} catch (const std::runtime_error& error) {
			ret = false;

		}

	}

	return ret;

}

bool Core::getItems(crow::json::rvalue& args,
                   crow::json::wvalue& jsonResult,
                   int skip /*= 0*/, int limit /*= -1*/) {


	bool ret = true;

	if (dbStatus.ok()) {

		try {
			int i  = -1;
			int count = (limit == -1) ? INT_MAX : limit;

			int lowerbound  = skip - 1;
			int upperbound = skip + count;

			crow::json::wvalue w;
			// iterate all entries
			for (auto key : args) {		
				std::string s = key.s();
				i++;
				if(i > lowerbound && (i < upperbound || limit == -1)) {
					if (dbStatus.ok()) {
						std::string value;
						rocksdb::Status status = db->Get(rocksdb::ReadOptions(), s, &value);
						if(status.ok()) {
							jsonResult[s] =  std::move(crow::json::load(value));
							ret = ret && true;
							
						}else{
							//jsonResult[s] = "not found";
							ret = false;
						}
					}
				}

			}

		} catch (const std::runtime_error& error) {
			ret = false;

		}

	}

	return ret;

}

/* body:
{
	"keys":"g1*",
	"splitby":"_",
	"selindex":5,
	join:[{"prefix":"g2","suffix":""}, {"prefix":"g3","suffix":""}],
	
}
*/

bool getJoinedMapThreaded(crow::json::rvalue& args,
                      std::vector<crow::json::wvalue>& matchedResults,
                      int skip /*= 0*/, int limit /*= -1*/){

	
	bool ret = dbStatus.ok(); 
	if (ret) {

		std::string wild = args["keys"].s();
		//CROW_LOG_INFO << "keys : " << wild.c_str();
		
		std::string splitby = args["splitby"].s();
		//CROW_LOG_INFO << "splitby : " << splitby;		
		
		size_t selindex = (size_t) args["selindex"].i();
		//CROW_LOG_INFO << "selindex : " << selindex;	
				
		const crow::json::rvalue& join = args["join"];
		

		std::size_t found = wild.find("*");
		if(found != std::string::npos && found == 0) {
			return false;
		}

		// create new iterator
		//rocksdb::ReadOptions ro;
		//rocksdb::Iterator* it = db->NewIterator(ro);

		std::string pre = wild.substr(0, found);

		rocksdb::Slice prefix(pre);

		//rocksdb::Slice prefixPrint = prefix;
		//CROW_LOG_INFO << "prefix : " << prefixPrint.ToString();


		int i  = -1;
		int count = (limit == -1) ? INT_MAX : limit;

		int lowerbound  = skip - 1;
		int upperbound = skip + count;

		std::vector<crow::json::wvalue> finalKeys;
		crow::json::wvalue k;							
		crow::json::wvalue w;							
								
		std::vector<std::string> sFinalKeys;
								
		std::vector<std::string> iteratorKeys;
		int nextKey = -1;
		int currentKey = -1;
		
		std::atomic<bool> writerExit(false);
		std::atomic<bool> readerExit(false);
		
		std::thread writer([&]{ 	
			//std::lock_guard<std::shared_mutex> writerLock(__readerwriter_mtx);			
			
			// create new iterator
			rocksdb::ReadOptions ro;
			rocksdb::Iterator* it = db->NewIterator(ro);
						
			for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {
				std::lock_guard<std::mutex> lock(__critical_mtx);  
				iteratorKeys.push_back(it->key().ToString().c_str());
				//CROW_LOG_INFO << "iterating : " << it->key().ToString().c_str();	
				nextKey++;
				
				//sleep_for_ms(1);
				//std::this_thread::yield();
				if(writerExit){
					break;
				}
			}	
			
			readerExit = true;
			
			ret = ret && it->status().ok();
			delete it;
		});	
		 
		std::thread reader([&]{ 
			//std::shared_lock<std::shared_mutex> readerLock(__readerwriter_mtx);
			
			const char* wildstr = wild.c_str();
			while(1){
				//if(wildcmp(wild.c_str(), it->key().ToString().c_str())) {
				bool compare = false;
				int next = -1;
				const char* iterKey = "";			
				{
					std::lock_guard<std::mutex> lock(__critical_mtx);  
					next = nextKey;
					if(currentKey < next){
						compare = true;
						iterKey = iteratorKeys[++currentKey].c_str();
					}
				}
					
				//CROW_LOG_INFO << "reading : " << iterKey;	
				if(compare && wildcmp(wildstr, iterKey)) {
					 
					i++;
	
					if(i > lowerbound && (i < upperbound || limit == -1)) {
	
						std::string matchedKey = iterKey;
						
						std::vector<std::string> tokens;
						
						//CROW_LOG_INFO << "matchedKey : " << matchedKey;
						tokenize(matchedKey, splitby[0], tokens);
						
						//CROW_LOG_INFO << "token size : " << tokens.size();	
					
						std::string finalKey = matchedKey;
						if(selindex < tokens.size()){
							finalKey = tokens[selindex];
							sFinalKeys.push_back(finalKey);
							//CROW_LOG_INFO << "reading finalKey : " << finalKey;	
							crow::json::wvalue f;
							f = finalKey;					
									
							finalKeys.push_back(std::move(f));
						}				
						
						/*std::string prefix = "";
						std::string suffix = "";
						std::string joinedKey = "";*/
						
										
					}
	
					if((i == upperbound) && (limit != -1)) {
						writerExit = true;
						break;
					}
	
				}
		
				if(currentKey == nextKey){
					if(readerExit)
						break;
				
					std::this_thread::yield();
					//CROW_LOG_INFO << "reader yielding .. ";	
					
				}
				
				//sleep_for_ms(1);
				
			}
		
			//std::cout << "reader exiting.." << std::endl;
			
		});	
		
		writer.join();
		reader.join();
		//writer.join();
		
		std::string prefx = "";
		std::string suffx = "";
		std::string joinedKey = "";
		
		for(auto& v : join) {					
			try {		
				prefx = v["prefix"].s();
				suffx = v["suffix"].s();							
				
				for(auto& finalKey : sFinalKeys){
				
					joinedKey = prefx + finalKey + suffx;		
					
					crow::json::wvalue out;
		 			
		 			if (dbStatus.ok()) {
						std::string value;
						rocksdb::Status status = db->Get(rocksdb::ReadOptions(), joinedKey, &value);

						if(status.ok()) {
							w[joinedKey] =  crow::json::load(value);
						}
					}
		 
				}
	
			} catch (const std::runtime_error& error) {
				CROW_LOG_INFO << "Runtime Error: " << joinedKey;
	
				ret = false;
			}
		}
		
		k = std::move(finalKeys);
		
		matchedResults.push_back(std::move(k));			
		matchedResults.push_back(std::move(w));		

		// do something after loop
		//ret = ret && it->status().ok();
		//delete it;
	}


	return ret;					  
}


bool getJoinedMapWildcardOnly(crow::json::rvalue& args,
                      std::vector<crow::json::wvalue>& matchedResults,
                      int skip /*= 0*/, int limit /*= -1*/){

	//return getJoinedMapThreaded(args, matchedResults, skip, limit);
	//return getJoinedMapOld(args, matchedResults, skip, limit);
	
	bool ret = dbStatus.ok(); 
	if (ret) {

		std::string wild = args["keys"].s();
		//CROW_LOG_INFO << "keys : " << wild.c_str();
		
		std::string splitby = args["splitby"].s();
		//CROW_LOG_INFO << "splitby : " << splitby;		
		
		size_t selindex = (size_t) args["selindex"].i();
		//CROW_LOG_INFO << "selindex : " << selindex;	
				
		const crow::json::rvalue& join = args["join"];
		

		std::size_t found = wild.find("*");
		if(found != std::string::npos && found == 0) {
			return false;
		}

		// create new iterator
		rocksdb::ReadOptions ro;
		rocksdb::Iterator* it = db->NewIterator(ro);

		std::string pre = wild.substr(0, found);

		rocksdb::Slice prefix(pre);

		//rocksdb::Slice prefixPrint = prefix;
		//CROW_LOG_INFO << "prefix : " << prefixPrint.ToString();


		int i  = -1;
		int count = (limit == -1) ? INT_MAX : limit;

		int lowerbound  = skip - 1;
		int upperbound = skip + count;

		std::vector<crow::json::wvalue> finalKeys;
		crow::json::wvalue k;							
		crow::json::wvalue w;							
								
		std::string joinedKey;
		std::vector<std::string> prefixes; 
		std::vector<std::string> suffixes;
		
		int joinSize = join.size();
		for(auto& v : join) {					
			try {		
				prefixes.push_back(v["prefix"].s());
				suffixes.push_back(v["suffix"].s());
	
			} catch (const std::runtime_error& error) {
				CROW_LOG_INFO << "Runtime Error: " << i << it->value().ToString();
	
				ret = false;
			}
		}						
								
		for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {

			if(wildcmp(wild.c_str(), it->key().ToString().c_str())) {

				i++;

				if(i > lowerbound && (i < upperbound || limit == -1)) {

					std::string matchedKey = it->key().ToString();
					
					std::vector<std::string> tokens;
					
					//CROW_LOG_INFO << "matchedKey : " << matchedKey;
					tokenize(matchedKey, splitby[0], tokens);
					
					//CROW_LOG_INFO << "token size : " << tokens.size();	
				
					std::string finalKey = matchedKey;
					if(selindex < tokens.size()){
						finalKey = tokens[selindex];
						//CROW_LOG_INFO << "finalKey : " << finalKey;	
						
						crow::json::wvalue out;				
						for(int i = 0; i < joinSize; i++){
							joinedKey = prefixes[i] + finalKey + suffixes[i];	
							
							if (dbStatus.ok()) {
								std::string value;
								rocksdb::Status status = db->Get(rocksdb::ReadOptions(), joinedKey, &value);
		
								if(status.ok()) {
									w[joinedKey] =  crow::json::load(value);
								}
							}	
						}	
					
						crow::json::wvalue f;
						f = finalKey;					
								
						finalKeys.push_back(std::move(f));
					}				
										
				}

				if((i == upperbound) && (limit != -1)) {
					break;
				}

			}
		}
		
		k = std::move(finalKeys);
		
		matchedResults.push_back(std::move(k));			
		matchedResults.push_back(std::move(w));		

		// do something after loop
		ret = ret && it->status().ok();
		delete it;
	}


	return ret;					  
}

bool Core::getJoinedMap(crow::json::rvalue& args,
                      std::vector<crow::json::wvalue>& matchedResults,
                      int skip /*= 0*/, int limit /*= -1*/){

	//return getJoinedMapThreaded(args, matchedResults, skip, limit);
	
	bool ret = dbStatus.ok(); 
	if (ret) {

		std::string wild = "";
		if(args.has("keys")){
			wild = args["keys"].s(); 
		}
		
		bool hasKeyList = args.has("keylist");
		//CROW_LOG_INFO << "keys : " << wild.c_str();
		bool hasSplitBy = args.has("splitby");
		std::string splitby = "";
		if(hasSplitBy){
			splitby = args["splitby"].s();
		}
		//CROW_LOG_INFO << "splitby : " << splitby;		
		bool hasSelIndex = args.has("selindex");
		int selindex =  hasSelIndex ? (size_t) args["selindex"].i() : -1;
		//CROW_LOG_INFO << "selindex : " << selindex;	
				
		const crow::json::rvalue& join = args["join"];
		
		bool hasKeys = true;
		std::size_t found = wild.find("*");
		if(found != std::string::npos && found == 0) {
			hasKeys = false;
			if(!hasKeyList){
				return false;
			}
			
		}
	

		std::vector<crow::json::wvalue> finalKeys;
		crow::json::wvalue k;							
		crow::json::wvalue w;							
									
		std::vector<std::string> sFinalKeys;						
			
		if(hasKeys){
			// create new iterator
			rocksdb::ReadOptions ro;
			rocksdb::Iterator* it = db->NewIterator(ro);
	
			std::string pre = wild.substr(0, found);
	
			rocksdb::Slice prefix(pre);
	
			//rocksdb::Slice prefixPrint = prefix;
			//CROW_LOG_INFO << "prefix : " << prefixPrint.ToString();
	
	
			int i  = -1;
			int count = (limit == -1) ? INT_MAX : limit;
	
			int lowerbound  = skip - 1;
			int upperbound = skip + count;
	
									
			for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {
	
				if(wildcmp(wild.c_str(), it->key().ToString().c_str())) {
	
					i++;
	
					if(i > lowerbound && (i < upperbound || limit == -1)) {
	
						std::string matchedKey = it->key().ToString();
						
						std::vector<std::string> tokens;
						//CROW_LOG_INFO << "matchedKey : " << matchedKey;
						if(hasSplitBy){
							tokenize(matchedKey, splitby[0], tokens);
						}
						
						//CROW_LOG_INFO << "token size : " << tokens.size();	
					
						std::string finalKey = matchedKey;
						if(selindex > -1 && (size_t)selindex < tokens.size()){
							finalKey = tokens[selindex];
						}
						sFinalKeys.push_back(finalKey);
						//CROW_LOG_INFO << "finalKey : " << finalKey;	
						crow::json::wvalue f;
						f = finalKey;					
								
						finalKeys.push_back(std::move(f));
					}
	
					if((i == upperbound) && (limit != -1)) {
						break;
					}
	
				}
			}
			
			// do something after loop
			ret = ret && it->status().ok();
			delete it;	
		}
		
		// add from keylists if provided
		if(hasKeyList){
			try{
					
				auto klist = args["keylist"];
				
				for(auto kl : klist){
					std::string matchedKey = kl.s();
					std::vector<std::string> tokens;
					if(hasSplitBy){
						tokenize(matchedKey, splitby[0], tokens);
					}
					std::string finalKey = matchedKey;
					if(selindex > -1 && (size_t)selindex < tokens.size()){
						finalKey = tokens[selindex];
					}
					
					sFinalKeys.push_back(finalKey);
						//CROW_LOG_INFO << "finalKey : " << finalKey;	
					crow::json::wvalue f;
					f = finalKey;					
								
					finalKeys.push_back(std::move(f));
				}
				
			} catch (const std::runtime_error& error) {
				CROW_LOG_INFO << "Runtime Error: keylist iterate .. ";
	
				ret = false;
			}
				
		}
		
		std::string prefx = "";
		std::string suffx = "";
		std::string joinedKey = "";
		
		for(auto& v : join) {					
			try {		
				prefx = v["prefix"].s();
				suffx = v["suffix"].s();							
				
				for(auto& finalKey : sFinalKeys){
				
					joinedKey = prefx + finalKey + suffx;		
					
					crow::json::wvalue out;
		 			
		 			if (dbStatus.ok()) {
						std::string value;
						rocksdb::Status status = db->Get(rocksdb::ReadOptions(), joinedKey, &value);

						if(status.ok()) {
							w[joinedKey] =  crow::json::load(value);
						}
					}
		 
				}
	
			} catch (const std::runtime_error& error) {
				CROW_LOG_INFO << "Runtime Error: prefix/suffix join, last joined key: " << joinedKey;
	
				ret = false;
			}
		}
		
		k = std::move(finalKeys);
		
		matchedResults.push_back(std::move(k));			
		matchedResults.push_back(std::move(w));		

		
	}


	return ret;					  
}

bool Core::getAfter(std::string key, std::string prefix, std::vector<crow::json::wvalue>& matchedResults,
						int skip /*= 0*/, int limit /*= -1*/) {

	bool ret = true;
	if (dbStatus.ok()) {

		// create new iterator
		rocksdb::ReadOptions ro;
		rocksdb::Iterator* it = db->NewIterator(ro);

		rocksdb::Slice startKey(key);
		rocksdb::Slice pre(prefix);

		int i  = -1;
		int count = (limit == -1) ? INT_MAX : limit;

		int lowerbound  = skip - 1;
		int upperbound = skip + count;

		it->Seek(startKey);
		if(it->Valid())
			it->Next(); // because we are searching after (excluding the key provided)
		for ( ; it->Valid() && it->key().starts_with(pre); it->Next()) {

			i++;

			if(i > lowerbound && (i < upperbound || limit == -1)) {
				crow::json::wvalue w;

				try {
					auto x = crow::json::load(it->value().ToString());

					if(!x) {
						w["value"] =  crow::json::load(std::string("[\"") +
						                               it->value().ToString() + std::string("\"]"));

					} else {
						w["value"] = x;
					}

				} catch (const std::runtime_error& error) {
					CROW_LOG_INFO << "Runtime Error: " << i << it->value().ToString();

					w["error"] =  it->value().ToString();

					matchedResults.push_back(std::move(w));

					ret = false;
				}

				w["key"] = it->key().ToString();

				//CROW_LOG_INFO << "w key fwd: " << crow::json::dump(w) << " skip: "
				//<< skip << ", limit: " << limit;

				matchedResults.push_back(std::move(w));

			}

			if((i == upperbound) && (limit != -1)) {
				break;
			}

		}



		// do something after loop

		delete it;

	}

	return ret && dbStatus.ok();

	
}

bool Core::getKeysAfter(crow::json::rvalue& args,
                   std::vector<crow::json::wvalue>& matchedResults,
                   int skip /*= 0*/, int limit /*= -1*/) {


	bool ret = true;

	if (dbStatus.ok()) {

		try {
			
			std::vector<crow::json::wvalue> out;
			crow::json::wvalue w;
			
			bool prefix = false;			
			std::string lastPrefix = "";
			// iterate all entries
			for (auto pair : args) {
				prefix = !prefix;
				
				std::string p = pair.s();
				if(prefix){
					lastPrefix = p;					
				}else{
					if(getAfter(p, lastPrefix, out, skip, limit)) {
						w["after"] = std::move(out);
						w["key"] = p;
						
						matchedResults.push_back(std::move(w));
					} 
				}

			}

		} catch (const std::runtime_error& error) {
			ret = false;

		}

	}

	return ret;

}


bool Core::getLast(std::string key, std::string prefix, std::vector<crow::json::wvalue>& matchedResults) {

	bool ret = true;
	if (dbStatus.ok()) {

		// create new iterator
		rocksdb::ReadOptions ro;
		rocksdb::Iterator* it = db->NewIterator(ro);

		rocksdb::Slice startKey(key);
		rocksdb::Slice pre(prefix);

		int count = 0;

		std::string lastKey;
		
		it->Seek(startKey);
		if(it->Valid())
			it->Next(); // because we are searching after (excluding the key provided)
		for ( ; it->Valid() && it->key().starts_with(pre); it->Next()) {

			count++;

		}

		if(count > 0){
			it->Prev();
			if(it->Valid()){
				crow::json::wvalue w;

				try {
					auto x = crow::json::load(it->value().ToString());

					if(!x) {
						w["value"] =  crow::json::load(std::string("[\"") +
						                               it->value().ToString() + std::string("\"]"));

					} else {
						w["value"] = x;
					}

				} catch (const std::runtime_error& error) {
					CROW_LOG_INFO << "Runtime Error: " << it->value().ToString();

					w["error"] =  it->value().ToString();

					matchedResults.push_back(std::move(w));

					ret = false;
				}
				
				w["index"] = count-1;

				w["key"] = it->key().ToString();

				
				matchedResults.push_back(std::move(w));
			
			}
		}
		


		// do something after loop
		delete it;

	}

	return ret && dbStatus.ok();

	
}

bool Core::getKeysLast(crow::json::rvalue& args,
                   std::vector<crow::json::wvalue>& matchedResults) {


	bool ret = true;

	if (dbStatus.ok()) {

		try {
			
			std::vector<crow::json::wvalue> out;
			crow::json::wvalue w;

			bool prefix = false;			
			std::string lastPrefix = "";
			// iterate all entries
			for (auto pair : args) {
				prefix = !prefix;
				
				std::string p = pair.s();
				if(prefix){
					lastPrefix = p;					
				}else{
					if(getLast(p, lastPrefix, out)) {
						w["last"] = std::move(out);
						w["key"] = p;
						
						matchedResults.push_back(std::move(w));
					} 
				}

			}

		} catch (const std::runtime_error& error) {
			ret = false;

		}

	}

	return ret;

}



bool Core::searchJson(crow::json::rvalue& args,
                      std::vector<crow::json::wvalue>& matchedResults,
                      int skip /*= 0*/, int limit /*= -1*/) {


	if (dbStatus.ok()) {

		Core& core = *this;

		v8Engine ve([](std::string log) {
			CROW_LOG_INFO << log.c_str();
		});

		std::string sArgs =  crow::json::dump(args);
		CROW_LOG_INFO << "args : " << sArgs;

		std::string wild = args["keys"].s();
		CROW_LOG_INFO << "keys : " << wild.c_str();

		crow::json::wvalue wArgs;
		wArgs["include"] = args["include"];
		CROW_LOG_INFO << "include : " << crow::json::dump(wArgs);

		crow::json::wvalue wFilter;
		wFilter["include"]= std::move(wArgs["include"]);
		CROW_LOG_INFO << "include : " << crow::json::dump(wFilter["include"]);

		std::string sFilter = crow::json::dump(wFilter["include"]);
		crow::json::rvalue filter = crow::json::load(sFilter);

		std::string mapField = filter["map"]["field"].s();
		std::string mapAs = filter["map"]["as"].s();

		std::string moduleName = filter["module"].s();
		std::string funcName = filter["filter"].s();
		std::string params = filter["params"].s();

		auto funcOnV8EngineLoaded = [&core, &ve, &wild, &funcName, &params, &mapField, &mapAs, &matchedResults, &skip, &limit]
		(v8Engine::v8Context& v8Ctx) -> int {

			// create new iterator
			rocksdb::Iterator* it = db->NewIterator(rocksdb::ReadOptions());

			bool prefixSearch = true;
			std::size_t found = wild.find("*");
			if(found != std::string::npos && found == 0) {
				prefixSearch = false;
			}

			rocksdb::Slice prefix(wild.substr(0, found));
			//for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next())

			if(!prefixSearch) {
				it->SeekToFirst();
			} else{
				it->Seek(prefix);
			}

			int i  = -1;
			int count = (limit == -1) ? INT_MAX : limit;

			int lowerbound  = skip - 1;
			int upperbound = skip + count;

			// iterate all entries
			for (  ; prefixSearch ? (it->Valid() && it->key().starts_with(prefix)) : it->Valid();
			it->Next()) {


				std::string elem = it->value().ToString();

				//CROW_LOG_INFO << "iterate : " << it->key().ToString()
				//<< ": " <<  elem; //<< endl;

				if(wildcmp(wild.c_str(), it->key().ToString().c_str())) {
					//crow::json::rvalue r;
					auto r = crow::json::load(it->value().ToString());
					if(!r) {
						r =  crow::json::load(std::string("[\"") +
						it->value().ToString() + std::string("\"]"));


					}


					//CROW_LOG_INFO << "r : " << elem;

					std::string mapKey = r[mapField.c_str()].s();
					crow::json::wvalue mapJson;

					crow::json::wvalue w = r;
					if(core.getJson(mapKey, mapJson)) {
						w[mapAs] = std::move(mapJson);
					}

					//std::string allow = ve.invoke("matcher", sfilter.c_str(), //filterParams.c_str());
					std::string e = crow::json::dump(w);
					std::string allow = ve.invoke(v8Ctx, funcName, e, params.c_str());
					if(atoi(allow.c_str()) == 1) {
						i++;

						///////////////////////////////////////

						//matchedResults.push_back(std::move(w));
						if(skip == 0 && limit == -1) {
							matchedResults.push_back(std::move(w));
							CROW_LOG_INFO << "w fwd: " << crow::json::dump(w);
						} else {
							if(i > lowerbound && i < upperbound) {
								matchedResults.push_back(std::move(w));
								CROW_LOG_INFO << "w fwd: " << crow::json::dump(w) << " skip: "
								              << skip << ", limit: " << limit;
							} else {
								break; // no need to search further
							}
						}

						///////////////////////////////////////
					}

				}
			}
			// do something after loop
			bool ret = it->status().ok();
			delete it;

			return ret;


		}; // funcOnV8EngineLoaded

		std::string moduleJS = moduleName + ".js";
		return ve.load(moduleJS, funcOnV8EngineLoaded);

	}

	return false;

}


bool Core::atom(std::string body, std::string& out) {
	auto x = crow::json::load(body);

	if (!x) {
		CROW_LOG_INFO << "invalid put body" << body;
		//out = "invalid put body";

		out = "{\"error\": \"Invalid put body\"}";

		return false;

	}

	out = std::string("{") + R"("result":false)" + std::string("}");

	bool ret = true;

	if(x.has("remove")) {
		ret &= removeAtom(crow::json::dump(x["remove"]), out);
	}
	if(x.has("removeall")) {
		int removed = removeAll(x["removeall"].s());
		out = std::string("{") + R"("result":)" + std::to_string(removed) + std::string("}");
	}
	if(ret) {
		if(x.has("put")) {
			ret &= putAtom(crow::json::dump(x["put"]), out);
		}
	}

	return ret;

}

/// counting stuff

bool Core::increment(std::string key, int stepBy, std::string& out) {
	std::lock_guard<std::mutex> _(mtx);

	out = R"({"result":false})";

	std::string val;
	bool ret = get(key, val);

	if(ret) {
		try {
			int value = std::stoi(val);
			value += stepBy;

			std::string writeValue = std::to_string(value);

			// same as dump 
			put(key, writeValue, out);

		} catch (const std::invalid_argument& ia) {
			//std::cerr << "Invalid argument: " << ia.what() << std::endl;
			out =  R"({"result":false, "error:"Value to increment is not an integer"})";
		}

		catch (const std::out_of_range& oor) {
			//std::cerr << "Out of Range error: " << oor.what() << std::endl;
			out =  R"({"result":false, "error":"Out of range"})";
		}

		catch (const std::exception& e)  {
			out =  R"({"result":false, "error:"Unknown"})";

		}

	}

	return ret;
}

bool Core::incrementValue(std::string body, std::string& out) {
	
	auto x = crow::json::load(body);

	if (!x) {
		CROW_LOG_INFO << "invalid put body" << body;
		//out = "invalid put body";

		out = "{\"error\": \"Invalid put body\"}";

		return false;

	}
	
	std::string key = x["key"].s();
	//CROW_LOG_INFO << "key found :  " << key;

	std::string result = "true";
	std::string error = "";

	std::string readValue;
	bool foundKey = getKeyValuePair(key, readValue, out);
	if(!foundKey) {
			result = "false";
			error = ",\"error\":\"key does not exist\"";

			out = std::string("{") + R"("result":)" + result + error + std::string("}");

			return false;
	}

	const crow::json::rvalue& xvalue = x["value"];
	crow::json::wvalue putValue = x["value"];
	
	crow::json::rvalue read = crow::json::load(readValue);
	crow::json::wvalue existingValues = read;
	
	std::vector<std::string> putValueKeys = putValue.keys();
	
	crow::json::wvalue w;
	
	int affected = 0;
	bool ret = false;
	// if put values is not an array then check and increment
	if(putValueKeys.size() > 0) {
		std::vector<std::string> existingKeys = existingValues.keys();

		for(auto e : existingKeys) {
			CROW_LOG_INFO << "existing key " << e;
			
			if(xvalue.has(e)){
			
				const crow::json::rvalue& evalue = read[e];
				int value = evalue.i();
				
				const crow::json::rvalue& pvalue = xvalue[e];				
				value += pvalue.i();
					
				w[e] = value;
				affected++;		
					
			} else{
				w[e] = std::move(existingValues[e]);
			}
			
		}	
		
		std::string writeValue = crow::json::dump(w);
		ret = dump(key, writeValue, out);
	}

	if(ret){
		out = std::string("{") + R"("result":)" + std::to_string(affected) + std::string("}");
	}
	
	return  true;

}

// backup and restore
bool Core::backup(std::string path){
	if(!path.compare("")){
		path = "quarks_backup";
	}

	rocksdb::Checkpoint* cp;
	rocksdb::Status status = rocksdb::Checkpoint::Create(db, &cp);
	
	if(status.ok()){
		status = cp->CreateCheckpoint(path);
	}
	
	return status.ok();
}

bool Core::restore(std::string path){
	if(!path.compare("")){
		path = "quarks_backup";
	}

	//rocksdb::BackupEngine* backup_engine = nullptr;
	//rocksdb::Status status = rocksdb::BackupEngine::Open(rocksdb::Env::Default(), rocksdb::BackupableDBOptions(path), &backup_engine);
	//backup_engine->RestoreDBFromLatestBackup(path, path);
	
	//delete backup_engine;
	
	return true;
}

// r&d 
bool Core::fileTransfer(std::string moduleName, std::string funcName, std::string channelName,
                        std::string remoteDescription) {

	//Core& core = *this;

	v8Engine ve([](std::string log) {
		CROW_LOG_INFO << log.c_str();
	});


	auto v8Loaded = [&ve, &funcName, &channelName, &remoteDescription]
	(v8Engine::v8Context& v8Ctx) -> int {

		std::string rawData = ve.invoke(v8Ctx, funcName, channelName, remoteDescription);

		//writeFile(fileName, rawData);
		CROW_LOG_INFO << rawData;

		return (rawData.size() > 0);

	};

	std::string moduleJS = moduleName + ".js";
	CROW_LOG_INFO << moduleJS;

	return ve.load(moduleJS, v8Loaded);
	// return true;
}


bool Core::openTCPSocketClient() {

	bool ret = false;

	boost::asio::io_service io_service;

	//socket creation
	tcp::socket socket(io_service);

	//connection
	socket.connect( tcp::endpoint( boost::asio::ip::address::from_string("127.0.0.1"), 1234 ));

	// request/message from client
	const std::string msg = "Hello from Client!\n";
	boost::system::error_code error;
	boost::asio::write( socket, boost::asio::buffer(msg), error );
	if( !error ) {
		CROW_LOG_INFO << "Client sent hello message!";
		ret = true;
	} else {
		ret = false;
		CROW_LOG_INFO << "send failed: " << error.message();
	}

	// getting a response from the server
	boost::asio::streambuf receive_buffer;
	boost::asio::read(socket, receive_buffer, boost::asio::transfer_all(), error);
	if( error && error != boost::asio::error::eof ) {
		ret = false;
		CROW_LOG_INFO << "receive failed: " << error.message();
	} else {
		ret = true;
		const char* data = boost::asio::buffer_cast<const char*>(receive_buffer.data());
		CROW_LOG_INFO << data;
	}

	return ret;
}

/// Socket Interceptor //////////

SocketInterceptor::SocketInterceptor(Core& quarksCore, bool notifyAllOnClose/*= true*/) {
	_core = &quarksCore;
	_notifyAllOnClose = notifyAllOnClose;
}

SocketInterceptor& SocketInterceptor::getInstance(Core& quarksCore, bool notifyAllOnClose/*= true*/) {
	static SocketInterceptor instance(quarksCore, notifyAllOnClose);
	return instance;
}

auto SocketInterceptor::lookup(const items& items, const std::string& key)
-> std::pair<items::const_iterator, items::const_iterator> {
	auto p = items.lower_bound(key);
	auto q = items.end();
	if (p != q && p->first == key) {
		return std::make_pair(p, std::next(p));
	} else {
		auto r = p;
		while (r != q && r->first.compare(0, key.size(), key) == 0) {
			++r;
		}
		return std::make_pair(p, r);
	}
}

auto SocketInterceptor::lookup(const pairs& items, const std::string& key)
-> std::pair<pairs::const_iterator, pairs::const_iterator> {
	auto p = items.lower_bound(key);
	auto q = items.end();
	if (p != q && p->first == key) {
		return std::make_pair(p, std::next(p));
	} else {
		auto r = p;
		while (r != q && r->first.compare(0, key.size(), key) == 0) {
			++r;
		}
		return std::make_pair(p, r);
	}
}

void SocketInterceptor::broadcast(std::string room, std::string data) {

	auto items = lookup(_connMap, room);
	auto itBegin = items.first;
	auto itEnd = items.second;

	if(_connMap.size() > 0) {
		for (auto it=itBegin; it!=itEnd; ++it) {
			auto u = it->second;
			u->send_text(data);
		}
	}
}

void SocketInterceptor::broadcast(std::string room, std::string data,
                                  crow::websocket::connection& skipconn) {

	auto items = lookup(_connMap, room);
	auto itBegin = items.first;
	auto itEnd = items.second;

	if(_connMap.size() > 0) {
		for (auto it=itBegin; it!=itEnd; ++it) {
			auto u = it->second;
			if(u != &skipconn) {
				u->send_text(data);
			}
		}
	}
}

void SocketInterceptor::onOpen(crow::websocket::connection& conn) {
	CROW_LOG_INFO << "QuarksSCIR::Open << " << conn.userdata();
	
	char* _id = (char*)conn.userdata();
	if(_id != nullptr) {
		std::string room = "default";
		
		std::string roomKey = room + std::string("_") + _id;
		_connMap[roomKey] = &conn;	

		CROW_LOG_INFO << "QuarksSCIR::usercreate << " << roomKey;
	}
	
}

void SocketInterceptor::onClose(crow::websocket::connection& conn) {
	char* _id = (char*)conn.userdata();
	if(_id != nullptr) {

		// Needs real improvement to find the rooms.
		std::string leaveId = _id;
		std::string userKey = leaveId + std::string("_");

		auto rooms = lookup(_userRoomsMap, userKey);
		auto itBegin = rooms.first;
		auto itEnd = rooms.second;

		if(_notifyAllOnClose) {
			
			for (auto it=itBegin; it!=itEnd; ++it) {
				auto room = it->second;
				
				crow::json::wvalue wsend;
				wsend["left"] = room;
				if(_id != nullptr) {
					wsend["from"] = _id;
				};
				wsend["timestamp"] = getCurrentTimestamp();
				std::string send = crow::json::dump(wsend);
				
				broadcast(room, send);

				std::string roomKey = room + std::string("_") + _id;
				_connMap.erase(roomKey);

			}

		}

		_userRoomsMap.erase(itBegin, itEnd);

		CROW_LOG_INFO << "QuarksSCIR::Close << " << _connMap.size() << " " << _userRoomsMap.size();

	}
}

bool SocketInterceptor::onQueryMessage(crow::websocket::connection& conn,
                                  const crow::json::rvalue& rdata, bool is_binary){
    try {

		//char* _id = (char*)conn.userdata();

		int skip = 0;
		int limit = -1;
		
		if(rdata.has("getkeys")){
			if(rdata.has("skip")){
				skip = rdata["skip"].i();
			}
			if(rdata.has("limit")){
				limit = rdata["limit"].i();
			}
			
			std::vector<crow::json::wvalue> out;
			
			if(Quarks().getKeys(rdata["getkeys"].s(), out, skip, limit)){
				crow::json::wvalue wresult;
				wresult["replygetkeys"] = std::move(out);
				wresult["skip"] = skip;
				wresult["limit"] =  limit;
				conn.send_text(crow::json::dump(wresult));
			}
				
		}

	} catch (const std::runtime_error& error) {
		CROW_LOG_INFO << "runtime error : invalid data parameters in onquerymessage - " << crow::json::dump(rdata);
	}

	return true;
		
}

bool SocketInterceptor::onMessage(crow::websocket::connection& conn,
                                  const std::string& data, bool is_binary) {

	if(data.size() == 0) {
		CROW_LOG_INFO << "empty data" << data;
		return true;

	}

	auto x = crow::json::load(data);
	if (!is_binary && !x) {
		CROW_LOG_INFO << "invalid message body: " << data;
		return true;

	}

	try {

		char* _id = (char*)conn.userdata();

		if(x.has("join")) {
			std::string room = x["join"].s();

			if(_id != nullptr) {
				std::string roomKey = room + std::string("_") + _id;

				if(x.has("notifyjoin")){
					if(x["notifyjoin"].b()){
						crow::json::wvalue wsend;
						wsend["joined"] = room;
						if(_id != nullptr) {
							wsend["from"] = _id;
						};
						if(x.has("data")){
							wsend["data"] = x["data"];
						}												
						wsend["timestamp"] = getCurrentTimestamp();
							
						std::string send = crow::json::dump(wsend);
						// saving conn later, so no need to pass it and perform extra checking
						
						if(x.has("key")) {
							std::string out;
							
							if(!Quarks().dump(x["key"].s(), send, out)) {
								crow::json::wvalue err;
								err["error"] = out;
								err["object"] = x;
								conn.send_text(crow::json::dump(err));
							}
							
						}
						
						broadcast(room, send); 
					}
				}				

				if(x.has("notifyleave")) {
					_notifyAllOnClose = x["notifyleave"].b();
				}

				_connMap[roomKey] = &conn;

				std::string userKey = std::string(_id) + std::string("_") + room;
				_userRoomsMap[userKey] = room;

			}

		} else if(x.has("userlist")) {
			std::string room = std::string(x["userlist"].s());
			std::vector<crow::json::wvalue> list;

			auto items = lookup(_connMap, room + "_" );
			auto itBegin = items.first;
			auto itEnd = items.second;

			int skip = 0;
			int limit = -1;
			
			if(x.has("skip")){
				skip = x["skip"].i();
			}
			if(x.has("limit")){
				limit = x["limit"].i();
			}
			
			int i = 0;
			skip--; // for easy comparing inside loop
			for (auto it=itBegin; it!=itEnd; ++it) {
				if(i > skip && i != limit){
					std::string s = it->first;
					crow::json::wvalue w;
					w = s;
					list.push_back(std::move(w));
				}
				i++;			
			}
			skip++;
			

			crow::json::wvalue wlist;
			wlist["room"] = room;
			wlist["replyuserlist"] = std::move(list);
			wlist["skip"] = skip;
			wlist["limit"] = limit;
			wlist["timestamp"] = getCurrentTimestamp();
			
			conn.send_text(crow::json::dump(wlist));

		} else if(x.has("send")) {			
			std::string room = "default"; // check onOpen
			if(x.has("room")){
				room = x["room"].s();
			}		
			
			crow::json::wvalue wsend;
			wsend["room"] = room;
			wsend["message"] = x["send"];	
			if(_id != nullptr) {
				wsend["from"] = _id;
			}
			wsend["timestamp"] = getCurrentTimestamp();
			
			std::string send = crow::json::dump(wsend);
			
			if(x.has("key")){
				std::string out;
				CROW_LOG_INFO << "Save Key: " << x["key"].s();
				bool ret = Quarks().dump(x["key"].s(), send, out);
				if(!ret){
					crow::json::wvalue err;
					err["error"] = out;
					err["object"] = x;
					conn.send_text(crow::json::dump(err));

					return true;
				}
			
			}
						
			if(x.has("to")){				
				std::string to = x["to"].s();
				std::string key = room + std::string("_") + to;
				auto u = _connMap[key];
				if(u) {
					if (is_binary){
						u->send_binary(data);
					}						
					else{
						u->send_text(send);
					}				
			 
				}
			}else{
				broadcast(room, is_binary?data : send, conn); // broadcast shouldn't send to broadcaster
			}			

		} else {
			return onQueryMessage(conn, x, is_binary);
		}

	} catch (const std::runtime_error& error) {
		CROW_LOG_INFO << "runtime error : invalid data parameters - " << data;
	}

	return true;
}

