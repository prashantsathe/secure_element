#define LOG_TAG "Google_SE-service:SecureElement"
#include "SecureElement.h"
#include "Transport.h"

#include <android-base/logging.h>
#include <android-base/stringprintf.h>

namespace android {
namespace hardware {
namespace secure_element {
namespace V1_2 {
namespace implementation {

#define UNUSED(expr) do { (void)(expr); } while (0)

SecureElement::SecureElement() {
    if (mSocketTransport == NULL)
        mSocketTransport = new SocketTransport();
}

Return<void> SecureElement::init(
    const sp<::android::hardware::secure_element::V1_0::ISecureElementHalCallback>& clientCallback) {
    LOG(INFO) << "init: onStateChange";
    clientCallback->onStateChange(true);
    return Void();
}

Return<void> SecureElement::init_1_1(
        const sp<::android::hardware::secure_element::V1_1::ISecureElementHalCallback>&
        clientCallback) {
    LOG(INFO) << "init_1_1: onStateChange_1_1";
    clientCallback->onStateChange_1_1(true, "SE Connected");
    return Void();
}

Return<void> SecureElement::getAtr(getAtr_cb _hidl_cb) {
    hidl_vec<uint8_t> response;
    LOG(INFO) << "Processing ATR.....";
    _hidl_cb(response);
    return Void();
}

Return<bool> SecureElement::isCardPresent() { return true; }

Return<void> SecureElement::transmit(const hidl_vec<uint8_t>& data,
                                     transmit_cb _hidl_cb) {
    LOG(INFO) << "transmit ";
    std::vector<uint8_t> output;
    mSocketTransport->sendData(&data[0], data.size(), output);
    _hidl_cb(output);
    return Void();
}

Return<void> SecureElement::openLogicalChannel(const hidl_vec<uint8_t>& aid, uint8_t p2,
        openLogicalChannel_cb _hidl_cb) {
    UNUSED(aid);
    std::vector<uint8_t> resApduBuff;
    uint8_t manageChannelCommand[] = {0x00, 0x70, 0x00, 0x00, 0x01};
    SecureElementStatus sestatus = SecureElementStatus::IOERROR;
    uint16_t outsize;
    LogicalChannelResponse cbLogicalResponse;
    cbLogicalResponse.channelNumber = 0xff;
    memset(&cbLogicalResponse, 0x00, sizeof(cbLogicalResponse));

    LOG(INFO) << "Start openLogicalChannel";

    if (!mSocketTransport->isConnected()) {
        if (!mSocketTransport->openConnection()) {
            LOG(ERROR) << "error while open Correction";
            _hidl_cb(cbLogicalResponse, SecureElementStatus::IOERROR);
            return Void();
        }
    }

    LOG(INFO) << "Socket is Connected, sending manage channel command";
    // send manage command (optional) but will need in FiRa multi-channel implementation
    mSocketTransport->sendData(manageChannelCommand, 5, resApduBuff);
    outsize = resApduBuff.size();
    if (!(resApduBuff[outsize - 2] == 0x90 && resApduBuff[outsize - 1] == 0x00)) {
        _hidl_cb(cbLogicalResponse, SecureElementStatus::IOERROR); // TODO: return should be as per Status
        return Void();
    }

    std::vector<uint8_t> selectCmd;
    if ((resApduBuff[0] > 0x03) && (resApduBuff[0] < 0x14)) {
        /* update CLA byte according to GP spec Table 11-12*/
        selectCmd.push_back(0x40 + (resApduBuff[0] - 4)); /* Class of instruction */
    } else if ((resApduBuff[0] > 0x00) && (resApduBuff[0] < 0x04)) {
        /* update CLA byte according to GP spec Table 11-11*/
        selectCmd.push_back((uint8_t) resApduBuff[0]); /* Class of instruction */
    } else {
        LOG(ERROR) << "Invalid Channel " << resApduBuff[0];
        resApduBuff[0] = 0xff;
        _hidl_cb(cbLogicalResponse, SecureElementStatus::IOERROR);
        return Void();
    }
    cbLogicalResponse.channelNumber = resApduBuff[0];
    LOG(INFO) << "manage channel command is done";

    // send select command
    LOG(INFO) << "sending Select command";
    selectCmd.push_back((uint8_t) 0xA4); /* Instruction code */
    selectCmd.push_back((uint8_t) 0x04);  /* Instruction parameter 1 */
    selectCmd.push_back(p2);    /* Instruction parameter 2 */
    selectCmd.push_back((uint8_t) aid.size()); // should be fine as AID is always less than 128
    selectCmd.insert(selectCmd.end(), aid.begin(), aid.end());
    selectCmd.push_back((uint8_t) 256);

    resApduBuff.clear();
    mSocketTransport->sendData(&selectCmd[0], selectCmd.size(), resApduBuff);
    outsize = resApduBuff.size();

    if (!((resApduBuff[outsize - 2] == 0x90 && resApduBuff[outsize - 1] == 0x00) ||
               (resApduBuff[outsize - 2] == 0x62) || (resApduBuff[outsize - 2] == 0x63))) {
        // sendData response failed
        if (outsize > 0 && (resApduBuff[outsize - 2] == 0x64 &&
                resApduBuff[outsize - 1] == 0xFF)) {
            sestatus = SecureElementStatus::IOERROR;
        } else {
            sestatus = SecureElementStatus::FAILED;
        }
        // Got an error, Close logical channel
        closeChannel(cbLogicalResponse.channelNumber);
    } else {
        LOG(INFO) << "Select command Success";
        sestatus = SecureElementStatus::SUCCESS;
    }

    cbLogicalResponse.selectResponse.resize(outsize);
    memcpy(&cbLogicalResponse.selectResponse[0], &resApduBuff[0], outsize);
    _hidl_cb(cbLogicalResponse, sestatus);
    return Void();
}

Return<void> SecureElement::openBasicChannel(const hidl_vec<uint8_t>& aid, uint8_t p2,
        openBasicChannel_cb _hidl_cb) {
    UNUSED(aid); UNUSED(p2); UNUSED(_hidl_cb);
    LOG(ERROR) << "openBasicChannel";
    // openLogicalChannel(aid, p2, _hidl_cb);
    return Void();
}

Return<SecureElementStatus> SecureElement::internalCloseChannel(uint8_t channelNumber) {
    UNUSED(channelNumber);
    LOG(INFO) << "internalCloseChannel";
    uint8_t manageChannelCommand[] = {0x00, 0x70, 0x80, 0x00, 0x00};
    std::vector<uint8_t> resApduBuff;

    // change class of instruction & p2 parameter
    manageChannelCommand[0] = channelNumber;
    // For Supplementary Channel update CLA byte according to GP
    if ((channelNumber > 0x03) && (channelNumber < 0x14)) {
        /* update CLA byte according to GP spec Table 11-12*/
        manageChannelCommand[0] = 0x40 + (channelNumber - 4);
    }
    manageChannelCommand[3] = channelNumber;  /* Instruction parameter 2 */

    mSocketTransport->sendData(manageChannelCommand, 5, resApduBuff);
    if (!(resApduBuff[0] == 0x90 && resApduBuff[1] == 0x00)) {
        LOG(ERROR) << "internalCloseChannel failed";
        return SecureElementStatus::FAILED;
    }

    return SecureElementStatus::SUCCESS;
}

Return<SecureElementStatus> SecureElement::closeChannel(uint8_t channelNumber) {
    UNUSED(channelNumber);
    LOG(INFO) << "CloseChannel";
    internalCloseChannel(channelNumber);
    return SecureElementStatus::SUCCESS;
}

void SecureElement::serviceDied(uint64_t /*cookie*/, const wp<IBase>& /*who*/) {
    LOG(INFO) << " SecureElement serviceDied!!!";
    if (mSocketTransport->isConnected()) {
        mSocketTransport->closeConnection();
    }
}

void SecureElement::seHalInit() {
    LOG(INFO) << "se Hal Init";
}

Return<::android::hardware::secure_element::V1_0::SecureElementStatus>
        SecureElement::reset() {
    ::android::hardware::secure_element::V1_0::SecureElementStatus status =
     ::android::hardware::secure_element::V1_0::SecureElementStatus::SUCCESS;
    LOG(ERROR) << "%s: Exit" << __func__;
    return status;
}

}  // namespace implementation
}  // namespace V1_2
}  // namespace secure_element
}  // namespace hardware
}  // namespace android
