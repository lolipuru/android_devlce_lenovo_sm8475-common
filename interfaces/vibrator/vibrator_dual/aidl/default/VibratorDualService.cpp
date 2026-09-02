#include "VibratorDualService.h"
#include <android-base/logging.h>
#include <utils/Log.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#ifdef TARGET_USE_CALIBRATION
#include <android-base/properties.h>
#include <thread>
#endif

namespace aidl::vendor::qti::hardware::vibrator_dual::device {

#define RAM_MAX_ID (54)
#define RTP_BASE_ID (91)
#define RTP_RIGHT_SHIFT (300)
#define AMPLITUDE_SHIF  (12)
#define AMPLITUDE_MASK  (0xfff)

#ifdef TARGET_USE_CALIBRATION
#define CALI_PATH_1 "/mnt/vendor/persist/haptic/aw_cali_lra_1.txt"
#define CALI_PATH_2 "/mnt/vendor/persist/haptic/aw_cali_lra_2.txt"
#define SYSFS_CALI_NODE "/sys/class/leds/vibrator_aw8697x/calibrate"
#define SYSFS_VIB_MAIN "/sys/class/leds/vibrator"
#else
static const char LED_DEVICE_L[] = "/sys/class/leds/vibrator_l";
static const char LED_DEVICE_R[] = "/sys/class/leds/vibrator_r";
#endif

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
#ifdef TARGET_USE_CALIBRATION
    loadCalibrationData();
#endif
}

#ifdef TARGET_USE_CALIBRATION
void VibratorDualService::loadCalibrationData() {
    std::thread([this]() {
        char cali_data[64] = {0};
        bool vib1_ok = false;
        bool vib2_ok = false;

        for (int retry = 0; retry < 50; ++retry) {
            if (!vib1_ok && read_value(CALI_PATH_1, cali_data) == 0) {
                LOG(INFO) << "Vibrator1 Read Persist Cali Data: " << cali_data;
                write_value("/sys/class/leds/vibrator/cali_lra", cali_data);
                vib1_ok = true;
            }
            memset(cali_data, 0, sizeof(cali_data));
            if (!vib2_ok && read_value(CALI_PATH_2, cali_data) == 0) {
                LOG(INFO) << "Vibrator2 Read Persist Cali Data: " << cali_data;
                write_value("/sys/class/leds/vibrator_aw8697x/cali_lra", cali_data);
                vib2_ok = true;
            }
            if (vib1_ok && vib2_ok) {
                break;
            }
            usleep(200 * 1000); // 200ms
        }

        if (!vib1_ok) {
            LOG(WARNING) << "Vibrator1 read persist file failed, loading default 0xff";
            write_value("/sys/class/leds/vibrator/cali_lra", "0xff");
        }
        if (!vib2_ok) {
            LOG(WARNING) << "Vibrator2 read persist file failed, loading default 0xff";
            write_value("/sys/class/leds/vibrator_aw8697x/cali_lra", "0xff");
        }

        android::base::SetProperty("vendor.haptic.calibrate.done", "1");
        LOG(INFO) << "calibration data loaded";
    }).detach();
}

int VibratorDualService::read_value(const char *file, char *value) {
    int fd;
    ssize_t ret;

    fd = TEMP_FAILURE_RETRY(open(file, O_RDONLY));
    if (fd < 0) {
        ALOGE("open %s failed, errno = %d", file, errno);
        return -errno;
    }

    ret = TEMP_FAILURE_RETRY(read(fd, value, 64));
    if (ret >= 0) {
        if (ret > 0 && value[ret - 1] == '\n') {
            value[ret - 1] = '\0';
        } else {
            value[ret] = '\0';
        }
        ret = 0;
    } else {
        ret = -errno;
    }

    errno = 0;
    close(fd);
    return ret;
}

int VibratorDualService::write_persist_value(const char *file, const char *value) {
    return write_value(file, value);
}

int VibratorDualService::runCalibration(int id) {
    const char* path = (id == 1) ? CALI_PATH_1 : CALI_PATH_2;
    const char* prop = (id == 1) ? "odm.haptic1.cali" : "odm.haptic2.cali";
    
    LOG(INFO) << "Vibrator" << id << " Start Cali";
    
    if (write_value(SYSFS_CALI_NODE, "1") != 0) {
        LOG(ERROR) << "Vibrator" << id << " write cali failed";
        return -1;
    }

    char cali_data[64] = {0};
    if (read_value(SYSFS_CALI_NODE, cali_data) != 0) {
        LOG(ERROR) << "Vibrator" << id << " read cali failed";
        return -1;
    }
    LOG(INFO) << "Vibrator" << id << " Read Cali : " << cali_data;

    write_persist_value(path, cali_data);
    
    char verify_data[64] = {0};
    read_value(path, verify_data);
    
    if (strcmp(cali_data, verify_data) == 0) {
        LOG(INFO) << "Vibrator" << id << " Cali Pass";
        android::base::SetProperty(prop, "true");
        return 0;
    } else {
        LOG(ERROR) << "Vibrator" << id << " Cali Failed";
        android::base::SetProperty(prop, "false");
        return -1;
    }
}
#endif

ndk::ScopedAStatus VibratorDualService::onDual(int32_t t, int32_t vibType, int32_t innerId, 
                                              int32_t innerIdSub, int32_t timeoutMs, 
                                              int32_t timeoutMsSub, Status* _aidl_return) {

    LOG(INFO) << "onDual() vibType=" << vibType << " id=" << innerId << " timeout=" << timeoutMs;

#ifdef TARGET_USE_CALIBRATION
    if (innerId == 0 && innerIdSub == 0xFFF) {
        runCalibration(2);
        *_aidl_return = Status::OK;
        return ndk::ScopedAStatus::ok();
    } else if (innerId == 0xFFF && innerIdSub == 0) {
        runCalibration(1);
        *_aidl_return = Status::OK;
        return ndk::ScopedAStatus::ok();
    }
    char path[128];
    snprintf(path, sizeof(path), "%s/double_duration", SYSFS_VIB_MAIN);
    
    int fd = TEMP_FAILURE_RETRY(open(path, O_WRONLY));
    if (fd >= 0) {
        uint32_t payload[5] = { (uint32_t)vibType, (uint32_t)innerId, (uint32_t)innerIdSub, (uint32_t)timeoutMs, (uint32_t)timeoutMsSub };
        ssize_t w = TEMP_FAILURE_RETRY(write(fd, payload, sizeof(payload)));
        close(fd);
        if (w != sizeof(payload)) {
            ALOGE("write double_duration failed");
        }
    } else {
        ALOGE("open %s failed, errno = %d", path, errno);
    }
    
    *_aidl_return = Status::OK;
    return ndk::ScopedAStatus::ok();

#else
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
#endif
}

ndk::ScopedAStatus VibratorDualService::dualCancel(int32_t vibType, Status* _aidl_return) {
    LOG(DEBUG) << "Dual_Cancel " << vibType;
    
#ifdef TARGET_USE_CALIBRATION
    char path[128];
    char value[24];
    snprintf(path, sizeof(path), "%s/dual_cancel", SYSFS_VIB_MAIN);
    snprintf(value, sizeof(value), "%u", vibType);
    write_value(path, value);
    
    *_aidl_return = Status::OK;
    return ndk::ScopedAStatus::ok();
#else
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
#endif
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
    close(fd);
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

#ifndef TARGET_USE_CALIBRATION
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
#endif

} // namespace