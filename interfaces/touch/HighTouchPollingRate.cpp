/*
 * SPDX-FileCopyrightText: 2025 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "vendor.lineage.touch-service.lenovo"

#include "HighTouchPollingRate.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/strings.h>

using ::android::base::ReadFileToString;
using ::android::base::Trim;
using ::android::base::WriteStringToFile;

namespace aidl {
namespace vendor {
namespace lineage {
namespace touch {

static const std::string GAME_MODE_PATH = "/sys/devices/virtual/touch/tp_dev/game_mode";

ndk::ScopedAStatus HighTouchPollingRate::getEnabled(bool* _aidl_return) {
    std::string buf;

    if (!ReadFileToString(GAME_MODE_PATH, &buf)) {
        LOG(ERROR) << "Failed to read from " << GAME_MODE_PATH;
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    *_aidl_return = (Trim(buf) == "1");
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus HighTouchPollingRate::setEnabled(bool enabled) {
    const std::string value = enabled ? "1" : "0";

    if (!WriteStringToFile(value, GAME_MODE_PATH)) {
        LOG(ERROR) << "Failed to write value " << value << " to " << GAME_MODE_PATH;
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    LOG(INFO) << "HighTouchPollingRate set to " << value;
    return ndk::ScopedAStatus::ok();
}

}  // namespace touch
}  // namespace lineage
}  // namespace vendor
}  // namespace aidl
