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
  	var key = QuarksAPI.getparam(ctx, "userId")
	var result = QuarksAPI.get(key)
	
	var ret = { "code" : 200, "result" : result }
	
	return JSON.stringify(ret)
  	
  }
  
  static getFeedList( ctx, params, body ) {
  	System.print("Feedinfo Requested .. ")
  	
  	var skip = QuarksAPI.getparam(ctx, "skip")
  	System.print("Skip: " + skip)
	if(!skip){
		skip = 0
	}
	var limit = QuarksAPI.getparam(ctx, "limit")
	System.print("Limit: " + limit)
	if(!limit){
		limit = 100
	}	
	var result = QuarksAPI.getkeysr("feed_*", Num.fromString(skip), Num.fromString(limit))
	
	var ret = { "code" : 200, "result" : result }
	
	return JSON.stringify(ret)
  
  }
  
  static getFeedReplies( ctx, params, body ) {
  	System.print("FeedReplies Requested .. ")
  	
  	var feedId = QuarksAPI.getparam(ctx, "feedid")
  	System.print("FeedId:" + feedId)
  	
  	var skip = QuarksAPI.getparam(ctx, "skip")
  	System.print("Skip: " + skip)
	if(!skip){
		skip = 0
	}
	var limit = QuarksAPI.getparam(ctx, "limit")
	System.print("Limit: " + limit)
	if(!limit){
		limit = 100
	}	
	var result = QuarksAPI.getkeys("feedreply_" + feedId + "_*", Num.fromString(skip), Num.fromString(limit))
	
	var ret = { "code" : 200, "result" : result }
	
	return JSON.stringify(ret)
  
  }
 
  static postFeedItem( ctx, params, body ) {
	var result = QuarksAPI.put(body)	
	var ret = { "code" : 200, "result" : result }
    
    return JSON.stringify(ret)

  }

  static postFeedReply( ctx, params, body ) {
	var result = QuarksAPI.put(body)	
	var ret = { "code" : 200, "result" : result }
    
    return JSON.stringify(ret)

  }
 
  static incrLikeCount( ctx, params, body ) {
	var result = QuarksAPI.incrval(body)	
	var ret = { "code" : 200, "result" : result }
    
    return JSON.stringify(ret)

  }
  
}

System.print( QuarksAPI.dispatch("") )
System.print( QuarksEnv.get("") )

Request.on( "/getuserinfo", "getUserInfo(_,_,_)" )
Request.on( "/getfeedlist", "getFeedList(_,_,_)" )
Request.on( "/getfeedreplies", "getFeedReplies(_,_,_)" )
Request.on( "/postfeeditem", "postFeedItem(_,_,_)" )
Request.on( "/postfeedreply", "postFeedReply(_,_,_)" )
Request.on( "/like", "incrLikeCount(_,_,_)" )

