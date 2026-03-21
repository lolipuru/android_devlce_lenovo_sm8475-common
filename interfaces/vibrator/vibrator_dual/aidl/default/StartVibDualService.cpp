#define LOG_TAG "VibratorDual"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include "VibratorDualService.h"

using aidl::vendor::qti::hardware::vibrator_dual::device::VibratorDualService;

int AddVibratorDualService() {
    std::shared_ptr<VibratorDualService> vibService = ndk::SharedRefBase::make<VibratorDualService>();

    ABinderProcess_setThreadPoolMaxThreadCount(1);

    const std::string instance = std::string() + VibratorDualService::descriptor + "/default";
    binder_status_t status = AServiceManager_addService(vibService->asBinder().get(), instance.c_str());

    if (status != STATUS_OK) {
        LOG(ERROR) << "Failed to register VibratorDual AIDL service. Error: " << status;
        return 1;
    }

    LOG(INFO) << "VibratorDual AIDL service registered successfully.";
    ABinderProcess_startThreadPool(); 

    return 0;
}