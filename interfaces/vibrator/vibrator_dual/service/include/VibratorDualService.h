/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef VIBRATOR_DUAL_SERVICE_H
#define VIBRATOR_DUAL_SERVICE_H

#include <android/hardware/vibrator/1.0/types.h>
#include <vendor/qti/hardware/vibrator_dual/device/1.0/IVibratorDual.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <utils/Log.h>
#include <string>

namespace vendor {
namespace qti {
namespace hardware {
namespace vibrator_dual {
namespace device {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::sp;
using ::vendor::qti::hardware::vibrator_dual::device::V1_0::IVibratorDual;
using ::vendor::qti::hardware::vibrator_dual::device::V1_0::Status;

class VibratorDualService : public IVibratorDual {
public:
    VibratorDualService();
    virtual ~VibratorDualService();

    static IVibratorDual* getInstance();

    // HIDL method implementations
    Return<Status> onDual(uint32_t t, uint32_t vibType, uint32_t innerId,
                          uint32_t innerIdSub, uint32_t timeoutMs, uint32_t timeoutMsSub) override;

    Return<Status> Dual_Cancel(uint32_t vibType) override;

private:
    static VibratorDualService* sInstance;

    // Internal control methods
    bool device_exists(const char* file);
    int write_value(const char* file, const char* value);

    int left_on(uint32_t innerId, uint32_t timeoutMs, int amplitude);
    int right_on(uint32_t innerId, uint32_t timeoutMs, int amplitude);

    int left_off();
    int right_off();
};

} // namespace implementation
} // namespace V1_0
} // namespace device
} // namespace vibrator_dual
} // namespace hardware
} // namespace qti
} // namespace vendor

#endif // VIBRATOR_DUAL_SERVICE_H
