cmake_minimum_required(VERSION 2.8)

include_directories( "."
/home/ramon/rc/matrix/_install/include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wextra -Wcomment -ggdb")

set(INCLUDE_FILES
sdrm_types.h
airspy_component.h
simple_msgpk_client.h
)

set(SOURCE_FILES
airspy_component.cc
airspyhf_handlers.cc
sdrm_types.cc
sdrm_main.cc
simple_msgpk_client.cc
)

add_executable(sdrm ${SOURCE_FILES})
target_link_libraries (sdrm LINK_PUBLIC matrix yaml-cpp zmq
fftw3 airspyhf rt boost_regex pthread -L/home/ramon/rc/matrix/_install/lib matrix)

# To install the .h files, try this recipie
install(TARGETS sdrm DESTINATION bin)

