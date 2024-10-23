#include "wSender.h"

// REQUIRES: None
// MODIFIES: chunks
// EFFECTS: Split file into FILE_CHUNK_SIZE chunks and append to chunks vector
void wSender::split_file_chunks(queue<string> &chunks, const string &file_in)
{
    std::ifstream instream(file_in);

    char file_chunk[FILE_CHUNK_SIZE];
    while (!instream.eof())
    {
        instream.read(file_chunk, FILE_CHUNK_SIZE);
        // # bytes actually read
        std::streamsize bytes_read = instream.gcount();
        if (bytes_read > 0)
            chunks.push(string(file_chunk, bytes_read));
    }

    instream.close();
}

// REQUIRES: None
// MODIFIES: None
// EFFECTS: Calculate checksum for and send START message
void wSender::send_start(int sockfd, sockaddr_in &recv_addr,
                         PacketHeader &header, std::ofstream &outfile)
{
    char start_buf[sizeof(PacketHeader)];
    char recv_buf[sizeof(PacketHeader)];
    memcpy(start_buf, &header, sizeof(header));
    // The checksum uses rest of the message besides itself to avoid a circular dependency
    header.checksum = crc32(start_buf, sizeof(header));
    // Send START
    sendto(sockfd, start_buf, strlen(start_buf), 0, (struct sockaddr *)&recv_addr, sizeof(recv_addr));
    outfile << header.type << header.seqNum << header.length << header.checksum << std::endl;
    // Receive ACK
    recvfrom(sockfd, recv_buf, sizeof(recv_buf), 0, NULL, NULL);
    // FIXME - We need to figure out how to decode
    outfile << header.type << header.seqNum << header.length << header.checksum << std::endl;
}

// REQUIRES: argc == 6
// MODIFIES: None
// EFFECTS: Driver for wSender functionality
wSender::wSender(char *argv[])
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    check_error(sockfd);

    // Receiver details
    struct sockaddr_in recv_addr;
    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = htons(std::stoi(argv[2]));
    recv_addr.sin_addr.s_addr = inet_addr(argv[1]);

    queue<string> chunks;
    split_file_chunks(chunks, string(argv[4]));

    // Send start and begin stream
    std::ofstream sender_log(argv[5]);
    PacketHeader header{0, 0, 0, 0};
    send_start(sockfd, recv_addr, header, sender_log);
    header.type = 2; // DATA for all of next stage

    // Initialize window and packet sizes
    char send_packet[MAX_SEND_DATA];
    char recv_packet[sizeof(PacketHeader)];
    size_t outstanding_limit = std::stoul(argv[3]);
    vector<PacketHeader> outstanding_packets(outstanding_limit);

    // 1 We will send as many as outstanding_limit packets at once
    // 2 We want to match a chunk to a given SeqNum
    // 3 Length corresponds to the size of that file chunk
    // 4 checksum calculated before it is added
    // 5 maintain a retransmit timer for 1st packet of window
    while (chunks.size() > 0)
    {
        header.seqNum++;
        header.length = chunks.front().size();

        // First copy header into the first 16 bytes
        memcpy(send_packet, &header, sizeof(header));
        // Then copy data into the next bytes according to size
        memcpy(send_packet + sizeof(header), chunks.front().c_str(), chunks.front().size());

        // <= 1472
        size_t packet_size = sizeof(header) + chunks.front().size();
        header.checksum = crc32(send_packet, packet_size);
        // Copy updated header with checksum back into send_packet
        memcpy(send_packet, &header, sizeof(header));

        // UDP Send
        sendto(sockfd, send_packet, packet_size, 0, (struct sockaddr *)&recv_addr, sizeof(recv_addr));
        recvfrom(sockfd, recv_packet, sizeof(recv_packet), 0, NULL, NULL);

        chunks.pop();
    }

    printf("Server: %s\n", recv_packet);

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
