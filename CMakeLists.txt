cmake_minimum_required(VERSION 3.10)
project(RUDP_Project)
set(CMAKE_CXX_STANDARD 17)



include_directories(include)

add_executable(rudp_app
        src/main.cpp
        src/UDPsocket.cpp
        src/UDPsocket.cpp
        include/UDPsocket.h
        include/RUDPpacket.h
)