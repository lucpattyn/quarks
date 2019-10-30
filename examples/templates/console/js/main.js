//main.js

var step = 0;
var offset = 3;

var teamIndex = 0;
var teams = [];
var players = [];

function getJSON(url, header, callback){
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        if (xhr.readyState === 4){
            callback(xhr.responseText);
        }
    };
    xhr.open('GET', url);
    if(header){
        http.setRequestHeader(header.type, header.value);
    }
    
    xhr.send();
    
}

function putJSON(url, header, data, callback){
    const http = new XMLHttpRequest()
    http.open('POST', url)
    http.setRequestHeader('Content-type', 'application/json');
    if(header){
        http.setRequestHeader(header.type, header.value);
    }
    
    http.send(JSON.stringify(data)) // Make sure to stringify
    http.onload = function() {
        // Do whatever with response
        callback(http.responseText);
    }
}


var scan = function (command, onScanned){
    var url = ("ai?msg=" + command).replace(" ", "_");
    getJSON(url, null, function(responseText){
        var text = "" + JSON.parse(responseText).cnt;
        console.log(text);
        //var tokens = text.split(/[,;]/);

        var n = 0;
        for(i = 0; i < text.length; i++){
            try{
                if(",;".search(new RegExp("[" + text[i] + "]")) != -1){
                    //if(!n){
                    //    print(text.substring(n, i + 1));
                    //}else{
                        print(text.substring(n, i + 1), " \\> "); // because i excludes last char
                        printLine("", ">");
                    //}
                    n = i + 1;
                }else if(i == text.length - 1){
            
                    print(text.substring(n, i + 1), " >> ");
                    printLine("", ">>>");
            
                    printLine(" anything else ? ");
            
                }
            }
            catch(e){
                console.log(e);
                console.log(text[i]);
            }
        }
            
    });
    
    /*if(command == "register"){
        step = 1;
        onScanned("enter team name:");
    }
    
    switch(step){
        case 1:
            teams[teamIndex] = command;
            step++;
            onScanned("enter player 1 name:");
        break;
            
        case 2:
            step++;
            players[step - offset] = command;
            onScanned("enter player 2 name:");
        break;
        
        case 3:
            step++;
            players[step - offset] = command;
            onScanned("enter player 3 name:");
        break;
            
        case 4:
            step++;
            players[step - offset] = command;
            onScanned("enter player 4 name:");
        break;
            
        case 5:
            step++;
            players[step - offset] = command;
            onScanned("enter sub 1 name:");
         break;
            
        case 6:
            step++;
            players[step - offset] = command;
            onScanned("enter sub 2 name:");
            break;
        
        case 7:
            step++;
            players[step - offset] = command;
            onScanned("enter sub 2 name:");
            break;
            
        case 8:
            step++;
            players[step - offset] = command;
        
            var str = "Thanks for registering!!\nRegistered Teams:\n" +
            teams[0];
            
            onScanned(str);
            
            break;
    }*/
   
};
