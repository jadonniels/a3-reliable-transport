#include "PacketHeader.h"
#include "crc32.h"

#include <chrono>
#include <deque>
#include <vector>
#include <string>
#include <unordered_set>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cstring>
#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>

// UDP Header 8B; IP Protocol Header 20B
// 1500 - (8 + 20) = 1472 for total send data
#define MAX_SEND_DATA 1472
// UDP Header 8B; IP Protocol Header 20B; PacketHeader 16B
// 1500 - (8 + 20 + 16) = 1456 for file data chunks
#define FILE_CHUNK_SIZE 1456

using std::deque;
using std::string;
using std::unordered_set;
using std::vector;

void check_error(int input)
{
    if (input < 0){
        throw std::runtime_error(std::strerror(errno));
    }
}

class wSender
{
public:
    wSender(char *argv[]);

    // **Match a chunk to a given SeqNum
    // We don't HAVE to do this now
    // But it's something to think about for OPT

private:
    // 1. Read in input file
    void split_file_chunks(deque<string> &chunks,
                           const string &file_in);

    // 2. Setup and send packets USING UDP!
    void send_start(int recv_sock,
                    sockaddr_in &recv_addr,
                    PacketHeader &header,
                    std::ofstream &outfile);
    void send_window(deque<string> &chunks,
                     PacketHeader &header,
                     size_t outstanding_limit,
                     int sockfd,
                     sockaddr_in &recv_addr,
                     std::ofstream &outfile);

    // 3. Receive and track the ACKs that we get and retransmit accordingly
    //      Retransmit ALL of the window if packet M + 1 ACK has not been recvd
    void try_receive(deque<string> &chunks,
                     PacketHeader &header,
                     size_t outstanding_limit,
                     uint32_t &cur_seq_num,
                     int sockfd,
                     std::ofstream &outfile);

    // 4. Send an END message
    void send_end(int sockfd,
                  sockaddr_in &recv_addr,
                  PacketHeader &header,
                  uint32_t cur_seq_num,
                  std::ofstream &outfile);

    // 5. LOGGING
};
