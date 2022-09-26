#ifndef __SE_TRANSPORT__
#define __SE_TRANSPORT__

namespace android {
namespace hardware {
namespace secure_element {
namespace V1_2 {
namespace implementation {
/**
 * ITransport is an abstract interface with a set of virtual methods that allow communication
 * between the JCserver and the secure element HAL.
 */
class ITransport {
    public:
    virtual ~ITransport(){}

    /**
     * Opens connection.
     */
	virtual bool openConnection() = 0;
    /**
     * Send data over communication channel and receives data back from the remote end.
     */
    virtual bool sendData(const uint8_t* inData, const size_t inLen, std::vector<uint8_t>& output) = 0;
    /**
     * Closes the connection.
     */
    virtual bool closeConnection() = 0;
    /**
     * Returns the state of the connection status. Returns true if the connection is active, false if connection is
     * broken.
     */
    virtual bool isConnected() = 0;

};


class SocketTransport : public ITransport {

public:
    SocketTransport() : mSocket(-1), socketStatus(false) {
    }
    /**
     * Creates a socket instance and connects to the provided server IP and port.
     */
	bool openConnection() override;
    /**
     * Sends data over socket and receives data back.
     */
    bool sendData(const uint8_t* inData, const size_t inLen, std::vector<uint8_t>& output) override;
    /**
     * Closes the connection.
     */
    bool closeConnection() override;
    /**
     * Returns the state of the connection status. Returns true if the connection is active, false
     * if connection is broken.
     */
    bool isConnected() override;
private:
    /**
     * Socket instance.
     */
    int mSocket;
    bool socketStatus;
    bool readData(std::vector<uint8_t>& output);
};

}  // namespace implementation
}  // namespace V1_2
}  // namespace secure_element
}  // namespace hardware
}  // namespace android
#endif /* __SE_TRANSPORT__ */
