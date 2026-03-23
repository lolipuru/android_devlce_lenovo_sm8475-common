/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include "RichtapVibrator.h"

#include <android-base/logging.h>
#include <string.h>
#include <inttypes.h>
#include <log/log.h>
#include <thread>

namespace aidl::vendor::aac::hardware::richtap::vibrator {
using aidl::vendor::aac::hardware::richtap::vibrator::RichtapVibrator;


void RichtapVibrator::send_handle_result(const std::shared_ptr<IRichtapCallback>& callback,
                                                               int32_t timeout,int32_t result){
    if (callback != nullptr) {
        std::thread([=] {
            usleep(timeout * 1000);
            callback->onCallback(result);
        }).detach();
    }
}

ndk::ScopedAStatus RichtapVibrator::init(const std::shared_ptr<IRichtapCallback>& callback) {
    int32_t deviceType = 0;
    if(m_richtap_support == false){//can not init serveral times
        int32_t ret = aac_vibra_init((uint32_t*)&deviceType);
        ALOGD("aac richtap vibrator service init!");
        if (ret != 0) {
            ALOGE("aac richtap init command failed : %s", strerror(-ret));
            m_richtap_support = false;
            send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_FAILED);
            return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
        }
        aac_vibra_looper_start();
        m_richtap_support = true;
        send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_SUCCESS);
    }else{
        send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_SUCCESS);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RichtapVibrator::off(const std::shared_ptr<IRichtapCallback>& callback) {
    if(m_richtap_support){
        int32_t ret = aac_vibra_off();
        if (ret != 0) {
            ALOGE("aac richtap off command failed : %s", strerror(-ret));
            send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_FAILED);
            return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_STATE));
        }
        send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_SUCCESS);
        return ndk::ScopedAStatus::ok();
    }else{
        send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_NOT_SUPPORT);
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }
}

ndk::ScopedAStatus RichtapVibrator::on(int32_t timeoutMs, const std::shared_ptr<IRichtapCallback>& callback) {
    if(m_richtap_support){
        int32_t timeout_ms = aac_vibra_looper_on(timeoutMs);
        if (timeout_ms < 0) {
            ALOGE("aac richtap on command failed");
            send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_FAILED);
            return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_STATE));
        }
        send_handle_result(callback,timeout_ms,RICHTAP_HANDLE_SUCCESS);
        return ndk::ScopedAStatus::ok();
    }else{
        ALOGE("aac richtap not support");
        send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_NOT_SUPPORT);
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }
   
}

ndk::ScopedAStatus RichtapVibrator::setAmplitude(int32_t amplitude,const std::shared_ptr<IRichtapCallback>& callback) {
    if(m_richtap_support){
        uint8_t tmp = (uint8_t)(amplitude);
        int32_t ret =aac_vibra_setAmplitude(tmp);
        if(ret != 0){
            ALOGE("aac setAmplitude command failed : %s", strerror(-ret));
            send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_FAILED);
            return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_STATE));
        }
        send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_SUCCESS);
        return ndk::ScopedAStatus::ok();
    }else{
        ALOGE("aac richtap not support");
        send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_NOT_SUPPORT);
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }
    
}


ndk::ScopedAStatus RichtapVibrator::perform(int32_t effect, int8_t es, const std::shared_ptr<IRichtapCallback>& callback,int32_t* _aidl_return) {
    if(m_richtap_support){
        long playLengthMs = 0;
        ALOGD("richtap Vibrator perform effect %d", effect);
        playLengthMs  = aac_vibra_looper_prebaked_effect(static_cast<uint32_t>(effect),static_cast<uint32_t>(es));
        if (playLengthMs < 0){
            ALOGE("Failed to perform richtap effect id");
            send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_FAILED);
            return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_SERVICE_SPECIFIC));
        }
        *_aidl_return = playLengthMs;
        send_handle_result(callback,playLengthMs,RICHTAP_HANDLE_SUCCESS);
        return ndk::ScopedAStatus::ok();
    }else{
        ALOGE("aac richtap not support");
        send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_NOT_SUPPORT);
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }
}

ndk::ScopedAStatus RichtapVibrator::performEnvelope( const std::vector<int32_t>& envInfo,  bool fastFlag,const std::shared_ptr<IRichtapCallback>& callback){
    if(m_richtap_support){
        if(envInfo.size() == 0){
            ALOGE("perform envInfo data is null");
            send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_FAILED);
            return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_STATE));
        }
        const int32_t *gcache_buf = envInfo.data();
        uint32_t data_len = envInfo.size();
        int32_t time_out = aac_vibra_looper_envelope(gcache_buf,data_len,fastFlag);
        ALOGD("data_len: %d,fastFlag: %d", data_len,fastFlag);
        if(time_out < 0)
        {
            ALOGE("perform command failed");
            send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_FAILED);
            return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_STATE));
        }
        send_handle_result(callback,time_out,RICHTAP_HANDLE_SUCCESS);
        return ndk::ScopedAStatus::ok();
    }else{
        ALOGE("aac richtap not support");
        send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_NOT_SUPPORT);
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }
   
}
ndk::ScopedAStatus RichtapVibrator::performRtp(const ndk::ScopedFileDescriptor& pfd, const std::shared_ptr<IRichtapCallback>& callback)
{
    if(m_richtap_support){
        if (pfd.get() < 0) {
            ALOGE("no pfd");
            send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_FAILED);
            return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_STATE));
        }
        int fd = fcntl(pfd.get(), F_DUPFD_CLOEXEC, 0);
        if (fd < 0) {
            ALOGE("invalid FD: %d\n", pfd.get());
            send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_FAILED);
            return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_STATE));
        }
        int32_t timeout_ms = aac_vibra_looper_rtp(fd);
        if(timeout_ms <= 0)
        {
            ALOGE("performRtp command failed ");
            send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_FAILED);
            return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_STATE));
        }
        send_handle_result(callback,timeout_ms,RICHTAP_HANDLE_SUCCESS);
        return ndk::ScopedAStatus::ok();
    }else{
        ALOGE("aac richtap not support");
        send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_NOT_SUPPORT);
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }
}
ndk::ScopedAStatus RichtapVibrator::performHe(int32_t looper,int32_t interval,int32_t amplitude,int32_t freq,
                      const std::vector<int32_t>& he,const std::shared_ptr<IRichtapCallback>& callback)
{
    if(m_richtap_support){
        if(he.size() == 0){
            ALOGE("performHe data is empty");
            send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_FAILED);
            return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_STATE));
        }
        ALOGD("looper:%d,interval:%d,amplitude:%d,freq:%d", looper,interval,amplitude,freq);
        const int32_t *he_data = he.data();
        int32_t data_len = static_cast<int32_t>(he.size());
        int32_t timeout_ms = aac_vibra_looper_post((const int32_t *)he_data, data_len,interval,looper,amplitude,freq);
        if(timeout_ms < 0){
            ALOGE("invalid perform he failed");
            send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_FAILED);
            return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_STATE));
        }
        send_handle_result(callback,timeout_ms,RICHTAP_HANDLE_SUCCESS);
        return ndk::ScopedAStatus::ok();
    }else{
        ALOGE("aac richtap not support");
        send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_NOT_SUPPORT);
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }
}

ndk::ScopedAStatus RichtapVibrator::performHeParam(int32_t interval,int32_t amplitude,int32_t freq,const std::shared_ptr<IRichtapCallback>& callback)
{
    if(m_richtap_support){
        ALOGD("update he param interval:%d,amplitude:%d,freq:%d", interval,amplitude,freq);
        if(interval == 0 && amplitude == 0 && freq == 0){
            bool ret = aac_vibra_looper_stopPerformHe();
            if(!ret){
                send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_FAILED);
                return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_STATE));
            }else{
                send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_SUCCESS);
                return ndk::ScopedAStatus::ok();
            }
        }else{
            bool ret = aac_vibra_looper_performParam(interval,amplitude,freq);
            if(!ret){
                send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_FAILED);
                return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_STATE));
            }else{
                send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_SUCCESS);
                return ndk::ScopedAStatus::ok();
            }
        }
        return ndk::ScopedAStatus::ok();
    }else{
        ALOGE("aac richtap not support");
        send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_NOT_SUPPORT);
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }
    
}

ndk::ScopedAStatus RichtapVibrator::setHapticParam(const std::vector<int32_t>& data, const int32_t length, const std::shared_ptr<IRichtapCallback>& callback)
{
    if (length == 2 && data[0] == HAPTIC_PARAM_DRC_MARK) {
        if (data[1] >= 0 && data[1] <= HAPTIC_PARAM_MAX_DRC) {
            aac_vibra_set_drc_mode(data[1]);
        }
        ALOGE("setHapticParam data[0]:%d data[1]:%d", data[0], data[1]);
    }
    else {
        ALOGI("parameters with size: %lu!", data.size());
        const int32_t * buf = data.data();
        aac_vibra_update_parameter(buf, length);
    }
    send_handle_result(callback, DEFAULT_RETURN_TIME_OUT, RICHTAP_HANDLE_SUCCESS);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RichtapVibrator::setDynamicScale(int32_t scale,const std::shared_ptr<IRichtapCallback>& callback){
    if(m_richtap_support){
        int32_t ret = aac_vibra_dynamic_scale(scale);
        if(ret != 0){
            ALOGE("setDynamicScale command failed : %s", strerror(-ret));
            send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_FAILED);
            return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_STATE));
        }
        send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_SUCCESS);
        return ndk::ScopedAStatus::ok();
    }else{
        ALOGE("aac richtap not support");
        send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_NOT_SUPPORT);
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }
}

ndk::ScopedAStatus RichtapVibrator::setF0(int32_t f0,const std::shared_ptr<IRichtapCallback>& callback){
    if(m_richtap_support){
        int32_t ret = aac_vibra_setting_f0(f0);
        if(ret != 0){
            ALOGE("set F0 command failed : %s", strerror(-ret));
            send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_FAILED);
            return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_STATE));
        }
        send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_SUCCESS);
        return ndk::ScopedAStatus::ok();
    }else{
        ALOGE("aac richtap not support");
        send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_NOT_SUPPORT);
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }

}
ndk::ScopedAStatus RichtapVibrator::stop(const std::shared_ptr<IRichtapCallback>& callback){
    if(m_richtap_support){
        bool ret = aac_vibra_looper_stopPerformHe();
        if(!ret){
            send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_FAILED);
            return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_STATE));
        }else{
            send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_SUCCESS);
            return ndk::ScopedAStatus::ok();
        }
    }else{
        ALOGE("aac richtap not support");
        send_handle_result(callback,DEFAULT_RETURN_TIME_OUT,RICHTAP_HANDLE_NOT_SUPPORT);
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }
}

}  // namespace aidl::android::hardware::richtap::vibrator