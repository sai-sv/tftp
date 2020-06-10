#include "tftp_client.h"

#include <cstring>
#include <fstream>

TFTPClient::TFTPClient(const std::string &serverAddress, uint16_t port)
    : m_remoteAddress(serverAddress)
    , m_port(port)
    , m_remotePort(0)
    , m_receivedBlock(0)
{
    m_status = m_socket.initSocket() ? Status::Success : Status::InvalidSocket;
}

TFTPClient::Status TFTPClient::get(const std::string &fileName)
{
    if (m_status != Status::Success) {
        return m_status;
    }

    std::fstream file(fileName.c_str(), std::ios_base::out | std::ios_base::binary);
    if (!file) {
        std::puts("\nError! Cant't opening file for writing!");
        return Status::OpenFileError;
    }

    // RRQ
    Result result = this->sendRequest(fileName, OperationCode::RRQ);
    if (result.first != Status::Success) {
        return result.first;
    }

    m_receivedBlock = 0;

    // FILE
    result = this->getFile(file);
    if (result.first == Status::Success) {
        //std::cout << "Success: " << result.second << " bytes received!\n";
    }
    return result.first;
}

TFTPClient::Status TFTPClient::put(const std::string &fileName)
{
    if (m_status != Status::Success) {
        return m_status;
    }

    std::fstream file(fileName.c_str(), std::ios_base::in | std::ios_base::binary);
    if (!file) {
        std::puts("\nError! Can't open file for reading!");
        return Status::OpenFileError;
    }

    // WRQ
    Result result = this->sendRequest(fileName, OperationCode::WRQ);
    if (result.first != Status::Success) {
        return result.first;
    }

    m_receivedBlock = 0;

    // ACK
    result = this->read();
    if (result.first != Status::Success) {
        return result.first;
    }

    // FILE
    result = putFile(file);
    if (result.first == Status::Success) {
        //std::cout << "Success: " << result.second << " bytes written!\n";
    }
    return result.first;
}

std::string TFTPClient::errorDescription(TFTPClient::Status code)
{
    switch (code) {
    case Status::Success:
        return "\nSuccess.";
    case InvalidSocket:
        return "\nError! Invalid Socket.";
    case WriteError:
        return "\nError! Write Socket.";
    case ReadError:
        return "\nError! Read Socket.";
    case UnexpectedPacketReceived:
        return "\nError! Unexpected Packet Received.";
    case EmptyFilename:
        return "\nError! Empty Filename.";
    case OpenFileError:
        return "\nError! Can't Open File.";
    case WriteFileError:
        return "\nError! Write File.";
    case ReadFileError:
        return "\nError! Read File.";
    default:
        return "\nError!";
    }
}

TFTPClient::Result TFTPClient::sendRequest(const std::string &fileName, OperationCode code)
{
    if (fileName.empty()) {
        return std::make_pair(Status::EmptyFilename, 0);
    }

    std::string mode("octet");

    m_buffer[0] = 0;
    m_buffer[1] = static_cast<char>(code);

    // filename
    char *end = std::strncpy(&m_buffer[2], fileName.c_str(), fileName.size()) + fileName.size();
    *end++ = '\0';

    // mode
    end = std::strncpy(end, mode.c_str(), mode.size()) + mode.size();
    *end++ = '\0';

    const auto packetSize = std::distance(&m_buffer[0], end);
    const auto writtenBytes =
            m_socket.writeDatagram(&m_buffer[0], packetSize, m_remoteAddress.c_str(), m_port);
    if (writtenBytes != packetSize) {
        return std::make_pair(Status::WriteError, writtenBytes);
    }

    return std::make_pair(Status::Success, writtenBytes);
}

TFTPClient::Result TFTPClient::sendAck(const char *host, uint16_t port)
{
    const std::size_t packetSize = 4;

    m_buffer[0] = 0;
    m_buffer[1] = static_cast<char>(OperationCode::ACK);

    const auto bytesWritten =
            m_socket.writeDatagram(&m_buffer[0], packetSize, host, port);
    const bool isSuccess = bytesWritten == packetSize;

    return std::make_pair(isSuccess ? Status::Success : Status::WriteError,
                          isSuccess ? packetSize : bytesWritten);
}

TFTPClient::Result TFTPClient::read()
{
    const auto receivedBytes =
            m_socket.readDatagram(&m_buffer[0], m_buffer.size(), &m_remoteAddress[0], &m_remotePort);
    if (receivedBytes == -1) {
        std::puts("\nError! No data received.");
        return std::make_pair(Status::ReadError, receivedBytes);
    }

    const auto code = static_cast<OperationCode>(m_buffer[1]);
    switch (code) {
    case OperationCode::DATA:
        m_receivedBlock = ((uint8_t)m_buffer[2] << 8) | (uint8_t)m_buffer[3];
        return std::make_pair(Status::Success, receivedBytes - m_headerSize);
    case OperationCode::ACK:
        m_receivedBlock = ((uint8_t)m_buffer[2] << 8) | (uint8_t)m_buffer[3];
        return std::make_pair(Status::Success, m_receivedBlock);
    case OperationCode::ERR:
        printf("\nError! Message from remote host: %s", &m_buffer[4]);
        return std::make_pair(Status::ReadError, receivedBytes);
    default:
        printf("\nError! Unexpected packet received! Type: %i", code);
        return std::make_pair(Status::UnexpectedPacketReceived, receivedBytes);
    }
}

TFTPClient::Result TFTPClient::getFile(std::fstream &file)
{
    uint16_t totalReceivedBlocks = 0;
    uint16_t receivedDataBytes = 0;
    unsigned long totalReceivedDataBytes = 0;
    Result result;

    while (true) {
        // DATA
        result = this->read();
        if (result.first != Status::Success) {
            return std::make_pair(result.first, totalReceivedDataBytes + std::max(result.second, 0));
        }
        receivedDataBytes = result.second;

        // write to file
        if ((receivedDataBytes > 0) && (m_receivedBlock > totalReceivedBlocks)) {
            ++totalReceivedBlocks;
            totalReceivedDataBytes += receivedDataBytes;

            file.write(&m_buffer[m_headerSize], receivedDataBytes);
            if (file.bad()) {
                return std::make_pair(Status::WriteFileError, totalReceivedDataBytes);
            }
        }

        // ACK
        result = this->sendAck(m_remoteAddress.c_str(), m_remotePort);
        if (result.first != Status::Success) {
            return std::make_pair(result.first, totalReceivedDataBytes);
        }

        printf("\r%lu bytes (%i blocks) received", totalReceivedDataBytes, totalReceivedBlocks);

        if (receivedDataBytes != m_dataSize) {
            break;
        }
    }
    return std::make_pair(Status::Success, totalReceivedDataBytes);
}

TFTPClient::Result TFTPClient::putFile(std::fstream &file)
{
    Result result;
    uint16_t currentBlock = 0;
    unsigned long totalWrittenBytes = 0;

    while (true) {
        if (currentBlock == m_receivedBlock) {
            if (file.eof()) {
                return std::make_pair(Status::Success, totalWrittenBytes);
            }
            ++currentBlock;

            m_buffer[0] = 0;
            m_buffer[1] = static_cast<char>(OperationCode::DATA);
            m_buffer[2] = static_cast<uint8_t>(currentBlock >> 8);
            m_buffer[3] = static_cast<uint8_t>(currentBlock & 0x00FF);

            // read from file
            file.read(&m_buffer[m_headerSize], m_dataSize);
            if (file.bad()) {
                return std::make_pair(Status::ReadFileError, totalWrittenBytes);
            }
        }

        // DATA
        const auto packetSize = m_headerSize + file.gcount();
        const auto writtenBytes =
                m_socket.writeDatagram(&m_buffer[0], packetSize, m_remoteAddress.c_str(), m_remotePort);
        if (writtenBytes != packetSize) {
            return std::make_pair(Status::WriteError, totalWrittenBytes + writtenBytes);
        }

        // ACK
        result = this->read();
        if (result.first != Status::Success) {
            return std::make_pair(result.first, totalWrittenBytes + packetSize);
        }

        totalWrittenBytes += file.gcount();

        printf("\r%lu bytes (%i blocks) written", totalWrittenBytes, currentBlock);
    }
}
