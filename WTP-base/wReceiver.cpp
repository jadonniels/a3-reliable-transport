#include "wReceiver.h"

int main()
{
    int sockfd;
    char buffer[1024];
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len;
    int n;

    // Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    check_error(sockfd);

    // Bind to port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8888);

    check_error(bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0);

    addr_len = sizeof(client_addr);

    // Receive data
    n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &addr_len);
    buffer[n] = '\0';
    printf("Client: %s\n", buffer);

    // Send response
    const char *message = "Hello from server";
    sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&client_addr, addr_len);

    close(sockfd);
    return 0;
}