#include "udp_socket.h"
#include <cstring> // memset

#include <WinSock2.h>
#include <Ws2tcpip.h>

UdpSocket::UdpSocket()
    : m_port(0)
    , m_socket(INVALID_SOCKET)
{
    ::memset(m_address, '\0', 15);
}

UdpSocket::~UdpSocket()
{
    abort();
    WSACleanup();
}

bool UdpSocket::initSocket()
{
    // Declare some variables
    WSADATA wsaData;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR) {
       return false;
    }

    // Creating socket file descriptor
    m_socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == INVALID_SOCKET) {
        WSACleanup();
        return false; // socket creation failed
    }

    // read/write timeout
    /*DWORD timeout = 6000;
    if (::setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof timeout) < 0) {
        return false;
    }
    if (::setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof timeout) < 0) {
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

    // https://docs.microsoft.com/en-us/windows/desktop/api/winsock/nf-winsock-setsockopt
    BOOL broadcast = TRUE;
    int result =
            ::setsockopt(m_socket, SOL_SOCKET, SO_BROADCAST, (char *)&broadcast, sizeof(broadcast));
    if (result == SOCKET_ERROR) {
        return false;
    }

    struct sockaddr_in localAddr;
    ::memset(&localAddr, '\0', sizeof(localAddr));

    // Filling server information
    localAddr.sin_family = AF_INET; // IPv4
    localAddr.sin_addr.s_addr = localAddress != nullptr ? ::inet_addr(localAddress) : INADDR_ANY;
    //local_addr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons(m_port);

    // Bind the socket with the server address
    result = ::bind(m_socket, (const struct sockaddr *)&localAddr, sizeof(localAddr));
    if (result == SOCKET_ERROR) { // bind failed
        ::closesocket(m_socket);
        WSACleanup();
        return false;
    }

    return true;
}

bool UdpSocket::bind(uint16_t localPort)
{
    return bind(nullptr, localPort);
}

void UdpSocket::abort()
{
    if (m_socket != INVALID_SOCKET) {
        ::shutdown(m_socket, 2); // The shutdown function disables sends or receives on a socket.
        ::closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
}

int64_t UdpSocket::readDatagram(char *data, int64_t maxlen, char *host, uint16_t *port)
{
    if ((data == nullptr) || (maxlen == 0) || (m_socket == INVALID_SOCKET)) {
        return 0;
    }

    struct sockaddr_in remoteAddr;
    socklen_t remoteAddrLen = sizeof(struct sockaddr_in);
    ::memset(&remoteAddr, '\0', sizeof(remoteAddr));

    // todo: check param 4!
    const int64_t size =
            ::recvfrom(m_socket, data, maxlen, 0, (struct sockaddr *)&remoteAddr, &remoteAddrLen);
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
    if ((data == nullptr) || (len == 0) || (m_socket == INVALID_SOCKET)) {
        return 0;
    }

    struct sockaddr_in remoteAddr;
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_addr.s_addr = host != nullptr ? ::inet_addr(host) : INADDR_ANY;
    remoteAddr.sin_port = htons(port);
    ::memset(remoteAddr.sin_zero, '\0', sizeof(remoteAddr.sin_zero));

    // todo: check param 4!
    const int64_t size =
            ::sendto(m_socket, data, len, 0, (struct sockaddr*)&remoteAddr, sizeof(remoteAddr));

    return size;
}

const char *UdpSocket::localAddress() const
{
    return m_address;
}

uint16_t UdpSocket::localPort() const
{
    return m_port;
}
