#include "udp_socket.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

#include <cstring> // memset

UdpSocket::UdpSocket()
    : m_port(0)
    , m_socket(-1)
{
    ::memset(m_address, '\0', 15);
}

UdpSocket::~UdpSocket()
{
    abort();
}

bool UdpSocket::initSocket()
{
    // Creating socket file descriptor
    m_socket = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (m_socket == -1) {
        return false; // socket creation failed
    }

    // read/write timeout
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    if (::setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        return false;
    }
    /*if (::setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        return false;
    }*/

    return true;
}

bool UdpSocket::bind(const char *localAddress, uint16_t localPort)
{
    if (localPort == 0) {
        return false;
    }

    if (localAddress != nullptr) {
        ::strcpy(m_address, localAddress);
    }
    m_port = localPort;

    if (!initSocket()) {
        return false;
    }

    int broadcast = 1;
    int result =
            ::setsockopt(m_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    if (result != 0) {
        return false;
    }

    struct sockaddr_in localAddr;
    ::memset(&localAddr, '\0', sizeof(localAddr));

    // Filling server information
    localAddr.sin_family = AF_INET; // IPv4
    //    local_addr.sin_addr.s_addr = local_address != nullptr ? ::inet_addr( local_address ) : INADDR_ANY;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons(m_port);

    // Bind the socket with the server address
    result = ::bind(m_socket, (const struct sockaddr *)&localAddr, sizeof(localAddr));
    if (result != 0) { // bind failed
        ::close(m_socket);
        return false;
    }

    return true;
}

bool UdpSocket::bind( uint16_t localPort )
{
    return bind(nullptr, localPort);
}

void UdpSocket::abort()
{
    if (m_socket != -1) {
        ::shutdown(m_socket, 2);
        ::close(m_socket);
        m_socket = -1;
    }
}

int64_t UdpSocket::readDatagram(char *data, int64_t maxlen, char *host, uint16_t *port)
{
    if ((data == nullptr) || (maxlen == 0) || (m_socket < 0)) {
        return 0;
    }

    struct sockaddr_in remoteAddr;
    socklen_t remoteAddrLen = sizeof(struct sockaddr_in);
    ::memset(&remoteAddr, '\0', sizeof(remoteAddr));

    const int64_t size =
            ::recvfrom(m_socket, data, maxlen,
                       MSG_WAITFORONE, // blocking operation! Use MSG_DONTWAIT for non blocking
                       (struct sockaddr *)&remoteAddr, &remoteAddrLen);
    if (size > 0) {
        if (host != nullptr) {
            //inet_pton(AF_INET, inet_ntoa( senderAddr.sin_addr ), host);
            ::strcpy(host, inet_ntoa(remoteAddr.sin_addr));
        }
        if (port != nullptr) {
            *port = ntohs(remoteAddr.sin_port);
        }
    }

    return size;
}

int64_t UdpSocket::writeDatagram(const char *data, int64_t len, const char *host, uint16_t port)
{
    if ((data == nullptr) || (len == 0) || (m_socket < 0)) {
        return 0;
    }

    struct sockaddr_in remoteAddr;
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_addr.s_addr = host != nullptr ? ::inet_addr(host) : INADDR_ANY;
    remoteAddr.sin_port = htons(port);
    ::memset(remoteAddr.sin_zero, '\0', sizeof(remoteAddr.sin_zero));

    const int64_t size =
            ::sendto(m_socket, data, len, MSG_DONTWAIT, (struct sockaddr*)&remoteAddr, sizeof(remoteAddr));

    return size;
}

const char* UdpSocket::localAddress() const
{
    return m_address;
}

uint16_t UdpSocket::localPort() const
{
    return m_port;
}
