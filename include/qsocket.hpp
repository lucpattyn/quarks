#if !defined(QSOCKET__INCLUDED_)
#define QSOCKET__INCLUDED_

#include <unordered_set>
#include <crow.h>

#include <mutex>

class HackWebSocketRule : public crow::BaseRule
{
    using self_t = HackWebSocketRule;
public:
    
    HackWebSocketRule(std::string rule)
    : BaseRule(std::move(rule))
    {
        
    }
    
    void validate() override
    {
    }
    
    void handle(const crow::request&, crow::response& res, const crow::routing_params&) override
    {
        res = crow::response(404);
        res.end();
    }
    
    void handle_upgrade(const crow::request& req, crow::response&, crow::SocketAdaptor&& adaptor) override
    {
        CROW_LOG_INFO << "custom upgrade << ";
        
        auto x = req.url_params.get("_id");
        auto k = (x == nullptr || !strlen(x) ? "" : x);
        CROW_LOG_INFO << k;
        
        crow::websocket::Connection<crow::SocketAdaptor>* conn =
        new crow::websocket::Connection<crow::SocketAdaptor>(req, std::move(adaptor), open_handler_, message_handler_, close_handler_, error_handler_, accept_handler_);
        
        char* _id = new char [strlen(k) + 1];
        strcpy(_id, k);
        
        conn->userdata(_id);
        
        
    }
#ifdef CROW_ENABLE_SSL
    void handle_upgrade(const crow::request& req, crow::response&, crow::SSLAdaptor&& adaptor) override
    {
        new crow::websocket::Connection<SSLAdaptor>(req, std::move(adaptor), open_handler_, message_handler_, close_handler_, error_handler_, accept_handler_);
    }
#endif
    
    template <typename Func>
    self_t& onopen(Func f)
    {
        open_handler_ = f;
        return *this;
    }
    
    template <typename Func>
    self_t& onmessage(Func f)
    {
        message_handler_ = f;
        return *this;
    }
    
    template <typename Func>
    self_t& onclose(Func f)
    {
        close_handler_ = f;
        return *this;
    }
    
    template <typename Func>
    self_t& onerror(Func f)
    {
        error_handler_ = f;
        return *this;
    }
    
    template <typename Func>
    self_t& onaccept(Func f)
    {
        accept_handler_ = f;
        return *this;
    }
    
    
protected:
    std::function<void(crow::websocket::connection&)> open_handler_;
    std::function<void(crow::websocket::connection&, const std::string&, bool)> message_handler_;
    std::function<void(crow::websocket::connection&, const std::string&)> close_handler_;
    std::function<void(crow::websocket::connection&)> error_handler_;
    std::function<bool(const crow::request&)> accept_handler_;
    
public:
    static HackWebSocketRule& Create(void* obj){
        auto p =new HackWebSocketRule(((self_t*)obj)->rule_);
        ((self_t*)obj)->rule_to_upgrade_.reset(p);
        return *p;
    }
    
};

struct HackTraits : public crow::RuleParameterTraits<crow::TaggedRule<>>{
    
    HackWebSocketRule& hackwebsocket() {
        //auto& ws = websocket();
        auto& ws = HackWebSocketRule::Create(this);
        return ws;
    }
};

class QSocket {
   
public:
    class Interceptor {
        
    public:
        ////////////////////////
        Interceptor(){
            
        }
        
        virtual void onOpen(crow::websocket::connection&){
            
        }
        
        virtual void onClose(crow::websocket::connection&){
            
        }
        
        virtual bool onMessage(crow::websocket::connection&,
                               const std::string&, bool /*is_binary*/){
            return false;
            
        }
        
       
        std::map<std::string, crow::websocket::connection*>& getConnectionMap(){
            return _connMap;
        }

	protected:		
 		std::map<std::string, crow::websocket::connection*> _connMap;
        
    
    };


	/////////File Interceptor///////////////////
	class InterceptorFile : public Interceptor {
        
    public:
        ////////////////////////
        InterceptorFile(std::string mode){
            _mode = mode;
        }
        
        virtual void onOpen(crow::websocket::connection&){
            
        }
        
        virtual void onClose(crow::websocket::connection&){
            
        }
        
        virtual bool onMessage(crow::websocket::connection&,
                               const std::string& msg, bool /*is_binary*/){
			
			auto x = crow::json::load(msg);
    		if (!x){
        		CROW_LOG_INFO << "invalid message body: " << data;
        		return true;
        
    		}
    
			try{
				
				char* _id = (char*)conn.userdata();
				
				if(x.has("cmd")){
				    int cmd = x["cmd"].i();

					switch(cmd){
						case 1: 
							FILE* fp = fopen(x["data"], "wb");
							mFiles[conn] = fp;

						break;

						case 2:
							fclose(mFiles[conn]);
						break;
					}
				    
				}else{
					
				}
 		
			}catch (const std::runtime_error& error){
		    	CROW_LOG_INFO << "runtime error : invalid data parameters - " << data;
			}

			return true;
            
        }

		auto getMode(){
			return _mode;
		}

	private:       
        std::string _mode;

		std::map<crow::websocket::connection*, FILE*> mFiles;
    };


	/////////////////End of Interceptors//////////////////////////


///////////////////////////QSocket //////////////////////////////////
    
public:
    
    QSocket(crow::RuleParameterTraits<crow::TaggedRule<>>& traits,
            QSocket::Interceptor& interceptor = DefaultInterceptor())
    : qsockInterceptor(interceptor){
        connect(traits, interceptor);
    }

 	static QSocket::Interceptor& DefaultInterceptor(){
        static QSocket::Interceptor defaultInterceptor;
        return defaultInterceptor;
        
    }

 	static QSocket::Interceptor& FileInterceptor(std::string mode){
        static QSocket::InterceptorFile fileInterceptor(mode);
        return fileInterceptor;
        
    }
    
    //void broadcast(std::string room, std::string data);
    

private:    
    void connect(crow::RuleParameterTraits<crow::TaggedRule<>>& traits,
                 QSocket::Interceptor& interceptor){
        
        qsockInterceptor = interceptor;
        
        ((HackTraits*)&traits)->hackwebsocket()
        .onopen([&](crow::websocket::connection& conn){
            CROW_LOG_INFO << "new websocket connection";
            
            std::lock_guard<std::mutex> _(mtx);
            users.insert(&conn);
            
            qsockInterceptor.onOpen(conn);
            
        })
        .onclose([&](crow::websocket::connection& conn, const std::string& reason){
            CROW_LOG_INFO << "websocket connection closed: " << reason;
            
            std::lock_guard<std::mutex> _(mtx);
            
            qsockInterceptor.onClose(conn);
            
            delete [] (char*)conn.userdata();
            users.erase(&conn);
            
            
        })
        .onmessage([&](crow::websocket::connection& conn, const std::string& data, bool is_binary){
            //std::string _id = (char*)conn.userdata();
            //CROW_LOG_INFO << "websocket msg for _id : " << (_id != nullptr)? _id : "";
            
            std::lock_guard<std::mutex> _(mtx);
          
            bool preventDefault = qsockInterceptor.onMessage(conn, data, is_binary);
                
            if(!preventDefault){
                for(auto u:users)
                    if (is_binary)
                        u->send_binary(data);
                    else
                        u->send_text(data);
            }
        
        });
        
    }
    
   
    
private:
    
    std::mutex mtx;

    std::unordered_set<crow::websocket::connection*> users;
    
    QSocket::Interceptor& qsockInterceptor;
    
    
};

#endif // !defined(QSOCKET__INCLUDED_)
