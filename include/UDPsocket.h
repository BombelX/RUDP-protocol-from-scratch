//
// Created by bombel on 5.02.2026.
//
#pragma once
#ifndef RUDP_PROTOCOL_FROM_SCRATCH_UDPSOCKET_H
#define RUDP_PROTOCOL_FROM_SCRATCH_UDPSOCKET_H

#include <atomic>
#include <string>
#include <netinet/in.h>
#include <mutex>
#include <queue>

#include "RUDPpacket.h"


namespace rudp {
    class UDPsocket {
    public:
        UDPsocket();
        ~UDPsocket();
        std::mutex queue_mutex;
        void reciveThread();

        void sendTo(const std::string& mess, const std::string& ip, int port) const;
        void bindPort(int port) const;

        int recive(char *buffer, size_t max_size, std::string sourceIP,
                   int &sourcePort) const;

        UDPsocket(const UDPsocket&) = delete;
        UDPsocket& operator=(const UDPsocket&) = delete;
    private:
        std::atomic<bool> running = true;
        std::queue<rudppacket::RUDPpacket> syncQueue;
        int socket_udp_;

    };
}



#endif //RUDP_PROTOCOL_FROM_SCRATCH_UDPSOCKET_H