#include "wSender.h"

class wReceiver
{
    // Steps:
    // 1. Receive and store the file from wSender from ONE connection
    // 1a) Ignore any additional START in the middle of the connection
    // 1b) Should be named FILE-i.out (i == num files we've recvd so far)
    // 1c) Calculate and validate checksum (drop if not correct)
    // 1d) Send cumulative ACK with seqNum expected to recv next
    //      Two scenarios
    //       If it receives a packet with seqNum not equal to N, it will send back an ACK with seqNum=N.
    //       If it receives a packet with seqNum=N, it will check for the highest sequence number (say M)
    //       of the in­order packets it has already received and send ACK with seqNum=M+1.
    //      If next expected N, drop all with seqNum >= N + window-size
    // 2. LOGGING (EVERY PACKET SENT + RECEIVED)
};