#include <map>
#include <chrono>
#include "PacketHeader.h"
#include <sys/select.h>

// ... [Other includes and initializations]

// Data structure to hold packet information
struct PacketData
{
    std::string data;
    PacketHeader header;
    std::chrono::steady_clock::time_point send_time;
};

int main()
{
    // Map to hold outstanding packets with sequence number as the key
    std::map<uint32_t, PacketData> outstanding_packets;

    // Initialize variables
    uint32_t seqNum = 0;

    // Main loop
    while (!chunks.empty() || !outstanding_packets.empty())
    {
        // Sending packets while window is not full
        while (outstanding_packets.size() < outstanding_limit && !chunks.empty())
        {
            seqNum++;

            // Prepare header
            PacketHeader header;
            header.type = 2; // DATA
            header.seqNum = seqNum;
            header.length = chunks.front().size();
            header.checksum = 0;

            // Prepare packet
            size_t data_size = chunks.front().size();
            size_t packet_size = sizeof(header) + data_size;
            std::vector<char> send_packet(packet_size);

            // Copy header and data into send_packet
            memcpy(send_packet.data(), &header, sizeof(header));
            memcpy(send_packet.data() + sizeof(header), chunks.front().data(), data_size);

            // Calculate checksum
            header.checksum = crc32(send_packet.data(), packet_size);

            // Update header with checksum in send_packet
            memcpy(send_packet.data(), &header, sizeof(header));

            // Send packet
            ssize_t sent_bytes = sendto(sockfd, send_packet.data(), packet_size, 0, (struct sockaddr *)&recv_addr, sizeof(recv_addr));
            if (sent_bytes == -1)
            {
                perror("sendto failed");
                // Handle error appropriately
            }

            // Record packet data for potential retransmission
            PacketData packetData;
            packetData.data = std::move(chunks.front());
            packetData.header = header;
            packetData.send_time = std::chrono::steady_clock::now();
            outstanding_packets[seqNum] = std::move(packetData);

            // Remove the chunk from the queue
            chunks.pop();
        }

        // Receiving acknowledgments
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; // 100 ms timeout

        int select_result = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);
        if (select_result > 0 && FD_ISSET(sockfd, &read_fds))
        {
            // Receive ACK
            ssize_t recv_bytes = recvfrom(sockfd, recv_packet, sizeof(recv_packet), 0, NULL, NULL);
            if (recv_bytes > 0)
            {
                PacketHeader *ack_header = reinterpret_cast<PacketHeader *>(recv_packet);
                if (ack_header->type == 3) // ACK type
                {
                    uint32_t ack_seqNum = ack_header->seqNum;
                    // Remove acknowledged packet
                    outstanding_packets.erase(ack_seqNum);
                }
            }
        }

        // Handling retransmissions
        auto current_time = std::chrono::steady_clock::now();
        for (auto &entry : outstanding_packets)
        {
            auto &packetData = entry.second;
            auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - packetData.send_time);
            if (elapsed_time.count() >= 500)
            {
                // Resend packet
                size_t data_size = packetData.data.size();
                size_t packet_size = sizeof(packetData.header) + data_size;
                std::vector<char> resend_packet(packet_size);

                // Copy header and data into resend_packet
                memcpy(resend_packet.data(), &packetData.header, sizeof(packetData.header));
                memcpy(resend_packet.data() + sizeof(packetData.header), packetData.data.data(), data_size);

                // Send packet
                ssize_t sent_bytes = sendto(sockfd, resend_packet.data(), packet_size, 0, (struct sockaddr *)&recv_addr, sizeof(recv_addr));
                if (sent_bytes == -1)
                {
                    perror("sendto failed");
                    // Handle error appropriately
                }

                // Update send time
                packetData.send_time = current_time;
            }
        }
    }

    return 0;
}