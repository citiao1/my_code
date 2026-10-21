"""Controller semantics; firmware/CAPS allowlist remains in attitude_tuner."""
from dataclasses import asdict, dataclass


@dataclass(frozen=True)
class LoopProfile:
    loop: str
    mode: int
    controller_form: str
    controller_period_s: float
    reference_unit: str
    output_unit: str
    output_limit: float
    parameters: tuple[str, str]
    derivative: str
    gain_decision_source: str
    automatic_writeback: bool


PROFILES = {
    "wheel_speed": LoopProfile("wheel_speed", 0, "incremental", 0.01,
                               "RPM", "PWM percent", 60, ("Kp", "Ki_step"), "Kd_step fixed zero",
                               "validated_downloaded_WLOG", True),
    "angular_velocity": LoopProfile("angular_velocity", 1, "parallel", 0.02,
                                    "deg/s", "RPM", 60, ("Kp", "Ki"), "none",
                                    "validated_downloaded_HLOG", True),
    "direction": LoopProfile("direction", 2, "parallel", 0.02,
                             "deg", "deg/s", 80, ("Kp", "Kd"), "measurement",
                             "validated_downloaded_HLOG", True),
}


def get_profile(loop):
    if loop not in PROFILES:
        raise ValueError("no automatic high-rate tuning profile for this loop")
    return asdict(PROFILES[loop])
