cmake_minimum_required(VERSION 2.8)
project( ocv_microservice_crow )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra -Wno-missing-field-initializers")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMakeModules)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMakeModules)

option(BUILD_DOCS "build docs alongside the project" OFF)

find_program(CTEST_MEMORYCHECK_COMMAND NAMES valgrind)
find_program(CTEST_COVERAGE_COMMAND NAMES gcov)
find_program(CMAKE_CXX_CPPCHECK NAMES cppcheck)

if (CMAKE_CXX_CPPCHECK)
    message("-- Cppcheck found " ${CMAKE_CXX_CPPCHECK})
    list(
            APPEND CMAKE_CXX_CPPCHECK
            "--enable=all"
            "--suppress=*:${PROJECT_SOURCE_DIR}/third-party*"
            "--suppress=*:${PROJECT_SOURCE_DIR}/tests*"
    )
else()
    message("-- Cppcheck not found")
    set(CMAKE_CXX_CPPCHECK "")
endif()

INCLUDE(Dart)

add_custom_target(memcheck COMMAND ${CMAKE_CTEST_COMMAND}  --force-new-ctest-process --test-action memcheck)
add_custom_target(coverage COMMAND ${CMAKE_CTEST_COMMAND}  --force-new-ctest-process --test-action coverage)

find_package( OpenCV 4.0.0 REQUIRED )
include_directories( ${PROJECT_SOURCE_DIR}/include )
include_directories( ${OpenCV_INCLUDE_DIRS} )

file(GLOB ocv_microservice_crow_SOURCES
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

add_executable( ocv_microservice_crow ${ocv_microservice_crow_SOURCES} )

set(CROW_ROOT_DIR PATH "${PROJECT_SOURCE_DIR}/third-party/crow")

find_package(Crow)
if (CROW_FOUND)
    include_directories(${CROW_INCLUDES})
else()
    message(FATAL_ERROR "Crow not found. Could not build this app.")
endif()

find_package(Boost COMPONENTS system REQUIRED)

if (Boost_FOUND)
    include_directories(${Boost_INCLUDEDIR})

    set(Boost_USE_STATIC_LIBS        ON)
    set(Boost_USE_MULTITHREADED      ON)
    set(Boost_USE_STATIC_RUNTIME    OFF)

    target_link_libraries(ocv_microservice_crow ${Boost_SYSTEM_LIBRARIES})

else()
    message(FATAL_ERROR "Boost not found. Could not build this app.")
endif()

target_link_libraries( ocv_microservice_crow ${OpenCV_LIBS} pthread )

if (BUILD_TESTS)
    find_package(GTest)
    if (GTEST_FOUND)
        include_directories(${GTEST_INCLUDE_DIRS})
    else()
        ADD_SUBDIRECTORY (third-party/googletest)
        include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
    endif()

    enable_testing()
    add_subdirectory(tests)
endif()

if (BUILD_DOCS)

    find_package(Doxygen
            REQUIRED dot
            OPTIONAL_COMPONENTS mscgen dia)

    if (DOXYGEN_FOUND)

        set(DOXYGEN_IN ${CMAKE_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/docs/Doxyfile)

        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

        add_custom_target(docs ALL
                COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs
                COMMENT "Generating doxygen documentation"
                VERBATIM)

        message("-- Adding doxygen documentation done")

    else(DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen need to be installed to generate the doxygen documentation")
    endif(DOXYGEN_FOUND)

endif()