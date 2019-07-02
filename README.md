
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

run the following commands to i) put a json object against key, ii) get that object against key and iii) do a wildcard search of keys

i) Put a json object against a key:
POST: http://0.0.0.0:18080/quarks/cache/putjson
BODY:
{"key":"g3_u3", "msg":"m3"}

Note: In the json body, it is required to have the "key" attribute as a part of the json object.  The whole json object {"key":"g3_u3", "msg":"m3"} is saved against the key "g3_u3"
We will improve this api later.

ii) Retrieve the json object by key:
POST: http://0.0.0.0:18080/quarks/cache/getjson
BODY: g3_u3

iii) Retrieve an array of json objects by wildcard matching of keys..
POST: http://0.0.0.0:18080/quarks/cache/findjson
BODY: g3_u*

You could  post a few values against keys with putjson, for example 
POST: http://0.0.0.0:18080/quarks/cache/putjson
BODY:
{"key":"g1_u2", "msg":"m1"}

POST: http://0.0.0.0:18080/quarks/cache/putjson
BODY:
{"key":"g1_u2", "msg":"m2"}

POST: http://0.0.0.0:18080/quarks/cache/putjson
BODY:
{"key":"g3_u3", "msg":"m3"}

and then check the results by 
POST: http://0.0.0.0:18080/quarks/cache/findjson
BODY: g1_u*

For those interested in testing OpenCV as plugin,
you should submit a POST request to http://localhost:18080/filters/gausian. The body of this request should be your
binary PNG image. 
The response should be a gausian filtered image from the submited image.
OpenCV however is a plugin (an additional feature) and not the main purpose behind Quarks.
