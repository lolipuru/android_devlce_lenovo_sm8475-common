package vendor.qti.hardware.vibrator_dual.device;

import vendor.qti.hardware.vibrator_dual.device.Status;

@VintfStability
interface IVibratorDual {
    Status onDual(in int t, in int vibType, in int innerId, in int innerIdSub, in int timeoutMs, in int timeoutMsSub);
    Status dualCancel(in int vibType);
}