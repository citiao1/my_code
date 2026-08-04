"""
MaixCAM Pro steel-ball vision and cascade balance control.

Control chain:
    gray ROI -> ball position -> velocity estimate
    position loop -> target ball velocity
    velocity loop -> target pipe angle
    pipe/motor calibration -> ZDT X42S absolute position command

RTSP is intentionally removed from the control path. Save the motor origin once
with MOTOR_SAVE_ORIGIN_ON_BOOT, then keep that option disabled during normal use.
"""

from maix import app, camera, display, image, time, uart
import cv2
import numpy as np


# ==================== Camera and vision ====================
DET_W = 640
DET_H = 480
DET_FPS = 60

# The ROI must contain only the inside of the horizontal pipe. Adjust these four
# values first while looking at the on-device image.
ROI_X1 = 20
ROI_X2 = 620
ROI_Y1 = 200
ROI_Y2 = 280

# Pixel calibration. Startup calibration only updates X_ZERO_PX; PX_PER_CM must
# be measured using two known marks on the real pipe.
X_ZERO_PX = 320.0
PX_PER_CM = 22.4
ENABLE_STARTUP_ZERO_CALIBRATION = True
CALIB_FRAMES = 20
CALIB_MAX_SPREAD_CM = 0.5
CALIB_TIMEOUT_MS = 15000

# One-dimensional gray-profile detector parameters.
PROFILE_BASELINE_SIGMA = 40.0
PROFILE_SMOOTH_SIGMA = 2.0
BALL_MIN_CONTRAST = 8.0
BALL_RADIUS_PX = 22
ROI_EDGE_MARGIN_PX = 20
MAX_FRAME_JUMP_CM = 4.0
DETECTOR_REACQUIRE_MS = 250

POSITION_FILTER_ALPHA = 0.45
VELOCITY_WINDOW_MS = 150
VELOCITY_FILTER_ALPHA = 0.35
VELOCITY_ABS_LIMIT_CM_S = 100.0


# ==================== Cascade control ====================
CONTROL_HZ = 30
CONTROL_INTERVAL_MS = 1000 // CONTROL_HZ

# Conservative initial values. Tune the velocity loop first, then the position
# loop. Units: POSITION_KP [1/s], VELOCITY_KP [deg/(cm/s)].
POSITION_KP = 0.50
POSITION_KI = 0.00
VELOCITY_KP = 0.10
VELOCITY_KI = 0.00

POSITION_INTEGRAL_LIMIT = 15.0
VELOCITY_INTEGRAL_LIMIT = 20.0
TARGET_SPEED_LIMIT_CM_S = 5.0
PIPE_ANGLE_LIMIT_DEG = 2.0
PIPE_ANGLE_SLEW_DEG_S = 8.0

# Set this to -1.0 if a positive pipe angle makes the ball move toward negative x.
CONTROL_DIRECTION = 1.0

# A brief dropout holds the last command. A longer dropout resets the controller
# and slews the pipe back to level.
LOST_RETURN_LEVEL_MS = 150


# ==================== Task ====================
# 0: hold TARGET_CM; 3: O -> +5 cm -> -5 cm and hold.
# Commission and tune in mode 0. Enable mode 3 only after mode 0 is stable.
TASK_MODE = 0
TARGET_CM = 0.0
TASK3_POS_A_CM = 5.0
TASK3_POS_B_CM = -5.0
TASK3_POSITION_TOLERANCE_CM = 0.5
TASK3_SPEED_TOLERANCE_CM_S = 1.0
TASK3_HOLD_MS = 500


# ==================== Motor ====================
UART_DEV = "/dev/ttyS0"
UART_BAUD = 115200
MOTOR_ID = 0x01
CHECKSUM = 0x6B
MOTOR_STATUS_SUCCESS = 0x02
MOTOR_EEPROM_SETTLE_MS = 1000

# Set True for one run only while the mechanism is physically level. Change it
# back to False before the next run so the stored single-turn origin is kept.
MOTOR_SAVE_ORIGIN_ON_BOOT = False
MOTOR_HOME_ON_BOOT = True
MOTOR_COMMAND_TIMEOUT_MS = 500
MOTOR_HOME_TIMEOUT_MS = 10000
MOTOR_HOME_POLL_MS = 150
MOTOR_HOME_TOLERANCE_DEG = 1.5
MOTOR_HOME_SETTLE_SAMPLES = 2
MOTOR_SPEED_RPM = 100
MOTOR_ACC = 80
# This is the verified mechanical motor range, not the normal balancing range.
# Normal commands are limited by PIPE_ANGLE_LIMIT_DEG and the calibration table.
MOTOR_POSITION_LIMIT_DEG = 40.0
MOTOR_COMMAND_DEADBAND_DEG = 0.10
PULSES_PER_DEGREE = 3200.0 / 360.0

# Each tuple is (real pipe angle, motor absolute angle). Replace the motor-angle
# values with measurements from the rocker mechanism. The interpolation handles
# a nonlinear linkage without changing the controller.
PIPE_MOTOR_CALIBRATION = [
    (-2.0, -6.0),
    (0.0, 0.0),
    (2.0, 6.0),
]


# ==================== Display and diagnostics ====================
DISPLAY_HZ = 10
DISPLAY_INTERVAL_MS = 1000 // DISPLAY_HZ
PERF_INTERVAL_MS = 3000
TELEMETRY_INTERVAL_MS = 200


def clamp(value, low, high):
    return max(low, min(high, value))


def elapsed_ms(now_ms, before_ms):
    # A normal control run is much shorter than the timer wrap period.
    return max(0, now_ms - before_ms)


def slew_limit(target, previous, rate_per_second, dt):
    max_change = max(0.0, rate_per_second * dt)
    return previous + clamp(target - previous, -max_change, max_change)


def pipe_angle_to_motor_angle(pipe_angle_deg):
    """Piecewise-linear inverse linkage calibration."""
    points = PIPE_MOTOR_CALIBRATION
    pipe_angle_deg = clamp(pipe_angle_deg, points[0][0], points[-1][0])

    for index in range(1, len(points)):
        pipe_0, motor_0 = points[index - 1]
        pipe_1, motor_1 = points[index]
        if pipe_angle_deg <= pipe_1:
            span = pipe_1 - pipe_0
            ratio = 0.0 if span == 0 else (pipe_angle_deg - pipe_0) / span
            return motor_0 + ratio * (motor_1 - motor_0)
    return points[-1][1]


def pixel_to_cm(x_px):
    return (x_px - X_ZERO_PX) / PX_PER_CM


def validate_config():
    if not (0 <= ROI_X1 < ROI_X2 <= DET_W):
        raise ValueError("ROI_X1/ROI_X2 are outside the camera image")
    if not (0 <= ROI_Y1 < ROI_Y2 <= DET_H):
        raise ValueError("ROI_Y1/ROI_Y2 are outside the camera image")
    if PX_PER_CM <= 0.0:
        raise ValueError("PX_PER_CM must be measured and greater than zero")
    if CONTROL_HZ <= 0 or DISPLAY_HZ <= 0:
        raise ValueError("CONTROL_HZ and DISPLAY_HZ must be greater than zero")
    if not (0.0 < POSITION_FILTER_ALPHA <= 1.0):
        raise ValueError("POSITION_FILTER_ALPHA must be in (0, 1]")
    if not (0.0 < VELOCITY_FILTER_ALPHA <= 1.0):
        raise ValueError("VELOCITY_FILTER_ALPHA must be in (0, 1]")
    if PIPE_ANGLE_LIMIT_DEG <= 0.0:
        raise ValueError("PIPE_ANGLE_LIMIT_DEG must be greater than zero")
    if len(PIPE_MOTOR_CALIBRATION) < 2:
        raise ValueError("PIPE_MOTOR_CALIBRATION needs at least two points")

    previous_pipe = PIPE_MOTOR_CALIBRATION[0][0]
    for pipe_angle, _ in PIPE_MOTOR_CALIBRATION[1:]:
        if pipe_angle <= previous_pipe:
            raise ValueError("PIPE_MOTOR_CALIBRATION pipe angles must increase")
        previous_pipe = pipe_angle
    for _, motor_angle in PIPE_MOTOR_CALIBRATION:
        if abs(motor_angle) > MOTOR_POSITION_LIMIT_DEG:
            raise ValueError(
                "PIPE_MOTOR_CALIBRATION exceeds MOTOR_POSITION_LIMIT_DEG"
            )
    if (
        PIPE_MOTOR_CALIBRATION[0][0] > -PIPE_ANGLE_LIMIT_DEG
        or PIPE_MOTOR_CALIBRATION[-1][0] < PIPE_ANGLE_LIMIT_DEG
    ):
        raise ValueError(
            "PIPE_MOTOR_CALIBRATION must cover PIPE_ANGLE_LIMIT_DEG"
        )


class MotorController:
    def __init__(self):
        self.ser = uart.UART(UART_DEV, UART_BAUD)
        self.current_motor_angle = None
        self.connected = False
        self.enable_attempted = False

    def _send(self, data):
        self.ser.write(bytes(data))

    def _drain_input(self):
        for _ in range(16):
            data = self.ser.read()
            if not data:
                break

    def _read_frame(self, function, frame_length, timeout_ms):
        start = time.ticks_ms()
        buf = b""
        while elapsed_ms(time.ticks_ms(), start) < timeout_ms:
            data = self.ser.read()
            if data:
                buf += bytes(data)
                last_start = len(buf) - frame_length
                for offset in range(max(0, last_start + 1)):
                    frame = buf[offset:offset + frame_length]
                    if (
                        frame[0] == MOTOR_ID
                        and frame[1] == function
                        and frame[-1] == CHECKSUM
                    ):
                        return frame
                if len(buf) > 64:
                    buf = buf[-64:]
            time.sleep_ms(2)
        return None

    def _cmd(self, data, response_length=4, wait=True,
             timeout_ms=MOTOR_COMMAND_TIMEOUT_MS):
        self._drain_input()
        self._send(data)
        if not wait:
            return None
        return self._read_frame(data[1], response_length, timeout_ms)

    def is_connected(self):
        self.connected = self.read_position() is not None
        return self.connected

    def enable(self):
        self.enable_attempted = True
        response = self._cmd(
            [MOTOR_ID, 0xF3, 0xAB, 0x01, 0x00, CHECKSUM]
        )
        return response is not None

    def disable(self):
        response = self._cmd(
            [MOTOR_ID, 0xF3, 0xAB, 0x00, 0x00, CHECKSUM]
        )
        self.enable_attempted = False
        return response

    def save_origin(self):
        response = self._cmd([MOTOR_ID, 0x93, 0x88, 0x01, CHECKSUM])
        if response is None:
            print("[MOTOR] no complete 0x93 response; origin was not saved")
            return False

        if response[2] != MOTOR_STATUS_SUCCESS:
            print("[MOTOR] 0x93 rejected: response={}, status=0x{:02X}".format(
                response.hex(), response[2]
            ))
            return False

        print("[MOTOR] origin save accepted: {}".format(response.hex()))
        time.sleep_ms(MOTOR_EEPROM_SETTLE_MS)
        self.read_position()
        return True

    def zero(self):
        # Compatibility with the previous name; this now saves the real origin.
        return self.save_origin()

    def read_position(self):
        frame = self._cmd(
            [MOTOR_ID, 0x36, CHECKSUM],
            response_length=8,
        )
        if frame is None:
            return None

        raw = (
            (frame[3] << 24)
            | (frame[4] << 16)
            | (frame[5] << 8)
            | frame[6]
        )
        angle = raw * 360.0 / 65536.0
        if frame[2] != 0:
            angle = -angle
        self.current_motor_angle = angle
        return angle

    def home(self):
        response = self._cmd([MOTOR_ID, 0x9A, 0x00, 0x00, CHECKSUM])
        if response is None:
            print("[MOTOR] no complete 0x9A response")
            return False

        started_ms = time.ticks_ms()
        settled_samples = 0
        while elapsed_ms(time.ticks_ms(), started_ms) < MOTOR_HOME_TIMEOUT_MS:
            angle = self.read_position()
            if angle is None:
                settled_samples = 0
            elif abs(angle) <= MOTOR_HOME_TOLERANCE_DEG:
                settled_samples += 1
                if settled_samples >= MOTOR_HOME_SETTLE_SAMPLES:
                    print("[MOTOR] homed at {:+.2f} deg".format(angle))
                    return True
            else:
                settled_samples = 0
            time.sleep_ms(MOTOR_HOME_POLL_MS)

        last_angle = self.current_motor_angle
        print("[MOTOR] homing timeout, last angle={}".format(
            "unknown" if last_angle is None
            else "{:+.2f} deg".format(last_angle)
        ))
        return False

    def stop(self):
        return self._cmd([MOTOR_ID, 0xFE, 0x98, 0x00, CHECKSUM])

    def goto_motor_angle(self, angle_deg, speed_rpm=MOTOR_SPEED_RPM,
                         acc=MOTOR_ACC):
        if not self.connected:
            return False

        angle_deg = clamp(
            angle_deg, -MOTOR_POSITION_LIMIT_DEG, MOTOR_POSITION_LIMIT_DEG
        )
        direction = 0x00 if angle_deg >= 0.0 else 0x01
        pulses = int(abs(angle_deg) * PULSES_PER_DEGREE + 0.5)
        speed_rpm = int(clamp(speed_rpm, 1, 3000))

        self._cmd([
            MOTOR_ID,
            0xFD,
            direction,
            (speed_rpm >> 8) & 0xFF,
            speed_rpm & 0xFF,
            acc,
            (pulses >> 24) & 0xFF,
            (pulses >> 16) & 0xFF,
            (pulses >> 8) & 0xFF,
            pulses & 0xFF,
            0x01,  # absolute position mode
            0x00,  # execute immediately
            CHECKSUM,
        ], wait=False)
        self.current_motor_angle = angle_deg
        return True


class GrayProfileBallDetector:
    """Fast ball detector for a fixed white pipe and a darker steel ball."""

    def __init__(self):
        self.previous_x = None
        self.last_seen_ms = 0

    def reset(self):
        self.previous_x = None
        self.last_seen_ms = 0

    def detect(self, img, now_ms):
        img_np = np.frombuffer(img.to_bytes(), dtype=np.uint8).reshape(
            img.height(), img.width(), 3
        )
        roi = img_np[ROI_Y1:ROI_Y2, ROI_X1:ROI_X2]
        if roi.size == 0:
            return None, None, 0.0

        gray = cv2.cvtColor(roi, cv2.COLOR_RGB2GRAY)
        gray = cv2.GaussianBlur(gray, (5, 5), 0)
        profile = np.mean(gray, axis=0).astype(np.float32)

        baseline = cv2.GaussianBlur(
            profile.reshape(1, -1), (0, 0), PROFILE_BASELINE_SIGMA
        ).reshape(-1)
        darkness = baseline - profile
        darkness = cv2.GaussianBlur(
            darkness.reshape(1, -1), (0, 0), PROFILE_SMOOTH_SIGMA
        ).reshape(-1)

        search_left = ROI_EDGE_MARGIN_PX
        search_right = len(darkness) - ROI_EDGE_MARGIN_PX
        tracking = (
            self.previous_x is not None
            and elapsed_ms(now_ms, self.last_seen_ms) < DETECTOR_REACQUIRE_MS
        )
        if tracking:
            previous_local = int(self.previous_x - ROI_X1)
            jump_px = int(MAX_FRAME_JUMP_CM * PX_PER_CM)
            search_left = max(search_left, previous_local - jump_px)
            search_right = min(search_right, previous_local + jump_px + 1)

        if search_right <= search_left:
            return None, None, 0.0

        peak_local = int(np.argmax(darkness[search_left:search_right]))
        peak_index = search_left + peak_local
        score = float(darkness[peak_index])
        if score < BALL_MIN_CONTRAST:
            if elapsed_ms(now_ms, self.last_seen_ms) >= DETECTOR_REACQUIRE_MS:
                self.previous_x = None
            return None, None, score

        left = max(search_left, peak_index - BALL_RADIUS_PX)
        right = min(search_right, peak_index + BALL_RADIUS_PX + 1)
        weights = np.maximum(darkness[left:right], 0.0)
        weight_sum = float(np.sum(weights))
        if weight_sum > 1e-6:
            positions = np.arange(left, right, dtype=np.float32)
            center_local = float(np.sum(positions * weights) / weight_sum)
        else:
            center_local = float(peak_index)

        x0 = max(0, int(center_local) - BALL_RADIUS_PX)
        x1 = min(gray.shape[1], int(center_local) + BALL_RADIUS_PX + 1)
        row_profile = np.mean(gray[:, x0:x1], axis=1)
        center_y_local = int(np.argmin(row_profile))

        center_x = ROI_X1 + center_local
        center_y = ROI_Y1 + center_y_local
        self.previous_x = center_x
        self.last_seen_ms = now_ms
        return center_x, center_y, score


class LowPassFilter:
    def __init__(self, alpha):
        self.alpha = alpha
        self.value = 0.0
        self.valid = False

    def reset(self, value=None):
        if value is None:
            self.value = 0.0
            self.valid = False
        else:
            self.value = value
            self.valid = True

    def update(self, value):
        if not self.valid:
            self.value = value
            self.valid = True
        else:
            self.value += self.alpha * (value - self.value)
        return self.value


class VelocityEstimator:
    """Least-squares slope over a short timestamped position window."""

    def __init__(self, window_ms, filter_alpha):
        self.window_ms = window_ms
        self.filter_alpha = filter_alpha
        self.samples = []
        self.velocity = 0.0
        self.valid = False

    def reset(self):
        self.samples = []
        self.velocity = 0.0
        self.valid = False

    def update(self, now_ms, position_cm):
        if self.samples and elapsed_ms(now_ms, self.samples[-1][0]) > 250:
            self.reset()

        self.samples.append((now_ms, position_cm))
        cutoff = now_ms - self.window_ms
        while len(self.samples) > 2 and self.samples[0][0] < cutoff:
            self.samples.pop(0)

        if len(self.samples) < 3:
            return self.velocity, False

        t0 = self.samples[0][0]
        times = [(sample[0] - t0) / 1000.0 for sample in self.samples]
        positions = [sample[1] for sample in self.samples]
        mean_t = sum(times) / len(times)
        mean_x = sum(positions) / len(positions)
        denominator = sum((value - mean_t) ** 2 for value in times)
        if denominator <= 1e-9:
            return self.velocity, False

        raw_velocity = sum(
            (times[index] - mean_t) * (positions[index] - mean_x)
            for index in range(len(times))
        ) / denominator
        raw_velocity = clamp(
            raw_velocity,
            -VELOCITY_ABS_LIMIT_CM_S,
            VELOCITY_ABS_LIMIT_CM_S,
        )

        if not self.valid:
            self.velocity = raw_velocity
            self.valid = True
        else:
            self.velocity += self.filter_alpha * (
                raw_velocity - self.velocity
            )
        return self.velocity, True


class CascadeController:
    """Position PI outer loop and ball-velocity PI inner loop."""

    def __init__(self):
        self.position_integral = 0.0
        self.velocity_integral = 0.0

    def reset(self):
        self.position_integral = 0.0
        self.velocity_integral = 0.0

    def update(self, position_cm, velocity_cm_s, target_cm, dt):
        position_error = target_cm - position_cm
        position_integral_candidate = clamp(
            self.position_integral + position_error * dt,
            -POSITION_INTEGRAL_LIMIT,
            POSITION_INTEGRAL_LIMIT,
        )
        speed_unsaturated = (
            POSITION_KP * position_error
            + POSITION_KI * position_integral_candidate
        )
        target_speed = clamp(
            speed_unsaturated,
            -TARGET_SPEED_LIMIT_CM_S,
            TARGET_SPEED_LIMIT_CM_S,
        )
        position_drives_further_into_saturation = (
            (speed_unsaturated > TARGET_SPEED_LIMIT_CM_S and position_error > 0.0)
            or (
                speed_unsaturated < -TARGET_SPEED_LIMIT_CM_S
                and position_error < 0.0
            )
        )
        if not position_drives_further_into_saturation:
            self.position_integral = position_integral_candidate

        velocity_error = target_speed - velocity_cm_s
        velocity_integral_candidate = clamp(
            self.velocity_integral + velocity_error * dt,
            -VELOCITY_INTEGRAL_LIMIT,
            VELOCITY_INTEGRAL_LIMIT,
        )
        raw_angle_unsaturated = (
            VELOCITY_KP * velocity_error
            + VELOCITY_KI * velocity_integral_candidate
        )
        angle_unsaturated = CONTROL_DIRECTION * raw_angle_unsaturated
        pipe_angle = clamp(
            angle_unsaturated, -PIPE_ANGLE_LIMIT_DEG, PIPE_ANGLE_LIMIT_DEG
        )
        velocity_drives_further_into_saturation = (
            (
                raw_angle_unsaturated > PIPE_ANGLE_LIMIT_DEG
                and velocity_error > 0.0
            )
            or (
                raw_angle_unsaturated < -PIPE_ANGLE_LIMIT_DEG
                and velocity_error < 0.0
            )
        )
        if not velocity_drives_further_into_saturation:
            self.velocity_integral = velocity_integral_candidate

        return target_speed, pipe_angle, position_error, velocity_error


class Task3StateMachine:
    """O -> +5 cm -> -5 cm, then keep holding -5 cm."""

    def __init__(self):
        self.state = 0
        self.target = TASK3_POS_A_CM
        self.hold_start_ms = 0
        self.start_ms = 0

    def start(self):
        self.state = 0
        self.target = TASK3_POS_A_CM
        self.hold_start_ms = 0
        self.start_ms = time.ticks_ms()
        print("[TASK3] target={:+.1f}cm".format(self.target))

    def update(self, position_cm, velocity_cm_s, measurement_valid):
        if not measurement_valid:
            return self.target, False

        now_ms = time.ticks_ms()
        stable = (
            abs(position_cm - self.target)
            <= TASK3_POSITION_TOLERANCE_CM
            and abs(velocity_cm_s) <= TASK3_SPEED_TOLERANCE_CM_S
        )
        target_changed = False

        if self.state in (0, 2):
            if stable:
                self.state += 1
                self.hold_start_ms = now_ms
                print("[TASK3] stable check started")
        elif self.state == 1:
            if not stable:
                self.state = 0
            elif elapsed_ms(now_ms, self.hold_start_ms) >= TASK3_HOLD_MS:
                self.state = 2
                self.target = TASK3_POS_B_CM
                target_changed = True
                print("[TASK3] switch target={:+.1f}cm".format(self.target))
        elif self.state == 3:
            if not stable:
                self.state = 2
            elif elapsed_ms(now_ms, self.hold_start_ms) >= TASK3_HOLD_MS:
                self.state = 4
                elapsed = elapsed_ms(now_ms, self.start_ms) / 1000.0
                print("[TASK3] complete in {:.2f}s; keep holding".format(elapsed))

        return self.target, target_changed

    @property
    def status(self):
        labels = ["TO_A", "HOLD_A", "TO_B", "HOLD_B", "DONE"]
        return "T3:{}".format(labels[self.state])


class PerfStats:
    def __init__(self):
        self.reset(time.ticks_ms())

    def reset(self, now_ms):
        self.started_ms = now_ms
        self.frames = 0
        self.control_steps = 0
        self.vision_sum = 0.0
        self.vision_max = 0.0
        self.control_sum = 0.0
        self.control_max = 0.0
        self.total_sum = 0.0
        self.total_max = 0.0

    def add(self, vision_ms, control_ms, total_ms, control_ran):
        self.frames += 1
        self.vision_sum += vision_ms
        self.vision_max = max(self.vision_max, vision_ms)
        self.total_sum += total_ms
        self.total_max = max(self.total_max, total_ms)
        if control_ran:
            self.control_steps += 1
            self.control_sum += control_ms
            self.control_max = max(self.control_max, control_ms)

    def report(self, now_ms):
        seconds = max(0.001, elapsed_ms(now_ms, self.started_ms) / 1000.0)
        fps = self.frames / seconds
        vision_avg = self.vision_sum / max(1, self.frames)
        control_avg = self.control_sum / max(1, self.control_steps)
        total_avg = self.total_sum / max(1, self.frames)
        return (
            "[PERF] fps={:.1f} control={} | vision={:.1f}/{:.0f}ms "
            "control={:.2f}/{:.0f}ms total={:.1f}/{:.0f}ms"
        ).format(
            fps,
            self.control_steps,
            vision_avg,
            self.vision_max,
            control_avg,
            self.control_max,
            total_avg,
            self.total_max,
        )


def calibrate_zero(cam, disp, detector):
    print("[CALIB] Put the ball at O; collecting {} frames".format(
        CALIB_FRAMES
    ))
    samples = []
    started_ms = time.ticks_ms()

    while not app.need_exit():
        now_ms = time.ticks_ms()
        if elapsed_ms(now_ms, started_ms) >= CALIB_TIMEOUT_MS:
            break

        img = cam.read()
        if img is None:
            continue
        center_x, center_y, score = detector.detect(img, now_ms)
        if center_x is not None:
            samples.append(center_x)
            img.draw_circle(
                int(center_x), int(center_y), BALL_RADIUS_PX,
                image.COLOR_GREEN, thickness=2
            )

        img.draw_string(
            4,
            4,
            "CALIB {}/{} score={:.1f}".format(
                min(len(samples), CALIB_FRAMES), CALIB_FRAMES, score
            ),
            color=image.Color.from_rgb(255, 255, 0),
            scale=1.0,
        )
        disp.show(img)

        if len(samples) >= CALIB_FRAMES:
            recent = samples[-CALIB_FRAMES:]
            spread_px = max(recent) - min(recent)
            if spread_px <= CALIB_MAX_SPREAD_CM * PX_PER_CM:
                zero_px = sum(recent) / len(recent)
                detector.reset()
                print("[CALIB] X_ZERO_PX={:.2f}, spread={:.2f}px".format(
                    zero_px, spread_px
                ))
                return zero_px
            samples = samples[-CALIB_FRAMES // 2:]

    detector.reset()
    print("[CALIB] timeout; use configured X_ZERO_PX={:.2f}".format(
        X_ZERO_PX
    ))
    return X_ZERO_PX


def initialize_motor():
    motor = MotorController()
    if not motor.is_connected():
        print("[MOTOR] no valid response; run vision only")
        return motor

    print("[MOTOR] connected")
    if not motor.enable():
        print("[MOTOR] enable command has no valid response; run vision only")
        motor.disable()
        motor.connected = False
        return motor
    time.sleep_ms(300)

    if MOTOR_SAVE_ORIGIN_ON_BOOT:
        print("[MOTOR] WARNING: saving the current LEVEL position as origin")
        if not motor.save_origin():
            print("[MOTOR] failed to save origin; motor control disabled")
            motor.stop()
            motor.disable()
            motor.connected = False
    elif MOTOR_HOME_ON_BOOT:
        print("[MOTOR] returning to the stored single-turn origin")
        if not motor.home():
            print("[MOTOR] homing failed; motor control disabled")
            motor.stop()
            motor.disable()
            motor.connected = False
    return motor


def main():
    global X_ZERO_PX

    motor = None
    try:
        validate_config()
        print("=" * 58)
        print("Steel-ball gray vision + position/velocity cascade control")
        print("=" * 58)
        print("[VISION] ROI=({}, {})-({}, {})".format(
            ROI_X1, ROI_Y1, ROI_X2, ROI_Y2
        ))

        cam = camera.Camera(
            DET_W, DET_H, image.Format.FMT_RGB888, fps=DET_FPS
        )
        disp = display.Display()
        detector = GrayProfileBallDetector()
        motor = initialize_motor()

        if ENABLE_STARTUP_ZERO_CALIBRATION:
            X_ZERO_PX = calibrate_zero(cam, disp, detector)

        position_filter = LowPassFilter(POSITION_FILTER_ALPHA)
        velocity_estimator = VelocityEstimator(
            VELOCITY_WINDOW_MS, VELOCITY_FILTER_ALPHA
        )
        controller = CascadeController()
        task3 = Task3StateMachine() if TASK_MODE == 3 else None
        if task3:
            task3.start()

        now_ms = time.ticks_ms()
        last_valid_ms = now_ms
        last_control_ms = now_ms
        last_display_ms = now_ms - DISPLAY_INTERVAL_MS
        last_telemetry_ms = now_ms - TELEMETRY_INTERVAL_MS
        perf = PerfStats()

        position_cm = 0.0
        velocity_cm_s = 0.0
        velocity_valid = False
        target_cm = task3.target if task3 else TARGET_CM
        target_speed_cm_s = 0.0
        requested_pipe_angle = 0.0
        pipe_angle_command = 0.0
        motor_target = pipe_angle_to_motor_angle(0.0)
        position_error = 0.0
        velocity_error = 0.0
        last_control_dt = 0.0
        score = 0.0
        was_lost = True

        print("[CONTROL] Kx={:.3f}, Kv={:.3f}, vmax={:.1f}cm/s, "
              "angle_limit={:.1f}deg".format(
                  POSITION_KP,
                  VELOCITY_KP,
                  TARGET_SPEED_LIMIT_CM_S,
                  PIPE_ANGLE_LIMIT_DEG,
              ))

        while not app.need_exit():
            frame_started_ms = time.ticks_ms()
            img = cam.read()
            if img is None:
                continue

            vision_started_ms = time.ticks_ms()
            now_ms = vision_started_ms
            center_x, center_y, score = detector.detect(img, now_ms)
            detection_valid = center_x is not None

            if detection_valid:
                raw_position_cm = pixel_to_cm(center_x)
                lost_duration_ms = elapsed_ms(now_ms, last_valid_ms)
                if was_lost and lost_duration_ms >= LOST_RETURN_LEVEL_MS:
                    position_filter.reset(raw_position_cm)
                    velocity_estimator.reset()
                    controller.reset()
                position_cm = position_filter.update(raw_position_cm)
                velocity_cm_s, velocity_valid = velocity_estimator.update(
                    now_ms, position_cm
                )
                last_valid_ms = now_ms
                was_lost = False
            else:
                lost_duration_ms = elapsed_ms(now_ms, last_valid_ms)
                if lost_duration_ms >= LOST_RETURN_LEVEL_MS:
                    was_lost = True
                    velocity_estimator.reset()
                    velocity_cm_s = 0.0
                    velocity_valid = False

            measurement_valid = detection_valid and velocity_valid
            target_changed = False
            if task3:
                target_cm, target_changed = task3.update(
                    position_cm, velocity_cm_s, measurement_valid
                )
                if target_changed:
                    controller.reset()
            else:
                target_cm = TARGET_CM

            vision_finished_ms = time.ticks_ms()
            control_ran = False
            control_ms = 0

            if elapsed_ms(now_ms, last_control_ms) >= CONTROL_INTERVAL_MS:
                control_started_ms = time.ticks_ms()
                dt = clamp(
                    elapsed_ms(now_ms, last_control_ms) / 1000.0,
                    0.005,
                    0.100,
                )
                last_control_ms = now_ms
                last_control_dt = dt
                control_ran = True

                if measurement_valid:
                    (
                        target_speed_cm_s,
                        requested_pipe_angle,
                        position_error,
                        velocity_error,
                    ) = (
                        controller.update(
                            position_cm,
                            velocity_cm_s,
                            target_cm,
                            dt,
                        )
                    )
                elif elapsed_ms(now_ms, last_valid_ms) >= LOST_RETURN_LEVEL_MS:
                    controller.reset()
                    target_speed_cm_s = 0.0
                    requested_pipe_angle = 0.0
                    position_error = target_cm - position_cm
                    velocity_error = 0.0

                pipe_angle_command = slew_limit(
                    requested_pipe_angle,
                    pipe_angle_command,
                    PIPE_ANGLE_SLEW_DEG_S,
                    dt,
                )
                motor_target = pipe_angle_to_motor_angle(pipe_angle_command)
                if (
                    motor.connected
                    and abs(motor_target - motor.current_motor_angle)
                    >= MOTOR_COMMAND_DEADBAND_DEG
                ):
                    motor.goto_motor_angle(motor_target)

                control_ms = elapsed_ms(time.ticks_ms(), control_started_ms)

            if elapsed_ms(now_ms, last_telemetry_ms) >= TELEMETRY_INTERVAL_MS:
                last_telemetry_ms = now_ms
                print(
                    "[CTRL] ok={} dt={:.3f} target={:+.2f} x={:+.2f} "
                    "v={:+.2f} ex={:+.2f} vr={:+.2f} ev={:+.2f} "
                    "pipe={:+.2f} motor={:+.2f}".format(
                        1 if measurement_valid else 0,
                        last_control_dt,
                        target_cm,
                        position_cm,
                        velocity_cm_s,
                        position_error,
                        target_speed_cm_s,
                        velocity_error,
                        pipe_angle_command,
                        motor_target,
                    )
                )

            if elapsed_ms(now_ms, last_display_ms) >= DISPLAY_INTERVAL_MS:
                last_display_ms = now_ms
                if detection_valid:
                    img.draw_circle(
                        int(center_x), int(center_y), BALL_RADIUS_PX,
                        image.COLOR_GREEN, thickness=2
                    )

                color = (
                    image.Color.from_rgb(0, 255, 0)
                    if measurement_valid
                    else image.Color.from_rgb(255, 0, 0)
                )
                task_status = task3.status if task3 else "HOLD"
                img.draw_string(
                    4,
                    4,
                    "x={:+5.2f} v={:+5.1f} t={:+4.1f}".format(
                        position_cm, velocity_cm_s, target_cm
                    ),
                    color=color,
                    scale=1.0,
                )
                img.draw_string(
                    4,
                    24,
                    "vr={:+4.1f} pipe={:+4.2f} {}".format(
                        target_speed_cm_s, pipe_angle_command, task_status
                    ),
                    color=color,
                    scale=1.0,
                )
                img.draw_string(
                    4,
                    44,
                    "motor={:+5.1f} score={:.1f} lost={}ms".format(
                        motor_target, score, elapsed_ms(now_ms, last_valid_ms)
                    ),
                    color=color,
                    scale=0.9,
                )
                disp.show(img)

            frame_finished_ms = time.ticks_ms()
            perf.add(
                elapsed_ms(vision_finished_ms, vision_started_ms),
                control_ms,
                elapsed_ms(frame_finished_ms, frame_started_ms),
                control_ran,
            )
            if elapsed_ms(frame_finished_ms, perf.started_ms) >= PERF_INTERVAL_MS:
                print(perf.report(frame_finished_ms))
                perf.reset(frame_finished_ms)

    finally:
        if motor is not None and motor.enable_attempted:
            motor.stop()
            if motor.connected:
                print("[EXIT] return pipe to level")
                motor.goto_motor_angle(pipe_angle_to_motor_angle(0.0))
                time.sleep_ms(500)
            motor.disable()
        print("[EXIT] complete")


if __name__ == "__main__":
    main()
