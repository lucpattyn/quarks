#ifndef __PLUGINS_H__
#define __PLUGINS_H__

#include <dlfcn.h>
#include <iostream>
#include <map>
#include <mutex>


namespace plugins {

	std::map<std::string, void*> _PluginHandles;
	std::mutex __pluginMutex;

	bool loadPlugin(std::string pluginName){
	
		if(pluginName.size() > 0){
			pluginName = std::string("./plugins/") + pluginName;
		}
	
		void* handle = dlopen(pluginName.c_str(), RTLD_LAZY);	
		//std::cout << "loaded plugin" <<  pluginName << " ";
		if(handle != nullptr){
			__pluginMutex.lock();
			_PluginHandles[pluginName] = handle;	
			__pluginMutex.unlock();							
		} 
		
		return (handle != nullptr);
	}
	
	bool unloadPlugin(std::string pluginName){
	
		if(pluginName.size() > 0){
			pluginName = std::string("./plugins/") + pluginName;
		}
			
		bool ret = false;
		void* handle = nullptr;
	
		__pluginMutex.lock();
		if(_PluginHandles.find(pluginName) != _PluginHandles.end()){
			handle = _PluginHandles[pluginName];
		}
		if(handle != nullptr){
			ret = (dlclose(handle) == 0);
		}		
		if(!ret){
			_PluginHandles.erase(pluginName);		
		}
		 
		__pluginMutex.unlock();
		
		return ret;
	}
	
	std::string executePluginFn(std::string pluginName, std::string params, std::string body){
		
		if(pluginName.size() > 0){
			pluginName = std::string("./plugins/") + pluginName;
		}
		
		std::string result = "";
		
		void* handle = nullptr;
	
		__pluginMutex.lock();
		if(_PluginHandles.find(pluginName) != _PluginHandles.end()){
			handle = _PluginHandles[pluginName];	
		}
		__pluginMutex.unlock();
		
		if(handle != nullptr){
			std::string(*exec)(std::string params, std::string body);
	  
	  		exec = (std::string (*)(std::string, std::string))dlsym(handle, "execute");
	  		if(exec){
				try{
	  				result = (std::string)exec(params, body);
	    
	  			} catch(std::exception& e){
	  				std::cout << "exception: " << e.what() << std::endl;
	  			}	
	  		}	
		}
		
		return result;
	}

}

#endif 
