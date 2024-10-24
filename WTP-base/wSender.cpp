#include "wSender.h"

// REQUIRES: None
// MODIFIES: chunks
// EFFECTS: Split file into FILE_CHUNK_SIZE chunks and append to chunks deque
void wSender::split_file_chunks(deque<string> &chunks,
                                const string &file_in)
{
    std::ifstream instream(file_in);

    char file_chunk[FILE_CHUNK_SIZE];
    while (!instream.eof())
    {
        instream.read(file_chunk, FILE_CHUNK_SIZE);

        // # bytes actually read
        std::streamsize bytes_read = instream.gcount();
        if (bytes_read > 0)
            chunks.push_back(string(file_chunk, bytes_read));
    }

    instream.close();
}

// FIXME - Consider that START and END may not be received - must confirm?
// REQUIRES: None
// MODIFIES: None
// EFFECTS: Calculate checksum for and send START message
void wSender::send_start(int sockfd,
                         sockaddr_in &recv_addr,
                         PacketHeader &header,
                         std::ofstream &outfile)
{
    char start_buf[sizeof(PacketHeader)];
    memcpy(start_buf, &header, sizeof(header));

    // Send START and log
    sendto(sockfd, start_buf, sizeof(start_buf),
           0, (struct sockaddr *)&recv_addr, sizeof(recv_addr));

    outfile << header.type << header.seqNum
            << header.length << header.checksum << std::endl;

    // Receive ACK, write to PacketHeader object and log
    char recv_buf[sizeof(PacketHeader)];
    recvfrom(sockfd, recv_buf, sizeof(recv_buf), 0, NULL, NULL);

    PacketHeader recv_header;
    memcpy(&recv_header, &recv_buf, sizeof(recv_header));

    outfile << recv_header.type << recv_header.seqNum
            << recv_header.length << recv_header.checksum << std::endl;
}

// REQUIRES: chunks.size() > 0
// MODIFIES: header, outfile
// EFFECTS: Copy data into char buffers and send min(chunks.size(), outstanding_limit) packets
void wSender::send_window(deque<string> &chunks,
                          PacketHeader &header,
                          size_t outstanding_limit,
                          int sockfd,
                          sockaddr_in &recv_addr,
                          std::ofstream &outfile)
{
    char send_packet[MAX_SEND_DATA];
    uint32_t i = 0;
    uint32_t sent_packets = 0;
    while (sent_packets < outstanding_limit &&
           sent_packets < chunks.size())
    {
        // type & seqNum were previously set correctly
        header.length = chunks[i].size();
        header.checksum = crc32(chunks[i].c_str(), chunks[i].size());

        // First copy header into the first 16 bytes
        memcpy(send_packet, &header, sizeof(header));
        // Then copy data into the next bytes according to size
        memcpy(send_packet + sizeof(header), chunks[i].c_str(), chunks[i].size());

        // <= 1472
        size_t packet_size = sizeof(header) + chunks[i].size();
        sendto(sockfd, send_packet, packet_size,
               0, (struct sockaddr *)&recv_addr, sizeof(recv_addr));

        outfile << header.type << header.seqNum
                << header.length << header.checksum << std::endl;

        ++sent_packets;
        ++header.seqNum;
        ++i;
    }
}

// REQUIRES: chunks.size() > 0
// MODIFIES: chunks, header, cur_seq_num
// EFFECTS: Call recvfrom() min(chunks.size(), outstanding_limit) times, shifting window if poss
//          Places received-packet seqNum into a set to evaluate which need resent. Resets seqNum.
void wSender::try_receive(deque<string> &chunks,
                          PacketHeader &header,
                          size_t outstanding_limit,
                          uint32_t &cur_seq_num,
                          int sockfd,
                          std::ofstream &outfile)
{
    char recv_packet[sizeof(PacketHeader)];
    unordered_set<uint32_t> received_seq;
    while (received_seq.size() < outstanding_limit &&
           received_seq.size() < chunks.size())
    {
        ssize_t bytes_received = recvfrom(
            sockfd, recv_packet, sizeof(PacketHeader), 0, NULL, NULL);

        if (bytes_received == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break; // Timeout occurred
            else
                throw std::runtime_error("Error receiving data");
        }

        // Decode the received bytes into a PacketHeader object
        PacketHeader received_header;
        memcpy(&received_header, recv_packet, sizeof(PacketHeader));

        outfile << received_header.type << received_header.seqNum
                << received_header.length << received_header.checksum << std::endl;

        // Insert the seqNum into the unordered_set
        received_seq.insert(received_header.seqNum);
    }

    // Process received_seq set and move window accordingly
    while (received_seq.find(cur_seq_num) != received_seq.end())
    {
        ++cur_seq_num;
        chunks.pop_front();
    }

    // It's possible that this doesn't get changed at all
    header.seqNum = cur_seq_num;
}

void wSender::send_end(int sockfd,
                       sockaddr_in &recv_addr,
                       PacketHeader &header,
                       uint32_t cur_seq_num,
                       std::ofstream &outfile)
{
    char end_buf[sizeof(PacketHeader)];
    header.type = 1;
    header.seqNum = cur_seq_num;
    header.length = 0;
    header.checksum = 0;

    // Copy PacketHeader into end_buf
    memcpy(end_buf, &header, sizeof(header));

    // Send END and log
    sendto(sockfd, end_buf, sizeof(end_buf),
           0, (struct sockaddr *)&recv_addr, sizeof(recv_addr));

    outfile << header.type << header.seqNum << header.length << header.checksum << std::endl;
}

// REQUIRES: argc == 6
// MODIFIES: None
// EFFECTS: Driver for wSender functionality
wSender::wSender(char *argv[])
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    check_error(sockfd);

    // Set the socket receive timeout to 500 ms
    // I'm using this as the general transmission timeout, not sure if correct
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000; // 500 milliseconds
    check_error(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)));

    // Receiver details
    struct sockaddr_in recv_addr;
    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = htons(std::stoi(argv[2]));
    recv_addr.sin_addr.s_addr = inet_addr(argv[1]);

    // Create chunks deque
    deque<string> chunks;
    split_file_chunks(chunks, string(argv[4]));

    // Send start and begin stream
    std::ofstream sender_log(argv[5]);
    PacketHeader header{0, 0, 0, 0};
    send_start(sockfd, recv_addr, header, sender_log);

    header.type = 2;   // DATA for all of next stage
    header.seqNum = 1; // Begin the sequence at 1
    uint32_t cur_seq_num = 1;

    uint32_t outstanding_limit = std::stoul(argv[3]);
    while (!chunks.empty())
    {
        send_window(chunks, header, outstanding_limit,
                    sockfd, recv_addr, sender_log);
        try_receive(chunks, header, outstanding_limit,
                    cur_seq_num, sockfd, sender_log);
    }

    send_end(sockfd, recv_addr, header, cur_seq_num, sender_log);
    close(sockfd);
}

int main(int argc, char *argv[])
{
    if (argc != 6)
    {
        std::cout << "Invalid Input.\nUsage: ./wSender "
                  << "<receiver-IP> <receiver-port> <window-size> <input-file> <log>" << std::endl;
        exit(1);
    }

    wSender sender(argv);

    return 0;
}
