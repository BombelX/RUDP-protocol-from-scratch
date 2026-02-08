//
// Created by bombel on 5.02.2026.
//
#pragma once
#ifndef RUDP_PROTOCOL_FROM_SCRATCH_UDPSOCKET_H
#define RUDP_PROTOCOL_FROM_SCRATCH_UDPSOCKET_H
#include <unordered_map>
#include <atomic>
#include <string>
#include <netinet/in.h>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <ctime>
#include "RUDPpacket.h"
#include <queue>

namespace rudp {
    struct TimerEntry {
        std::chrono::steady_clock::time_point expireTime;
        uint32_t seqNumber;
        int attempt_cnt;

        bool operator>(const TimerEntry& other) const {
            return expireTime > other.expireTime;
        }
        bool operator<(const TimerEntry& other) const {
            return expireTime < other.expireTime;
        }
    };
    class UDPsocket {
    public:
        const int ATTEMPT_LIMIT = 10;
        UDPsocket();
        ~UDPsocket();
        void reciveThread();

        bool addToPq(TimerEntry packet_identifier);

        void sendRaw(const rudppacket::RUDPpacket &packet);

        void sendReliable(const std::string &mess, const std::string &ip, int port);

        void processPackets();

        void sendTo(const std::string& mess, const std::string& ip, int port) const;
        void bindPort(int port) const;

        int recive(char *buffer, size_t max_size, std::string sourceIP,
                   int &sourcePort) const;

        UDPsocket(const UDPsocket&) = delete;
        UDPsocket& operator=(const UDPsocket&) = delete;
    private:
        std::priority_queue<TimerEntry, std::vector<TimerEntry>, std::greater<>> packets_pq;
        std::condition_variable process_cv;
        std::mutex process_mtx;
        std::atomic<bool> running = true;
        std::queue<rudppacket::RUDPpacket> syncQueue;
        std::mutex queue_mutex;
        int socket_udp_;
        int ack_number = 1;
        std::unordered_map<uint32_t, rudppacket::RUDPpacket> pendingPackets;

    };
}



#endif //RUDP_PROTOCOL_FROM_SCRATCH_UDPSOCKET_H