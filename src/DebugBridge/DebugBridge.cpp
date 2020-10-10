#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

void downloadCoreDump(int socketFD) {
    std::cout << "donwloading core dump file" << std::endl;
    int64_t nameLength;
    /* TODO: support big endian */
    if (recv(socketFD, &nameLength, 8, MSG_WAITALL) != 8) {
        throw std::logic_error("could not read file name length: " + std::to_string(errno));
    }
    std::cout << "name length " << nameLength << std::endl;
    std::string name;
    name.resize(nameLength);
    if (recv(socketFD, name.data(), nameLength, MSG_WAITALL) != nameLength) {
        throw std::logic_error("could not read file name");
    }

    std::cout << "donwloading core dump file for " << name << std::endl;

    std::ofstream file(name + ".CoreDump");
    if (!file.is_open()) {
        throw std::logic_error(std::string("could not open core dump file for ") + name);
    }

    int64_t dataLength;
    /* TODO: support big endian */
    if (recv(socketFD, &dataLength, 8, MSG_WAITALL) != 8) {
        throw std::logic_error("could not read dump data length");
    }
    std::cout << "data length " << dataLength << std::endl;
    std::vector<char> data;
    data.resize(dataLength);
    if (recv(socketFD, data.data(), dataLength, MSG_WAITALL) != dataLength) {
        throw std::logic_error("could not dump data: " + std::to_string(errno));
    }
    file.write(data.data(), dataLength);
    file.close();
    std::cout << "successfully donwloaded core dump file" << std::endl;
}

int main(int, const char **) {
    int socketFD;
    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0) {
        throw std::runtime_error("unable to create socket");
    }
    sockaddr_in serverAddress = {
        .sin_family = AF_INET,
        .sin_port = htons(30000),
        .sin_addr = {0},
        .sin_zero = {}
    };
    int result = inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);
    if (result < 0) {
        throw std::runtime_error("converting address failed");
    }

    sleep(1);
    result = connect(socketFD,
                     reinterpret_cast<sockaddr *>(&serverAddress),
                     sizeof(sockaddr_in));
    if (result < 0) {
        throw std::runtime_error("connection failed: " + std::to_string(errno));
    }

    while (true) {
        uint8_t byte;
        int numRead = read(socketFD, &byte, 1);
        if (numRead == 1) {
            if (byte == 0xf2) {
                downloadCoreDump(socketFD);
            } else {
                std::cout << byte;
            }
        } else {
            std::cout << "DebugBridge connection closed" << std::endl;
            return 0;
        }
    }
}
