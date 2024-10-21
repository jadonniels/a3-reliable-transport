#include "PacketHeader.h"
#include "crc32.h"

class wSender
{
    // Steps:
    // 1. Read in input file
    // 2. Setup and send packets USING UDP !
    // 2a) Split file into appropriate chunks
    // 2b) Append a checksum (presumably the whole header?) to the packet
    //      Use crc32.h function
    // 2c) *Only send through packets based on size of sliding window
    // 2d) Handle the following cases:
    //      Loss of arbitrary levels;
    //      Re­ordering of ACK messages;
    //      Duplication of any amount for any packet;
    //      Delay in the arrivals of ACKs.

    //      *Retransmission timer: 500 ms
    // 2e) Receive and track the ACKs that we get and retransmit accordingly
    //      Retransmit ALL of the window if packet M + 1 ACK has not been recvd
    // 2f) LOGGING
    // 3) Send an END message
};