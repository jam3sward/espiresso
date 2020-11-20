#include "network.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>

std::string getPrimaryIPString() 
{
    // create socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if ( sock == -1 ) return std::string();

    // construct the address of Google's DNS server 8.8.8.8,
    // with the DNS port number 53
    struct sockaddr_in servAddr{};
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr("8.8.8.8");  // Google's DNS server
    servAddr.sin_port = htons(53);  // DNS port

    // this will receive the socket address
    sockaddr_in sockAddr{};
    socklen_t sockAddrLen = sizeof(sockAddr);

    // attempt to connect to the server
    auto result = connect(sock, (const sockaddr*)&servAddr, sizeof(servAddr));
    if ( result != -1 ) {
        // get the address that the socket is bound to
        result = getsockname(sock, (sockaddr*)&sockAddr, &sockAddrLen);
    }

    // close the socket
    close(sock);

    // connect or getsockname failed
    if ( result == -1 ) return std::string();

    // convert the address to text
    std::vector<char> buffer(INET_ADDRSTRLEN);
    if ( inet_ntop(AF_INET, &sockAddr.sin_addr, buffer.data(), buffer.size()) ) {
        // success: return IP address string
        return buffer.data();
    } else {
        // failure: return empty string
        return std::string();
    }
}
