#include "include.h"
#include "LQ_device.h"
#include "diansai_app.h"

#define APP_LOOP_MS                 10U
#define TELEMETRY_PERIOD_MS        500U
#define DEBUG_PERIOD_MS            2000U
#define OLED_PERIOD_MS             200U
#define COMMAND_TIMEOUT_MS         500U
#define MOTOR_PWM_FREQUENCY_HZ     20000U
#define MOTOR1_PWM_TIMER           LQ_TIMERA_1
#define MOTOR2_PWM_TIMER           LQ_TIMERG_0
#define MOTOR1_IN1_PWM             LQ_TIMERA1_PWM_CH0_Pin_B_2
#define MOTOR1_IN2_PWM             LQ_TIMERA1_PWM_CH1_Pin_B_3
#define MOTOR2_IN1_PWM             LQ_TIMERG0_PWM_CH0_Pin_B_10
#define MOTOR2_IN2_PWM             LQ_TIMERG0_PWM_CH1_Pin_B_11
#define LEFT_COUNTS_PER_METER      7514
#define RIGHT_COUNTS_PER_METER     7263
#define MAX_TEST_SPEED_MM_S        600
#define UART_LINE_SIZE             64U
#define UART_TX_BUFFER_SIZE        1024U

#define IMU_SCK_PIN                GPIO_Pin_A_12
#define IMU_CS_PIN                 GPIO_Pin_A_2
#define IMU_WHO_AM_I_REG           0x0FU
#define IMU_EXPECTED_ID            0x6BU

typedef struct
{
    int8_t motor_left;
    int8_t motor_right;
    int32_t encoder_left;
    int32_t encoder_right;
    int32_t speed_left_mm_s;
    int32_t speed_right_mm_s;
    int16_t ax;
    int16_t ay;
    int16_t az;
    int16_t gx;
    int16_t gy;
    int16_t gz;
    int16_t yaw10;
    int16_t pitch10;
    int16_t roll10;
    uint8_t imu_id;
    uint8_t imu_ok;
    uint8_t imu_mosi_number;
    uint8_t imu_miso_number;
    uint8_t link_active;
    uint8_t gray_white_valid;
    uint8_t gray_black_valid;
} AppState;

static AppState app;
static LQConfig_Encoder_InitTypeDef_t encoder_left_cfg = {
    .pinA = GPIO_Pin_A_7, .pinB = GPIO_Pin_A_3,
    .encoder_cnt = 0, .count = 0, .gpio_flag = 0,
};
static LQConfig_Encoder_InitTypeDef_t encoder_right_cfg = {
    .pinA = GPIO_Pin_A_8, .pinB = GPIO_Pin_B_7,
    .encoder_cnt = 0, .count = 0, .gpio_flag = 0,
};

static uint8_t uart_tx_buffer[UART_TX_BUFFER_SIZE];
static uint16_t uart_tx_head;
static uint16_t uart_tx_tail;
static char uart_line[UART_LINE_SIZE];
static uint8_t uart_line_length;
static uint16_t gray_white[8];
static uint16_t gray_black[8];
static uint16_t gray_raw[8];
static uint32_t now_ms;
static volatile uint32_t app_tick_ms;
static uint32_t last_loop_ms;
static uint32_t last_imu_ms;
static uint32_t last_motor_command_ms;
static uint32_t last_rx_ms;
static uint32_t last_telemetry_ms;
static uint32_t last_debug_ms;
static uint32_t last_oled_ms;
static float gyro_x_bias;
static float gyro_y_bias;
static float gyro_z_bias;
static float yaw_angle_deg;
static float yaw_rate_filtered_dps;
static uint16_t imu_still_samples;
static LQEnum_GPIO_Pin_t imu_mosi_pin = GPIO_Pin_A_13;
static LQEnum_GPIO_Pin_t imu_miso_pin = GPIO_Pin_A_14;

static void WheeltecUartInit(void)
{
    LQConfig_UART_InitTypeDef_t uart = {
        .Tx = UART0_TX_Pin_A_10,
        .Rx = UART0_RX_Pin_A_11,
        .BaudRate = 9600U,
        .Mode = DL_UART_MODE_NORMAL,
        .Direction = DL_UART_DIRECTION_TX_RX,
        .StopBits = DL_UART_STOP_BITS_ONE,
        .Parity = DL_UART_PARITY_NONE,
        .FlowControl = DL_UART_FLOW_CONTROL_NONE,
        .WordLength = DL_UART_WORD_LENGTH_8_BITS,
    };
    LQ_UART_Init(LQ_UART0, &uart);
    DL_UART_disable(UART0);
    DL_UART_enableFIFOs(UART0);
    DL_UART_setRXFIFOThreshold(UART0, DL_UART_RX_FIFO_LEVEL_ONE_ENTRY);
    DL_UART_enable(UART0);
}

static int32_t ClampInt32(int32_t value, int32_t low, int32_t high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static uint8_t ImuTransfer(uint8_t tx)
{
    uint8_t bit;
    uint8_t rx = 0U;
    for (bit = 0U; bit < 8U; bit++)
    {
        LQ_GPIO_WritePin(imu_mosi_pin, (tx & 0x80U) ? 1 : 0);
        LQ_GPIO_WritePin(IMU_SCK_PIN, 0);
        tx <<= 1;
        rx <<= 1;
        LQ_GPIO_WritePin(IMU_SCK_PIN, 1);
        if (LQ_GPIO_ReadPin(imu_miso_pin)) rx |= 1U;
    }
    return rx;
}

static void ImuWrite(uint8_t reg, uint8_t value)
{
    LQ_GPIO_WritePin(IMU_CS_PIN, 0);
    ImuTransfer(reg & 0x7FU);
    ImuTransfer(value);
    LQ_GPIO_WritePin(IMU_CS_PIN, 1);
}

static void ImuRead(uint8_t reg, uint8_t *data, uint8_t length)
{
    uint8_t i;
    LQ_GPIO_WritePin(IMU_CS_PIN, 0);
    ImuTransfer(reg | 0x80U);
    for (i = 0U; i < length; i++) data[i] = ImuTransfer(0xFFU);
    LQ_GPIO_WritePin(IMU_CS_PIN, 1);
}

static void ImuConfigureDataPins(LQEnum_GPIO_Pin_t mosi, LQEnum_GPIO_Pin_t miso)
{
    LQConfig_GPIO_InitTypeDef_t gpio = {
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_RESISTOR_NO_PULL,
        .Speed = GPIO_SPEED_HIGH,
    };

    imu_mosi_pin = mosi;
    imu_miso_pin = miso;
    LQ_GPIO_Init(imu_mosi_pin, &gpio);
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_RESISTOR_PULL_DOWN;
    LQ_GPIO_Init(imu_miso_pin, &gpio);
}

static uint8_t ImuProbePinMap(LQEnum_GPIO_Pin_t mosi, LQEnum_GPIO_Pin_t miso)
{
    uint8_t attempt;
    uint8_t id = 0U;
    ImuConfigureDataPins(mosi, miso);
    for (attempt = 0U; attempt < 3U; attempt++)
    {
        ImuRead(IMU_WHO_AM_I_REG, &id, 1U);
        if (id == IMU_EXPECTED_ID) break;
        delay_ms(2);
    }
    return id;
}

static uint8_t ImuInit(void)
{
    uint8_t id;
    LQConfig_GPIO_InitTypeDef_t gpio = {
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_RESISTOR_NO_PULL,
        .Speed = GPIO_SPEED_HIGH,
    };

    LQ_GPIO_Init(IMU_SCK_PIN, &gpio);
    LQ_GPIO_Init(IMU_CS_PIN, &gpio);
    LQ_GPIO_WritePin(IMU_CS_PIN, 1);
    LQ_GPIO_WritePin(IMU_SCK_PIN, 1);
    delay_ms(10);

    id = ImuProbePinMap(GPIO_Pin_A_13, GPIO_Pin_A_14);
    app.imu_mosi_number = 13U;
    app.imu_miso_number = 14U;
    if (id != IMU_EXPECTED_ID)
    {
        id = ImuProbePinMap(GPIO_Pin_A_14, GPIO_Pin_A_13);
        app.imu_mosi_number = 14U;
        app.imu_miso_number = 13U;
    }
    if (id != IMU_EXPECTED_ID) return id;

    ImuWrite(0x12U, 0x44U); /* BDU, register auto increment. */
    ImuWrite(0x10U, 0x20U); /* Accelerometer 52 Hz, 2 g. */
    ImuWrite(0x18U, 0x38U); /* Enable accelerometer XYZ. */
    ImuWrite(0x15U, 0x50U);
    ImuWrite(0x16U, 0x80U);
    ImuWrite(0x11U, 0x4CU); /* Gyroscope 104 Hz, 2000 dps, 70 mdps/LSB. */
    ImuWrite(0x19U, 0x38U); /* Enable gyroscope XYZ. */
    delay_ms(10);
    return id;
}

static void ImuRead6Axis(void)
{
    uint8_t data[12];
    ImuRead(0x22U, data, 12U);
    app.gx = (int16_t)(((uint16_t)data[1] << 8) | data[0]);
    app.gy = (int16_t)(((uint16_t)data[3] << 8) | data[2]);
    app.gz = (int16_t)(((uint16_t)data[5] << 8) | data[4]);
    app.ax = (int16_t)(((uint16_t)data[7] << 8) | data[6]);
    app.ay = (int16_t)(((uint16_t)data[9] << 8) | data[8]);
    app.az = (int16_t)(((uint16_t)data[11] << 8) | data[10]);
}

static void ImuCalibrateGyro(void)
{
    uint16_t sample;
    int32_t sum_x = 0;
    int32_t sum_y = 0;
    int32_t sum_z = 0;
    const uint16_t samples = 256U;

    for (sample = 0U; sample < 32U; sample++)
    {
        ImuRead6Axis();
        delay_ms(10);
    }

    for (sample = 0U; sample < samples; sample++)
    {
        ImuRead6Axis();
        sum_x += app.gx;
        sum_y += app.gy;
        sum_z += app.gz;
        delay_ms(10);
    }
    gyro_x_bias = (float)sum_x / (float)samples;
    gyro_y_bias = (float)sum_y / (float)samples;
    gyro_z_bias = (float)sum_z / (float)samples;
    yaw_angle_deg = 0.0f;
    yaw_rate_filtered_dps = 0.0f;
    imu_still_samples = 0U;
    app.yaw10 = 0;
}

static uint16_t UartTxFree(void)
{
    if (uart_tx_head >= uart_tx_tail)
    {
        return (uint16_t)(UART_TX_BUFFER_SIZE - (uart_tx_head - uart_tx_tail) - 1U);
    }
    return (uint16_t)(uart_tx_tail - uart_tx_head - 1U);
}

static void UartQueueText(const char *text)
{
    size_t length = strlen(text);
    if (length > UartTxFree()) return;

    while (length-- > 0U)
    {
        uart_tx_buffer[uart_tx_head] = (uint8_t)*text++;
        uart_tx_head = (uint16_t)((uart_tx_head + 1U) % UART_TX_BUFFER_SIZE);
    }
}

static void UartTxService(void)
{
    while (uart_tx_tail != uart_tx_head && !DL_UART_isTXFIFOFull(UART0))
    {
        DL_UART_transmitData(UART0, uart_tx_buffer[uart_tx_tail]);
        uart_tx_tail = (uint16_t)((uart_tx_tail + 1U) % UART_TX_BUFFER_SIZE);
    }
}

static void SendText(const char *text)
{
    UartQueueText(text);
}

static void MotorPwmTimerInit(LQEnum_Timer_t timer,
                              LQEnum_PWM_Pin_t input1,
                              LQEnum_PWM_Pin_t input2)
{
    PWM_ConfigTypeDef cfg = LQ_PWM_CalcOptimal(timer, MOTOR_PWM_FREQUENCY_HZ);
    LQConfig_PWM_InitTypeDef_t pwm = {
        .DivideRatio = (DL_TIMER_CLOCK_DIVIDE)(cfg.DivideRatio - 1U),
        .Prescaler = cfg.Prescaler,
        .Period = cfg.Period,
        .PwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN_UP,
        .startTimer = false,
    };

    LQ_TIMER_PWMInit(timer, &pwm);
    LQ_TIMER_EnablePWMChannel(input1);
    LQ_TIMER_EnablePWMChannel(input2);
    LQ_TIMER_PWMSetCaptureCompare(input1, 0U);
    LQ_TIMER_PWMSetCaptureCompare(input2, 0U);
    LQ_TIMER_PWM_Start(timer);
}

static void MotorSetOne(LQEnum_Timer_t timer, LQEnum_PWM_Pin_t in1_pwm,
                        LQEnum_PWM_Pin_t in2_pwm, int32_t percent)
{
    uint32_t load = LQ_TIMER_Regs[timer]->COUNTERREGS.LOAD;
    uint32_t compare;

    percent = ClampInt32(percent, -100, 100);
    compare = load * (uint32_t)abs(percent) / 100U;

    /* Clear both bridge inputs before changing direction. */
    LQ_TIMER_PWMSetCaptureCompare(in1_pwm, 0U);
    LQ_TIMER_PWMSetCaptureCompare(in2_pwm, 0U);
    if (percent == 0)
    {
        return;
    }

    delay_us(2);
    if (percent > 0)
    {
        LQ_TIMER_PWMSetCaptureCompare(in1_pwm, compare);
    }
    else
    {
        LQ_TIMER_PWMSetCaptureCompare(in2_pwm, compare);
    }
}

static void MotorSet(int32_t left_percent, int32_t right_percent)
{
    app.motor_left = (int8_t)ClampInt32(left_percent, -100, 100);
    app.motor_right = (int8_t)ClampInt32(right_percent, -100, 100);
    MotorSetOne(MOTOR1_PWM_TIMER, MOTOR1_IN1_PWM, MOTOR1_IN2_PWM, app.motor_left);
    MotorSetOne(MOTOR2_PWM_TIMER, MOTOR2_IN1_PWM, MOTOR2_IN2_PWM, app.motor_right);
}

static void MotorStop(void)
{
    MotorSet(0, 0);
}

static void ReadGrayRaw(void)
{
    uint8_t channel;
    uint8_t sample;
    uint32_t sum;

    for (channel = 0U; channel < 8U; channel++)
    {
        LQ_GPIO_WritePin(Tracking_S0_PIN, (channel & 0x01U) ? 1 : 0);
        LQ_GPIO_WritePin(Tracking_S1_PIN, (channel & 0x02U) ? 1 : 0);
        LQ_GPIO_WritePin(Tracking_S2_PIN, (channel & 0x04U) ? 1 : 0);
        delay_us(5);

        (void)LQ_ADC_GetValue(Tracking_ADC_CH);
        sum = 0U;
        for (sample = 0U; sample < 4U; sample++)
        {
            sum += LQ_ADC_GetValue(Tracking_ADC_CH);
        }
        gray_raw[channel] = (uint16_t)(sum / 4U);
    }
}

static void CaptureGray(uint16_t target[8])
{
    uint8_t i;
    ReadGrayRaw();
    for (i = 0U; i < 8U; i++) target[i] = gray_raw[i];
}

static void SendGrayCalibration(void)
{
    char line[160];
    snprintf(line, sizeof(line),
             "CAL,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\r\n",
             gray_white[0], gray_white[1], gray_white[2], gray_white[3],
             gray_white[4], gray_white[5], gray_white[6], gray_white[7],
             gray_black[0], gray_black[1], gray_black[2], gray_black[3],
             gray_black[4], gray_black[5], gray_black[6], gray_black[7]);
    UartQueueText(line);
}

static void ResetEncoder(void)
{
    __disable_irq();
    encoder_left_cfg.encoder_cnt = 0;
    encoder_left_cfg.count = 0;
    encoder_right_cfg.encoder_cnt = 0;
    encoder_right_cfg.count = 0;
    app.encoder_left = 0;
    app.encoder_right = 0;
    __enable_irq();
}

static void ProcessCommand(char *line)
{
    int throttle;
    int steering;
    int left;
    int right;

    last_rx_ms = now_ms;
    app.link_active = 1U;

    if (sscanf(line, "DRV,%d,%d", &throttle, &steering) == 2)
    {
        throttle = (int)ClampInt32(throttle, -100, 100);
        steering = (int)ClampInt32(steering, -100, 100);
        MotorSet(throttle + steering, throttle - steering);
        last_motor_command_ms = now_ms;
    }
    else if (sscanf(line, "MOTOR,%d,%d", &left, &right) == 2)
    {
        MotorSet(left, right);
        last_motor_command_ms = now_ms;
    }
    else if (strcmp(line, "STOP") == 0)
    {
        MotorStop();
        app.link_active = 0U;
        SendText("ACK,STOP\r\n");
    }
    else if (strcmp(line, "PING") == 0)
    {
        /* Heartbeat only. Avoid periodic replies on the low-bandwidth BLE link. */
    }
    else if (strcmp(line, "ZERO") == 0 || strcmp(line, "ENCZERO") == 0)
    {
        ResetEncoder();
        yaw_angle_deg = 0.0f;
        app.yaw10 = 0;
        SendText("ACK,ZERO\r\n");
    }
    else if (strcmp(line, "IMUZERO") == 0)
    {
        MotorStop();
        if (app.imu_ok) ImuCalibrateGyro();
        SendText("ACK,IMUZERO\r\n");
    }
    else if (strcmp(line, "GRAYWHITE") == 0)
    {
        CaptureGray(gray_white);
        app.gray_white_valid = 1U;
        SendGrayCalibration();
    }
    else if (strcmp(line, "GRAYBLACK") == 0)
    {
        CaptureGray(gray_black);
        app.gray_black_valid = 1U;
        SendGrayCalibration();
    }
    else if (strcmp(line, "GRAYCAL") == 0 || strcmp(line, "GRAY") == 0)
    {
        SendGrayCalibration();
    }
    else if (strcmp(line, "HELP") == 0)
    {
        SendText("ACK,DRV MOTOR STOP ZERO ENCZERO IMUZERO GRAY GRAYWHITE GRAYBLACK PING\r\n");
    }
    else
    {
        SendText("ERR,UNKNOWN\r\n");
    }
}

static void ProcessUart(void)
{
    while (!DL_UART_isRXFIFOEmpty(UART0))
    {
        char byte = (char)DL_UART_receiveData(UART0);
        if (byte == '\r' || byte == '\n')
        {
            if (uart_line_length > 0U)
            {
                uart_line[uart_line_length] = '\0';
                ProcessCommand(uart_line);
                uart_line_length = 0U;
            }
        }
        else if (uart_line_length < UART_LINE_SIZE - 1U)
        {
            uart_line[uart_line_length++] = byte;
        }
        else
        {
            uart_line_length = 0U;
        }
    }
}

static void UpdateEncoder(void)
{
    int32_t left_delta;
    int32_t right_delta;
    __disable_irq();
    left_delta = encoder_left_cfg.encoder_cnt + encoder_left_cfg.count;
    right_delta = encoder_right_cfg.encoder_cnt + encoder_right_cfg.count;
    encoder_left_cfg.encoder_cnt = 0;
    encoder_right_cfg.encoder_cnt = 0;
    encoder_left_cfg.count = 0;
    encoder_right_cfg.count = 0;
    __enable_irq();
    app.encoder_left += left_delta;
    app.encoder_right += right_delta;
    app.speed_left_mm_s = left_delta * 100000 / LEFT_COUNTS_PER_METER;
    app.speed_right_mm_s = right_delta * 100000 / RIGHT_COUNTS_PER_METER;
}

static void UpdateImu(void)
{
    float ax;
    float ay;
    float az;
    float pitch;
    float roll;
    float yaw_rate_dps;
    float gx_rate_dps;
    float gy_rate_dps;
    float accel_norm;
    float dt;
    uint32_t elapsed_ms;

    if (!app.imu_ok) return;
    ImuRead6Axis();

    gx_rate_dps = ((float)app.gx - gyro_x_bias) * 0.070f;
    gy_rate_dps = ((float)app.gy - gyro_y_bias) * 0.070f;
    yaw_rate_dps = ((float)app.gz - gyro_z_bias) * 0.070f;
    ax = (float)app.ax;
    ay = (float)app.ay;
    az = (float)app.az;
    accel_norm = sqrtf(ax * ax + ay * ay + az * az);

    if (app.motor_left == 0 && app.motor_right == 0 &&
        app.speed_left_mm_s == 0 && app.speed_right_mm_s == 0 &&
        accel_norm > 14000.0f && accel_norm < 19000.0f &&
        fabsf(gx_rate_dps) < 6.0f && fabsf(gy_rate_dps) < 6.0f &&
        fabsf(yaw_rate_dps) < 6.0f)
    {
        if (imu_still_samples < 1000U) imu_still_samples++;
        if (imu_still_samples >= 30U)
        {
            gyro_x_bias = 0.98f * gyro_x_bias + 0.02f * (float)app.gx;
            gyro_y_bias = 0.98f * gyro_y_bias + 0.02f * (float)app.gy;
            gyro_z_bias = 0.98f * gyro_z_bias + 0.02f * (float)app.gz;
            yaw_rate_dps = ((float)app.gz - gyro_z_bias) * 0.070f;
        }
    }
    else
    {
        imu_still_samples = 0U;
    }
    if (fabsf(yaw_rate_dps) < 0.20f) yaw_rate_dps = 0.0f;
    yaw_rate_filtered_dps = 0.80f * yaw_rate_filtered_dps + 0.20f * yaw_rate_dps;

    elapsed_ms = (last_imu_ms == 0U) ? APP_LOOP_MS : (now_ms - last_imu_ms);
    last_imu_ms = now_ms;
    if (elapsed_ms == 0U) elapsed_ms = APP_LOOP_MS;
    if (elapsed_ms > 100U) elapsed_ms = 100U;
    dt = (float)elapsed_ms / 1000.0f;

    pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * 57.29578f;
    roll = atan2f(ay, az) * 57.29578f;
    app.pitch10 = (int16_t)(pitch * 10.0f);
    app.roll10 = (int16_t)(roll * 10.0f);
    yaw_angle_deg += yaw_rate_filtered_dps * dt;
    if (yaw_angle_deg > 180.0f) yaw_angle_deg -= 360.0f;
    if (yaw_angle_deg < -180.0f) yaw_angle_deg += 360.0f;
    app.yaw10 = (int16_t)(yaw_angle_deg * 10.0f);
}

static uint8_t CountActiveGray(void)
{
    uint8_t i;
    uint8_t count = 0U;
    for (i = 0U; i < 8U; i++) if (gray_raw[i] >= 2048U) count++;
    return count;
}

static void SendTelemetry(void)
{
    char line[320];
    int32_t target_left = app.motor_left * MAX_TEST_SPEED_MM_S / 100;
    int32_t target_right = app.motor_right * MAX_TEST_SPEED_MM_S / 100;
    int32_t yaw_rate10 = (int32_t)(yaw_rate_filtered_dps * 10.0f);
    uint8_t active = CountActiveGray();

    snprintf(line, sizeof(line),
             "TEL,%lu,%u,%u,%d,%ld,%ld,%ld,%ld,%d,%d,0,%ld,0,0,0,%u,0,0,0,0,0,0,0\r\n",
             (unsigned long)now_ms,
             (unsigned int)((app.motor_left != 0 || app.motor_right != 0) ? 1U : 0U),
             (unsigned int)app.link_active, app.yaw10,
             (long)app.speed_left_mm_s, (long)app.speed_right_mm_s,
             (long)target_left, (long)target_right,
             app.motor_left * 168, app.motor_right * 168,
             (long)yaw_rate10, (unsigned int)app.imu_ok);
    UartQueueText(line);

    snprintf(line, sizeof(line),
             "STA,%lu,%d,%d,%ld,%ld,0,0,0,0,0,0,0,0,0,0,0,0,0,0,30,%u,90,"
             "%u,%u,%u,%u,%u,%u,%u,%u,60,%u,%u,%u,900,500,"
             "%u,%u,%u,%u,%u,%u,%u,%u\r\n",
             (unsigned long)now_ms, app.pitch10, app.roll10,
             (long)app.encoder_left, (long)app.encoder_right,
             (unsigned int)active,
             gray_raw[0], gray_raw[1], gray_raw[2], gray_raw[3],
             gray_raw[4], gray_raw[5], gray_raw[6], gray_raw[7],
             (unsigned int)active,
             (unsigned int)app.gray_white_valid,
             (unsigned int)app.gray_black_valid,
             gray_raw[0], gray_raw[1], gray_raw[2], gray_raw[3],
             gray_raw[4], gray_raw[5], gray_raw[6], gray_raw[7]);
    UartQueueText(line);
}

static void SendDebug(void)
{
    char line[96];
    snprintf(line, sizeof(line), "DBG,%u,%d,%d,%d,%d,%d,%d,%u,%u\r\n",
             app.imu_id, app.ax, app.ay, app.az, app.gx, app.gy, app.gz,
             app.imu_mosi_number, app.imu_miso_number);
    UartQueueText(line);
}

static void OledShowLine(uint8_t row, const char *text)
{
    uint8_t i = 0U;
    unsigned char padded[22];
    memset(padded, ' ', 21U);
    while (i < 21U && text[i] != '\0')
    {
        padded[i] = (unsigned char)text[i];
        i++;
    }
    padded[21] = '\0';
    LQ_OLED_ShowString(row, 0, padded, 8);
}

static void UpdateOled(void)
{
    char line[32];
    snprintf(line, sizeof(line), "WT 9600 %s", app.link_active ? "LINK" : "IDLE");
    OledShowLine(0, line);
    snprintf(line, sizeof(line), "M L%d R%d", app.motor_left, app.motor_right);
    OledShowLine(1, line);
    snprintf(line, sizeof(line), "E %ld %ld", (long)app.encoder_left, (long)app.encoder_right);
    OledShowLine(2, line);
    snprintf(line, sizeof(line), "G0 %u G1 %u", gray_raw[0], gray_raw[1]);
    OledShowLine(3, line);
    snprintf(line, sizeof(line), "G2 %u G3 %u", gray_raw[2], gray_raw[3]);
    OledShowLine(4, line);
    snprintf(line, sizeof(line), "G4 %u G5 %u", gray_raw[4], gray_raw[5]);
    OledShowLine(5, line);
    snprintf(line, sizeof(line), "G6 %u G7 %u", gray_raw[6], gray_raw[7]);
    OledShowLine(6, line);
    if (app.imu_ok)
    {
        snprintf(line, sizeof(line), "IMU %02X M%u I%u", app.imu_id,
                 app.imu_mosi_number, app.imu_miso_number);
    }
    else
    {
        snprintf(line, sizeof(line), "IMU %02X AUTO FAIL", app.imu_id);
    }
    OledShowLine(7, line);
    LQ_OLED_Refresh();
}

void DiansaiApp_Init(void)
{
    DL_SYSTICK_disable();
    DL_SYSTICK_init(80000U);
    DL_SYSTICK_enableInterrupt();
    DL_SYSTICK_enable();
    __disable_irq();
    app_tick_ms = 0U;
    __enable_irq();
    last_loop_ms = 0U;
    memset(&app, 0, sizeof(app));
    memset(gray_white, 0, sizeof(gray_white));
    memset(gray_black, 0, sizeof(gray_black));
    memset(gray_raw, 0, sizeof(gray_raw));
    WheeltecUartInit();
    MotorPwmTimerInit(MOTOR1_PWM_TIMER, MOTOR1_IN1_PWM, MOTOR1_IN2_PWM);
    MotorPwmTimerInit(MOTOR2_PWM_TIMER, MOTOR2_IN1_PWM, MOTOR2_IN2_PWM);
    MotorStop();
    LQ_Encoder_Init(500U, &encoder_left_cfg);
    LQ_Encoder_Init(500U, &encoder_right_cfg);
    LQ_Tracking_Polling_Init();
    LQ_OLED_Init();
    app.imu_id = ImuInit();
    app.imu_ok = (app.imu_id == IMU_EXPECTED_ID) ? 1U : 0U;
    if (app.imu_ok)
    {
        OledShowLine(0, "IMU CAL HOLD STILL");
        OledShowLine(1, "ABOUT 3 SECONDS");
        LQ_OLED_Refresh();
        ImuCalibrateGyro();
    }
    now_ms = app_tick_ms;
    last_imu_ms = 0U;
    UpdateOled();
    SendText("BOOT,MSPM0G3507_DIANSAI_TEST,V8,WHEELTEC,9600\r\n");
    SendText("ACK,READY,V8,WHEELTEC; send HELP for commands\r\n");
}

void SysTick_Handler(void)
{
    app_tick_ms++;
}

void DiansaiApp_Run(void)
{
    uint32_t tick;
    UartTxService();
    ProcessUart();
    tick = app_tick_ms;
    if (tick - last_loop_ms < APP_LOOP_MS) return;
    last_loop_ms = tick;

    now_ms = tick;
    UpdateEncoder();
    UpdateImu();
    if ((now_ms % 20U) == 0U) ReadGrayRaw();
    if ((app.motor_left != 0 || app.motor_right != 0) &&
        (now_ms - last_motor_command_ms > COMMAND_TIMEOUT_MS))
    {
        MotorStop();
    }
    if (app.link_active && (now_ms - last_rx_ms > 1200U)) app.link_active = 0U;
    if (now_ms - last_telemetry_ms >= TELEMETRY_PERIOD_MS)
    {
        last_telemetry_ms = now_ms;
        SendTelemetry();
    }
    if (now_ms - last_debug_ms >= DEBUG_PERIOD_MS)
    {
        last_debug_ms = now_ms;
        SendDebug();
    }
    if (now_ms - last_oled_ms >= OLED_PERIOD_MS)
    {
        last_oled_ms = now_ms;
        UpdateOled();
    }
    UartTxService();
}
