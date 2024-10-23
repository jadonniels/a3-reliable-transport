#include "PacketHeader.h"
#include "crc32.h"

#include <chrono>
#include <queue>
#include <vector>
#include <string>
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

using std::queue;
using std::string;
using std::vector;

void check_error(int input)
{
    if (input < 0)
        throw std::runtime_error("ERROR on result");
}

class wSender
{
public:
    wSender(char *argv[]);

private:
    // DONE:
    // 1. Read in input file
    // 1a) Split file into appropriate chunks
    void split_file_chunks(queue<string> &chunks, const string &file_in);
    // 2. Setup and send packets USING UDP !
    void send_start(int recv_sock, sockaddr_in &recv_addr,
                    PacketHeader &header, std::ofstream &outfile);
    // 2a) Append a checksum (presumably the whole header?) to the packet
    //      Use crc32.h function

    // TODO:
    // 2b) ! Send through packets based on current window
    // 2c) Handle the following cases:
    //      Loss of arbitrary levels;
    //      Re­ordering of ACK messages;
    //      Duplication of any amount for any packet;
    //      Delay in the arrivals of ACKs.

    //      ! Retransmission timer: 500 ms
    // 2d) Receive and track the ACKs that we get and retransmit accordingly
    //      Retransmit ALL of the window if packet M + 1 ACK has not been recvd
    // 2e) LOGGING
    // 3) Send an END message
};