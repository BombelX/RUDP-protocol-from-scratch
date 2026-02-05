//
// Created by bombel on 5.02.2026.
//

#include <iostream>
#include <ostream>

#include "UDPsocket.h"

int main () {
    rudp::UDPsocket socket;
    socket.sendTo("dupa jaś","127.0.0.1",8080);

    char MTUbuffer[1500];

    std::string from;
    int port;

    int data_cnt = socket.recive(MTUbuffer,sizeof(MTUbuffer),from,port);
    if (data_cnt > 0) {
        std::string mess(MTUbuffer,data_cnt);
        std::cout<<"Wiadomość od:"<<from<<" port: "<<port<<std::endl;
        std::cout<< "\n" <<mess<<std::endl;
    }

    return 0;

}
