# Secure Element HAL Integration

## Introduction
SE HAL is a dummy service which interacts with JCserver(Socket based java application) to forward APDU from Omapi service to SE applets. This document is based on testing VTS test cases which interact with vendor Hal service (strongBox/FiRa) to SE applet (keymaster/FiRa) through Omapi and SE HAL services.


## Integration Steps (Keymaster Applet testing)
* First follow building an AOSP and strongbox/ Omapi integration steps mentioned in the [External] xTS Setup Guide for Keymint100
* Checkout SE hal source code present at  SE HAL in hardware/google location
Add following code in device/google/cuttlefish/shared/config/device.mk
```
PRODUCT_PACKAGES += \
    android.hardware.secure_element@1.2-service.google \
```
* In device/google/cuttlefish/shared/config/manifest.xml
```
    <hal format="hidl">
        <name>android.hardware.secure_element</name>
        <transport>hwbinder</transport>
        <version>1.2</version>
        <interface>
            <name>ISecureElement</name>
            <instance>eSE1</instance>
        </interface>
    </hal>
```
* In device/google/cuttlefish/shared/sepolicy/vendor/file_contexts
```
/vendor/bin/hw/android\.hardware\.secure_element@1\.2-service\.google  u:object_r:hal_secure_element_default_exec:s0
```
* In device/google/cuttlefish/shared/BoardConfig.mk
```
BOARD_KERNEL_CMDLINE += androidboot.selinux=permissive
```

* Add new file at device/google/cuttlefish/shared/sepolicy/vendor/hal_secure_element_google.te with following context
```
type hal_secure_element_google, domain;
hal_server_domain(hal_secure_element_google, hal_secure_element)

type hal_secure_element_google_exec, exec_type, vendor_file_type, file_type;
init_daemon_domain(hal_secure_element_google)

vndbinder_use(hal_secure_element_google)
get_prop(hal_secure_element_google, vendor_security_patch_level_prop);

allow hal_secure_element_google secure_element_service:service_manager find;

# Allow access to sockets
allow hal_secure_element_google self:tcp_socket { connect create write read getattr getopt setopt };
allow hal_secure_element_google port_type:tcp_socket name_connect;
allow hal_secure_element_google port:tcp_socket { name_connect };
allow hal_secure_element_google vendor_data_file:file { open read getattr };
```

* In system/sepolicy/public/hal_neverallows.te
NOTE in self:global_capability_class_set { net_admin net_raw }; and }:tcp_socket *;
```
-hal_keymint_server
```

* Build the AOSP and verify that all mentioned Binaries are present 
  - android.hardware.secure_element@1.2-service.google
  - Android.hardware.security.keymint-service.strongbox
  - SecureElement.apk

* Build and Run the JCserver code present at JCserver (JcardSim) and provisioned the Keymaster applet using Provisioning Tool
Launch the emulator by “$ launch_cvd --start_webrtc=true” command



## Execute VTS test

Follow section How to Execute VTS of StrongBox xTS Setup Guide