#pragma once

#include <aidl/vendor/qti/hardware/vibrator_dual/device/BnVibratorDual.h>
#include <aidl/vendor/qti/hardware/vibrator_dual/device/Status.h>

namespace aidl::vendor::qti::hardware::vibrator_dual::device {

class VibratorDualService : public BnVibratorDual {
public:
    VibratorDualService();

    ndk::ScopedAStatus onDual(int32_t in_t, int32_t in_vibType, int32_t in_innerId, 
                            int32_t in_innerIdSub, int32_t in_timeoutMs, 
                            int32_t in_timeoutMsSub, Status* _aidl_return) override;

    ndk::ScopedAStatus dualCancel(int32_t vibType, Status* _aidl_return) override;

    static constexpr int32_t VIBRATOR_LEFT  = 1;
    static constexpr int32_t VIBRATOR_RIGHT = 2;
    static constexpr int32_t VIBRATOR_DUAL  = 3;

private:
    bool device_exists(const char *file);
    int write_value(const char *file, const char *value);
    int left_on(uint32_t innerId, uint32_t timeoutMs, int amplitude);
    int right_on(uint32_t innerId, uint32_t timeoutMs, int amplitude);
    int left_off();
    int right_off();
#ifdef TARGET_USE_CALIBRATION
    void loadCalibrationData();
    int read_value(const char *file, char *value);
    int write_persist_value(const char *file, const char *value);
    int runCalibration(int id);
#endif
};

} // namespace