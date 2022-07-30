import "quarks" for QuarksAPI
import "quarks" for QuarksEnv

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
    System.print("\n")
    
    var result = ""
    if(url == "/setuserinfo"){
    	result = QuarksAPI.put(body)
		System.print("wren result:" + result)
		System.print("setuserinfo ends")
	}
    
    return result
  }
}

System.print( QuarksAPI.request("") )
System.print( QuarksEnv.request("") )
