#ifndef ANDROID_HARDWARE_SECURE_ELEMENT_V1_2_SECUREELEMENT_H
#define ANDROID_HARDWARE_SECURE_ELEMENT_V1_2_SECUREELEMENT_H

//#include <SyncEvent.h>
#include <android-base/stringprintf.h>
#include <android/hardware/secure_element/1.0/types.h>
#include <android/hardware/secure_element/1.2/ISecureElement.h>
#include <hardware/hardware.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <pthread.h>
#include "Transport.h"

namespace android {
namespace hardware {
namespace secure_element {
namespace V1_2 {
namespace implementation {

using ::android::sp;
using android::base::StringPrintf;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::secure_element::V1_0::LogicalChannelResponse;
using ::android::hardware::secure_element::V1_0::SecureElementStatus;
using ::android::hardware::secure_element::V1_2::ISecureElement;
using ::android::hidl::base::V1_0::IBase;

#ifndef MIN_APDU_LENGTH
#define MIN_APDU_LENGTH 0x04
#endif
#ifndef DEFAULT_BASIC_CHANNEL
#define DEFAULT_BASIC_CHANNEL 0x00
#endif

struct SecureElement : public V1_2::ISecureElement,
                       public hidl_death_recipient {
  SecureElement();
  Return<void> init(
      const sp<
          ::android::hardware::secure_element::V1_0::ISecureElementHalCallback>&
          clientCallback) override;
  Return<void> init_1_1(
      const sp<
          ::android::hardware::secure_element::V1_1::ISecureElementHalCallback>&
          clientCallback) override;
  Return<void> getAtr(getAtr_cb _hidl_cb) override;
  Return<bool> isCardPresent() override;
  Return<void> transmit(const hidl_vec<uint8_t>& data,
                        transmit_cb _hidl_cb) override;
  Return<void> openLogicalChannel(const hidl_vec<uint8_t>& aid, uint8_t p2,
                                  openLogicalChannel_cb _hidl_cb) override;
  Return<void> openBasicChannel(const hidl_vec<uint8_t>& aid, uint8_t p2,
                                openBasicChannel_cb _hidl_cb) override;
  Return<SecureElementStatus> closeChannel(uint8_t channelNumber) override;

  void serviceDied(uint64_t /*cookie*/, const wp<IBase>& /*who*/);

  Return<::android::hardware::secure_element::V1_0::SecureElementStatus>
  reset();
  void seHalInit();

 private:
  static std::vector<bool> mOpenedChannels;
  static sp<V1_0::ISecureElementHalCallback> mCallbackV1_0;
  static sp<V1_1::ISecureElementHalCallback> mCallbackV1_1;
  SocketTransport *mSocketTransport;
  Return<SecureElementStatus> internalCloseChannel(uint8_t channelNumber);
};

}  // namespace implementation
}  // namespace V1_2
}  // namespace secure_element
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_SECURE_ELEMENT_V1_1_SECUREELEMENT_H
