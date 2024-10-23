#include "wSender.h"

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
    // FIXME - I don't really know how we should decode here
    outfile << header.type << header.seqNum << header.length << header.checksum << std::endl;
}

// REQUIRES: None
// MODIFIES: None
// EFFECTS: Split file into FILE_CHUNK_SIZE chunks and return a vector of each chunk
vector<string> wSender::split_file_chunks(const string &file_in)
{
    std::ifstream instream(file_in);
    vector<string> res;

    char file_chunk[FILE_CHUNK_SIZE];
    while (!instream.eof())
    {
        instream.read(file_chunk, FILE_CHUNK_SIZE);
        // # bytes actually read
        std::streamsize bytes_read = instream.gcount();
        if (bytes_read > 0)
            res.push_back(string(file_chunk, bytes_read));
    }

    instream.close();
    return res;
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

    // Initialize needed objects
    int outstanding_limit = std::stoi(argv[3]);
    char send_packet[MAX_SEND_DATA];
    char recv_packet[sizeof(PacketHeader)];
    std::ofstream sender_log(argv[5]);
    PacketHeader header{0, 0, 0, 0};

    vector<string> chunks = split_file_chunks(string(argv[4]));
    send_start(sockfd, recv_addr, header, sender_log);

    // FIXME - Need to correspond to split_file_chunks vec + logging
    while (true)
    {
        sendto(sockfd, send_packet, strlen(send_packet), 0, (struct sockaddr *)&recv_addr, sizeof(recv_addr));
        // Receive ACK
        recvfrom(sockfd, recv_packet, sizeof(recv_packet), 0, NULL, NULL);
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
