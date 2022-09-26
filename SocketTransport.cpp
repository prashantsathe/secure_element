#define LOG_TAG "Google_SE-service:SocketTransport"
#include <arpa/inet.h>
#include <errno.h>
#include <log/log.h>
#include <memory>
#include <vector>

#include <android-base/logging.h>
#include <sys/socket.h>

#include "Transport.h"

#define PORT 8080
#define IPADDR  "192.168.1.195"
#define MAX_RECV_BUFFER_SIZE 2500

namespace android {
namespace hardware {
namespace secure_element {
namespace V1_2 {
namespace implementation {

using std::shared_ptr;
using std::vector;

bool SocketTransport::openConnection() {
    struct sockaddr_in serv_addr;
    if ((mSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        LOG(ERROR) << "Socket creation failed" << " Error: " << strerror(errno);
        return false;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, IPADDR, &serv_addr.sin_addr) <= 0) {
        LOG(ERROR) << "Invalid address/ Address not supported.";
        return false;
    }

    if (connect(mSocket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(mSocket);
        LOG(ERROR) << "Connection failed. Error: " << strerror(errno);
        return false;
    }

    LOG(INFO) << "openConnection done";
    socketStatus = true;
    return true;
}

bool SocketTransport::sendData(const uint8_t* inData, const size_t inLen, std::vector<uint8_t>& output) {
    int count = 1;
    while (!socketStatus && count++ < 5) {
        sleep(1);
        LOG(ERROR) << "Trying to open socket connection... count: " << count;
        openConnection();
    }

    if (count >= 5) {
        LOG(ERROR) << "Failed to open socket connection";
        return false;
    }
    // Prepend the input length to the inputData before sending.
    vector<uint8_t> inDataPrependedLength;
    inDataPrependedLength.push_back(inLen >> 8);
    inDataPrependedLength.push_back(inLen & 0xFF);
    inDataPrependedLength.insert(inDataPrependedLength.end(), inData, inData + inLen);

    LOG(INFO) << "Send Data size = " << inDataPrependedLength.size();
    if (0 > send(mSocket, inDataPrependedLength.data(), inDataPrependedLength.size(), 0)) {
        static int connectionResetCnt = 0; /* To avoid loop */
        if (ECONNRESET == errno && connectionResetCnt == 0) {
            // Connection reset. Try open socket and then sendData.
            socketStatus = false;
            connectionResetCnt++;
            return sendData(inData, inLen, output);
        }
        LOG(ERROR) << "Failed to send data over socket err: " << errno;
        connectionResetCnt = 0;
        return false;
    }

    if (!readData(output)) {
        LOG(ERROR) << "Read Data error";
        return false;
    }
    LOG(INFO) << "Read Data size = " << output.size();
    return true;
}

bool SocketTransport::closeConnection() {
    close(mSocket);
    socketStatus = false;
    return true;
}

bool SocketTransport::isConnected() {
    return socketStatus;
}

bool SocketTransport::readData(std::vector<uint8_t>& output) {
    uint8_t buffer[MAX_RECV_BUFFER_SIZE];
    ssize_t expectedResponseLen = 0;
    ssize_t totalBytesRead = 0;

    // The first 2 bytes in the response contains the expected response length.
    do {
        size_t i = 0;
        ssize_t numBytes = read(mSocket, buffer, MAX_RECV_BUFFER_SIZE);

        if (0 > numBytes) {
            LOG(ERROR) << "Failed to read data from socket.";
            return false;
        }
        totalBytesRead += numBytes;
        if (expectedResponseLen == 0) {
            // First two bytes in the response contains the expected response length.
            expectedResponseLen |=  static_cast<ssize_t>(buffer[1] & 0xFF);
            expectedResponseLen |=  static_cast<ssize_t>((buffer[0] << 8) & 0xFF00);
            // 2 bytes for storing the length.
            expectedResponseLen += 2;
            i = 2;
        }

        for (; i < numBytes; i++) {
            output.push_back(buffer[i]);
        }
    } while(totalBytesRead < expectedResponseLen);

    return true;
}

}  // namespace implementation
}  // namespace V1_2
}  // namespace secure_element
}  // namespace hardware
}  // namespace android
