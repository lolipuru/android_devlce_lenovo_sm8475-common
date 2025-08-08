#
# Copyright (C) 2023 The Android Open Source Project
#
# SPDX-License-Identifier: Apache-2.0
#

COMMON_PATH := device/lenovo/sm8475-common

BUILD_BROKEN_DUP_RULES := true
BUILD_BROKEN_ELF_PREBUILT_PRODUCT_COPY_FILES := true

# A/B
AB_OTA_UPDATER := true

AB_OTA_PARTITIONS += \
    boot \
    dtbo \
    odm \
    product \
    recovery \
    system \
    system_ext \
    vbmeta \
    vbmeta_system \
    vendor \
    vendor_boot \
    vendor_dlkm

# Architecture
TARGET_ARCH := arm64
TARGET_ARCH_VARIANT := armv8-a-branchprot
TARGET_CPU_ABI := arm64-v8a
TARGET_CPU_ABI2 :=
TARGET_CPU_VARIANT := generic
TARGET_CPU_VARIANT_RUNTIME := kryo300

TARGET_2ND_ARCH := arm
TARGET_2ND_ARCH_VARIANT := armv8-2a
TARGET_2ND_CPU_ABI := armeabi-v7a
TARGET_2ND_CPU_ABI2 := armeabi
TARGET_2ND_CPU_VARIANT := generic
TARGET_2ND_CPU_VARIANT_RUNTIME := cortex-a75

# Audio
AUDIO_FEATURE_ENABLED_DLKM := true
AUDIO_FEATURE_ENABLED_DTS_EAGLE := false
AUDIO_FEATURE_ENABLED_GEF_SUPPORT := true
AUDIO_FEATURE_ENABLED_HW_ACCELERATED_EFFECTS := false
AUDIO_FEATURE_ENABLED_INSTANCE_ID := true
AUDIO_FEATURE_ENABLED_PAL_HIDL := true
AUDIO_FEATURE_ENABLED_PROXY_DEVICE := true
BOARD_SUPPORTS_OPENSOURCE_STHAL := true
TARGET_USES_QCOM_MM_AUDIO := true

# Boot
BOARD_BOOT_HEADER_VERSION := 4
BOARD_MKBOOTIMG_ARGS += --header_version $(BOARD_BOOT_HEADER_VERSION)

# Bootloader
TARGET_BOOTLOADER_BOARD_NAME := taro
TARGET_NO_BOOTLOADER := true

# Filesystem
TARGET_FS_CONFIG_GEN := $(COMMON_PATH)/configs/config.fs

# Kernel
BOARD_KERNEL_PAGESIZE   := 4096
BOARD_KERNEL_BASE       := 0x00000000
BOARD_KERNEL_IMAGE_NAME := Image

BOARD_BOOTCONFIG := \
    androidboot.hardware=qcom \
    androidboot.memcg=1 \
    androidboot.usbcontroller=a600000.dwc3

BOARD_KERNEL_CMDLINE := \
    video=vfb:640x400,bpp=32,memsize=3072000

BOARD_KERNEL_IMAGE_NAME := Image
BOARD_INCLUDE_DTB_IN_BOOTIMG := true
BOARD_RAMDISK_USE_LZ4 := true
BOARD_USES_GENERIC_KERNEL_IMAGE := true

# Metadata
BOARD_USES_METADATA_PARTITION := true

# Partitions
-include vendor/lineage/config/BoardConfigReservedSize.mk

BOARD_BOOTIMAGE_PARTITION_SIZE := 100663296
BOARD_DTBOIMG_PARTITION_SIZE := 24117248
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 104857600
BOARD_VENDOR_BOOTIMAGE_PARTITION_SIZE := 100663296
BOARD_FLASH_BLOCK_SIZE := 262144

ifeq ($(TARGET_IS_ASPHALT),true)
BOARD_SUPER_PARTITION_SIZE := 6441926656
BOARD_QTI_DYNAMIC_PARTITIONS_SIZE := 6437732352
else
BOARD_SUPER_PARTITION_SIZE := 10737418240
BOARD_QTI_DYNAMIC_PARTITIONS_SIZE := 10733223936
endif
BOARD_SUPER_PARTITION_GROUPS := qti_dynamic_partitions
BOARD_QTI_DYNAMIC_PARTITIONS_PARTITION_LIST := odm product system system_ext vendor vendor_dlkm

BOARD_PARTITION_LIST := $(call to-upper, $(BOARD_QTI_DYNAMIC_PARTITIONS_PARTITION_LIST))
$(foreach p, $(BOARD_PARTITION_LIST), $(eval BOARD_$(p)IMAGE_FILE_SYSTEM_TYPE := ext4))
$(foreach p, $(BOARD_PARTITION_LIST), $(eval TARGET_COPY_OUT_$(p) := $(call to-lower, $(p))))

# Platform
BOARD_USES_QCOM_HARDWARE := true
TARGET_BOARD_PLATFORM := taro

# Properties
TARGET_ODM_PROP += $(COMMON_PATH)/configs/properties/odm.prop
TARGET_PRODUCT_PROP += $(COMMON_PATH)/configs/properties/product.prop
TARGET_SYSTEM_PROP += $(COMMON_PATH)/configs/properties/system.prop
TARGET_SYSTEM_EXT_PROP += $(COMMON_PATH)/configs/properties/system_ext.prop
TARGET_VENDOR_PROP += $(COMMON_PATH)/configs/properties/vendor.prop

# Recovery
BOARD_EXCLUDE_KERNEL_FROM_RECOVERY_IMAGE := true
TARGET_RECOVERY_PIXEL_FORMAT := RGBX_8888
ifeq ($(TARGET_IS_ASPHALT),true)
TARGET_RECOVERY_FSTAB := $(COMMON_PATH)/init/etc/fstab-cust.qcom
else
TARGET_RECOVERY_FSTAB := $(COMMON_PATH)/init/etc/fstab.qcom
endif
TARGET_USERIMAGES_USE_EXT4 := true
TARGET_USERIMAGES_USE_F2FS := true

# RIL
ENABLE_VENDOR_RIL_SERVICE := true

# Rootdir
ifeq ($(TARGET_IS_ASPHALT),true)
$(call soong_config_set,LENOVO_DEVICE,LENOVO_CUST_SUPPORT,true)
else
$(call soong_config_set,LENOVO_DEVICE,LENOVO_CUST_SUPPORT,false)
endif

# Security
BOOT_SECURITY_PATCH := 2025-02-05
VENDOR_SECURITY_PATCH := $(BOOT_SECURITY_PATCH)

# Sepolicy
include device/qcom/sepolicy_vndr/SEPolicy.mk
SYSTEM_EXT_PRIVATE_SEPOLICY_DIRS += $(COMMON_PATH)/sepolicy/private
SYSTEM_EXT_PUBLIC_SEPOLICY_DIRS += $(COMMON_PATH)/sepolicy/public
BOARD_VENDOR_SEPOLICY_DIRS += $(COMMON_PATH)/sepolicy/vendor

# Verified Boot
BOARD_AVB_ENABLE := true
BOARD_AVB_MAKE_VBMETA_IMAGE_ARGS += --flags 3
BOARD_AVB_RECOVERY_KEY_PATH := external/avb/test/data/testkey_rsa2048.pem
BOARD_AVB_RECOVERY_ALGORITHM := SHA256_RSA2048

BOARD_AVB_RECOVERY_ROLLBACK_INDEX := $(PLATFORM_SECURITY_PATCH_TIMESTAMP)
BOARD_AVB_RECOVERY_ROLLBACK_INDEX_LOCATION := 1

BOARD_AVB_VBMETA_SYSTEM := system system_ext product
BOARD_AVB_VBMETA_SYSTEM_KEY_PATH := external/avb/test/data/testkey_rsa2048.pem
BOARD_AVB_VBMETA_SYSTEM_ALGORITHM := SHA256_RSA2048
BOARD_AVB_VBMETA_SYSTEM_ROLLBACK_INDEX := $(PLATFORM_SECURITY_PATCH_TIMESTAMP)
BOARD_AVB_VBMETA_SYSTEM_ROLLBACK_INDEX_LOCATION := 2

BOARD_MOVE_GSI_AVB_KEYS_TO_VENDOR_BOOT := true

# VINTF
DEVICE_FRAMEWORK_COMPATIBILITY_MATRIX_FILE := \
    $(COMMON_PATH)/configs/vintf/compatibility_matrix.lenovo.xml \
    hardware/qcom-caf/common/vendor_framework_compatibility_matrix.xml \
    vendor/lineage/config/device_framework_matrix.xml

DEVICE_MATRIX_FILE := \
    hardware/qcom-caf/common/compatibility_matrix.xml

DEVICE_MANIFEST_SKUS := cape
DEVICE_MANIFEST_CAPE_FILES := \
    $(COMMON_PATH)/configs/vintf/manifest_lenovo.xml \
    $(COMMON_PATH)/configs/vintf/manifest_cape.xml

ifneq ($(TARGET_IS_ASPHALT),true)
DEVICE_MANIFEST_CAPE_FILES += \
    $(COMMON_PATH)/configs/vintf/manifest_cape_ds.xml
endif

# WiFi
BOARD_WLAN_DEVICE := qcwcn
BOARD_HOSTAPD_DRIVER := NL80211
BOARD_HOSTAPD_PRIVATE_LIB := lib_driver_cmd_$(BOARD_WLAN_DEVICE)
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_$(BOARD_WLAN_DEVICE)
CONFIG_IEEE80211AX := true
WIFI_DRIVER_DEFAULT := qca_cld3
WIFI_DRIVER_STATE_CTRL_PARAM := "/dev/wlan"
WIFI_DRIVER_STATE_OFF := "OFF"
WIFI_DRIVER_STATE_ON := "ON"
WIFI_HIDL_FEATURE_DUAL_INTERFACE := true
WIFI_HIDL_UNIFIED_SUPPLICANT_SERVICE_RC_ENTRY := true
WPA_SUPPLICANT_VERSION := VER_0_8_X

# Inherit from the proprietary version
-include vendor/lenovo/sm8475-common/BoardConfigVendor.mk