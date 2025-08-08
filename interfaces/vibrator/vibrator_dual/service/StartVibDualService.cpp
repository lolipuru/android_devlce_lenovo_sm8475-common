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

#define LOG_TAG "VibratorDual"

#include <android/log.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <hardware/hardware.h>
#include <hardware/vibrator.h>

//#include "Vibrator.h"
#include "StartVibDualService.h"
#include "VibratorDualService.h"
#include <vendor/qti/hardware/vibrator_dual/device/1.0/IVibratorDual.h>
#include <vendor/qti/hardware/vibrator_dual/device/1.0/types.h>

using vendor::qti::hardware::vibrator_dual::device::V1_0::IVibratorDual;
using vendor::qti::hardware::vibrator_dual::device::V1_0::implementation::VibratorDualService;

using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::sp;

int AddVibratorDualService() {

    android::sp<IVibratorDual> service = VibratorDualService::getInstance();

    configureRpcThreadpool(1, true /*callerWillJoin*/);

    if (service != nullptr) {
        if (::android::OK != service->registerAsService()) {
            return 1;
        }
    } else {
        ALOGE("Can't create instance of GoodixFPExtendService, nullptr");
    }

    //joinRpcThreadpool();

    return 0; // should never get here
}