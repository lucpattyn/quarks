class QuarksAPI {
  foreign static dispatch( x )
  foreign static get( x )
  foreign static getkeys( x, skip, limit )
  foreign static getkeysr( x, skip, limit ) 
  foreign static put( x )
  foreign static incrval( x )
}

class QuarksEnv {
  foreign static get( x )
  foreign static loadplugin( x )
  foreign static unloadplugin( x )
  foreign static callplugin(x, params, body)
}

class Request {
  foreign static on( url, handler )	
  foreign static getheader( x )
  foreign static getparam( x )
}
