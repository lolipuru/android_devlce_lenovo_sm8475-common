#include "VibratorDualService.h"
#include <android-base/logging.h>
#include <utils/Log.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

namespace aidl::vendor::qti::hardware::vibrator_dual::device {

#define RAM_MAX_ID (54)
#define RTP_BASE_ID (91)
#define RTP_RIGHT_SHIFT (300)
#define AMPLITUDE_SHIF  (12)
#define AMPLITUDE_MASK  (0xfff)
static const char LED_DEVICE_L[] = "/sys/class/leds/vibrator_l";
static const char LED_DEVICE_R[] = "/sys/class/leds/vibrator_r";

static unsigned int maplist[RAM_MAX_ID]{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    10, 11, 12, 13, 14, 15,16, 17, 18, 19,
    20, 8, 1, 1, 1, 1, 2, 1, 1, 1,
    1, 2, 1, 1, 1, 1, 1, 2, 2, 5,
    5, 3, 4, 4, 1, 2, 2, 7, 2, 5,
    5, 6, 6, 9,
};

VibratorDualService::VibratorDualService() {
    LOG(INFO) << "VibratorDual AIDL Service initialized";
}

ndk::ScopedAStatus VibratorDualService::onDual(int32_t t, int32_t vibType, int32_t innerId, 
                                              int32_t innerIdSub, int32_t timeoutMs, 
                                              int32_t timeoutMsSub, Status* _aidl_return) {
    
    int amplitude = (int)((innerId >> AMPLITUDE_SHIF) & AMPLITUDE_MASK);
    innerId &= AMPLITUDE_MASK;
    int amplitudeSub = (int)((innerIdSub >> AMPLITUDE_SHIF) & AMPLITUDE_MASK);
    innerIdSub &= AMPLITUDE_MASK;

    LOG(INFO) << "onDual() vibType=" << vibType << " id=" << innerId << " timeout=" << timeoutMs;

    if (!device_exists(LED_DEVICE_L) || !device_exists(LED_DEVICE_R)) {
        LOG(ERROR) << "vibrator device does not exist";
        *_aidl_return = Status::UNKNOWN_ERROR;
        return ndk::ScopedAStatus::ok();
    }

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
            LOG(ERROR) << "Vibtype err!";
            *_aidl_return = Status::BAD_VALUE;
            return ndk::ScopedAStatus::ok();
    }

    *_aidl_return = Status::OK;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus VibratorDualService::dualCancel(int32_t vibType, Status* _aidl_return) {
    LOG(DEBUG) << "Dual_Cancel " << vibType;
    
    if (!device_exists(LED_DEVICE_L) || !device_exists(LED_DEVICE_R)) {
        *_aidl_return = Status::UNKNOWN_ERROR;
        return ndk::ScopedAStatus::ok();
    }

    switch (vibType) {
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
            *_aidl_return = Status::BAD_VALUE;
            return ndk::ScopedAStatus::ok();
    }

    *_aidl_return = Status::OK;
    return ndk::ScopedAStatus::ok();
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

} // namespace