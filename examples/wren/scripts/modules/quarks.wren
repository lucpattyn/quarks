class QuarksAPI {
  foreign static getheader( ctx, x )
  foreign static getparam( ctx, x )
  foreign static dispatch( x )
  foreign static get( x )
  foreign static getkeys( x, skip, limit )
  foreign static getkeysr( x, skip, limit ) 
  foreign static put( x )
  foreign static incrval( x )
}

class QuarksEnv {
  foreign static get( x )
}

class Request {
  foreign static on( url, handler )	
}
