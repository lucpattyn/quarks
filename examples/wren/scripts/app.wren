import "wren/scripts/modules/quarks" for QuarksAPI
import "wren/scripts/modules/quarks" for QuarksEnv
import "wren/scripts/modules/quarks" for Request

import "wren/scripts/modules/json" for JSON

class App {
  static handleRequest( url, params, body ) {
    System.print( "url:" + url )
	var header = Request.getheader("Connection")
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
  
  static getUserInfo( params, body ){
  	System.print("UserInfo Requested .. ")
  	var key = Request.getparam("userId")
	var result = QuarksAPI.get(key)
	
	var ret = { "code" : 200, "result" : result }
	
	return JSON.stringify(ret)
  	
  }
  
  static getFeedList( params, body ) {
  	System.print("Feedinfo Requested .. ")
  	
  	var skip = Request.getparam("skip")
  	System.print("Skip: " + skip)
	if(!skip){
		skip = 0
	}
	var limit = Request.getparam("limit")
	System.print("Limit: " + limit)
	if(!limit){
		limit = 100
	}	
	var result = QuarksAPI.getkeysr("feed_*", Num.fromString(skip), Num.fromString(limit))
	
	var ret = { "code" : 200, "result" : result }
	
	return JSON.stringify(ret)
  
  }
  
  static getFeedReplies( params, body ) {
  	System.print("FeedReplies Requested .. ")
  	
  	var feedId = Request.getparam("feedid")
  	System.print("FeedId:" + feedId)
  	
  	var skip = Request.getparam("skip")
  	System.print("Skip: " + skip)
	if(!skip){
		skip = 0
	}
	var limit = Request.getparam("limit")
	System.print("Limit: " + limit)
	if(!limit){
		limit = 100
	}	
	var result = QuarksAPI.getkeys("feedreply_" + feedId + "_*", Num.fromString(skip), Num.fromString(limit))
	
	var ret = { "code" : 200, "result" : result }
	
	return JSON.stringify(ret)
  
  }
 
  static postFeedItem( params, body ) {
	var result = QuarksAPI.put(body)	
	var ret = { "code" : 200, "result" : result }
    
    return JSON.stringify(ret)

  }

  static postFeedReply( params, body ) {
	var result = QuarksAPI.put(body)	
	var ret = { "code" : 200, "result" : result }
    
    return JSON.stringify(ret)

  }
 
  static incrLikeCount( params, body ) {
	var result = QuarksAPI.incrval(body)	
	var ret = { "code" : 200, "result" : result }
    
    return JSON.stringify(ret)

  }
  
}

//System.print( QuarksAPI.dispatch("") )
System.print( QuarksEnv.loadplugin("ffmpegplugin.so") )
System.print( QuarksEnv.callplugin("ffmpegplugin.so", "mp4tohls", "feluda.mp4") )
System.print( QuarksEnv.unloadplugin("ffmpegplugin.so") )

Request.on( "/getuserinfo", "getUserInfo(_,_)" )
Request.on( "/getfeedlist", "getFeedList(_,_)" )
Request.on( "/getfeedreplies", "getFeedReplies(_,_)" )
Request.on( "/postfeeditem", "postFeedItem(_,_)" )
Request.on( "/postfeedreply", "postFeedReply(_,_)" )
Request.on( "/like", "incrLikeCount(_,_)" )

