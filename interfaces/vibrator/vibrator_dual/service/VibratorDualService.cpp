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

#include <inttypes.h>
#include <unistd.h>
#include <log/log.h>

#include "VibratorDualService.h"

extern "C" {
#include "libsoc_helper.h"
}
namespace vendor {
namespace qti {
namespace hardware {
namespace vibrator_dual {
namespace device {
namespace V1_0  {
namespace implementation {

#define DEBUG false
//[TASK][PIKE-1817][vibrator] Add amplitude parse for ZUI by luoming0812 at 20210930
#define RAM_MAX_ID (54)
#define RTP_BASE_ID (91)
//[PIKE-2121] Solve the crash caused by using the game key to enter the game mode by luoming0812 at 20211011
#define RTP_RIGHT_SHIFT (300)
#define AMPLITUDE_SHIF  (12)
#define AMPLITUDE_MASK  (0xfff)
static const char LED_DEVICE_L[] = "/sys/class/leds/vibrator_l";
static const char LED_DEVICE_R[] = "/sys/class/leds/vibrator_r";

//[TASK][PIKE-1145][vibrator] Remap effect id and wave id by luoming0812 at 20210927
static unsigned int maplist[RAM_MAX_ID]{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    10, 11, 12, 13, 14, 15,16, 17, 18, 19,
    20, 8, 1, 1, 1, 1, 2, 1, 1, 1,
    1, 2, 1, 1, 1, 1, 1, 2, 2, 5,
    5, 3, 4, 4, 1, 2, 2, 7, 2, 5,
    5, 6, 6, 9,
};
VibratorDualService *VibratorDualService::sInstance = nullptr;

VibratorDualService::VibratorDualService() {
    sInstance = this; // keep track of the most recent instance
    ALOGV("VibratorDualService init");
}

VibratorDualService::~VibratorDualService() {
    ALOGV("~VibratorDualService()");
    //mDevice = nullptr;
}

IVibratorDual* VibratorDualService::getInstance() {
    if (!sInstance) {
      ALOGE("VibratorDualService::getInstance init");
      sInstance = new VibratorDualService();
    }
    return sInstance;
}

Return<Status> VibratorDualService::onDual(uint32_t t __unused,uint32_t vibType,uint32_t innerId,uint32_t innerIdSub,
    uint32_t timeoutMs,uint32_t timeoutMsSub) {
//[TASK][PIKE-2976][vibrator] Add gain value configuration for King glory scene by luoming0812 at 20220309
    int amplitude;
    int amplitudeSub;

    amplitude = (int)((innerId >> AMPLITUDE_SHIF) & AMPLITUDE_MASK);
    innerId = (innerId & AMPLITUDE_MASK);
    amplitudeSub = (int)((innerIdSub >> AMPLITUDE_SHIF) & AMPLITUDE_MASK);
    innerIdSub = (innerIdSub & AMPLITUDE_MASK);

    ALOGI("onDual() vibType = %d, innerId = %d, innerIdSub = %d, timeoutMs = %d, timeoutMsSub = %d, \
    amplitude =%d, amplitudeSub = %d", vibType, innerId, innerIdSub, timeoutMs, timeoutMsSub, \
    amplitude, amplitudeSub);

    if( device_exists(LED_DEVICE_L)&&device_exists(LED_DEVICE_R) ){
        switch (vibType) {
            case VIBRATOR_LEFT:
                left_on(innerId, timeoutMs, amplitude);
                break;
            case VIBRATOR_RIGHT:
                right_on(innerIdSub, timeoutMsSub, amplitudeSub);
                break;
            case VIBRATOR_DUAL:
                left_on(innerId, timeoutMs, amplitude);
                right_on(innerIdSub, timeoutMsSub, amplitudeSub);
                break;

            default:
                ALOGE("Vibtype err!");
                break;
        }
    }else{
         ALOGE("vibrator device is not exist");
        return Status::UNKNOWN_ERROR;
    }

    return Status::OK;
}

Return<Status> VibratorDualService::Dual_Cancel(uint32_t vibType) {

     ALOGD("VibratorDualService::Dual_Cancel %d", vibType);
  if( device_exists(LED_DEVICE_L)&&device_exists(LED_DEVICE_R) ){
    switch (vibType){
        case VIBRATOR_LEFT:
            left_off();
            break;
        case VIBRATOR_RIGHT:
            right_off();
            break;
        case VIBRATOR_DUAL:
            left_off();
            right_off();
            break;

        default:
             ALOGE("Vibtype err!");
            break;
    }
  }else{
        ALOGE("vibrator device is not exist");
        return Status::UNKNOWN_ERROR;
  }
    return Status::OK;
}

bool VibratorDualService::device_exists(const char *file)
{
    char devicename[PATH_MAX];
    int fd;

    snprintf(devicename, sizeof(devicename), "%s/%s", file, "activate");
    fd = TEMP_FAILURE_RETRY(open(devicename, O_RDWR));
    if (fd < 0) {
         ALOGE("open %s failed, errno = %d", file, errno);
        return false;
    }
    return true;
}

int VibratorDualService::write_value(const char *file, const char *value) {
    int fd;
    int ret;

    fd = TEMP_FAILURE_RETRY(open(file, O_WRONLY));
    if (fd < 0) {
        ALOGE("open %s failed, errno = %d", file, errno);
        return -errno;
    }

    ret = TEMP_FAILURE_RETRY(write(fd, value, strlen(value) + 1));
    if (ret == -1) {
        ret = -errno;
    } else if (ret != strlen(value) + 1) {
        /* even though EAGAIN is an errno value that could be set
           by write() in some cases, none of them apply here.  So, this return
           value can be clearly identified when debugging and suggests the
           caller that it may try to call vibrator_on() again */
        ret = -EAGAIN;
    } else {
        ret = 0;
    }

    errno = 0;
    close(fd);

    return ret;
}

int VibratorDualService::left_on(uint32_t innerId, uint32_t timeoutMs, int amplitude)
{
    int ret;
    char file_str[50];
    char value[100];

    ALOGD( "left_on enter!");
    if(amplitude == 0)
        amplitude = 100;
    ALOGI( "left_on real amplitude = %d", (128/(100/amplitude)));
    snprintf(value, sizeof(value), "%u\n", (128/(100/amplitude)));
    snprintf(file_str, sizeof(file_str), "%s/%s", LED_DEVICE_L, "gain");
    ret = write_value(file_str, value) ;
    if (ret){
        ALOGE("write  %s failed, errno = %d\n", file_str, ret);
        return ret;
    }
    if (innerId < RAM_MAX_ID) {
//[TASK][PIKE-2378][vibrator] Wrong waveform number caused by clerical error by luoming0812 at 20211012
        innerId = maplist[innerId];
        if(innerId == 2 || innerId == 10){
            snprintf(file_str, sizeof(file_str), "%s/%s", LED_DEVICE_L, "activate_mode");
            ret = write_value(file_str, "5");
            if (ret){
                ALOGE("write  %s failed, errno = %d\n", file_str, ret);
                return ret;
            }
        }else{
            snprintf(file_str, sizeof(file_str), "%s/%s", LED_DEVICE_L, "activate_mode");
            ret = write_value(file_str, "1");
            if (ret){
                ALOGE("write  %s failed, errno = %d\n", file_str, ret);
                return ret;
            }
        }
        ALOGI( "left_on innerld = %d", innerId);
        snprintf(value, sizeof(value), "%u\n", innerId);
        snprintf(file_str, sizeof(file_str), "%s/%s", LED_DEVICE_L, "index");
        ret = write_value(file_str, value) ;
        if (ret){
            ALOGE("write  %s failed, errno = %d\n", file_str, ret);
            return ret;
        }

        snprintf(value, sizeof(value), "%u\n", timeoutMs);
        snprintf(file_str, sizeof(file_str), "%s/%s", LED_DEVICE_L, "duration");
        ret = write_value(file_str, value) ;
        if (ret){
            ALOGE("write  %s failed, errno = %d\n", file_str, ret);
            return ret;
        }

        snprintf(file_str, sizeof(file_str), "%s/%s", LED_DEVICE_L, "activate");
        ret =  write_value(file_str, "1") ;
        if (ret){
            ALOGE("write  %s failed, errno = %d\n", file_str, ret);
            return ret;
        }
    } else if (innerId >= RTP_BASE_ID){
        snprintf(value, sizeof(value), "%u\n", innerId);
        snprintf(file_str, sizeof(file_str), "%s/%s", LED_DEVICE_L, "double");
        ret = write_value(file_str, value) ;
        if (ret){
            ALOGE("write  %s failed, errno = %d\n", file_str, ret);
            return ret;
        }
    }else{
        ALOGE("undefine id = %d ", innerId);
    }
    return 0;
}

int VibratorDualService::right_on(uint32_t innerId, uint32_t timeoutMs, int amplitude)
{
    int ret;
    char file_str[50];
    char value[100]; 

    ALOGD( "right_on enter!");
    if(amplitude == 0)
        amplitude = 100;
    ALOGI( "right_on real amplitude = %d", (128/(100/amplitude)));
    snprintf(value, sizeof(value), "%u\n", (128/(100/amplitude)));
    snprintf(file_str, sizeof(file_str), "%s/%s", LED_DEVICE_R, "gain");
    ret = write_value(file_str, value) ;
    if (ret){
        ALOGE("write  %s failed, errno = %d\n", file_str, ret);
        return ret;
    }
    if ( innerId < RAM_MAX_ID ) {
        innerId = maplist[innerId];
        if(innerId == 2 || innerId == 10){
            snprintf(file_str, sizeof(file_str), "%s/%s", LED_DEVICE_R, "activate_mode");
            ret = write_value(file_str, "5");
            if (ret){
                ALOGE("write  %s failed, errno = %d\n", file_str, ret);
                return ret;
            }
        }else{
            snprintf(file_str, sizeof(file_str), "%s/%s", LED_DEVICE_R, "activate_mode");
            ret = write_value(file_str, "1");
            if (ret){
                ALOGE("write  %s failed, errno = %d\n", file_str, ret);
                return ret;
            }
        }
        ALOGI( "right_on innerld = %d", innerId);
        snprintf(value, sizeof(value), "%u\n", innerId);
        snprintf(file_str, sizeof(file_str), "%s/%s", LED_DEVICE_R, "index");
        ret = write_value(file_str, value) ;
        if (ret){
            ALOGE("write  %s failed, errno = %d\n", file_str, ret);
            return ret;
        }

        snprintf(value, sizeof(value), "%u\n", timeoutMs);
        snprintf(file_str, sizeof(file_str), "%s/%s", LED_DEVICE_R, "duration");
        ret = write_value(file_str, value) ;
        if (ret){
            ALOGE("write  %s failed, errno = %d\n", file_str, ret);
            return ret;
        }

        snprintf(file_str, sizeof(file_str), "%s/%s", LED_DEVICE_R, "activate");
        ret =  write_value(file_str, "1") ;
        if (ret){
            ALOGE("write  %s failed, errno = %d\n", file_str, ret);
            return ret;
        }
    } else if (innerId >= RTP_BASE_ID){
        innerId = innerId + RTP_RIGHT_SHIFT;
        snprintf(value, sizeof(value), "%u\n", innerId);
        snprintf(file_str, sizeof(file_str), "%s/%s", LED_DEVICE_R, "double");
        ret = write_value(file_str, value) ;
        if (ret){
            ALOGE("write  %s failed, errno = %d\n", file_str, ret);
            return ret;
        }
    }else{
        ALOGE("undefine id = %d ", innerId);
    }
    return 0;

}

int VibratorDualService::left_off(void)
{
    int ret;
    char file_str[50];

    snprintf(file_str, sizeof(file_str), "%s/%s", LED_DEVICE_L, "activate");
    ret = write_value(file_str, "0") ;
    if (ret){
        ALOGE("write  %s failed, errno = %d\n", file_str, ret);
        return ret;
    }
    snprintf(file_str, sizeof(file_str), "%s/%s", LED_DEVICE_L, "gain");
    ret = write_value(file_str, "128") ;
    if (ret){
        ALOGE("write  %s failed, errno = %d\n", file_str, ret);
        return ret;
    }
    return 0;
}
int VibratorDualService::right_off(void)
{
    int ret;
    char file_str[50];

    snprintf(file_str, sizeof(file_str), "%s/%s", LED_DEVICE_R, "activate");
    ret = write_value(file_str, "0") ;
    if (ret){
        ALOGE("write  %s failed, errno = %d\n", file_str, ret);
        return ret;
    }
    snprintf(file_str, sizeof(file_str), "%s/%s", LED_DEVICE_R, "gain");
    ret = write_value(file_str, "128") ;
    if (ret){
        ALOGE("write  %s failed, errno = %d\n", file_str, ret);
        return ret;
    }
    return 0;
}

}
}  // namespace implementation
}  // namespace V2_0
}  // namespace service
}  // namespace extend
}  // namespace goodix
}  // namespace vendor