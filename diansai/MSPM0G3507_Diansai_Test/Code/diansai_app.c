#include "include.h"
#include "LQ_device.h"
#include "diansai_app.h"

#include "board_io.h"
#include "vehicle_battery.h"
#include "vehicle_cascade_control.h"
#include "vehicle_encoder.h"
#include "vehicle_gray.h"
#include "vehicle_imu.h"
#include "vehicle_line_control.h"
#include "vehicle_motor.h"
#include "vehicle_square_test.h"
#include "wheeltec_link.h"

/*
 * V16 应用层只负责五件事：
 *   1. 按固定周期调度各硬件模块；
 *   2. 解释上位机命令并维护控制模式；
 *   3. 配置并调用三级串级控制器；
 *   4. 组装与 V15 完全兼容的 TEL/STA/SPD 遥测；
 *   5. 执行通信看门狗和板载按键急停等安全逻辑。
 *
 * 电机、编码器、IMU、灰度、串口、按键和蜂鸣器的硬件细节全部位于
 * 各自模块中。应用层只读取只读状态快照，避免同一硬件状态有两份副本。
 */

#define APP_LOOP_MS                    10U
#define TELEMETRY_PERIOD_MS           500U
#define SPEED_MODE_TELEMETRY_MS      4000U
#define SPEED_TELEMETRY_PERIOD_MS     300U
#define LINE_SPEED_TELEMETRY_MS       1000U
#define LINE_TELEMETRY_PERIOD_MS      400U
#define DEBUG_PERIOD_MS               4000U
#define GRAY_PERIOD_MS                  20U
#define BATTERY_PERIOD_MS              100U
#define COMMAND_TIMEOUT_MS             500U
#define LINK_TIMEOUT_MS               1200U

#define MAX_TEST_SPEED_MM_S             600

/*
 * 速度 PI 参数沿用实车已经确认的 4000/800/0。
 * 参考工程的参数作用于“m/s + 16799 计数 PWM”，本工程控制器使用
 * “mm/s + 百分比”，因此只在配置入口进行一次等价单位换算。
 */
#define SPEED_PID_LEGACY_PERIOD     16799.0f
#define SPEED_PID_GAIN_SCALE           (100.0f / (SPEED_PID_LEGACY_PERIOD * 1000.0f))
#define SPEED_PID_DEFAULT_KP         4000
#define SPEED_PID_DEFAULT_KI          800
#define SPEED_PID_DEFAULT_KD            0
#define SPEED_PID_OUTPUT_LIMIT       100.0f

/* 角速度环参数使用 diansai_test 的微单位整数，便于网页和 VOFA+ 调参。 */
#define DEFAULT_MAX_YAW_RATE_DPS      150
#define YAW_PID_DEFAULT_KP_MICRO     1000
#define YAW_PID_DEFAULT_KI_MICRO     2000
#define YAW_PID_DEFAULT_KD_MICRO        0
#define YAW_PID_DEFAULT_KFF_MICRO    1205
#define YAW_PID_MICRO_TO_MM_S       0.001f
#define YAW_INTEGRAL_LIMIT_DPS_S    300.0f

/*
 * 遥控方向环完全沿用 diansai_test 的参数单位和默认值。它只在遥控转向归零
 * 后的直行保持或 HEADSET 阶跃测试中启用；手动打方向仍直接控制目标角速度。
 */
#define HEADING_PID_DEFAULT_KP_MILLI  4000
#define HEADING_PID_DEFAULT_KD_MILLI   300
#define HEADING_PID_DEFAULT_KFF_MILLI 1000
#define HEADING_DEFAULT_MAX_RATE_DPS    80
#define HEADING_PID_KP_MAX_MILLI      20000
#define HEADING_PID_KD_MAX_MILLI      10000
#define HEADING_PID_KFF_MAX_MILLI      2000
#define HEADING_RATE_MIN_DPS               5
#define HEADING_RATE_MAX_DPS             360
#define HEADING_PERIOD_S               0.030f
#define HEADING_CORRECTION_DEADBAND_DEG 0.50f
#define HEADING_MIN_CORRECTION_DPS       8.0f
#define HEADING_CORRECTION_RATE_GATE_DPS 5.0f

/*
 * V20 启用独立灰度方向外环；V21 在此基础上增加直角弯丢线恢复状态机。
 * 参数使用千分制整数传输：650 表示 0.650，避免 9600 波特率文本协议解析浮点。
 * 当前实车确认的默认方向 PID 为 200/0/350，角速度环输出换算出的最大
 * 差速比例为 0.65。网页仍可按千分制命令临时调整这些值。
 */
#define LINE_PID_DEFAULT_KP_MILLI   200000
#define LINE_PID_DEFAULT_KI_MILLI        0
#define LINE_PID_DEFAULT_KD_MILLI   350000
#define LINE_DIFF_DEFAULT_MILLI        650
#define LINE_PID_GAIN_MAX_MILLI    1000000
#define LINE_DIFF_MAX_MILLI           1000
#define LINE_DIRECTION_OUTPUT_LIMIT  8000.0f
#define LINE_TARGET_YAW_LIMIT_DPS       60.0f
#define LINE_INTEGRAL_LIMIT_PERCENT_S   80.0f
#define LINE_FILTER_NEW_WEIGHT           0.35f
#define LINE_VISIBLE_SUM_MIN            200U
#define LINE_GAP_HOLD_MS                150U
#define LINE_BLIND_TURN_MS             1200U
#define LINE_TURN_MEMORY_MS             350U
#define LINE_EDGE_TARGET_MIN            550U
#define LINE_REACQUIRE_MAX_ACTIVE         4U
#define LINE_REACQUIRE_CONFIRM_SAMPLES    2U
#define LINE_TURN_MEMORY_ERROR_PERCENT   35.0f
#define LINE_BLIND_YAW_RATE_DPS          90.0f
#define LINE_BLIND_DIFF_MILLI          1000
#define LINE_SPEED_MIN_MM_S              50
#define LINE_SPEED_MAX_MM_S             300
#define LOCAL_LINE_SPEED_MM_S           200

/*
 * 当前处于纯 PID 调试阶段，死区标定值保留但不加入电机输出。
 * 重新启用前馈时，只需切换此宏，不需要改动控制器算法。
 */
#define SPEED_FEEDFORWARD_ENABLED       0U
#define SPEED_PID_FEEDBACK_LIMIT       (SPEED_FEEDFORWARD_ENABLED ? 35.0f : 100.0f)
#define LEFT_FORWARD_DEADZONE           8.0f
#define LEFT_REVERSE_DEADZONE           7.0f
#define RIGHT_FORWARD_DEADZONE          7.0f
#define RIGHT_REVERSE_DEADZONE          7.0f

#define KEY_BEEP_ON_MS                  80U
#define KEY_BEEP_OFF_MS                 80U
#define WARNING_BEEP_ON_MS             250U
#define WARNING_BEEP_OFF_MS            120U
#define LOCAL_START_KEY_MASK \
    ((uint8_t)((1U << BOARD_KEY_K1) | (1U << BOARD_KEY_K2)))

typedef enum
{
    MOTOR_MODE_IDLE = 0,
    MOTOR_MODE_RAW,
    MOTOR_MODE_SPEED,
    MOTOR_MODE_LINE
} MotorControlMode;

typedef enum
{
    TRACK_MODE_BLACK_ON_WHITE = 0,
    TRACK_MODE_WHITE_ON_BLUE
} TrackColorMode;

/*
 * 这里只保存“应用语义”状态，不复制硬件模块的数据。
 * 例如实际轮速始终来自 VehicleEncoder_GetState()，实际角速度始终来自
 * VehicleImu_GetState()；这样复位编码器或重新标定 IMU 时不会产生状态分叉。
 */
typedef struct
{
    int32_t target_forward_mm_s;
    int32_t target_yaw_rate10;
    int32_t fallback_wheel_correction_mm_s;
    int32_t target_left_mm_s;
    int32_t target_right_mm_s;
    int32_t error_left_mm_s;
    int32_t error_right_mm_s;
    int32_t pid_left_kp;
    int32_t pid_left_ki;
    int32_t pid_left_kd;
    int32_t pid_right_kp;
    int32_t pid_right_ki;
    int32_t pid_right_kd;
    int32_t yaw_pid_kp_micro;
    int32_t yaw_pid_ki_micro;
    int32_t yaw_pid_kd_micro;
    int32_t yaw_pid_kff_micro;
    int32_t max_yaw_rate_dps;
    int32_t heading_pid_kp_milli;
    int32_t heading_pid_kd_milli;
    int32_t heading_pid_kff_milli;
    int32_t max_heading_rate_dps;
    int32_t line_pid_kp_milli;
    int32_t line_pid_ki_milli;
    int32_t line_pid_kd_milli;
    int32_t line_diff_milli;
    uint8_t link_active;
    uint8_t yaw_control_enabled;
    uint8_t heading_control_enabled;
    uint8_t heading_hold_active;
    uint8_t local_line_running;
    uint8_t local_start_arming;
    uint8_t local_start_key_mask;
    uint8_t suppress_short_mask;
    float target_heading_deg;
    TrackColorMode track_color_mode;
    MotorControlMode motor_mode;
} AppState;

static AppState app;
static VehicleCascadeControl vehicle_control;
static VehicleCascadeOutput vehicle_output;

/* SysTick 中断只递增毫秒计数；所有业务都在主循环中执行。 */
static volatile uint32_t app_tick_ms;
static uint32_t now_ms;
static uint32_t last_loop_ms;
static uint32_t last_motor_command_ms;
static uint32_t last_rx_ms;
static uint32_t last_telemetry_ms;
static uint32_t last_speed_telemetry_ms;
static uint32_t last_line_telemetry_ms;
static uint32_t last_debug_ms;
static uint32_t last_gray_ms;
static uint32_t last_battery_ms;

static void UpdateEnabledControlLoops(void);
static void SendSquareTelemetry(void);

static int32_t ClampInt32(int32_t value, int32_t low, int32_t high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static float WrapAngleDegrees(float angle)
{
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

static int32_t RoundFloatToInt32(float value)
{
    return (int32_t)(value >= 0.0f ? value + 0.5f : value - 0.5f);
}

static void SendText(const char *text)
{
    (void)WheeltecLink_SendText(text);
}

/* 清零所有目标与最近一次控制输出，防止停止后遥测继续显示旧目标。 */
static void ClearControlTargets(void)
{
    app.target_forward_mm_s = 0;
    app.target_yaw_rate10 = 0;
    app.fallback_wheel_correction_mm_s = 0;
    app.target_left_mm_s = 0;
    app.target_right_mm_s = 0;
    app.error_left_mm_s = 0;
    app.error_right_mm_s = 0;
    memset(&vehicle_output, 0, sizeof(vehicle_output));
}

/* 所有主动停车路径最终都汇合到这里，保证控制器历史和 H 桥同步清零。 */
static void StopControl(void)
{
    if (VehicleSquare_GetState()->active)
    {
        VehicleSquare_Stop(VEHICLE_SQUARE_IDLE);
    }
    app.heading_hold_active = 0U;
    app.target_heading_deg = VehicleImu_GetState()->yaw_deg;
    app.local_line_running = 0U;
    app.local_start_arming = 0U;
    app.local_start_key_mask = 0U;
    BoardBuzzer_SetContinuous(0U);
    app.motor_mode = MOTOR_MODE_IDLE;
    ClearControlTargets();
    VehicleCascade_Reset(&vehicle_control);
    VehicleMotor_Stop();
    UpdateEnabledControlLoops();
}

static void ConfigureSpeedPid(VehiclePidId pid_id,
                              int32_t kp,
                              int32_t ki,
                              int32_t kd)
{
    VehiclePidConfig config = {
        .kp = (float)kp * SPEED_PID_GAIN_SCALE,
        .ki = (float)ki * SPEED_PID_GAIN_SCALE,
        .kd = (float)kd * SPEED_PID_GAIN_SCALE,
        .integral_limit = 0.0f,
        .feedback_limit = SPEED_PID_FEEDBACK_LIMIT,
        .output_limit = SPEED_PID_OUTPUT_LIMIT,
    };

    VehicleCascade_ConfigurePid(&vehicle_control, pid_id, &config);
    if (pid_id == VEHICLE_PID_SPEED_LEFT)
    {
        app.pid_left_kp = kp;
        app.pid_left_ki = ki;
        app.pid_left_kd = kd;
    }
    else
    {
        app.pid_right_kp = kp;
        app.pid_right_ki = ki;
        app.pid_right_kd = kd;
    }
}

static void ConfigureYawPid(int32_t kp_micro,
                            int32_t ki_micro,
                            int32_t kd_micro,
                            int32_t kff_micro)
{
    VehiclePidConfig config = {
        .kp = (float)kp_micro * YAW_PID_MICRO_TO_MM_S,
        .ki = (float)ki_micro * YAW_PID_MICRO_TO_MM_S,
        .kd = (float)kd_micro * YAW_PID_MICRO_TO_MM_S,
        .integral_limit = YAW_INTEGRAL_LIMIT_DPS_S,
        .feedback_limit = (float)MAX_TEST_SPEED_MM_S,
        .output_limit = (float)MAX_TEST_SPEED_MM_S,
    };

    VehicleCascade_ConfigurePid(&vehicle_control, VEHICLE_PID_YAW_RATE, &config);
    vehicle_control.yaw_rate_feedforward_mm_s_per_dps =
        (float)kff_micro * YAW_PID_MICRO_TO_MM_S;
    app.yaw_pid_kp_micro = kp_micro;
    app.yaw_pid_ki_micro = ki_micro;
    app.yaw_pid_kd_micro = kd_micro;
    app.yaw_pid_kff_micro = kff_micro;
}

static void ConfigureHeadingPid(int32_t kp_milli,
                                int32_t kd_milli,
                                int32_t kff_milli,
                                int32_t max_rate_dps)
{
    VehiclePidConfig config = {
        .kp = (float)kp_milli / 1000.0f,
        .ki = 0.0f,
        .kd = (float)kd_milli / 1000.0f,
        .integral_limit = 0.0f,
        .feedback_limit = (float)max_rate_dps,
        .output_limit = (float)max_rate_dps,
    };

    VehicleCascade_ConfigurePid(&vehicle_control, VEHICLE_PID_HEADING, &config);
    vehicle_control.heading_feedforward = (float)kff_milli / 1000.0f;
    vehicle_control.heading_period_s = HEADING_PERIOD_S;
    vehicle_control.heading_correction_deadband_deg =
        HEADING_CORRECTION_DEADBAND_DEG;
    vehicle_control.heading_min_correction_dps = HEADING_MIN_CORRECTION_DPS;
    vehicle_control.heading_correction_rate_gate_dps =
        HEADING_CORRECTION_RATE_GATE_DPS;
    app.heading_pid_kp_milli = kp_milli;
    app.heading_pid_kd_milli = kd_milli;
    app.heading_pid_kff_milli = kff_milli;
    app.max_heading_rate_dps = max_rate_dps;
}

/* 将网页千分制参数换算为方向模块使用的浮点系数。 */
static void ConfigureLinePid(int32_t kp_milli,
                             int32_t ki_milli,
                             int32_t kd_milli)
{
    VehicleLineConfig config = {
        .kp = (float)kp_milli / 1000.0f,
        .ki = (float)ki_milli / 1000.0f,
        .kd = (float)kd_milli / 1000.0f,
        .integral_limit = LINE_INTEGRAL_LIMIT_PERCENT_S,
        .output_limit = LINE_DIRECTION_OUTPUT_LIMIT,
        .target_yaw_limit_dps = LINE_TARGET_YAW_LIMIT_DPS,
        .blind_turn_yaw_rate_dps = LINE_BLIND_YAW_RATE_DPS,
        .filter_new_weight = LINE_FILTER_NEW_WEIGHT,
        .turn_memory_error_percent = LINE_TURN_MEMORY_ERROR_PERCENT,
        .visible_sum_min = LINE_VISIBLE_SUM_MIN,
        .gap_hold_ms = LINE_GAP_HOLD_MS,
        .blind_turn_ms = LINE_BLIND_TURN_MS,
        .turn_memory_ms = LINE_TURN_MEMORY_MS,
        .edge_line_min = LINE_EDGE_TARGET_MIN,
        .reacquire_max_active = LINE_REACQUIRE_MAX_ACTIVE,
        .reacquire_confirm_samples = LINE_REACQUIRE_CONFIRM_SAMPLES,
    };

    app.line_pid_kp_milli = kp_milli;
    app.line_pid_ki_milli = ki_milli;
    app.line_pid_kd_milli = kd_milli;
    VehicleLine_Configure(&config);
}

static void UpdateEnabledControlLoops(void)
{
    const VehicleImuState *imu = VehicleImu_GetState();
    uint8_t loops = VEHICLE_LOOP_SPEED;

    if (app.yaw_control_enabled && imu->ok) loops |= VEHICLE_LOOP_YAW_RATE;
    if (app.heading_control_enabled && app.heading_hold_active &&
        app.yaw_control_enabled && imu->ok)
    {
        loops |= VEHICLE_LOOP_HEADING;
    }
    VehicleCascade_SetEnabledLoops(&vehicle_control, loops);
}

static void CascadeControlInit(void)
{
#if SPEED_FEEDFORWARD_ENABLED
    VehicleMotorFeedforward left_feedforward = {
        .forward_min_percent = LEFT_FORWARD_DEADZONE,
        .reverse_min_percent = LEFT_REVERSE_DEADZONE,
        .forward_percent_per_mm_s =
            (SPEED_PID_OUTPUT_LIMIT - LEFT_FORWARD_DEADZONE) /
            (float)MAX_TEST_SPEED_MM_S,
        .reverse_percent_per_mm_s =
            (SPEED_PID_OUTPUT_LIMIT - LEFT_REVERSE_DEADZONE) /
            (float)MAX_TEST_SPEED_MM_S,
    };
    VehicleMotorFeedforward right_feedforward = {
        .forward_min_percent = RIGHT_FORWARD_DEADZONE,
        .reverse_min_percent = RIGHT_REVERSE_DEADZONE,
        .forward_percent_per_mm_s =
            (SPEED_PID_OUTPUT_LIMIT - RIGHT_FORWARD_DEADZONE) /
            (float)MAX_TEST_SPEED_MM_S,
        .reverse_percent_per_mm_s =
            (SPEED_PID_OUTPUT_LIMIT - RIGHT_REVERSE_DEADZONE) /
            (float)MAX_TEST_SPEED_MM_S,
    };
#else
    VehicleMotorFeedforward left_feedforward = {0};
    VehicleMotorFeedforward right_feedforward = {0};
#endif

    VehicleCascade_Init(&vehicle_control);
    vehicle_control.max_speed_mm_s = (float)MAX_TEST_SPEED_MM_S;
    vehicle_control.max_wheel_correction_mm_s = (float)MAX_TEST_SPEED_MM_S;
    app.max_yaw_rate_dps = DEFAULT_MAX_YAW_RATE_DPS;
    vehicle_control.max_yaw_rate_dps = (float)app.max_yaw_rate_dps;
    VehicleCascade_SetMotorFeedforward(&vehicle_control,
                                       &left_feedforward,
                                       &right_feedforward);
    ConfigureSpeedPid(VEHICLE_PID_SPEED_LEFT,
                      SPEED_PID_DEFAULT_KP,
                      SPEED_PID_DEFAULT_KI,
                      SPEED_PID_DEFAULT_KD);
    ConfigureSpeedPid(VEHICLE_PID_SPEED_RIGHT,
                      SPEED_PID_DEFAULT_KP,
                      SPEED_PID_DEFAULT_KI,
                      SPEED_PID_DEFAULT_KD);
    ConfigureYawPid(YAW_PID_DEFAULT_KP_MICRO,
                    YAW_PID_DEFAULT_KI_MICRO,
                    YAW_PID_DEFAULT_KD_MICRO,
                    YAW_PID_DEFAULT_KFF_MICRO);
    ConfigureHeadingPid(HEADING_PID_DEFAULT_KP_MILLI,
                        HEADING_PID_DEFAULT_KD_MILLI,
                        HEADING_PID_DEFAULT_KFF_MILLI,
                        HEADING_DEFAULT_MAX_RATE_DPS);

    /* 只初始化寻线配置，不设置 VEHICLE_LOOP_HEADING，也不参与当前控制输出。 */
    ConfigureLinePid(LINE_PID_DEFAULT_KP_MILLI,
                     LINE_PID_DEFAULT_KI_MILLI,
                     LINE_PID_DEFAULT_KD_MILLI);
    app.line_diff_milli = LINE_DIFF_DEFAULT_MILLI;
    UpdateEnabledControlLoops();
}

static void BeginSpeedControl(int32_t forward_mm_s,
                              int32_t target_yaw_rate10,
                              int32_t fallback_correction_mm_s)
{
    if (forward_mm_s == 0 && target_yaw_rate10 == 0 &&
        fallback_correction_mm_s == 0)
    {
        StopControl();
        return;
    }
    if (app.motor_mode != MOTOR_MODE_SPEED)
    {
        VehicleCascade_Reset(&vehicle_control);
    }
    app.motor_mode = MOTOR_MODE_SPEED;
    app.target_forward_mm_s = ClampInt32(forward_mm_s,
                                         -MAX_TEST_SPEED_MM_S,
                                         MAX_TEST_SPEED_MM_S);
    app.target_yaw_rate10 = ClampInt32(target_yaw_rate10,
                                       -app.max_yaw_rate_dps * 10,
                                       app.max_yaw_rate_dps * 10);
    app.fallback_wheel_correction_mm_s = ClampInt32(fallback_correction_mm_s,
                                                    -MAX_TEST_SPEED_MM_S,
                                                    MAX_TEST_SPEED_MM_S);
}

static void BeginHeadingControl(int32_t forward_mm_s,
                                float target_heading_deg,
                                uint8_t reset_reference)
{
    const VehicleImuState *imu = VehicleImu_GetState();

    if (app.motor_mode != MOTOR_MODE_SPEED)
    {
        VehicleCascade_Reset(&vehicle_control);
    }
    app.motor_mode = MOTOR_MODE_SPEED;
    app.target_forward_mm_s = ClampInt32(forward_mm_s,
                                         -MAX_TEST_SPEED_MM_S,
                                         MAX_TEST_SPEED_MM_S);
    app.target_yaw_rate10 = 0;
    app.fallback_wheel_correction_mm_s = 0;
    app.target_heading_deg = WrapAngleDegrees(target_heading_deg);
    app.heading_hold_active = 1U;
    UpdateEnabledControlLoops();
    if (reset_reference)
    {
        VehicleCascade_ResetHeadingReference(&vehicle_control, imu->yaw_deg);
    }
}

/*
 * 进入灰度寻线模式。首次启动必须已经完成黑白归一化、IMU 正常且角速度环
 * 已启用；否则拒绝启动，不能退化成没有方向反馈的直行。
 */
static uint8_t BeginLineControl(int32_t forward_mm_s)
{
    const VehicleGrayState *gray = VehicleGray_GetState();
    const VehicleImuState *imu = VehicleImu_GetState();
    const VehicleLineState *line;

    if (app.motor_mode != MOTOR_MODE_LINE)
    {
        StopControl();
        VehicleLine_Reset();
    }
    if (imu->ok) app.yaw_control_enabled = 1U;
    if (!gray->normalization_valid || !imu->ok)
    {
        return 0U;
    }
    forward_mm_s = ClampInt32(forward_mm_s,
                              LINE_SPEED_MIN_MM_S,
                              LINE_SPEED_MAX_MM_S);
    if (app.motor_mode != MOTOR_MODE_LINE)
    {
        if (!VehicleLine_Update(gray, now_ms)) return 0U;
    }

    line = VehicleLine_GetState();
    /* 寻线方向由灰度方向环给出，绝不叠加遥控航向保持环。 */
    app.heading_hold_active = 0U;
    UpdateEnabledControlLoops();
    app.motor_mode = MOTOR_MODE_LINE;
    app.target_forward_mm_s = forward_mm_s;
    app.target_yaw_rate10 =
        RoundFloatToInt32(line->target_yaw_rate_dps * 10.0f);
    app.fallback_wheel_correction_mm_s = 0;
    return 1U;
}

static void BeginRawControl(int32_t left_percent, int32_t right_percent)
{
    if (app.motor_mode != MOTOR_MODE_RAW)
    {
        VehicleCascade_Reset(&vehicle_control);
        ClearControlTargets();
    }
    app.motor_mode = MOTOR_MODE_RAW;
    VehicleMotor_Set(left_percent, right_percent);
}

static void SendGrayCalibration(void)
{
    const VehicleGrayState *gray = VehicleGray_GetState();
    char line[160];

    snprintf(line, sizeof(line),
             "CAL,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\r\n",
             gray->white[0], gray->white[1], gray->white[2], gray->white[3],
             gray->white[4], gray->white[5], gray->white[6], gray->white[7],
             gray->black[0], gray->black[1], gray->black[2], gray->black[3],
             gray->black[4], gray->black[5], gray->black[6], gray->black[7],
             (unsigned int)app.track_color_mode);
    SendText(line);

    /*
     * NRM 是 V17 新增的低频标定结果帧，只在查询/重新标定时发送，不增加
     * 常规 9600 波特率链路负担。无论赛道颜色如何，0=背景、1000=目标线。
     */
    snprintf(line, sizeof(line),
             "NRM,%lu,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\r\n",
             (unsigned long)now_ms,
             (unsigned int)gray->normalization_valid,
             gray->normalized[0], gray->normalized[1],
             gray->normalized[2], gray->normalized[3],
             gray->normalized[4], gray->normalized[5],
             gray->normalized[6], gray->normalized[7],
             (unsigned int)app.track_color_mode);
    SendText(line);
}

/*
 * K1 捕获背景、K2 捕获目标线。白底黑线模式对应白/黑，蓝底白线模式对应
 * 蓝/白。底层始终映射为背景=0、目标线=1000，因此允许蓝大白小的负分母。
 * 每次参考采集完成响 1 声；当两组参考均有效且八路对比度都足够时，
 * 再独立响 3 声表示 0..1000 归一化已经可供后续循迹算法使用。
 */
static void CalibrateGrayReference(uint8_t capture_background)
{
    const VehicleGrayState *gray;
    uint8_t normalized;

    StopControl();
    BoardBuzzer_Stop();
    normalized = capture_background ? VehicleGray_CaptureBackground() :
                                      VehicleGray_CaptureLine();
    gray = VehicleGray_GetState();
    SendGrayCalibration();
    (void)BoardBuzzer_Play(KEY_BEEP_ON_MS, KEY_BEEP_OFF_MS, 1U);

    if (normalized)
    {
        (void)BoardBuzzer_Play(KEY_BEEP_ON_MS, KEY_BEEP_OFF_MS, 3U);
    }
    else if (gray->white_valid && gray->black_valid)
    {
        SendText("ERR,GRAY_NORMALIZATION_RANGE\r\n");
    }
}

/*
 * K3 与 IMUZERO 共用同一条重标定路径。标定期间车辆已停止；成功响 2 声，
 * IMU 未通过设备检查时响 3 次长警告音，绝不把失败误报成成功。
 */
static uint8_t CalibrateGyroWithFeedback(void)
{
    StopControl();
    BoardBuzzer_Stop();
    if (VehicleImu_CalibrateGyro())
    {
        (void)BoardBuzzer_Play(KEY_BEEP_ON_MS, 100U, 2U);
        return 1U;
    }

    (void)BoardBuzzer_Play(WARNING_BEEP_ON_MS,
                           WARNING_BEEP_OFF_MS,
                           3U);
    return 0U;
}

/* KEY 帧中的六个掩码均以 bit0/bit1/bit2 对应 K1/K2/K3。 */
static void SendKeyFrame(BoardKeyEvents events)
{
    char line[96];

    snprintf(line, sizeof(line), "KEY,%lu,%u,%u,%u,%u,%u\r\n",
             (unsigned long)now_ms,
             (unsigned int)events.pressed_mask,
             (unsigned int)events.released_mask,
             (unsigned int)events.short_press_mask,
             (unsigned int)events.long_press_mask,
             (unsigned int)BoardIo_GetPressedMask());
    SendText(line);
}

static void SendModeFrame(void)
{
    const VehicleGrayState *gray = VehicleGray_GetState();
    const VehicleBatteryState *battery = VehicleBattery_GetState();
    uint8_t switches = BoardIo_GetSwitchDownMask();
    char line[96];

    snprintf(line, sizeof(line), "MOD,%lu,%u,%u,%u,%u,%u,%u,%lu\r\n",
             (unsigned long)now_ms,
             (unsigned int)app.track_color_mode,
             (unsigned int)((switches >> BOARD_SWITCH_1) & 1U),
             (unsigned int)((switches >> BOARD_SWITCH_2) & 1U),
             (unsigned int)app.local_line_running,
             (unsigned int)gray->white_valid,
             (unsigned int)gray->black_valid,
             (unsigned long)(battery->valid ? battery->voltage_mv : 0U));
    SendText(line);
}

static void ApplyTrackModeFromSwitch(uint8_t reset_calibration)
{
    TrackColorMode mode =
        (BoardIo_GetSwitchDownMask() & (uint8_t)(1U << BOARD_SWITCH_1)) ?
        TRACK_MODE_BLACK_ON_WHITE : TRACK_MODE_WHITE_ON_BLUE;

    if (!reset_calibration && mode == app.track_color_mode) return;
    if (reset_calibration)
    {
        StopControl();
        BoardBuzzer_Stop();
        VehicleLine_Reset();
        VehicleGray_ResetCalibration();
    }
    app.track_color_mode = mode;
    SendModeFrame();
    if (reset_calibration) SendGrayCalibration();
}

static uint8_t BeginLocalLineControl(void)
{
    if (!BeginLineControl(LOCAL_LINE_SPEED_MM_S))
    {
        BoardBuzzer_Stop();
        (void)BoardBuzzer_Play(WARNING_BEEP_ON_MS,
                               WARNING_BEEP_OFF_MS,
                               3U);
        SendText("ERR,LOCAL_LINE_NOT_READY\r\n");
        SendModeFrame();
        return 0U;
    }

    app.local_line_running = 1U;
    SendText("ACK,LOCAL_LINE,1\r\n");
    SendModeFrame();
    return 1U;
}

static void HandleBoardIo(void)
{
    BoardKeyEvents events;
    const VehicleGrayState *gray;
    uint8_t switch_changes;
    uint8_t key_activity;
    uint8_t pressed_start;
    uint8_t released_start;
    uint8_t long_start;
    uint8_t short_mask;
    uint8_t suppressed;

    BoardIo_Update(now_ms);
    events = BoardIo_TakeKeyEvents();
    switch_changes = BoardIo_TakeSwitchChangedMask();
    key_activity = events.pressed_mask | events.released_mask |
                   events.short_press_mask | events.long_press_mask;
    if (key_activity == 0U && switch_changes == 0U)
    {
        return;
    }

    if (switch_changes & (uint8_t)(1U << BOARD_SWITCH_1))
    {
        ApplyTrackModeFromSwitch(1U);
    }
    else if (switch_changes != 0U)
    {
        /* 第二位拨码已完成读取与上报，暂不绑定用户尚未指定的车辆功能。 */
        SendModeFrame();
    }
    if (key_activity == 0U) return;

    SendKeyFrame(events);
    pressed_start = events.pressed_mask & LOCAL_START_KEY_MASK;
    released_start = events.released_mask & LOCAL_START_KEY_MASK;
    suppressed = app.suppress_short_mask;

    /* 本地运行时 K1/K2 的按下沿立即停车，不等待释放或长按计时。 */
    if (app.local_line_running && pressed_start != 0U)
    {
        app.suppress_short_mask |= pressed_start;
        StopControl();
        BoardBuzzer_Stop();
        SendText("ACK,LOCAL_LINE,0\r\n");
        SendModeFrame();
        return;
    }
    if (app.local_line_running) return;

    gray = VehicleGray_GetState();
    if (pressed_start != 0U && gray->normalization_valid)
    {
        if (app.motor_mode != MOTOR_MODE_IDLE) StopControl();
        app.local_start_arming = 1U;
        app.local_start_key_mask |= pressed_start;
        BoardBuzzer_SetContinuous(1U);
    }

    if (app.local_start_arming &&
        (released_start & app.local_start_key_mask) != 0U)
    {
        app.local_start_arming = 0U;
        app.local_start_key_mask = 0U;
        BoardBuzzer_SetContinuous(0U);
    }

    long_start = events.long_press_mask & app.local_start_key_mask &
                 (uint8_t)~suppressed;
    if (app.local_start_arming && long_start != 0U)
    {
        app.local_start_arming = 0U;
        app.local_start_key_mask = 0U;
        BoardBuzzer_SetContinuous(0U);
        (void)BeginLocalLineControl();
        return;
    }

    /* K3 长按仍保留为本地急停；用于停车的 K1/K2 后续长按事件被抑制。 */
    if ((events.long_press_mask & (uint8_t)~suppressed) != 0U)
    {
        StopControl();
        BoardBuzzer_Stop();
        (void)BoardBuzzer_Play(WARNING_BEEP_ON_MS,
                               WARNING_BEEP_OFF_MS,
                               3U);
        return;
    }

    short_mask = events.short_press_mask & (uint8_t)~suppressed;
    app.suppress_short_mask &= (uint8_t)~events.released_mask;
    if (short_mask & (uint8_t)(1U << BOARD_KEY_K1))
    {
        CalibrateGrayReference(1U);
    }
    else if (short_mask & (uint8_t)(1U << BOARD_KEY_K2))
    {
        CalibrateGrayReference(0U);
    }
    else if (short_mask & (uint8_t)(1U << BOARD_KEY_K3))
    {
        (void)CalibrateGyroWithFeedback();
    }
}

static void ProcessCommand(char *line)
{
    int throttle;
    int steering;
    int left;
    int right;
    int kp;
    int ki;
    int kd;
    int kff;
    int yaw_pid_fields;
    int yaw_enable;
    int max_yaw_rate;
    int heading_enable;
    int heading_kp_milli;
    int heading_kd_milli;
    int heading_kff_milli;
    int heading_max_rate;
    int heading_target10;
    int beep_ms;
    int line_diff;
    int line_enable;
    int line_speed;
    int square_command;
    char response[96];
    const VehicleImuState *imu = VehicleImu_GetState();

    last_rx_ms = now_ms;
    app.link_active = 1U;

    /*
     * 本地发车倒计时和运行期间，蓝牙只允许 PING 维持链路显示；所有会改变
     * 车辆状态的远程命令都被拒绝。这样网页尚未停下的周期 DRV/LINE 命令
     * 不会打断 500 ms 蜂鸣倒计时，也不会干扰已经开始的本地寻线。
     */
    if (app.local_start_arming || app.local_line_running)
    {
        if (strcmp(line, "PING") != 0) SendModeFrame();
        return;
    }

    /* 正方形测试只接受续命、停止和心跳，避免残留 DRV/LINE 命令打断轨迹。 */
    if (VehicleSquare_GetState()->active &&
        strncmp(line, "SQUARE,", 7U) != 0 &&
        strcmp(line, "STOP") != 0 && strcmp(line, "PING") != 0)
    {
        SendSquareTelemetry();
        return;
    }

    if (sscanf(line, "SQUARE,%d", &square_command) == 1)
    {
        const VehicleEncoderState *encoder = VehicleEncoder_GetState();

        if (square_command == 0)
        {
            StopControl();
            SendText("ACK,SQUARE,0\r\n");
            SendSquareTelemetry();
        }
        else if (square_command == 1)
        {
            if (!VehicleSquare_GetState()->active)
            {
                if (!imu->ok)
                {
                    SendText("ERR,SQUARE_NOT_READY\r\n");
                    return;
                }
                StopControl();
                app.yaw_control_enabled = 1U;
                app.heading_control_enabled = 1U;
                VehicleSquare_Start(now_ms, encoder->total_left,
                                    encoder->total_right, imu->yaw_deg);
                SendText("ACK,SQUARE,1,1000,90,4,200\r\n");
                SendSquareTelemetry();
                (void)VehicleSquare_TakeStatusChanged();
            }
            last_motor_command_ms = now_ms;
        }
        else if (square_command == 2)
        {
            /* 2 只续命，测试结束后不会因网页仍在发送而重新启动。 */
            if (VehicleSquare_GetState()->active)
                last_motor_command_ms = now_ms;
        }
        else
        {
            SendText("ERR,SQUARE_RANGE,0,1,2\r\n");
        }
    }
    else if (sscanf(line, "DRV,%d,%d", &throttle, &steering) == 2)
    {
        int32_t forward_mm_s;

        throttle = (int)ClampInt32(throttle, -100, 100);
        steering = (int)ClampInt32(steering, -100, 100);
        forward_mm_s = throttle * MAX_TEST_SPEED_MM_S / 100;
        /* 遥控模式自动启用已锁定航向环；手动打方向时仍由角速度环直接接管。 */
        if (imu->ok)
        {
            app.yaw_control_enabled = 1U;
            app.heading_control_enabled = 1U;
        }
        if (steering == 0 && throttle != 0 && app.heading_control_enabled &&
            app.yaw_control_enabled && imu->ok)
        {
            uint8_t reset_reference =
                (!app.heading_hold_active || app.motor_mode != MOTOR_MODE_SPEED) ?
                1U : 0U;
            float target = reset_reference ? imu->yaw_deg : app.target_heading_deg;

            BeginHeadingControl(forward_mm_s, target, reset_reference);
        }
        else
        {
            app.heading_hold_active = 0U;
            UpdateEnabledControlLoops();
            BeginSpeedControl(forward_mm_s,
                              -steering * app.max_yaw_rate_dps * 10 / 100,
                              -steering * MAX_TEST_SPEED_MM_S / 100);
        }
        last_motor_command_ms = now_ms;
    }
    else if (sscanf(line, "MOTOR,%d,%d", &left, &right) == 2)
    {
        app.heading_hold_active = 0U;
        UpdateEnabledControlLoops();
        BeginRawControl(left, right);
        last_motor_command_ms = now_ms;
    }
    else if (sscanf(line, "PIDL,%d,%d,%d", &kp, &ki, &kd) == 3)
    {
        SendText("ERR,SPEED_PID_LOCKED,4000,800,0\r\n");
    }
    else if (sscanf(line, "PIDR,%d,%d,%d", &kp, &ki, &kd) == 3)
    {
        SendText("ERR,SPEED_PID_LOCKED,4000,800,0\r\n");
    }
    else if (sscanf(line, "PID,%d,%d,%d", &kp, &ki, &kd) == 3)
    {
        SendText("ERR,SPEED_PID_LOCKED,4000,800,0\r\n");
    }
    else if (sscanf(line, "YAW,%d", &yaw_enable) == 1)
    {
        StopControl();
        app.yaw_control_enabled = (yaw_enable != 0 && imu->ok) ? 1U : 0U;
        if (!app.yaw_control_enabled) app.heading_control_enabled = 0U;
        UpdateEnabledControlLoops();
        snprintf(response, sizeof(response), "ACK,YAW,%u\r\n",
                 (unsigned int)app.yaw_control_enabled);
        SendText(response);
    }
    else if (sscanf(line, "YAWRATE,%d", &max_yaw_rate) == 1)
    {
        if (max_yaw_rate >= 10 && max_yaw_rate <= 360)
        {
            StopControl();
            app.max_yaw_rate_dps = max_yaw_rate;
            vehicle_control.max_yaw_rate_dps = (float)max_yaw_rate;
            snprintf(response, sizeof(response),
                     "ACK,YAWRATE,%d\r\n", max_yaw_rate);
            SendText(response);
        }
        else
        {
            SendText("ERR,YAWRATE_RANGE\r\n");
        }
    }
    else if (strncmp(line, "YAWPID,", 7U) == 0)
    {
        kff = app.yaw_pid_kff_micro;
        yaw_pid_fields = sscanf(line, "YAWPID,%d,%d,%d,%d",
                                &kp, &ki, &kd, &kff);
        if ((yaw_pid_fields == 3 || yaw_pid_fields == 4) &&
            kp >= 0 && kp <= 100000 && ki >= 0 && ki <= 100000 &&
            kd >= 0 && kd <= 100000 && kff >= 0 && kff <= 100000)
        {
            StopControl();
            ConfigureYawPid(kp, ki, kd, kff);
            snprintf(response, sizeof(response),
                     "ACK,YAWPID,%d,%d,%d,%d\r\n", kp, ki, kd, kff);
            SendText(response);
        }
        else
        {
            SendText("ERR,YAWPID_RANGE\r\n");
        }
    }
    else if (sscanf(line, "HEADPID,%d,%d,%d,%d",
                    &heading_kp_milli, &heading_kd_milli,
                    &heading_kff_milli, &heading_max_rate) == 4)
    {
        (void)heading_kp_milli;
        (void)heading_kd_milli;
        (void)heading_kff_milli;
        (void)heading_max_rate;
        SendText("ERR,HEADING_PID_LOCKED,4000,300,1000,80\r\n");
    }
    else if (sscanf(line, "HEADSET,%d", &heading_target10) == 1)
    {
        if (heading_target10 >= -1800 && heading_target10 <= 1800 &&
            app.heading_control_enabled && app.yaw_control_enabled && imu->ok)
        {
            float target = (float)heading_target10 / 10.0f;
            uint8_t reset_reference =
                (!app.heading_hold_active || app.motor_mode != MOTOR_MODE_SPEED ||
                 fabsf(WrapAngleDegrees(target - app.target_heading_deg)) >= 0.05f) ?
                1U : 0U;

            BeginHeadingControl(0, target, reset_reference);
            last_motor_command_ms = now_ms;
            if (reset_reference)
            {
                snprintf(response, sizeof(response),
                         "ACK,HEADSET,%d\r\n", heading_target10);
                SendText(response);
            }
        }
        else
        {
            SendText("ERR,HEAD_NOT_READY\r\n");
        }
    }
    else if (sscanf(line, "HEAD,%d", &heading_enable) == 1)
    {
        StopControl();
        app.heading_control_enabled =
            (heading_enable != 0 && app.yaw_control_enabled && imu->ok) ? 1U : 0U;
        app.target_heading_deg = imu->yaw_deg;
        UpdateEnabledControlLoops();
        snprintf(response, sizeof(response), "ACK,HEAD,%u\r\n",
                 (unsigned int)app.heading_control_enabled);
        SendText(response);
    }
    else if (strcmp(line, "HEADCFG") == 0)
    {
        snprintf(response, sizeof(response),
                 "ACK,HEADCFG,%u,%ld,%ld,%ld,%ld\r\n",
                 (unsigned int)app.heading_control_enabled,
                 (long)app.heading_pid_kp_milli,
                 (long)app.heading_pid_kd_milli,
                 (long)app.heading_pid_kff_milli,
                 (long)app.max_heading_rate_dps);
        SendText(response);
    }
    else if (sscanf(line, "LINEPID,%d,%d,%d", &kp, &ki, &kd) == 3)
    {
        if (kp >= 0 && kp <= LINE_PID_GAIN_MAX_MILLI &&
            ki >= 0 && ki <= LINE_PID_GAIN_MAX_MILLI &&
            kd >= 0 && kd <= LINE_PID_GAIN_MAX_MILLI)
        {
            StopControl();
            ConfigureLinePid(kp, ki, kd);
            snprintf(response, sizeof(response),
                     "ACK,LINEPID,%d,%d,%d\r\n", kp, ki, kd);
            SendText(response);
        }
        else
        {
            SendText("ERR,LINEPID_RANGE\r\n");
        }
    }
    else if (sscanf(line, "LINEDIFF,%d", &line_diff) == 1)
    {
        if (line_diff >= 0 && line_diff <= LINE_DIFF_MAX_MILLI)
        {
            StopControl();
            app.line_diff_milli = line_diff;
            snprintf(response, sizeof(response),
                     "ACK,LINEDIFF,%d\r\n", line_diff);
            SendText(response);
        }
        else
        {
            SendText("ERR,LINEDIFF_RANGE\r\n");
        }
    }
    else if (strcmp(line, "LINECFG") == 0)
    {
        snprintf(response, sizeof(response),
                 "ACK,LINECFG,%ld,%ld,%ld,%ld\r\n",
                 (long)app.line_pid_kp_milli,
                 (long)app.line_pid_ki_milli,
                 (long)app.line_pid_kd_milli,
                 (long)app.line_diff_milli);
        SendText(response);
    }
    else if (sscanf(line, "LINE,%d,%d", &line_enable, &line_speed) == 2)
    {
        if (line_enable == 0)
        {
            StopControl();
            SendText("ACK,LINE,0\r\n");
        }
        else if (line_enable == 1 &&
                 line_speed >= LINE_SPEED_MIN_MM_S &&
                 line_speed <= LINE_SPEED_MAX_MM_S)
        {
            if (BeginLineControl(line_speed))
            {
                last_motor_command_ms = now_ms;
            }
            else
            {
                SendText("ERR,LINE_NOT_READY\r\n");
            }
        }
        else
        {
            StopControl();
            SendText("ERR,LINE_RANGE\r\n");
        }
    }
    else if (sscanf(line, "BEEP,%d", &beep_ms) == 1)
    {
        if (beep_ms < 10 || beep_ms > 5000)
        {
            SendText("ERR,BEEP_RANGE,10,5000\r\n");
        }
        else if (BoardBuzzer_Play((uint16_t)beep_ms, 0U, 1U))
        {
            snprintf(response, sizeof(response), "ACK,BEEP,%d\r\n", beep_ms);
            SendText(response);
        }
        else
        {
            SendText("ERR,BUZZER_BUSY\r\n");
        }
    }
    else if (strcmp(line, "KEYS") == 0)
    {
        BoardKeyEvents no_events = {0};
        SendKeyFrame(no_events);
    }
    else if (strcmp(line, "STOP") == 0)
    {
        StopControl();
        app.link_active = 0U;
        SendText("ACK,STOP\r\n");
    }
    else if (strcmp(line, "PING") == 0)
    {
        /* 心跳只维护链路显示，不刷新 500 ms 电机命令看门狗。 */
    }
    else if (strcmp(line, "ZERO") == 0 || strcmp(line, "ENCZERO") == 0)
    {
        StopControl();
        VehicleEncoder_Reset();
        VehicleImu_ResetYaw();
        SendText("ACK,ZERO\r\n");
    }
    else if (strcmp(line, "IMUZERO") == 0)
    {
        if (CalibrateGyroWithFeedback())
        {
            SendText("ACK,IMUZERO\r\n");
        }
        else
        {
            SendText("ERR,IMU_NOT_READY\r\n");
        }
    }
    else if (strcmp(line, "GRAYWHITE") == 0)
    {
        CalibrateGrayReference(1U);
    }
    else if (strcmp(line, "GRAYBLACK") == 0)
    {
        CalibrateGrayReference(0U);
    }
    else if (strcmp(line, "GRAYCAL") == 0 || strcmp(line, "GRAY") == 0)
    {
        SendGrayCalibration();
    }
    else if (strcmp(line, "HELP") == 0)
    {
        SendText("ACK,DRV MOTOR SQUARE LINE YAW YAWRATE YAWPID HEAD HEADPID_LOCKED HEADSET HEADCFG LINEPID LINEDIFF LINECFG PID_LOCKED STOP ZERO ENCZERO IMUZERO GRAY GRAYWHITE GRAYBLACK BEEP KEYS PING\r\n");
    }
    else
    {
        SendText("ERR,UNKNOWN\r\n");
    }
}

/* 盲转只临时放宽到单侧轮停转，不允许内轮反转；正常跟踪仍使用网页参数。 */
static int32_t GetEffectiveLineDiffMilli(void)
{
    const VehicleLineState *line = VehicleLine_GetState();

    return (line->mode == VEHICLE_LINE_BLIND_TURN) ?
           LINE_BLIND_DIFF_MILLI : app.line_diff_milli;
}

static void SendLineTelemetry(void)
{
    const VehicleLineState *line = VehicleLine_GetState();
    const VehicleGrayState *gray = VehicleGray_GetState();
    const VehicleImuState *imu = VehicleImu_GetState();
    uint32_t recovery_ms = 0U;
    char frame[192];

    if (line->mode == VEHICLE_LINE_GAP_HOLD ||
        line->mode == VEHICLE_LINE_BLIND_TURN)
    {
        recovery_ms = now_ms - line->recovery_started_ms;
    }
    snprintf(frame, sizeof(frame),
             "LIN,%lu,%u,%u,%u,%u,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%lu,%u,%lu,%u\r\n",
             (unsigned long)now_ms,
             (unsigned int)(app.motor_mode == MOTOR_MODE_LINE),
             (unsigned int)gray->normalization_valid,
             (unsigned int)line->visible,
             (unsigned int)line->lost,
             (long)RoundFloatToInt32(line->raw_error_percent * 10.0f),
             (long)RoundFloatToInt32(line->filtered_error_percent * 10.0f),
             (long)RoundFloatToInt32(line->pid_output),
             (long)RoundFloatToInt32(line->target_yaw_rate_dps * 10.0f),
             (long)RoundFloatToInt32(imu->yaw_rate_dps * 10.0f),
             (long)RoundFloatToInt32(vehicle_output.wheel_correction_mm_s),
             (long)app.target_left_mm_s,
             (long)app.target_right_mm_s,
             (long)app.target_forward_mm_s,
             (long)GetEffectiveLineDiffMilli(),
             (unsigned long)line->normalized_sum,
             (unsigned int)line->mode,
             (unsigned long)recovery_ms,
             (unsigned int)line->active_count);
    SendText(frame);
}

/* 每次灰度更新后运行一次方向外环，并把输出写成角速度环目标。 */
static void SendSquareTelemetry(void)
{
    const VehicleSquareState *square = VehicleSquare_GetState();
    uint8_t displayed_leg = square->leg < 4U ? (uint8_t)(square->leg + 1U) : 4U;
    char frame[112];

    snprintf(frame, sizeof(frame),
             "SQR,%lu,%u,%u,%u,%ld,%ld,%ld\r\n",
             (unsigned long)now_ms,
             (unsigned int)square->active,
             (unsigned int)square->phase,
             (unsigned int)displayed_leg,
             (long)square->progress_mm,
             (long)square->remaining_mm,
             (long)RoundFloatToInt32(square->target_heading_deg * 10.0f));
    SendText(frame);
}

/*
 * 状态机只生成前进速度和绝对航向，执行仍统一经过
 * 航向环 -> 角速度环 -> 左右轮速度环。
 */
static void UpdateSquareControl(void)
{
    const VehicleEncoderState *encoder = VehicleEncoder_GetState();
    const VehicleImuState *imu = VehicleImu_GetState();
    const VehicleSquareState *square;
    uint8_t target_changed;
    uint8_t status_changed;

    if (!VehicleSquare_GetState()->active) return;
    VehicleSquare_Update(now_ms,
                         encoder->total_left, encoder->total_right,
                         encoder->filtered_left_mm_s_float,
                         encoder->filtered_right_mm_s_float,
                         imu->yaw_deg, imu->yaw_rate_dps);
    target_changed = VehicleSquare_TakeTargetChanged();
    status_changed = VehicleSquare_TakeStatusChanged();
    square = VehicleSquare_GetState();

    if (!square->active)
    {
        StopControl();
        if (status_changed) SendSquareTelemetry();
        return;
    }

    BeginHeadingControl(square->forward_command_mm_s,
                        square->target_heading_deg,
                        target_changed);
    if (status_changed) SendSquareTelemetry();
}

static uint8_t UpdateLineDirection(void)
{
    const VehicleLineState *line;
    uint8_t was_local;

    if (app.motor_mode != MOTOR_MODE_LINE) return 1U;
    if (!VehicleLine_Update(VehicleGray_GetState(), now_ms))
    {
        was_local = app.local_line_running;
        SendLineTelemetry();
        StopControl();
        SendText("ERR,LINE_LOST\r\n");
        if (was_local) SendModeFrame();
        return 0U;
    }

    line = VehicleLine_GetState();
    app.target_yaw_rate10 =
        RoundFloatToInt32(line->target_yaw_rate_dps * 10.0f);
    return 1U;
}

static void UpdateSpeedControl(float dt_s)
{
    const VehicleEncoderState *encoder = VehicleEncoder_GetState();
    const VehicleImuState *imu = VehicleImu_GetState();
    VehicleCascadeInput input;

    if (app.motor_mode != MOTOR_MODE_SPEED &&
        app.motor_mode != MOTOR_MODE_LINE) return;

    memset(&input, 0, sizeof(input));
    input.requested_forward_speed_mm_s = (float)app.target_forward_mm_s;
    input.requested_heading_deg = app.target_heading_deg;
    input.requested_yaw_rate_dps = (float)app.target_yaw_rate10 / 10.0f;
    input.direct_wheel_correction_mm_s =
        (float)app.fallback_wheel_correction_mm_s;
    input.measured_left_speed_mm_s = encoder->filtered_left_mm_s_float;
    input.measured_right_speed_mm_s = encoder->filtered_right_mm_s_float;
    input.measured_heading_deg = imu->yaw_deg;
    input.measured_yaw_rate_dps = imu->yaw_rate_dps;
    if (app.motor_mode == MOTOR_MODE_LINE)
    {
        input.max_wheel_correction_mm_s =
            fabsf((float)app.target_forward_mm_s) *
            (float)GetEffectiveLineDiffMilli() / 1000.0f;
        input.wheel_correction_limit_valid = 1U;
    }
    VehicleCascade_Step(&vehicle_control, &input, dt_s, &vehicle_output);

    app.target_left_mm_s =
        RoundFloatToInt32(vehicle_output.target_left_speed_mm_s);
    app.target_right_mm_s =
        RoundFloatToInt32(vehicle_output.target_right_speed_mm_s);
    app.error_left_mm_s = app.target_left_mm_s - encoder->filtered_left_mm_s;
    app.error_right_mm_s = app.target_right_mm_s - encoder->filtered_right_mm_s;
    if (vehicle_output.motor_output_valid)
    {
        VehicleMotor_Set(RoundFloatToInt32(vehicle_output.left_motor_percent),
                         RoundFloatToInt32(vehicle_output.right_motor_percent));
    }
}

static void SendTelemetry(void)
{
    const VehicleMotorState *motor = VehicleMotor_GetState();
    const VehicleEncoderState *encoder = VehicleEncoder_GetState();
    const VehicleImuState *imu = VehicleImu_GetState();
    const VehicleGrayState *gray = VehicleGray_GetState();
    const VehicleBatteryState *battery = VehicleBattery_GetState();
    char line[384];
    int32_t target_left = app.target_left_mm_s;
    int32_t target_right = app.target_right_mm_s;
    int32_t yaw_rate10 = RoundFloatToInt32(imu->yaw_rate_dps * 10.0f);
    uint8_t active =
        VehicleGray_CountLineChannels(LINE_EDGE_TARGET_MIN);
    if (app.motor_mode != MOTOR_MODE_SPEED &&
        app.motor_mode != MOTOR_MODE_LINE)
    {
        target_left = motor->left_percent * MAX_TEST_SPEED_MM_S / 100;
        target_right = motor->right_percent * MAX_TEST_SPEED_MM_S / 100;
    }

    /* TEL 字段顺序必须保持兼容，网页依赖固定下标读取目标与实际速度。 */
    snprintf(line, sizeof(line),
             "TEL,%lu,%u,%u,%d,%ld,%ld,%ld,%ld,%d,%d,%ld,%ld,%ld,%ld,%u,%u,%ld,%ld,%ld,%ld,%u,%u,%lu\r\n",
             (unsigned long)now_ms,
             (unsigned int)((motor->left_percent != 0 ||
                             motor->right_percent != 0) ? 1U : 0U),
             (unsigned int)app.link_active,
             imu->yaw10,
             (long)encoder->filtered_left_mm_s,
             (long)encoder->filtered_right_mm_s,
             (long)target_left,
             (long)target_right,
             motor->left_percent * 168,
             motor->right_percent * 168,
             (long)RoundFloatToInt32(vehicle_output.target_yaw_rate_dps * 10.0f),
             (long)yaw_rate10,
             (long)RoundFloatToInt32(vehicle_output.yaw_error_dps * 10.0f),
             (long)RoundFloatToInt32(vehicle_output.wheel_correction_mm_s),
             (unsigned int)app.yaw_control_enabled,
             (unsigned int)imu->ok,
             (long)RoundFloatToInt32(vehicle_output.yaw_feedforward_mm_s),
             (long)RoundFloatToInt32(app.target_heading_deg * 10.0f),
             (long)RoundFloatToInt32(vehicle_output.heading_error_deg * 10.0f),
             (long)RoundFloatToInt32(vehicle_output.heading_output_dps * 10.0f),
             (unsigned int)app.heading_control_enabled,
             (unsigned int)app.heading_hold_active,
             (unsigned long)(battery->valid ? battery->voltage_mv : 0U));
    SendText(line);

    /* STA 同样保留 V15 的占位字段，避免旧网页状态页整体错位。 */
    snprintf(line, sizeof(line),
             "STA,%lu,%d,%d,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,0,0,0,0,0,0,0,0,30,%u,90,"
             "%u,%u,%u,%u,%u,%u,%u,%u,60,%u,%u,%u,900,500,"
             "%u,%u,%u,%u,%u,%u,%u,%u,%u,%ld,%ld,%ld,%ld,%ld,"
             "%ld,%ld,%ld,%ld,%u,%ld,%ld,%ld,%ld\r\n",
             (unsigned long)now_ms,
             imu->pitch10,
             imu->roll10,
             (long)encoder->total_left,
             (long)encoder->total_right,
             (long)app.pid_left_kp,
             (long)app.pid_left_ki,
             (long)app.pid_left_kd,
             (long)app.pid_right_kp,
             (long)app.pid_right_ki,
             (long)app.pid_right_kd,
             (unsigned int)active,
             gray->raw[0], gray->raw[1], gray->raw[2], gray->raw[3],
             gray->raw[4], gray->raw[5], gray->raw[6], gray->raw[7],
             (unsigned int)active,
             (unsigned int)gray->white_valid,
             (unsigned int)gray->black_valid,
             gray->raw[0], gray->raw[1], gray->raw[2], gray->raw[3],
             gray->raw[4], gray->raw[5], gray->raw[6], gray->raw[7],
             (unsigned int)app.yaw_control_enabled,
             (long)app.max_yaw_rate_dps,
             (long)app.yaw_pid_kp_micro,
             (long)app.yaw_pid_ki_micro,
             (long)app.yaw_pid_kd_micro,
             (long)app.yaw_pid_kff_micro,
             (long)app.line_pid_kp_milli,
             (long)app.line_pid_ki_milli,
             (long)app.line_pid_kd_milli,
             (long)app.line_diff_milli,
             (unsigned int)app.heading_control_enabled,
             (long)app.max_heading_rate_dps,
             (long)app.heading_pid_kp_milli,
             (long)app.heading_pid_kd_milli,
             (long)app.heading_pid_kff_milli);
    SendText(line);
}

static void SendSpeedTelemetry(void)
{
    const VehicleMotorState *motor = VehicleMotor_GetState();
    const VehicleEncoderState *encoder = VehicleEncoder_GetState();
    const VehicleImuState *imu = VehicleImu_GetState();
    char line[448];

    snprintf(line, sizeof(line),
             "SPD,%lu,1,%ld,%ld,%ld,%ld,%ld,%d,%ld,%ld,%ld,%ld,%ld,%d,"
             "%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,"
             "%u,%ld,%ld,%ld,%ld,%ld,"
             "%ld,%ld,%ld,%ld,%ld,%ld,%u,%u,%ld,%ld,%ld,%ld\r\n",
             (unsigned long)now_ms,
             (long)app.target_left_mm_s,
             (long)encoder->filtered_left_mm_s,
             (long)app.error_left_mm_s,
             (long)RoundFloatToInt32(vehicle_output.left_feedforward_percent * 10.0f),
             (long)RoundFloatToInt32(vehicle_output.left_pid_percent * 10.0f),
             motor->left_percent,
             (long)app.target_right_mm_s,
             (long)encoder->filtered_right_mm_s,
             (long)app.error_right_mm_s,
             (long)RoundFloatToInt32(vehicle_output.right_feedforward_percent * 10.0f),
             (long)RoundFloatToInt32(vehicle_output.right_pid_percent * 10.0f),
             motor->right_percent,
             (long)app.pid_left_kp,
             (long)app.pid_left_ki,
             (long)app.pid_left_kd,
             (long)app.pid_right_kp,
             (long)app.pid_right_ki,
             (long)app.pid_right_kd,
             (long)RoundFloatToInt32(vehicle_output.target_yaw_rate_dps * 10.0f),
             (long)RoundFloatToInt32(imu->yaw_rate_dps * 10.0f),
             (long)RoundFloatToInt32(vehicle_output.yaw_error_dps * 10.0f),
             (long)RoundFloatToInt32(vehicle_output.yaw_feedforward_mm_s),
             (long)RoundFloatToInt32(vehicle_output.yaw_pid_mm_s),
             (long)RoundFloatToInt32(vehicle_output.wheel_correction_mm_s),
             (unsigned int)app.yaw_control_enabled,
             (long)app.max_yaw_rate_dps,
             (long)app.yaw_pid_kp_micro,
             (long)app.yaw_pid_ki_micro,
             (long)app.yaw_pid_kd_micro,
             (long)app.yaw_pid_kff_micro,
             (long)RoundFloatToInt32(app.target_heading_deg * 10.0f),
             (long)RoundFloatToInt32(vehicle_output.heading_reference_deg * 10.0f),
             (long)RoundFloatToInt32(vehicle_output.heading_reference_rate_dps * 10.0f),
             (long)imu->yaw10,
             (long)RoundFloatToInt32(vehicle_output.heading_error_deg * 10.0f),
             (long)RoundFloatToInt32(vehicle_output.heading_output_dps * 10.0f),
             (unsigned int)app.heading_control_enabled,
             (unsigned int)app.heading_hold_active,
             (long)app.max_heading_rate_dps,
             (long)app.heading_pid_kp_milli,
             (long)app.heading_pid_kd_milli,
             (long)app.heading_pid_kff_milli);
    SendText(line);
}

static void SendDebug(void)
{
    const VehicleImuState *imu = VehicleImu_GetState();
    char line[96];

    snprintf(line, sizeof(line), "DBG,%u,%d,%d,%d,%d,%d,%d,%u,%u\r\n",
             imu->id,
             imu->ax,
             imu->ay,
             imu->az,
             imu->gx,
             imu->gy,
             imu->gz,
             imu->mosi_pin_number,
             imu->miso_pin_number);
    SendText(line);
}

void DiansaiApp_Init(void)
{
    const VehicleImuState *imu;

    /* 将 SysTick 配成 1 ms，后续调度均使用无符号时间差，允许计数自然回绕。 */
    DL_SYSTICK_disable();
    DL_SYSTICK_init(80000U);
    DL_SYSTICK_enableInterrupt();
    DL_SYSTICK_enable();
    __disable_irq();
    app_tick_ms = 0U;
    __enable_irq();

    memset(&app, 0, sizeof(app));
    memset(&vehicle_output, 0, sizeof(vehicle_output));
    now_ms = 0U;
    last_loop_ms = 0U;

    WheeltecLink_Init();
    BoardIo_Init(now_ms);
    app.track_color_mode =
        (BoardIo_GetSwitchDownMask() & (uint8_t)(1U << BOARD_SWITCH_1)) ?
        TRACK_MODE_BLACK_ON_WHITE : TRACK_MODE_WHITE_ON_BLUE;
    VehicleMotor_Init();
    VehicleEncoder_Init();
    VehicleSquare_Init();
    VehicleGray_Init();
    VehicleLine_Init();
    VehicleBattery_Init();
    VehicleBattery_Update();
    CascadeControlInit();
    StopControl();

    (void)VehicleImu_Init();
    imu = VehicleImu_GetState();
    app.yaw_control_enabled = imu->ok;
    app.heading_control_enabled = imu->ok;
    app.target_heading_deg = imu->yaw_deg;
    UpdateEnabledControlLoops();
    if (imu->ok && VehicleImu_CalibrateGyro())
    {
        (void)BoardBuzzer_Play(80U, 100U, 2U);
    }
    else
    {
        (void)BoardBuzzer_Play(300U, 150U, 3U);
    }

    now_ms = app_tick_ms;
    last_loop_ms = now_ms;
    last_gray_ms = now_ms;
    last_battery_ms = now_ms;
    last_motor_command_ms = now_ms;
    last_rx_ms = now_ms;
    /* 保持 V15 行为：IMU 标定结束后的第一轮立即发送状态和诊断帧。 */
    last_telemetry_ms = 0U;
    last_speed_telemetry_ms = 0U;
    last_line_telemetry_ms = 0U;
    last_debug_ms = 0U;

    SendText("BOOT,MSPM0G3507_DIANSAI_TEST,V24,WHEELTEC,9600\r\n");
    SendText("ACK,READY,V24,LOCKED_REMOTE_HEADING_SQUARE_DMA_TX; send HELP for commands\r\n");
    SendModeFrame();
}

void SysTick_Handler(void)
{
    app_tick_ms++;
}

void DiansaiApp_Run(void)
{
    const VehicleMotorState *motor;
    const VehicleEncoderState *encoder;
    uint32_t tick;
    uint32_t elapsed_ms;
    uint8_t stationary;

    /* 串口收发不受 10 ms 控制周期限制，降低 9600 波特率链路的接收延迟。 */
    WheeltecLink_ServiceTx();
    WheeltecLink_Poll(ProcessCommand);

    tick = app_tick_ms;
    elapsed_ms = tick - last_loop_ms;
    if (elapsed_ms < APP_LOOP_MS) return;
    last_loop_ms = tick;
    now_ms = tick;

    HandleBoardIo();
    VehicleEncoder_Update(elapsed_ms);
    motor = VehicleMotor_GetState();
    encoder = VehicleEncoder_GetState();
    stationary = (motor->left_percent == 0 && motor->right_percent == 0 &&
                  encoder->filtered_left_mm_s == 0 &&
                  encoder->filtered_right_mm_s == 0) ? 1U : 0U;
    VehicleImu_Update(now_ms, stationary);

    /*
     * 灰度方向环必须先于角速度环和速度环更新。这样同一控制周期中，
     * 新灰度偏差先生成目标角速度，随后角速度环立即把它换成左右轮差速。
     */
    if (now_ms - last_gray_ms >= GRAY_PERIOD_MS)
    {
        last_gray_ms = now_ms;
        VehicleGray_Update();
        (void)UpdateLineDirection();
    }
    if (now_ms - last_battery_ms >= BATTERY_PERIOD_MS)
    {
        last_battery_ms = now_ms;
        VehicleBattery_Update();
    }

    if (app.motor_mode != MOTOR_MODE_IDLE && !app.local_line_running &&
        now_ms - last_motor_command_ms > COMMAND_TIMEOUT_MS)
    {
        if (VehicleSquare_GetState()->active)
        {
            VehicleSquare_Stop(VEHICLE_SQUARE_ERROR);
            SendSquareTelemetry();
        }
        StopControl();
    }
    else
    {
        UpdateSquareControl();
        UpdateSpeedControl((float)elapsed_ms / 1000.0f);
    }

    if (app.link_active && now_ms - last_rx_ms > LINK_TIMEOUT_MS)
    {
        app.link_active = 0U;
    }
    if ((app.motor_mode == MOTOR_MODE_SPEED ||
         app.motor_mode == MOTOR_MODE_LINE) &&
        now_ms - last_speed_telemetry_ms >=
            (app.motor_mode == MOTOR_MODE_LINE ? LINE_SPEED_TELEMETRY_MS :
                                                 SPEED_TELEMETRY_PERIOD_MS))
    {
        last_speed_telemetry_ms = now_ms;
        SendSpeedTelemetry();
        if (VehicleSquare_GetState()->phase != VEHICLE_SQUARE_IDLE)
            SendSquareTelemetry();
    }
    if (app.motor_mode == MOTOR_MODE_LINE &&
        now_ms - last_line_telemetry_ms >= LINE_TELEMETRY_PERIOD_MS)
    {
        last_line_telemetry_ms = now_ms;
        SendLineTelemetry();
    }
    if (now_ms - last_telemetry_ms >=
        ((app.motor_mode == MOTOR_MODE_SPEED ||
          app.motor_mode == MOTOR_MODE_LINE) ? SPEED_MODE_TELEMETRY_MS :
                                               TELEMETRY_PERIOD_MS))
    {
        last_telemetry_ms = now_ms;
        SendTelemetry();
    }
    if (now_ms - last_debug_ms >= DEBUG_PERIOD_MS)
    {
        last_debug_ms = now_ms;
        SendDebug();
    }
    WheeltecLink_ServiceTx();
}
