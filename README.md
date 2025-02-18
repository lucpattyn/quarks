
# Quarks
A modern C++ based off-the-shelf server framework for storing, retrieving, processing data with high scalability and plugging in business logics.

Quarks provides a highly scalable and distributable open source system based on actor model which can be easily deployed in closed networks.
The ultimate aim is to come up with open source solutions to well known problems like chatting, image/video processing, transcoding, voice recognition etc. thus reducing dependencies
on cloud platforms like AWS and GCP.
Standardized chat and feed systems would eliminate the need to make private data available to public social networks, thus provisioning to safeguard user's own valuable data.
Adding a new functionality or solution should be as easy as spinning up a new Quarks node and integrate it to the system following a few guidelines.

The core implementation concept, guidance and inspiration behind quarks can be found in this link:
[quarks philosophy](https://dev.to/lucpattyn/quarks-a-new-approach-with-a-new-mindset-to-programming-10lk)

Thanks Arthur de Araújo Farias for providing a good example of using CROW with OpenCV to use as a template.
[arthurafarias/microservice-opencv-filter-gausian]

The current codebase uses a compiled version of RocksDB, Chrome v8 Engine and ZeroMQ.
It requires the following packages:

- Crow Library v0.1
- GCC with support for C++17
- Cmake 1.13
- Boost::System (1.69)
- RocksDB
- v8 Javascript Engine
- ZeroMQ
- OpenCV 4.0.0 (Optional)
- Curl (Experimental Optional)

## How to build
```
mkdir build
cd build
cmake .. -G Ninja
ninja
```

Thanks Tareq Ahmed Siraj  (https://github.com/tareqsiraj) for introduing Ninja,
made life way easier

See end of readme for quick start with ubuntu environment.

### Run

```
./ocv_microservice_crow
```

It is possible to specify a server time out value with the 'timeout' parameter:
```
./ocv_microservice_crow -timeout 125

```

It is possible to change default db name where data will be written through 'store' parameter:
```
./ocv_microservice_crow -store quarks_1

```


### Testing

After running the executable, perform get and post requests as follows:


### GET REQUESTS

### Description

1)  Put key vs value

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


2) Get value against key

```
 http://0.0.0.0:18080/get?key=g1_u1
```

3) List values by wildcard search with keys You can specifiy skip and limit optionally
```
 http://0.0.0.0:18080/getall?keys=g1_u*&skip=5&limit=10
```

4) List sorted values by wildcard search with keys and specifying the sortby attribute of value. 
   It is preferable to already keep the stored key value pairs sorted by using keys instead of invoking sort api,
   since stored elements are pre-sorted by keys. You can specifiy skip and limit optionally
```
 http://0.0.0.0:18080/getsorted?keys=g1_u*&sortby=msg&skip=5&limit=10

```
  You can reverse the order by specifying des=true
```
 http://0.0.0.0:18080/getsorted?keys=g1_u*&sortby=msg&des=true&skip=5&limit=10

```
Apply equal-to filter on a value (using eq) :
```
http://0.0.0.0:18080/getsorted?keys=g1_u*&skip=0&limit=10&filter={"where":{"messageTo":{"eq":"u2"}}}

```
* if you want to do a not equal to filter, use neq
_http://0.0.0.0:18080/getsorted?keys=g1_u*&skip=0&limit=10&filter={"where":{"messageTo":{"neq":"u2"}}}_

Apply equal-to filter on a value performing multiple comparisons (using eq_any):
```
http://0.0.0.0:18080/getsorted?keys=g1_u*&filter={"where":{"messageTo":{"eq_any":["u2","u4"]}}}

```
* you can do a not in check also as follows
_http://0.0.0.0:18080/getsorted?keys=g1_u*&filter={"where":{"messageTo":{"not_in":["u2","u4"]}}}_

**ORM style operators 'or', 'and' can be used also.**

Apply "or" :
```
http://0.0.0.0:18080/getsorted?keys=g1_u*&filter={
  "where": {
    "or": [
      { "messageTo": { "eq": "u2" } },
      { "messageTo": { "eq": "u3" } }
    ]
  }
}


```
Apply "and" :
```
http://0.0.0.0:18080/getsorted?keys=g1_u*&filter={
  "where": {
    "and": [
      { "messageTo": { "eq": "u2" } },
      { "msg": { "eq": "m2" } }
    ]
  }
}

```


5) List keys vs values by wildcard search with key prefixes (you can specifiy skip and limit optionally)

```
 http://0.0.0.0:18080/getkeys?keys=g1_u*&skip=5&limit=10
```
To get the keys in reverse order run
```
http://0.0.0.0:18080/getkeys?keys=g1_u*&skip=5&limit=10&reverse=true
```

6) Get count of keys matched by wildcard search
```
http://0.0.0.0:18080/getcount?keys=g1*
```

7)  remove a key
```
http://0.0.0.0:18080/remove?key=g1_u1
```
number of keys successfully deleted would be returned

8) remove keys by wildcard search
```
http://0.0.0.0:18080/removeall?keys=g1_u*
```
number of keys successfully deleted would be returned


9) check if a key already exists
```
http://0.0.0.0:18080/exists?key=g1_u1
```

10) get a list of key value pair given a list of keys
```
http://0.0.0.0:18080/getlist?body=["g1_u1", "g2_u2"]
```
(You can specify skip and limit to this as well but should not need it)

11) get a json object containing values against a given a list of keys
```
http://0.0.0.0:18080/getitems?body=["g1_u1", "g2_u2"]
```
(You can specify skip and limit to this as well but should not need it)

12) increment a value saved as integer by a specified amount
```
http://0.0.0.0:18080/incr?body={"key":"somecounter","step":5}

Note: Value to increment must be saved as integer with a previous call to put -
http://0.0.0.0:18081/put?body={"key":"somecounter", "value":1}
```
The more advanced version is incrval where you can specify the specific attribute (must be integer) to increment
```
http://0.0.0.0:18080/incrval?body={"key":"feed_user_johnwick", "value":{"points":3}}
```
In the above example if points were previously set as 7, after the API call it becomes 10.
Both incr and incrval works with POST methods as well

l3) Execute Atoms: Atoms are set of Put and Remove operations which can be executed in a single API call

To run a set of put operations together, run:

```
GET: http://0.0.0.0:18080/put/atom?body=
[
{"key":"g1_u2", "value":{"msg":"m1"}},
{"key":"g2_u2", "value":{"msg":"m2"}},
{"key":"g3_u3", "value":{"msg":"m3"}}
]

```

To run a set of remove operations together, run:
```
GET: http://0.0.0.0:18080/remove/atom?body=
["g1_u1","g1_u2", "g3_u3"]

```

To run a set of remove operations followed by a set of put operations, run:
```
GET: http://0.0.0.0:18080/atom?body=
{
put:[
{"key":"g1_u2", "value":{"msg":"m1"}},
{"key":"g2_u2", "value":{"msg":"m2"}},
{"key":"g3_u3", "value":{"msg":"m3"}}
}],
remove:["g1_u1","g1_u2", "g3_u3"]

}

```

Notes about Atoms:
```
   i)   "Remove" operations will always be executed before "Put" in ../atom call
   
   ii)  Atoms should be used sparingly - if you have only a single put/remove operation then,
        use the put/remove apis provided for the specific purpose, not atomic ones
      
   iii) If you have a number of put operations and no removes then use  ../put/atom (and not  ../atom)
   
   iv)  If you have a number of remove operations and no puts then use  ../remove/atom (and not ../atom)
```

14) autogenerate key with prefix and value provided

```
 http://0.0.0.0:18080/make?body={"prefix":"dev_","value":"101"}

```
* returns the key value pair as json object; if "key" is specified along with prefix
 then a key is formed with prefix+key and no key generation occurs


15) provide a prefix, key pair for which all keys (along with values) greater than the passed compare key,
   starting with the prefix are returned. You can specify skip and limit optionally.
   Notice there is no '*' while specifying the prefix

```
http://0.0.0.0:18080/getkeysafter?body=["key_prefix", "comparekey"]

```
Multiple prefix, key pair can be provided like the following:
```
http://0.0.0.0:18080/getkeysafter?body=["key_pre1", "key1", "key_pre2", "key2", ... "key_preN", "keyN"]

```

16) provide a prefix, key pair for which the highest key (along with values and index) greater than the passed compare key,
   starting with the prefix is returned (Useful for fetching things like last sent message).
   Notice there is no '*' while specifying the prefix

```
http://0.0.0.0:18080/getkeyslast?body=["key_prefix", "comparekey"]

```
Multiple prefix, key pair can be provided like the following:
```
http://0.0.0.0:18080/getkeyslast?body=["key_pre1", "key1", "key_pre2", "key2", ... "key_preN", "keyN"]

```

17) List keys vs values by wildcard search with array of key prefixes. You can specifiy skip and limit optionally.
   The key prefix format is the same as /getkeys API 
```
http://0.0.0.0:18080/getkeysmulti?body=["key1_pre*", "key*_pre2", ... key*_preN"]&skip=0&limit=10

```

18) List keys vs values by iterating entries in the range specified by skip and limit
```
http://0.0.0.0:18080/iter?skip=3&limit=5

```
(You can use reverse=true to iterate in reverse order)

19) Testing API that halts the request processing thread for specified number of seconds as mentioned by the timeout parameter
(Useful to check if the https requests are actualy being processed by threads spawned by multiple cpu cores)

```
 http://0.0.0.0:18080/test?timeout=10

```


### POST REQUESTS


### Description

1) Put a json object against a key:
 POST: http://0.0.0.0:18080/putjson
```
BODY:
{"key":"g3_u3", "value":{ "msg":"m3"}}
```
Note: In the json body, it is required to have a "key" attribute and a "value" attribute as a part of the json object.  The json object {"msg":"m3"}  under attribute "value" is saved against the key "g3_u3" in the persistent storage

If the intention is to insert if and only if the key doesn't exist then use the following api:

```
POST: http://0.0.0.0:18080/postjson
BODY:
{"key":"g3_u3", "value":{ "msg":"m3"}}
```
If the key already exists then the call fails.
This is more useful than calling the "exists" api to check if key exists and then call putjson,
since it's reduces an extra api call

2) Retrieve the json object by key:
```
POST: http://0.0.0.0:18080/getjson
BODY: {"key":"g3_u3"}
```

3) List keys vs values by wildcard search with array of key prefixes:
```
POST: http://0.0.0.0:18080/getkeysmulti&skip=0&limit=10
BODY: ["g3_u*", "*_u3" ]
```

To test these set of APIs,
You could  post a few values against keys with putjson, for example

```
POST: http://0.0.0.0:18080/putjson
BODY:
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
POST: http://0.0.0.0:18080/getkeysmulti?skip=0&limit=20
BODY: ["g*_u1", "g2_u*", "g*" ]

```

4) Get a list of key value pair given a list of keys
```
POST: http://0.0.0.0:18080/getlist
BODY: ["g1_u1", "g2_u2"]

```
(You can specify skip and limit as query parameters but should not need it)

5) Get a json object containing values against a list of keys
```
POST: http://0.0.0.0:18080/getitems
BODY: ["g1_u1", "g2_u2"]

```
(You can specify skip and limit as query parameters but should not need it)


6) Execute Atoms: Atoms are set of Put and Remove operations which can be executed in a single API call

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

Notes about Atoms:
```
   i)   "Remove" operations will always be executed before "Put" in ../atom call
   
   ii)  Atoms should be used sparingly - if you have only a single put/remove operation then,
        use the put/remove apis provided for the specific purpose, not atomic ones
      
   iii) If you have a number of put operations and no removes then use  ../put/atom (and not  ../atom)
   
   iv)  If you have a number of remove operations and no puts then use  ../remove/atom (and not ../atom)
```


7) Autogenerate key and make a key value pair given a key-prefix and value
```
POST: http://0.0.0.0:18080/make
BODY:
{"prefix":"MSGID_","value":"101"}

```
* returns the key value pair as json object; if "key" is specified along with prefix
then a key is formed with prefix+key and no key generation occurs

8) provide a prefix, key pair for which all keys (along with values) greater than the passed key,
   starting with the prefix are returned

```
POST: http://0.0.0.0:18080/getkeysafter
BODY:
["key_prefix", "key"]

```
Multiple prefix, key pair can be provided like the following:
```
POST: http://0.0.0.0:18080/getkeysafter
BODY:
["key_pre1", "key1", "key_pre2", "key2", ... "key_preN", "keyN"]

```

9) provide a prefix, key pair for which the highest key (along with value and index) greater than the passed key,
   starting with the prefix is returned

```
POST: http://0.0.0.0:18080/getkeyslast
BODY:
["key_prefix", "key"]

```

Multiple prefix, key pair can be provided like the following:
```
POST: http://0.0.0.0:18080/getkeyslast
BODY:
["key_pre1", "key1", "key_pre2", "key2", ... "key_preN", "keyN"]

```

### Geodata Storing, Nearby Lookup with Latitude/Longitude 

1. Store data same way as putjson with additional parameters lat and lng. Geohashing is used to store data for quick retrieval.

Post Version:
```
POST: http://0.0.0.0:18080/geo/put
BODY:
{"key":"key_area1", "value": {"area":"xyz", "lat":23.79441056011852, "lng":90.41478673773013}
```

Get Version:
```
GET: http://0.0.0.0:18080/geo/put?body=
	{"key":"key_area2", "value": {"area":"xyz", "lat":23.79441056011852, "lng":90.41478673773013}
```

2. Find nearby co-ordinates and associated key/value by providing lat, lng, radius (km).
   You can also optionally provide precision and keys

Get Version:
```
GET:
http://0.0.0.0:18080/geo/near?body=
	{"lat":23.794803234501487, "lng":90.41410009224322, "radius":5.0, 
	"keys":"key_area*", "precision":5}
```

Post Version:
```
POST: 
http://0.0.0.0:18080/geo/near
BODY:
{"lat":23.794803234501487, "lng":90.41410009224322, "radius":5.0, 
	"keys":"key_area*", "precision":5}
```

"precision" is optional and mainly used to specify the grid area for lat/lng geo hashing for quick lookup
If not specified, precision is estimated from the radius provided 

"keys" (optional) is provided for filtering capabilities through wildcard search and works same as keys in getall api


### Joins and Filters

In practical situaions, the need arose to incorporate the getjoinedmap api which joins multiple resultsets in a single query.
What this api does is take a wildcard argument to iterate a range of keys (main keys).
Then find a "subkey" inside each main key by splitting the key with a delimeter (splitby) and selecting one of the split tokens (selindex).
Next add a prefix and suffix to the "subkey" and find values mapped against the newly "formed key".
To add prefix, an array of prefix, suffix pairs is supplied to come up with relevant values.
This provides a way to having multiple results from a well-formed main key item.

The first item in the result set is the array of "subkeys".
The second item in the result set is a json object whose attributes are the "formed keys" (from prefix, suffix)
with relevant values placed against each attribute.

You can specify skip and limit in this query as well.

It's preferable to use the POST method in this case.

```
GET:
http://0.0.0.0:18080/getjoinedmap?body=
{ 	"keys":"roomkeys_*","splitby":"_","selindex":5,
	"join":[{"prefix":"usercount_","suffix":""},
		    {"prefix":"messagecount_","suffix":""},
		    {"prefix":"notificationcount_","suffix":"user"}
		   ]
}&skip=2&limit=3

```

You can also provide your own keylist, if you don't want a wildcard search or want both.
If you provide both, keylist keys are processed after the wildcard search keys.
Skip and Limit are only applicable for wildcard search of keys

```
GET:
http://0.0.0.0:18080/getjoinedmap?body=
{ 	"keys":"roomkeys_*",
	keylist":["roomkeys_roomid1", "roomkeys_roomid2"],
	"splitby":"_","selindex":1,
	"join":[{"prefix":"usercount_","suffix":""},
		    {"prefix":"messagecount_","suffix":""},
		    {"prefix":"notificationcount_","suffix":"user"}
		   ]
}&skip=2&limit=3

```

POST version:

```
POST: http://0.0.0.0:18080/getjoinedmap&skip=2&limit=3
BODY:
{ 	"keys":"roomkeys_*",
	"keylist":["roomkeys_roomid1", "roomkeys_roomid2"],
	"splitby":"_","selindex":1,
	"join":[{"prefix":"usercount_","suffix":""},
		    {"prefix":"messagecount_","suffix":""},
		    {"prefix":"notificationcount_","suffix":"user"}
		   ]
}

```

* All attributes (keys, splitby, selindex, join) mentioned above are mandatory but values can be left as empty strings.
For example, if no prefix or suffix joining is needed, then prefix and suffix can be kept as empty strings.


There is also provision to run ORM style queries with searchjson and applying filters

POST: http://0.0.0.0:18080/searchjson

searchjson can be used to retrieve data in similar manner to left join.

For example if you have user1 with key user1, and another user with key user2,
saved by /put, for example: http://0.0.0.0:18080/put?body={"key":"user1", "value":{"name":"u1", "age":34}}

And then you have an item which is sold by user1 and saved with 
http://0.0.0.0:18080/put?body={
"key":"item1",
"value":{
"id": "item1",
"seller_id": "user1"
}

Then you can query those items including the user info in the following forma  
http://0.0.0.0:18080/searchjson?body=
{
"keys":"item*",
    "include":{
        "map": {"field":"seller_id", "as":"seller"}
    }
}

It is recommended to use the POST format which would be:

POST: http://0.0.0.0:18080/searchjson
```
{
    "keys":"item*",
    "include":{
        "map": {"field":"seller_id", "as":"seller"}
    }

}

```

Then the result will fetch all the items and exact value of seller_id will be searched in the whole database 
and if found the result will come in this format:

```
{"result":[{"key":"item1","value":{"seller":{"name":"u2","age":39},
			"id":"item1","seller_id":"user1","rating":4,"approved":"1"}}]}

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
"seller_id": "user1"
}

{
"key":"item2",
"value":{
"id": "item2",
"seller_id": "user2"
}

```

Finally, check the results by
POST: http://0.0.0.0:18080/searchjson
BODY:
```
{
    "keys":"item*",
    "include":{
        "map": {"field":"seller_id", "as":"seller"}
    }

}

```

Expected output:

```
{"result":
	[
		{"key":"item1","value":{"seller":{"name":"u2","age":39},"id":"item1","seller_id":"user1"}},
		{"key":"item2","value":{"seller":{"name":"u2","age":43},"id":"item1","seller_id":"user2"}}
	]
}

```

* skip and limit is allowed like other api calls


To apply server side filter v8 Engine has to be enabled (currently disabled).
Sample Query Format when V8 Engine is Enabled
"querying items which are up for sale with key like item* (i.e item1, item2 etc.) , 
then find the sellers of such items (items has a seller_id field that contains the user_id of the seller) "

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

So we are able to iterate items (by "keys":"item*") and then run a join operation with the filter attribute ("filter":...) 
through the keyword map ({"map": {"field":"seller_id", "as":"seller"}})

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

The idea is the mentioned script main.js will have a filter function with a predefined form filter(elem, params), 
or a sort function with predefined form sort(elem1, elem2, params) to further fitler/sort the data.

'elem' is an individual item (one of many) found by the Quarks lookup through "keys":"item*" .
We are invoking the JS module and the function while finding and iterating the matching items in C++.

It is up to the user to interpret the params in the server side and write the script codes accordingly.

In our example, we named the function - "jsFilter" in main.js.

Quarks will allow minimum usage of scripting to ensure the server side codes remain super optimized.

### Fuzzy Search

Fuzzy search now works for queries of varying lengths.
You can retrieve words like "world" even when searching for "worl".
Levenshtein distance is being used to count the number of edits, and you can set a threshold (maxEdits) 
for how "fuzzy" the match should be.
You can adjust the maxEdits parameter to control the fuzziness of the search.


Sample Usage:

Store some items for quick search by /fuzzy/insert
You can supply a tag to group items by category and supply userdata with meta attribute
When retriving the exact components will be returned if a match found so you have context data

```
http://0:0:0:0:18080/fuzzy/insert?body={"word":"world","tag":"noun","meta":"planet"}
http://0:0:0:0:18080/fuzzy/insert?body={"word":"wordl","tag":"noun","meta":"globe"}

```
Now look up world or wordl by passing worl and maxedits (role of maxedits has been discussed already)

```
http://0:0:0:0:18080/fuzzy/query?body={"word":"worl","maxedits":1}

```

Expected output:

```
{"result":[{"meta":"globe","tag":"noun","word":"wordl"},{"meta":"planet","tag":"noun","word":"world"}]}

```

You can also find an exact match instead of fuzzy search by the following api:

```
http://0:0:0:0:18080/fuzzy/match?body={"word":"wordl"}

```

Expected output:

```
{"result":[{"meta":"globe","tag":"noun","word":"wordl"}]}

```

There is provision for efficient prefix and substring search as well

// prefix search

First make a few entries
```
http://0:0:0:0:18080/fuzzy/insert?body={"word":"apple","tag":"fruit","meta":"red"}
http://0:0:0:0:18080/fuzzy/insert?body={"word":"apex","tag":"noun","meta":"globe"}

```

Lookup using prefix "ap"
```
http://0:0:0:0:18080/fuzzy/prefix?body={"word":"ap","maxedits":1}

```

Expected output:

```
{"result":[{"meta":"peak","tag":"word","word":"apex"},{"meta":"red","tag":"fruit","word":"apple"}]}

```
*For conventional prefix search without fuzzy logic use maxedits:-1

Lookup using prefix "ape"
```
http://0:0:0:0:18080/fuzzy/prefix?body={"word":"ape","maxedits":-1}

```

Expected output:

```
{"result":[{"meta":"peak","tag":"word","word":"apex"}]}

```

For substring search use the following


```
http://0:0:0:0:18080/fuzzy/substring?body={"word":"pex"}

```
Expected output:

```
{"result":[{"meta":"peak","tag":"word","word":"apex"}]}

```

*** All of the fuzzy urls support POST format and is recommended to use

Example:
http://0:0:0:0:18080/fuzzy/prefix

POST BODY : 

```
{"word":"ap","maxedits":1}

```
Expected output:

```
{"result":[{"meta":"peak","tag":"word","word":"apex"},{"meta":"red","tag":"fruit","word":"apple"}]}

```

*After search implementation, v8 engine integration and scripting support,
the next target was to allow listener support through zero mq to communicate with other processes and services
and creating the Quarks Cloud which is partially done.


### Quarks Cloud

Quarks Cloud provides the functionalities for scaling and replicating nodes
(through extensive use of ZeroMQ).

Genearlly each Quarks server is called a core.
When we are using the cloud features the cores are called nodes.

There are three types of nodes:
1. Broker Node
2. Writer Node
3. Reader Node

Broker nodes are used to publish data across multiple nodes.
All writes through api calls are written to a writer node.
The writer node sends the message to broker node which publishes to multiple reader nodes.
Reader nodes are dedicated for only data reading related api calls.
This helps serving huge amount of requests because the readers are plain replica of writer node.

Conceptual flow:

user->write apis-> [writer] -> [broker] -> [reader] <-read apis<-user

("/put" is an example of write api and "/get" is read api example)

Following are the commands to start up broker, writer and readers:

Start broker node:
```
 ./ocv_microservice_crow -port 18081 -broker tcp://*:5555

```
* Opens a socket for communication in port 5555 to accept writer requests
  Opens a publisher at port 5556 port for subscribers(i.e readers) to listen to

Start writer node:
```
./ocv_microservice_crow -port 18082 -writer tcp://localhost:5555

```
* Connects to broker at port 5555

Start reader node:
```
./ocv_microservice_crow -port 18083 -reader tcp://localhost:5556

```
* Listens to broker at port 5556
* There can be multiple readers started in different ports.

### LOGGER / REPLICATION
Quarks can send all put and remove requests made in it's core db to a logger

To specify the address of the logger start by specifying the log parameter:
```
./ocv_microservice_crow -port 18080 -log http://0.0.0.0:18081

```

This means a logger has been started at port 18081 and listening to
http://0.0.0.0:18081/putjson and http://0.0.0.0:18081/remove api calls.
These apis respectively get invoked whenever a put or remove operation has been made in the core db


If you start another quarks server in the 18081 port specifying a new database, it simply becomes a replica node
```
./ocv_microservice_crow -store replica -port 18081

```

Instead of a Quarks server, you can start any server which implements and handles
http://0.0.0.0:18081/putjson and http://0.0.0.0:18081/remove api calls


### CACHING
To enable caching for fast lookup and iteration (specially for getkeys and getcount),
specify the cached parameter at start up :
```
./ocv_microservice_crow -port 18080 -cached 

```

## Backup and Restore
```
For backing up the database try:
http://0.0.0.0:18080/backup?body={"path":"quarks_backup_1"}

To restore simply run quarks next time using the "store" commandline parameter
 ./ocv_microservice_crow -store quarks_backup_1
 
 -store followed by the path denotes the rocksdb directory path to use when starting quarks

```


### WEBSOCKETS
Websocket support has been added (Still not optimized).

#Initiate a socket:
```
var sock = new WebSocket("ws://0.0.0.0:18080/ws?_id=" + userId );
```
*Here userId is the id which would be used to uniquely identify a user, otherwise socket chat fails.
Usually this id would be used by the other party (i.e a messege sender) to send messages to this user.
By default, all users are auto joined to a room named "default".


#Room join:
```
sock.onopen = ()=>{
	console.log('open');
	// join room
	sock.send('{"join":"testroom", "notifyjoin":true, "notifyleave":true}');

}

```
#Error Handling and closing

```
sock.onerror = (e)=>{
	console.log('error',e);
}
			
sock.onclose = ()=>{
    console.log('close');
}

```

#Message Sending:

```
	var msg = {};
	msg.room = "testroom";
	msg.send = usrmsg;
	
	// to send to a specific user use the following:
	//msg.to = "useridxxx"; // specifying room is optional in this case
	//msg.key = msg.room + "_" + useridxxx; 
	//specifying a key saves {msg.send, msg.room, timestamp} as Value in Quarks against that key
	
	var m = JSON.stringify(msg);
	sock.send(m);

```

#Message Handling:

```
sock.onmessage = (e)=>{
			
		let msg = JSON.parse(e.data);
		console.log(msg);
			
		var room = "";
		var from = "";
		var data = "";
		
		if(msg["joined"]){
			room = msg.joined;
			from = msg.from
			data = " I am online!";
			
		}else if(msg["left"]){
			room = msg.left;
			from = msg.from;
			data = " I went offline!";
			
		}else if(msg["message"]){
			room = msg.room;
			from = msg.from;
			data = msg.message;
			
		}else if(msg["replyuserlist"]){
			room = msg.room;
			data = msg.replyuserlist;
			from = "system";
		}
		
```

#List Users in a Room :

```
	var msg = {};
	msg.userlist = "testroom";
	msg.skip = 0;
	msg.limit = -1;
	
	var m = JSON.stringify(msg);
	// check the message handling (sock.onmessage) section 
	// to see how to receive the list
	sock.send(m); 

```

### TCP SOCKETS

Inorder for faster communicaton, Quarks provides pure tcp socket communication.
You need to run Quarks TCP Service with the following:
```
    ./ocv_microservice_crow -tcpserver 127.0.0.1:18070

```
Then use any TCP Client to query Quarks to avail the same features as GET request section.
NodeJS TCP Client example can be found in "examples/node" folder

The body of the request should be of this form:
```
    var req = {};
    req.query = "/getkeys";
    req.keys = "g1_u*";
    req.skip = 0;
    req.limit = 10;

```
Another example: 
```
    var req = {};
    req.query = "/getlist";
    req.body=["g1_u1", "g2_u2"]
    req.skip = 0;
    req.limit = 10;

```
(Skip, limits are as usual optional.)
Basically, all the GET Requests can be formed as a query for TCP Communication. 
You only need to specify the url in query parameter as shown above, 
and the other GET arguments as properties of JSON Body,


### PLUGINS and SCRIPTING

Quarks has introduced dynamic plugins integration with the help of wren scripts to extend its capabilities.
Copy the "plugins" and "wren" directory residing in "examples" to build directory for plugin examples.
"app.wren" is executed immediately after Quarks starts running.
"quarks.wren" provides the bridge between wren and Quarks interaction.
Currently get, getkeys, getkeysreversed, put and incrval Quarks core APIs (C++) can be invoked with Wren. 

The benefit of wren scripts is, now it can be used as BLL (business logic layer) 
for further customization in Quarks API Handling.
A good example of that is feed.html in wren directory. 
The frontend simply calls apis as specified by the wren script in app.wren (lines 120 - 125)
The example can be seen in action by calling http://0.0.0.0:18080/wrenfeed

Dynamic plugin loading and execution is done using similar codes as lines 116 -118 :
```
System.print( QuarksEnv.loadplugin("ffmpegplugin.so") )
System.print( QuarksEnv.callplugin("ffmpegplugin.so", "mp4tohls", "some.mp4") )
System.print( QuarksEnv.unloadplugin("ffmpegplugin.so") )
```
This dynamically loaded plugin can convert an mp4 to hls when invoked (Can be in response to an API call).

Seeing wren operating so efficiently, currently v8 engine has been commented out 
which was initially introduced to do the same job

Currently "Wren" feature for plugins and scripting is turned off in the source codes,
using #ifdef _USE_WREN in main.cpp and option(_USE_WREN "Use wren" OFF) in CMakeLists.txt

As legacy codes, OpenCV has been kept as an integrated plugin (codes commented).

For those interested in testing OpenCV as plugin (uncommenting the relevant codes),
you should submit a POST request to http://0.0.0.0:18080/filters/gausian.
The body of this request should be your binary PNG image.
The response should be a gausian filtered image from the submited image.

Currently it is turned off by using #ifdef _USE_PLUGIN in the codes and if (_USE_PLUGINS) in CMakeLists.txt

If turned on, you would need the correct version of libwren.so for the OS placed in lib directory
The current version in lib folder is compiled for Ubuntu 20.04


### EDITOR

A browser based editor has been provided to run Quarks queries and visualize and update data in a JSON Editor
(Thanks to https://github.com/json-editor/json-editor).
To view the editor at work,
Copy the "templates" folder inside "/examples" in the "build" folder and then hit the following in browser:
http://0.0.0.0:18080/console

Definitely Quarks has to be running to view the editor


### EXAMPLES

A guideline is provided for basic twitter like feed and chatrooms.

Copy the "templates" folder inside "/examples" in the "build" folder and then hit the following in browser:
http://0.0.0.0:18080/feed for feed example
http://0.0.0.0:18080/chat for chat example

Definitely Quarks has to be running to view the examples


### Quick Start: Dependencies installation for Ubuntu 18.04

 environment setup (assuming cmake already installed):

 -$ sudo apt-get update -y

 -$ sudo apt-get install build-essential

 -$ sudo apt-get install ninja-build


 main dependency libraries installation:

 -$ sudo apt-get install libboost-system-dev

 -$ sudo apt-get install libv8-dev
    (Not needed if you don't want to use the v8 engine)

 -$ sudo apt-get install librocksdb-dev

 -$ sudo apt-get install libzmq3-dev

 Build and Run:
 Check #How to Build section for compilation and binary creation and #Run section for how to run
 
 ## Changes for Ubuntu 20.04

  1. Downgrade Boost version to 1.69
  
 ## Get started with Docker image:
 To quickly get started, follow these steps:

1. **Install Docker**: Ensure that Docker is installed on your system. If not, follow the official Docker installation instructions for your platform: [Get Docker](https://docs.docker.com/get-docker/).

2. **Pull the Docker Image**: You can pull the Docker image from Docker Hub using the following command:

    ```bash
    docker pull kingrayhan/quarks:latest
    ```

3. **Run the Docker Container**: Once the image is pulled, you can run a container from it using the following command:

    ```bash
    docker run -v <local-path>:/quarks/build/quarks_db -p 18080:18080 kingrayhan/quarks:latest
    ```

    Replace `<local-path>` with the absolute path on your local machine where you want to map the volume for the Quarks database.

    The port 18080 in the container is mapped to port 18080 on the host machine.
=======
 ## Docker:
 
   Please refer to: https://hub.docker.com/r/kingrayhan/quarks

   To build the docker image locally 
   
 ```
 	git clone https://github.com/lucpattyn/quarks.git
   	cd quarks

	docker build -t kingrayhan/quarks .
 ```


### BENCHMARKING

The repo for benchmarking quarks is available on : 
https://github.com/kaisarh/quarks/tree/dev/benchmark

The results are stored here:

```
https://github.com/kaisarh/quarks/tree/dev/benchmark/results?
fbclid=IwAR2ea_PuZ6drbdg4PUuFfhirXdHC4rtlQ3I1KDR9G-PSaIJlFfA0FXNjUw8

```
Thanks Kaisar Haq (https://github.com/kaisarh) :)



