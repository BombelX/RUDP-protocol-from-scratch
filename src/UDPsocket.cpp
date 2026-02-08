//
// Created by bombel on 5.02.2026.
//

#include <sys/socket.h>
#include "UDPsocket.h"
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <queue>

#include "RUDPpacket.h"


namespace rudp {
    UDPsocket::UDPsocket() {
        std::cout << "Start";
        socket_udp_ = socket(AF_INET, SOCK_DGRAM, 0);

        if (socket_udp_ < 0) {
            std::string error_msg = "Creating socket failed error: " + std::string(std::strerror(errno));
            throw std::runtime_error(error_msg);
        }
    };
    UDPsocket::~UDPsocket() {
        running = false;
        process_cv.notify_all();

        std::cout << "Socket closed";
        if (socket_udp_ >= 0) {
            close(socket_udp_);
        }
    };



    void UDPsocket::reciveThread() {
        char buffer[65535];
        std::cout << "Receive Thread Started" << std::endl;
        struct sockaddr_in client_address{};

        socklen_t client_address_len = sizeof(client_address);

        while (running) {
            ssize_t bytes_recived = recvfrom(
                socket_udp_,
                buffer,
                65535,
                0,
                reinterpret_cast<struct sockaddr *>(&client_address),
                &client_address_len);
            if (bytes_recived < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                else {
                    std::string error_msg = "Receiving failed error: " + std::string(std::strerror(errno));
                    std::cout << error_msg << std::endl;
                    break;
                }
            }
            ssize_t header_size = sizeof(rudppacket::RUDPheader);
            if (bytes_recived < header_size) {
                continue;
            }
            rudppacket::RUDPheader header{};
            rudppacket::RUDPpacket packet;

            std::memcpy(&packet.header, buffer, sizeof(rudppacket::RUDPheader));
            packet.header.seq_number = ntohl(packet.header.seq_number);
            packet.header.ack_number = ntohl(packet.header.ack_number);
            packet.header.checksum   = ntohs(packet.header.checksum);
            packet.header.data_size  = ntohs(packet.header.data_size);
            size_t payload_size = bytes_recived - header_size;
            auto payload = std::make_unique<char[]>(payload_size);
            std::memcpy(payload.get(), &buffer[header_size], payload_size);
            packet.payload = std::move(payload);
            packet.address = client_address;
            packet.payload_size = payload_size;
            {
            std::lock_guard<std::mutex> lock(queue_mutex);
            syncQueue.push(std::move(packet));
            }
        }
        std::cout << "End" << std::endl;

    }

    bool UDPsocket::addToPq(TimerEntry packet_identifier) { // require process mutex
        if (packet_identifier.attempt_cnt <= ATTEMPT_LIMIT) {
            TimerEntry new_timer_entry;
            new_timer_entry.attempt_cnt = packet_identifier.attempt_cnt+1;
            new_timer_entry.expireTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(200);
            new_timer_entry.seqNumber = packet_identifier.seqNumber;

            packets_pq.push(new_timer_entry);
            return true;
        }
        return false;
    }

    void UDPsocket::sendRaw(const rudppacket::RUDPpacket& packet) {
        // TODO implement sending
    }

    void UDPsocket::sendReliable(const std::string &mess, const std::string &ip, int port) {
        uint32_t seq_number = 0;
        size_t MAX_PAYLOAD = 1200;
        size_t offset = 0;
        while (offset < mess.size()) {
            int chunk_size = std::min(MAX_PAYLOAD,mess.size() - offset);
            rudppacket::RUDPpacket packet;
            packet.header.seq_number = seq_number++;
            packet.header.ack_number = ack_number++;
            packet.header.data_size = chunk_size;

            std::memcpy(packet.payload.get(), mess.data()+offset, chunk_size);
            packet.address.sin_family = AF_INET;
            packet.address.sin_port = htons(port);
            inet_pton(AF_INET, ip.c_str(), &packet.address.sin_addr.s_addr);
            this -> sendRaw(packet);
            {
                std::lock_guard<std::mutex> lock(process_mtx);
                TimerEntry timer;
                timer.seqNumber = packet.header.seq_number;
                timer.attempt_cnt = 0;
                timer.expireTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(200);
                packets_pq.push(timer);
                pendingPackets[packet.header.seq_number] = std::move(packet);
            }
            process_cv.notify_one();
            offset += chunk_size;


        }
    }

    void UDPsocket::processPackets() {

        while (running) {
            std::unique_lock<std::mutex> lock(process_mtx);
            rudppacket::RUDPpacket recived_packet ;
            if (syncQueue.empty() && running) {
                if (packets_pq.empty()) {
                    process_cv.wait(lock, [this] { return !syncQueue.empty() || !running; });
                } else {
                    process_cv.wait_until(lock, packets_pq.top().expireTime, [this] {
                        return !syncQueue.empty() || !running;
                    });
                }
            }

            if (!running) break;
            while (!syncQueue.empty()) {
                {
                std::lock_guard<std::mutex> lock_sq(queue_mutex);
                recived_packet = std::move(syncQueue.front());
                syncQueue.pop();
                }
                if (pendingPackets.count(recived_packet.header.seq_number) > 0) {
                    pendingPackets.erase(recived_packet.header.seq_number);

                }

                else {
                    //TODO processing new packages another then ACK

                }

                //clearing a pq
                }
            auto now = std::chrono::steady_clock::now();
            while (!packets_pq.empty() && packets_pq.top().expireTime < now ) {
                bool to_resend = addToPq(packets_pq.top());
                if (to_resend) {
                    std::cout << "resending" << std::endl;
                }
                packets_pq.pop();
            }
        }
    }


    void UDPsocket::bindPort(int port) const {
        std::cout << port << std::endl;
        struct sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        address.sin_addr.s_addr = INADDR_ANY;
        int result = bind(socket_udp_,reinterpret_cast<struct sockaddr *>(&address),sizeof(address));
        if (result < 0) {
            throw std::runtime_error(std::string("Binding failed error: ") + std::string(std::strerror(errno)));
        }
    }
    void setSocketTimeout(int socket_fd, int timeoutInMicroSec) {
        struct timeval timeout{};
        timeout.tv_usec = timeoutInMicroSec;
        if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            perror("setsockopt failed");
        }
    }

    int UDPsocket::recive(char* buffer, size_t max_size,std::string sourceIP , int& sourcePort) const {

        struct sockaddr_in sender_address{};
        socklen_t adress_len = sizeof(sender_address);
        memset(&sender_address,0,sizeof(sender_address));

        ssize_t bytes_recived = recvfrom(
            socket_udp_,
            buffer,
            max_size,
            0,
            reinterpret_cast<struct sockaddr *>(&sender_address),
            &adress_len);

        if (bytes_recived < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
            }
            else {
                std::string error_msg = "Receiving failed error: " + std::string(std::strerror(errno));
                throw std::runtime_error(error_msg);
            }
        }


        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET,&sender_address.sin_addr,ip,INET_ADDRSTRLEN);
        sourceIP = std::string(ip);
        sourcePort = ntohs(sender_address.sin_port);




        return bytes_recived;
    }

    void UDPsocket::sendTo(const std::string &mess, const std::string &ip, int port) const {
        struct sockaddr_in destination_address{};
        memset(&destination_address, 0, sizeof(destination_address));
        destination_address.sin_family = AF_INET;
        destination_address.sin_port = htons(port);
        int result = inet_pton(AF_INET,ip.c_str(),&destination_address.sin_addr);

        sendto(socket_udp_,
            mess.c_str(),
            mess.size(),
            0,
            reinterpret_cast<struct sockaddr *>(&destination_address),
            sizeof(destination_address)
            );
        std::cout << "\n" <<"sent to" << ip << "message:" << mess << "port:" << port << std::endl;
    }

}
