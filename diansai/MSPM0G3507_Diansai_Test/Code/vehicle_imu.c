#include "vehicle_imu.h"

#include "include.h"

#define IMU_SCK_PIN        GPIO_Pin_A_12
#define IMU_CS_PIN         GPIO_Pin_A_2
#define IMU_WHO_AM_I_REG   0x0FU
#define IMU_EXPECTED_ID    0x6BU
#define DEFAULT_SAMPLE_MS  10U

/* 实车验证表明板上安装方向与芯片 GZ 正方向相反，必须统一翻转为车辆坐标。 */
#define IMU_YAW_RATE_SIGN  (-1.0f)

static VehicleImuState imu_state;
static LQEnum_GPIO_Pin_t imu_mosi_pin = GPIO_Pin_A_13;
static LQEnum_GPIO_Pin_t imu_miso_pin = GPIO_Pin_A_14;
static float gyro_x_bias;
static float gyro_y_bias;
static float gyro_z_bias;
static float yaw_rate_filtered_dps;
static uint16_t still_samples;
static uint32_t last_update_ms;

static uint8_t TransferByte(uint8_t tx)
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

static void WriteRegister(uint8_t reg, uint8_t value)
{
    LQ_GPIO_WritePin(IMU_CS_PIN, 0);
    TransferByte(reg & 0x7FU);
    TransferByte(value);
    LQ_GPIO_WritePin(IMU_CS_PIN, 1);
}

static void ReadRegisters(uint8_t reg, uint8_t *data, uint8_t length)
{
    uint8_t index;

    LQ_GPIO_WritePin(IMU_CS_PIN, 0);
    TransferByte(reg | 0x80U);
    for (index = 0U; index < length; index++) data[index] = TransferByte(0xFFU);
    LQ_GPIO_WritePin(IMU_CS_PIN, 1);
}

static void ConfigureDataPins(LQEnum_GPIO_Pin_t mosi, LQEnum_GPIO_Pin_t miso)
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

/*
 * 龙邱资料对 PA13/PA14 的 MOSI/MISO 标注曾有冲突，因此保留自动探测。
 * 每组映射连续读三次 WHO_AM_I，识别到 0x6B 后锁定，不在运行中切换。
 */
static uint8_t ProbePinMap(LQEnum_GPIO_Pin_t mosi, LQEnum_GPIO_Pin_t miso)
{
    uint8_t attempt;
    uint8_t id = 0U;

    ConfigureDataPins(mosi, miso);
    for (attempt = 0U; attempt < 3U; attempt++)
    {
        ReadRegisters(IMU_WHO_AM_I_REG, &id, 1U);
        if (id == IMU_EXPECTED_ID) break;
        delay_ms(2);
    }
    return id;
}

static void ReadSixAxis(void)
{
    uint8_t data[12];

    ReadRegisters(0x22U, data, 12U);
    imu_state.gx = (int16_t)(((uint16_t)data[1] << 8) | data[0]);
    imu_state.gy = (int16_t)(((uint16_t)data[3] << 8) | data[2]);
    imu_state.gz = (int16_t)(((uint16_t)data[5] << 8) | data[4]);
    imu_state.ax = (int16_t)(((uint16_t)data[7] << 8) | data[6]);
    imu_state.ay = (int16_t)(((uint16_t)data[9] << 8) | data[8]);
    imu_state.az = (int16_t)(((uint16_t)data[11] << 8) | data[10]);
}

static float ConvertYawRate(void)
{
    return IMU_YAW_RATE_SIGN *
           ((float)imu_state.gz - gyro_z_bias) * 0.070f;
}

uint8_t VehicleImu_Init(void)
{
    LQConfig_GPIO_InitTypeDef_t gpio = {
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_RESISTOR_NO_PULL,
        .Speed = GPIO_SPEED_HIGH,
    };

    memset(&imu_state, 0, sizeof(imu_state));
    gyro_x_bias = 0.0f;
    gyro_y_bias = 0.0f;
    gyro_z_bias = 0.0f;
    yaw_rate_filtered_dps = 0.0f;
    still_samples = 0U;
    last_update_ms = 0U;

    LQ_GPIO_Init(IMU_SCK_PIN, &gpio);
    LQ_GPIO_Init(IMU_CS_PIN, &gpio);
    LQ_GPIO_WritePin(IMU_CS_PIN, 1);
    LQ_GPIO_WritePin(IMU_SCK_PIN, 1);
    delay_ms(10);

    imu_state.id = ProbePinMap(GPIO_Pin_A_13, GPIO_Pin_A_14);
    imu_state.mosi_pin_number = 13U;
    imu_state.miso_pin_number = 14U;
    if (imu_state.id != IMU_EXPECTED_ID)
    {
        imu_state.id = ProbePinMap(GPIO_Pin_A_14, GPIO_Pin_A_13);
        imu_state.mosi_pin_number = 14U;
        imu_state.miso_pin_number = 13U;
    }
    imu_state.ok = (imu_state.id == IMU_EXPECTED_ID) ? 1U : 0U;
    if (!imu_state.ok) return 0U;

    WriteRegister(0x12U, 0x44U); /* BDU，寄存器地址自动递增。 */
    WriteRegister(0x10U, 0x20U); /* 加速度计 52 Hz，量程 2 g。 */
    WriteRegister(0x18U, 0x38U); /* 使能加速度 XYZ。 */
    WriteRegister(0x15U, 0x50U);
    WriteRegister(0x16U, 0x80U);
    WriteRegister(0x11U, 0x4CU); /* 陀螺仪 104 Hz，2000 dps，70 mdps/LSB。 */
    WriteRegister(0x19U, 0x38U); /* 使能陀螺仪 XYZ。 */
    delay_ms(10);
    return 1U;
}

uint8_t VehicleImu_CalibrateGyro(void)
{
    uint16_t sample;
    int32_t sum_x = 0;
    int32_t sum_y = 0;
    int32_t sum_z = 0;
    const uint16_t samples = 256U;

    if (!imu_state.ok) return 0U;

    /* 先丢弃上电后尚未稳定的样本，再对约 2.56 s 数据求平均。 */
    for (sample = 0U; sample < 32U; sample++)
    {
        ReadSixAxis();
        delay_ms(10);
    }
    for (sample = 0U; sample < samples; sample++)
    {
        ReadSixAxis();
        sum_x += imu_state.gx;
        sum_y += imu_state.gy;
        sum_z += imu_state.gz;
        delay_ms(10);
    }

    gyro_x_bias = (float)sum_x / (float)samples;
    gyro_y_bias = (float)sum_y / (float)samples;
    gyro_z_bias = (float)sum_z / (float)samples;
    yaw_rate_filtered_dps = 0.0f;
    still_samples = 0U;
    last_update_ms = 0U;
    VehicleImu_ResetYaw();
    return 1U;
}

void VehicleImu_Update(uint32_t now_ms, uint8_t vehicle_stationary)
{
    float ax;
    float ay;
    float az;
    float gx_rate_dps;
    float gy_rate_dps;
    float yaw_rate_dps;
    float accel_norm;
    float pitch;
    float roll;
    float dt;
    uint32_t elapsed_ms;

    if (!imu_state.ok) return;
    ReadSixAxis();

    gx_rate_dps = ((float)imu_state.gx - gyro_x_bias) * 0.070f;
    gy_rate_dps = ((float)imu_state.gy - gyro_y_bias) * 0.070f;
    yaw_rate_dps = ConvertYawRate();
    ax = (float)imu_state.ax;
    ay = (float)imu_state.ay;
    az = (float)imu_state.az;
    accel_norm = sqrtf(ax * ax + ay * ay + az * az);

    /*
     * 只有上层确认电机和编码器均静止，同时本地加速度/角速度也合理时，
     * 才缓慢跟踪温漂。这样不会把真实转弯误吸收到零偏里。
     */
    if (vehicle_stationary && accel_norm > 14000.0f && accel_norm < 19000.0f &&
        fabsf(gx_rate_dps) < 6.0f && fabsf(gy_rate_dps) < 6.0f &&
        fabsf(yaw_rate_dps) < 6.0f)
    {
        if (still_samples < 1000U) still_samples++;
        if (still_samples >= 30U)
        {
            gyro_x_bias = 0.98f * gyro_x_bias + 0.02f * (float)imu_state.gx;
            gyro_y_bias = 0.98f * gyro_y_bias + 0.02f * (float)imu_state.gy;
            gyro_z_bias = 0.98f * gyro_z_bias + 0.02f * (float)imu_state.gz;
            yaw_rate_dps = ConvertYawRate();
        }
    }
    else
    {
        still_samples = 0U;
    }

    if (fabsf(yaw_rate_dps) < 0.20f) yaw_rate_dps = 0.0f;
    yaw_rate_filtered_dps = 0.80f * yaw_rate_filtered_dps +
                            0.20f * yaw_rate_dps;
    imu_state.yaw_rate_dps = yaw_rate_filtered_dps;

    elapsed_ms = (last_update_ms == 0U) ? DEFAULT_SAMPLE_MS :
                 (now_ms - last_update_ms);
    last_update_ms = now_ms;
    if (elapsed_ms == 0U) elapsed_ms = DEFAULT_SAMPLE_MS;
    if (elapsed_ms > 100U) elapsed_ms = 100U;
    dt = (float)elapsed_ms / 1000.0f;

    pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * 57.29578f;
    roll = atan2f(ay, az) * 57.29578f;
    imu_state.pitch10 = (int16_t)(pitch * 10.0f);
    imu_state.roll10 = (int16_t)(roll * 10.0f);
    imu_state.yaw_deg += yaw_rate_filtered_dps * dt;
    if (imu_state.yaw_deg > 180.0f) imu_state.yaw_deg -= 360.0f;
    if (imu_state.yaw_deg < -180.0f) imu_state.yaw_deg += 360.0f;
    imu_state.yaw10 = (int16_t)(imu_state.yaw_deg * 10.0f);
}

void VehicleImu_ResetYaw(void)
{
    imu_state.yaw_deg = 0.0f;
    imu_state.yaw10 = 0;
}

const VehicleImuState *VehicleImu_GetState(void)
{
    return &imu_state;
}
