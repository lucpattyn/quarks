
# quarks
Modern C++ based server side framework for optimal solutions

The philosophy and technological guidance behind Quarks can be found in this link:
[quarks philosophy](https://dev.to/lucpattyn/quarks-a-new-approach-with-a-new-mindset-to-programming-10lk)

This is an example using [Crow](https://github.com/ipkn/crow) library  to serve an OpenCV image filter.
We will have a mechanism in later stages to support various plugins. 
We will provide OpenCV as a readily available plugin.
This example users a compiled version of OpenCV 4.0.0 and requires following packages:

- OpenCV 4.0.0
- Crow Library v0.1
- GCC with support to C++17
- Cmake 1.13
- Boost::System

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

You should submit a POST request to http://localhost:18080/filters/gausian. The body of this request should be your
binary PNG image. The response should be a gausian filtered image from the submited image.

This however is not the purpose behind Quarks. It's just to get things started..
