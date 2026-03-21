#define LOG_TAG "vendor.qti.hardware.vibrator_dual-service"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include "VibratorDualService.h"

using aidl::vendor::qti::hardware::vibrator_dual::device::VibratorDualService;

int main() {
    android::base::InitLogging(nullptr, android::base::LogdLogger());

    LOG(INFO) << "VibratorDual AIDL service starting...";
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    auto vibService = ndk::SharedRefBase::make<VibratorDualService>();
    const std::string instance = std::string() + VibratorDualService::descriptor + "/default";
    binder_status_t status = AServiceManager_addService(vibService->asBinder().get(), instance.c_str());

    if (status != STATUS_OK) {
        LOG(ERROR) << "Could not register VibratorDual service, status: " << status;
        return EXIT_FAILURE;
    }

    LOG(INFO) << "VibratorDual service registered as: " << instance;
    ABinderProcess_joinThreadPool();

    return EXIT_FAILURE; // Should never be reached
}