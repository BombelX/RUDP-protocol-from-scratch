//
// Created by bombel on 6.02.2026.
//
#pragma once
#ifndef RUDP_PROJECT_RUDPPACKET_H
#define RUDP_PROJECT_RUDPPACKET_H
#include <cstdint>
#include <memory>
#include <vector>
#include <netinet/in.h>


namespace rudppacket {
    #pragma pack(push, 1)
    class RUDPheader { //13 bytes
    public:
        uint32_t seq_number;
        uint32_t ack_number;
        uint8_t flags;
        uint16_t checksum;
        uint16_t data_size;
    };

    // ||    4B     |     4B      |    1B     |       2B      |     2B      ||
    // ||  seq_num  |   ack_num   |   flags   |   check_sum   |  data_size  ||
    // ||           |             |           |               |             ||

#pragma pack(pop)
    class RUDPpacket {
    public:
        sockaddr_in address;
        RUDPheader header;
        std::unique_ptr<char[]> payload;
        size_t payload_size;

        RUDPpacket() : payload_size(0) {};
    };
}



#endif //RUDP_PROJECT_RUDPPACKET_H