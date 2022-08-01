import "wren/scripts/modules/quarks" for QuarksAPI
import "wren/scripts/modules/quarks" for QuarksEnv
import "wren/scripts/modules/quarks" for Request

import "wren/scripts/modules/json" for JSON

class App {
  static handleRequest( ctx, url, params, body ) {
    System.print( "url:" + url )
	System.print("ctx:" + ctx)
	var header = QuarksAPI.getheader(ctx, "Connection")
	if(header){
		System.print("header: " + header)
	}
  	
    System.print( "params:" + params )
    System.print( "wren body:" + body )
    
    var result = ""
    if(url == "/setuserinfo"){
    	result = QuarksAPI.put(body)
		System.print("wren result:" + result)
		System.print("setuserinfo ends")
		
		var ret = { "code" : 200, "result" : result }
    
    	return JSON.stringify(ret)
	}
    
    return result
   
  }
  
  static getUserInfo( ctx, params, body ){
  	System.print("UserInfo Requested .. ")
  	var key = QuarksAPI.getParam(ctx, "userId")
	var result = QuarksAPI.get(key)
	
	var ret = { "code" : 200, "result" : result }
	
	return JSON.stringify(ret)
  	
  }
  
  static getFeedList( ctx, params, body ) {
  	System.print("Feedinfo Requested .. ")
  	
	var result = QuarksAPI.getkeysr("feed_*", 0, 100 )
	
	var ret = { "code" : 200, "result" : result }
	
	return JSON.stringify(ret)
  
  }
}

System.print( QuarksAPI.dispatch("") )
System.print( QuarksEnv.get("") )

Request.on("/getuserinfo", "getUserInfo(_,_,_)")
Request.on("/getfeedlist", "getFeedList(_,_,_)")

