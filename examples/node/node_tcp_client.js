const WaitQueue = require('wait-queue');
const wqWriter = new WaitQueue();
const wqReader = new WaitQueue();

function write(data){
	wqWriter.push(data);
}

async function read(){
	return new Promise((resolve, reject) =>{
		wqReader.shift().then(function(item) {
	  		// will wait until got value
	  		//console.log(item);
	  		// "foo"
	  		resolve(item);
	  	});
	});
	
}

var quit = false;
  	
const net = require('net');  
const client = net.connect({port: 18070}, () => {//use same port of server  

	console.log('connected to server!');  
  	
	var runWriter = function(){
		wqWriter.shift().then(function(item) {
	  		// will wait until got value
	  		console.log(item);
	  		// "foo"
	  		client.write(item); 
	  		
	  		
	  		if(!quit){
	  			setTimeout(runWriter, 0); // wait for the next write
	  		}
	  	});
	};
	//client.write('world!\r\n');  
	
	runWriter();
}); 
 
client.on('data', (data) => {  
  //console.log(data.toString());  
  wqReader.push(data.toString());
  if(data.toString() == "quit"){
  	quit = true;
  	client.end();
  }
});  
client.on('end', () => {  
  console.log('disconnected from server');  
});  

 
setTimeout(runMain = async function() {
  
  var req = {};
  req.query = "/getkeys";
  req.keys = "COM*";
  req.limit = 10;
   
  write(JSON.stringify(req));
  var response = await read();
  console.log(response);
  
  //write("quit");
  //var resultQuit = await read();
  //console.log(resultQuit);
   
}, 1000);
