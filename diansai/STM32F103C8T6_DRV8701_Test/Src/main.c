#include "drv8701.h"

#define TEST_MOTOR_COMMAND 200

static TIM_HandleTypeDef htim3;
static bool motor_ready;

static void SystemClock_Config(void);
static void GPIO_Init(void);
static void TIM3_PWM_Init(void);
static void Error_Handler(void);

static DRV8701_HandleTypeDef motor = {
    .pwm_timer = &htim3,
    .pwm_channel = TIM_CHANNEL_1,
    .phase_port = GPIOB,
    .phase_pin = GPIO_PIN_0,
    .sleep_port = GPIOB,
    .sleep_pin = GPIO_PIN_1,
    .fault_port = GPIOB,
    .fault_pin = GPIO_PIN_10,
    .reverse_phase = false,
    .awake = false,
};

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    TIM3_PWM_Init();

    if (DRV8701_Init(&motor) != HAL_OK)
    {
        Error_Handler();
    }
    motor_ready = true;

    /* Fixed 20% forward duty. Use a negative value for reverse direction. */
    if (DRV8701_SetOutput(&motor, TEST_MOTOR_COMMAND) != HAL_OK)
    {
        Error_Handler();
    }

    while (1)
    {
        if (DRV8701_IsFaultActive(&motor))
        {
            Error_Handler();
        }
    }
}

static void GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();

    /* Keep DRV8701 asleep before configuring the other pins. */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_1, GPIO_PIN_RESET);

    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);

    /* nFAULT needs a pull-up. An external 10-kohm pull-up is preferred. */
    gpio.Pin = GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &gpio);

    /* PA6 is configured as TIM3_CH1 PWM in HAL_TIM_PWM_MspInit(). */
}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *timer)
{
    GPIO_InitTypeDef gpio = {0};

    if (timer->Instance == TIM3)
    {
        __HAL_RCC_TIM3_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        gpio.Pin = GPIO_PIN_6;
        gpio.Mode = GPIO_MODE_AF_PP;
        gpio.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &gpio);
    }
}

static void TIM3_PWM_Init(void)
{
    TIM_OC_InitTypeDef pwm = {0};

    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 0U;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 3599U; /* 72 MHz / 3600 = 20 kHz. */
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
    {
        Error_Handler();
    }

    pwm.OCMode = TIM_OCMODE_PWM1;
    pwm.Pulse = 0U;
    pwm.OCPolarity = TIM_OCPOLARITY_HIGH;
    pwm.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(&htim3, &pwm, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef oscillator = {0};
    RCC_ClkInitTypeDef clock = {0};

    oscillator.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    oscillator.HSEState = RCC_HSE_ON;
    oscillator.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    oscillator.HSIState = RCC_HSI_ON;
    oscillator.PLL.PLLState = RCC_PLL_ON;
    oscillator.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    oscillator.PLL.PLLMUL = RCC_PLL_MUL9;

    if (HAL_RCC_OscConfig(&oscillator) != HAL_OK)
    {
        Error_Handler();
    }

    clock.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                      RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clock.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clock.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clock.APB1CLKDivider = RCC_HCLK_DIV2;
    clock.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&clock, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

static void Error_Handler(void)
{
    if (motor_ready)
    {
        DRV8701_Coast(&motor);
    }

    while (1)
    {
        /* A debugger can inspect PB10 here: low means DRV8701 fault. */
    }
}
