#ifndef TFTPCLIENT_H
#define TFTPCLIENT_H

#include "udp_socket.h"
#include <array>

class TFTPClient
{
    enum OperationCode {
        RRQ = 1, WRQ, DATA, ACK, ERR, OACK
    };

public:
    enum Status {
        Success = 0,
        InvalidSocket,
        WriteError,
        ReadError,
        UnexpectedPacketReceived,
        EmptyFilename,
        OpenFileError,
        WriteFileError,
        ReadFileError
    };

    TFTPClient(const std::string &serverAddress, uint16_t port);

    Status get(const std::string &fileName);
    Status put(const std::string &fileName);

    std::string errorDescription(Status code);

private:
    static constexpr uint8_t m_headerSize = 4;
    static constexpr uint16_t m_dataSize = 512;

    using Result = std::pair<Status, int32_t>;
    using Buffer = std::array<char, m_headerSize + m_dataSize>;

    Result sendRequest(const std::string &fileName, OperationCode code);
    Result sendAck(const char *host, uint16_t port);
    Result read();
    Result getFile(std::fstream &file);
    Result putFile(std::fstream &file);

    UdpSocket m_socket;
    std::string m_remoteAddress;
    uint16_t m_port;
    uint16_t m_remotePort;
    Buffer m_buffer;
    uint16_t m_receivedBlock;
    Status m_status;
};

#endif // TFTPCLIENT_H
