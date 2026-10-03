from __future__ import annotations

import argparse
import time

try:
    from .servo_pwm import PwmError, ServoLimits, SysfsPwm
except ImportError:
    from servo_pwm import PwmError, ServoLimits, SysfsPwm


def parse_sequence(value: str) -> list[int]:
    try:
        pulses = [int(item.strip()) for item in value.split(",") if item.strip()]
    except ValueError as exc:
        raise argparse.ArgumentTypeError(
            "pulse sequence must be comma-separated integers"
        ) from exc
    if not pulses:
        raise argparse.ArgumentTypeError("pulse sequence cannot be empty")
    return pulses


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Test one servo through a Linux sysfs PWM channel."
    )
    parser.add_argument(
        "--chip",
        default="/sys/class/pwm/pwmchip0",
        help="PWM chip sysfs path on Orange Pi.",
    )
    parser.add_argument(
        "--channel",
        type=int,
        default=3,
        help="PWM channel; PWM3 is the documented horizontal servo output.",
    )
    parser.add_argument(
        "--pulse-us",
        type=int,
        default=None,
        help="Hold one pulse width instead of running --sequence.",
    )
    parser.add_argument(
        "--sequence",
        type=parse_sequence,
        default=[1500],
        help="Comma-separated pulse widths, for example 1400,1500,1600,1500.",
    )
    parser.add_argument("--hold-seconds", type=float, default=2.0)
    parser.add_argument("--min-us", type=int, default=1000)
    parser.add_argument("--center-us", type=int, default=1500)
    parser.add_argument("--max-us", type=int, default=2000)
    parser.add_argument(
        "--keep-enabled",
        action="store_true",
        help="Keep the final pulse active until Ctrl+C.",
    )
    return parser.parse_args()


def wait_seconds(seconds: float) -> None:
    if seconds < 0:
        raise ValueError("hold time must be non-negative")
    time.sleep(seconds)


def main() -> int:
    args = parse_args()
    if args.hold_seconds < 0:
        raise SystemExit("--hold-seconds must be non-negative")

    limits = ServoLimits(args.min_us, args.center_us, args.max_us)
    pulses = [args.pulse_us] if args.pulse_us is not None else args.sequence
    for pulse in pulses:
        limits.validate(pulse)

    pwm = SysfsPwm(chip_path=args.chip, channel=args.channel)
    print(
        f"opening {args.chip}/pwm{args.channel}; "
        f"safe range={limits.min_us}-{limits.max_us} us",
        flush=True,
    )
    try:
        pwm.open(limits.center_us * 1000)
        print(f"center: {limits.center_us} us", flush=True)
        for pulse in pulses:
            pwm.set_pulse_us(pulse)
            print(f"pulse: {pulse} us", flush=True)
            wait_seconds(args.hold_seconds)

        if args.keep_enabled:
            print("holding final pulse; press Ctrl+C to stop", flush=True)
            while True:
                wait_seconds(1.0)
    except KeyboardInterrupt:
        print("\ninterrupted; returning to center", flush=True)
    except (PwmError, OSError, ValueError) as exc:
        print(f"servo test failed: {exc}", flush=True)
        return 1
    finally:
        if pwm.is_open:
            try:
                pwm.set_pulse_us(limits.center_us)
                wait_seconds(min(args.hold_seconds, 1.0))
            except (PwmError, OSError, ValueError):
                pass
        pwm.close()
        print("PWM stopped and released", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
