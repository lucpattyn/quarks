
# quarks
Modern C++ based server side framework for optimal solutions

The philosophy and technological guidance behind Quarks can be found in this link:
[quarks philosophy](https://dev.to/lucpattyn/quarks-a-new-approach-with-a-new-mindset-to-programming-10lk)

We will have a mechanism in later stages to support various plugins. 
We will provide OpenCV as a readily available plugin.

Thanks Arthur de Araújo Farias for providing a good example of using CROW with OpenCV.
[arthurafarias/microservice-opencv-filter-gausian]

This current example uses a compiled version of RocksDB, OpenCV 4.0.0 and Chrome v8 Engine and requires the following packages:

- Crow Library v0.1
- GCC with support to C++17
- Cmake 1.13
- Boost::System
- RocksDB
- v8 Javascript Engine 
- OpenCV 4.0.0 (Optional)
- Curl (Experimental Optional)

## Docker setup
To build the docker image:
```
docker build -t quarks:disco-gcc8
```

To run the docker image:
```
docker run -it -v $PWD:/quarks -p 18080:18080 --cap-add sys_ptrace quarks:disco-gcc
```

## How to build
```
mkdir build
cd build
cmake .. -G Ninja
ninja
```

### Testing

After initializing by issuing following command

```
./ocv_microservice_crow
```
In most cases the following apis returns results in Json format
{"result":true} or {"error":"error description"}

### GET REQUESTS

Available features:
    a)  Put a key value pair 
    b)  Get value object (json or string) against key 
    c)  List values by wildcard search with keys
    d)  List sorted values by wildcard search with keys
    e)  List keys vs values by wildcard search with keys
    f)   Get count of keys by wildcard search
    g)  Remove a key
    h)  Remove keys by wildcard search
    i)   Check if a key already exists  
    k)  Execute Atoms: atoms are set of Put and Remove operations which can be executed in a single API call

### Description

a)  Put key vs value 

```
 http://0.0.0.0:18080/put?body={"key":"g1_u1","value":{"msg":"m1"}}
```
 It is recommended to "URI encode"  the body parameters . Example JS codes:

```
 jsonobj = {"key":"g1_u1", "value":{"msg":"m1"}}
 var url = "put?body=" + encodeURIComponent(JSON.stringify(jsonobj));

 $.get(url, function( data ) {
     $( ".result" ).html( data );    
 });

```
 *If request is successful then the key would be returned as result
 
 However, GET type of requests have a limitation with parameter lengths and body param cannot be too big.
 In those cases you have to use the methods in POST section (putjson, postjson etc.)


b) Get value against key

```
 http://0.0.0.0:18080/get?key=g1_u1
```

c) List values by wildcard search with keys (you can specifiy skip and limit optionally)
```
 http://0.0.0.0:18080/getall?keys=g1_u*&skip=5&limit=10 
```
d) List sorted values by wildcard search with keys (you can specifiy skip and limit optionally)
```
 http://0.0.0.0:18080/getsorted?keys=g1_u*&sortby=msg&skip=5&limit=10 

```
  You can reverse the order by specifying des=true
```
 http://0.0.0.0:18080/getsorted?keys=g1_u*&sortby=msg&des=true&skip=5&limit=10 

```

e) List keys vs values by wildcard search with keys (you can specifiy skip and limit optionally)

```
 http://0.0.0.0:18080/getkeys?keys=g1_u*&skip=5&limit=10 
```
To get the keys in reverse order run 
```
http://0.0.0.0:18080/getkeys?keys=g1_u*&skip=5&limit=10&reverse=true 
```

f) Get count of keys matched by wildcard search 
```
http://0.0.0.0:18080/getcount?keys=g1*
```

g)  remove a key
```
http://0.0.0.0:18080/remove?key=g1_u1
```
number of keys successfully deleted would be returned

h) remove keys by wildcard search
```
http://0.0.0.0:18080/removeall?keys=g1_u*
``` 

number of keys successfully deleted would be returned

i) check if a key already exists
```
http://0.0.0.0:18080/exists?key=g1_u1
``` 

j) get a list of key value pair given a list of keys
```
http://0.0.0.0:18080/getlist?body=["g1_u1", "g2_u2"]
``` 
(You can specify skip and limit to this as well but should not need it)


k) Execute Atoms: Atoms are set of Put and Remove operations which can be executed in a single API call

To run a set of put operations together, run:

```
POST: http://0.0.0.0:18080/put/atom?body=
[
{"key":"g1_u2", "value":{"msg":"m1"}},
{"key":"g2_u2", "value":{"msg":"m2"}},
{"key":"g3_u3", "value":{"msg":"m3"}}
]

```

To run a set of remove operations together, run:
```
POST: http://0.0.0.0:18080/remove/atom?body=
["g1_u1","g1_u2", "g3_u3"]

```

To run a set of remove operations followed by a set of put operations, run:
```
POST: http://0.0.0.0:18080/atom?body=
{
put:[
{"key":"g1_u2", "value":{"msg":"m1"}},
{"key":"g2_u2", "value":{"msg":"m2"}},
{"key":"g3_u3", "value":{"msg":"m3"}}
}],
remove:["g1_u1","g1_u2", "g3_u3"]   

}

```
* Notes about Atoms,
  1) Remove operations will always be executed before put in ../atom call
  2) Atoms should be used sparingly - if you have only a single put/remove operation then,
      use the put/remove apis, not atomic ones
  3) If you have a number of put operations and no removes then use the ../put/atom (and not  ../atom)
  4) If you have a number of remove operations and no puts then use the ../remove/atom (and not ../atom)
  

### POST REQUESTS

Available features:
    i)   Put a json object against key, 
    ii)  Get that object against key 
    iii) Do a wildcard search of keys 
    iv) Get a list of key value pair given a list of keys
    v)  Execute atoms: atoms are set of Put and Remote operations which can be executed in a single API call
    vi) Apply filters and joins

### Description

 i) Put a json object against a key:
 POST: http://0.0.0.0:18080/putjson
```
BODY:
{"key":"g3_u3", "value":{ "msg":"m3"}}
```
Note: In the json body, it is required to have a "key" attribute and a "value" attribute as a part of the json object.  The json object {"msg":"m3"}  under attribute "value" is saved against the key "g3_u3" in the persistent storage

If the intention is to insert only if the key doesn't exist then use the following api:

POST: http://0.0.0.0:18080/postjson
```
BODY:
{"key":"g3_u3", "value":{ "msg":"m3"}}
```
If the key already exists then the call fails.
This is more useful than calling the "exists" api to check if key exists and then call putjson,
since it's reduces an extra api call


ii) Retrieve the json object by key:
POST: http://0.0.0.0:18080/getjson
```
BODY: {"key":"g3_u3"}
```
iii) Retrieve an array of json objects by wildcard matching of keys..
POST: http://0.0.0.0:18080/iterjson
```
BODY: {"keys":"g3_u*"}
```
To test this API,
You could  post a few values against keys with putjson, for example 

```
BODY:
POST: http://0.0.0.0:18080/putjson
{"key":"g1_u2", "value":{"msg":"m1"}}

POST: http://0.0.0.0:18080/putjson
BODY:
{"key":"g2_u2", "value":{"msg":"m2"}}

POST: http://0.0.0.0:18080/putjson
BODY:
{"key":"g3_u3", "value":{"msg":"m3"}}

```
and then check the results by 
```
POST: http://0.0.0.0:18080/iterjson
BODY: {"keys":"g3_u*"}
```

iv) Get a list of key value pair given a list of keys
```
POST: http://0.0.0.0:18080/getlist
BODY: ["g1_u1", "g2_u2"]

```
(You can specify skip and limit as query parameters but should not need it)


v) Execute Atoms: Atoms are set of Put and Remove operations which can be executed in a single API call

To run a set of put operations together, run:

```
POST: http://0.0.0.0:18080/put/atom

BODY:
[
    {"key":"g1_u2", "value":{"msg":"m1"}},
    {"key":"g2_u2", "value":{"msg":"m2"}},
    {"key":"g3_u3", "value":{"msg":"m3"}}
]

```

To run a set of remove operations together, run:
```
POST: http://0.0.0.0:18080/remove/atom

BODY:
["g1_u1","g1_u2", "g3_u3"]

```

To run a set of remove operations followed by a set of put operations, run:
```
POST: http://0.0.0.0:18080/atom

BODY:
{
put:[
    {"key":"g1_u2", "value":{"msg":"m1"}},
    {"key":"g2_u2", "value":{"msg":"m2"}},
    {"key":"g3_u3", "value":{"msg":"m3"}}
    }],
remove:["g1_u1","g1_u2", "g3_u3"]   

}

```
* Notes about Atoms,
1) Remove operations will always be executed before put in ../atom call
2) Atoms should be used sparingly - if you have only a single put/remove operation then,
    use the put/remove apis, not atomic ones
3) If you have a number of put operations and no removes then use the ../put/atom (and not  ../atom)
4) If you have a number of remove operations and no puts then use the ../remove/atom (and not ../atom)



vi) Filters and joins: There is also provision to run ORM style queries with searchjson and applying filters

POST: http://0.0.0.0:18080/searchjson

Sample Query Format for
"querying items which are up for sale with key like item* (i.e item1, item2 etc.) , then find the sellers of such items (items has a seller_id field that contains the user_id of the seller) "
```
BODY:
{
"keys":"item*",
"filter":{"map": {"as":"seller", "field":"seller_id"}}
}      
```

To test it out,
First insert some users->

POST: http://0.0.0.0:18080/putjson
BODY:
```
{"key":"user1", "value":{"name":"u1", "age":34}}
{"key":"user2", "value":{"name":"u2", "age":43}}
```

then insert some items->
POST: http://0.0.0.0:18080/putjson
BODY:
```
{
"key":"item1",
"value":{
"id": "item1",
"seller_id": "user1",
"rating": 4,
"approved": "1"
}

{
"key":"item2",
"value":{
"id": "item2",
"seller_id": "user2",
"rating": 3,
"approved": "1"
}
```

Finally, check the results by 
POST: http://0.0.0.0:18080/searchjson
BODY:
```
{
"keys":"item*",
"filter":{"map": {"field":"seller_id", "as":"seller"}}
}    
```
So we are able to iterate items (by "keys":"item*") and then run a join operation with the filter attribute ("filter":...) through the keyword map ({"map": {"field":"seller_id", "as":"seller"}})

v8 engine has been integrated to support scripting in server side to further filter/sort queried results. 

Now the post body looks like the following 
with the js based extended filtering:

```
{
"keys":"item*",
    "include":{
        "map": {"field":"seller_id", "as":"seller"},
        "module":"main",
        "filter":"jsFilter",
        "params":"{\"approved\":1}"
    }

}

```

And the JS in server side looks like this:

```
function jsFilter() {
    var elem = JSON.parse(arguments[0]);
    var args = JSON.parse(arguments[1]);
    var match = 0;
    if(elem.approved == args.approved) {
        match = 1;        
    }

    return match;
}
```

Here module main is the main.js file residing in the server in the same path as the executable.
function is the name of the JS Function which we will use to further filter the data.

The idea is the mentioned script main.js will have a filter function with a predefined form filter(elem, params), or a sort function with predefined form sort(elem1, elem2, params) to further fitler/sort the data.

'elem' is an individual item (one of many) found by the Quarks lookup through "keys":"item*" .
We are invoking the JS module and the function while finding and iterating the matching items in C++.

It is up to the user to interpret the params in the server side and write the script codes accordingly.

In our example, we named the function - "jsFilter" in main.js. 

Quarks will allow minimum usage of scripting to ensure the server side codes remain super optimized.

After v8 engine integration and scripting support,
the next target is to allow listener support through zero mq to communicate with other processes and services.

Websocket support has been added too.

For those interested in testing OpenCV as plugin,
you should submit a POST request to http://localhost:18080/filters/gausian. 
The body of this request should be your binary PNG image. 
The response should be a gausian filtered image from the submited image.

OpenCV however is a plugin (an additional feature) and not the main purpose behind Quarks.
Currently it is turned off by using #ifdef _USE_PLUGIN in the codes and if (_USE_PLUGINS) in CMakeLists.txt

