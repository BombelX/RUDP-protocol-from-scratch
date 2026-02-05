//
// Created by bombel on 5.02.2026.
//
#pragma once
#ifndef RUDP_PROTOCOL_FROM_SCRATCH_UDPSOCKET_H
#define RUDP_PROTOCOL_FROM_SCRATCH_UDPSOCKET_H

#include <string>
#include <netinet/in.h>


namespace rudp {
    class UDPsocket {
    public:
        UDPsocket();
        ~UDPsocket();
        void sendTo(const std::string& mess, const std::string& ip, int port) const;
        void bindPort(int port) const;

        int recive(char *buffer, size_t max_size, std::string sourceIP,
                   int &sourcePort) const;

        UDPsocket(const UDPsocket&) = delete;
        UDPsocket& operator=(const UDPsocket&) = delete;
    private:
        int socket_udp_;
    };
}



#endif //RUDP_PROTOCOL_FROM_SCRATCH_UDPSOCKET_H