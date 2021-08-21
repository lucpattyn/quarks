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
const client = net.connect({port: 18071}, () => {//use same port of server  

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
  console.log(data.toString());  
  wqReader.push(data.toString());
  if(data.toString() == "quit"){
  	quit = true;
  	client.end();
  }
});  
client.on('end', () => {  
  console.log('disconnected from server');  
});  

 
setTimeout(runReader = function() {
  write("hello");
  var result1 = read();
  console.log(result1);
  
  write("world");
  var result2 = read();
  console.log(result2);
  
  write("quit");
  var resultQuit = read();
  console.log(resultQuit);
  if(!quit){
  	//runReader();
  }
}, 1000);
