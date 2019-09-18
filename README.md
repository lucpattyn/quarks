
# quarks
Modern C++ based server side framework for optimal solutions

The philosophy and technological guidance behind Quarks can be found in this link:
[quarks philosophy](https://dev.to/lucpattyn/quarks-a-new-approach-with-a-new-mindset-to-programming-10lk)

We will have a mechanism in later stages to support various plugins. 
We will provide OpenCV as a readily available plugin.

Thanks Arthur de Araújo Farias for providing a good example of using CROW with OpenCV.
[arthurafarias/microservice-opencv-filter-gausian]

This current example uses a compiled version of RocksDB and  OpenCV 4.0.0 and requires the following packages:

- Crow Library v0.1
- GCC with support to C++17
- Cmake 1.13
- Boost::System
- RocksDB
- OpenCV 4.0.0

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

run the following commands to i) put a json object against key, ii) get that object against key and iii) do a wildcard search of keys iv) apply filters and joins

i) Put a json object against a key:
POST: http://0.0.0.0:18080/quarks/cache/putjson
BODY:
{"key":"g3_u3", "value":{ "msg":"m3"}}

Note: In the json body, it is required to have a "key" attribute and a "value" attribute as a part of the json object.  The json object {"msg":"m3"}  under attribute "value" is saved against the key "g3_u3" in the persistent storage


ii) Retrieve the json object by key:
POST: http://0.0.0.0:18080/quarks/cache/getjson
BODY: {"key":"g3_u3"}

iii) Retrieve an array of json objects by wildcard matching of keys..
POST: http://0.0.0.0:18080/quarks/cache/findjson
BODY: {"wild":"g3_u*"}

To test this API,
You could  post a few values against keys with putjson, for example 
POST: http://0.0.0.0:18080/quarks/cache/putjson
BODY:
{"key":"g1_u2", "value":{"msg":"m1"}}

POST: http://0.0.0.0:18080/quarks/cache/putjson
BODY:
{"key":"g1_u2", "value":{"msg":"m2"}}

POST: http://0.0.0.0:18080/quarks/cache/putjson
BODY:
{"key":"g3_u3", "value":{"msg":"m3"}}

and then check the results by 
POST: http://0.0.0.0:18080/quarks/cache/findjson
BODY: {"wild":"g3_u*"}

iv) There is also provision to run ORM style queries with filterjson
POST: http://0.0.0.0:18080/quarks/cache/filterjson

Sample Query Format , ..
query items which are up for sale with key like item* (i.e item1, item2 etc.) , then find the sellers of such items (items has a seller_id field that contains the user_id of the seller) 

BODY:
{
"keys":"item*",
"filter":{"map": {"as":"seller", "field":"seller_id"}}
}      

To test it out,
First insert some users->
POST: http://0.0.0.0:18080/quarks/cache/putjson
BODY:
{"key":"user1", "value":{"name":"u1", "age":34}}
{"key":"user2", "value":{"name":"u2", "age":43}}

then insert some items->
POST: http://0.0.0.0:18080/quarks/cache/putjson
BODY:
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

and then check the results by 
POST: http://0.0.0.0:18080/quarks/cache/filterjson
BODY:
{
"keys":"item*",
"filter":{"map": {"field":"seller_id", "as":"seller"}}
}    

So we are able to iterate items (by "keys":"item*") and then run a join operation with the filter attribute ("filter":...) through the keyword map ({"map": {"field":"seller_id", "as":"seller"}})

We aim to provide more complicated queries in future such as:
{
    keys: "item*",
    where: [{rating:{gt:3}},{approved:{eq:1}}],
    filter:
   {                
        map: {field:"seller_id", as:"seller"},          
        filter:
       {
       include:
         {
           prefix:"ord*_",field:"user_id",suffix:"", 
          as:"orders"                                        
         },
        where:{deliveryType:{eq:"pickup"}}              
     }
  }

}      

For those interested in testing OpenCV as plugin,
you should submit a POST request to http://localhost:18080/filters/gausian. The body of this request should be your
binary PNG image. 
The response should be a gausian filtered image from the submited image.
OpenCV however is a plugin (an additional feature) and not the main purpose behind Quarks.
Currently it is turned off by using #ifdef _USE_PLUGIN in the codes and if (_USE_PLUGINS) in CMakeLists.txt

