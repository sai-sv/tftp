#include <tftp_client.h>
#include <algorithm>
#include <chrono>

int main(int argc, char **argv)
{
    // Usage: 192.168.1.104 get example.txt

    auto printInfo = []() {
        printf("\nTFTP Client\n  "
               "Usage:   tftp 'server' [get | put] 'file'\n  "
               "Example: tftp 192.168.1.104 get example.txt\n\n");
    };

    if (argc < 4) {
        printInfo();
        return 1;
    }

    const uint16_t port = 69;

    std::string host(argv[1]);
    std::string operation(argv[2]);
    std::string filename(argv[3]);

    // TODO: check args!!!

    std::transform(operation.begin(), operation.end(), operation.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    const int opCode = operation.compare("get") == 0 ? 1 : (operation.compare("put") == 0 ? 2 : 0);
    if (opCode == 0) {
        printInfo();
        return 1;
    }

    printf("\nStart TFTP Client %s:%u\n", host.c_str(), port);

    TFTPClient client(host, port);

    const auto begin = std::chrono::steady_clock::now();
    const TFTPClient::Status status = opCode == 1 ? client.get(filename) : client.put(filename);
    const auto end = std::chrono::steady_clock::now();

    if (status != TFTPClient::Status::Success) {
        printf("%s\n\n", client.errorDescription(status).c_str());
        return 1;
    }

    printf("\nElapsed time: %ld [ms]\n\n",
           std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());
    return 0;
}
