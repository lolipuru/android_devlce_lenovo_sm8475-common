package vendor.qti.hardware.vibrator_dual.device;

@Backing(type="int")
@VintfStability
enum Status {
    OK = 0,
    UNKNOWN_ERROR = 1,
    BAD_VALUE = 2,
    UNSUPPORTED_OPERATION = 3,
}