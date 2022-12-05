#define LOG_TAG "Google_SE-service"
#include <android/hardware/secure_element/1.2/ISecureElement.h>
#include <hidl/LegacySupport.h>
#include "SecureElement.h"
#include <android-base/logging.h>

using android::OK;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
// Generated HIDL files
using android::hardware::secure_element::V1_2::ISecureElement;
using android::hardware::secure_element::V1_2::implementation::SecureElement;

using android::status_t;

int main() {
    status_t status;
    char terminalID[5] = "eSE1";
    android::sp<ISecureElement> se_service = nullptr;

    try {
        LOG(INFO) << "Secure Element HAL Service is starting.\n";
        se_service = new SecureElement();

        if (se_service == nullptr) {
            LOG(ERROR) << "Can not create an instance of Secure Element HAL exiting.";
            goto shutdown;
        }
        configureRpcThreadpool(1, true /*callerWillJoin*/);
        status = se_service->registerAsService(terminalID);
        if (status != OK) {
            LOG(ERROR) << "Could not register service for Secure Element HAL \
                    status = (%d)." << status;
            goto shutdown;
        }
        LOG(INFO) << "Secure Element Service is ready";
        joinRpcThreadpool();
    } catch (const std::__1::ios_base::failure& e) {
        LOG(ERROR) << "ios failure Exception occurred = %s " << e.what();
    }

shutdown:
    // In normal operation, we don't expect the thread pool to exit
    LOG(ERROR) << "Secure Element Service is shutting down";
    return 1;
}
