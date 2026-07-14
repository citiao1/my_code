/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)


#define GPIO_HFXT_PORT                                                     GPIOA
#define GPIO_HFXIN_PIN                                             DL_GPIO_PIN_5
#define GPIO_HFXIN_IOMUX                                         (IOMUX_PINCM10)
#define GPIO_HFXOUT_PIN                                            DL_GPIO_PIN_6
#define GPIO_HFXOUT_IOMUX                                        (IOMUX_PINCM11)
#define CPUCLK_FREQ                                                     80000000



/* Defines for PWM_Motor */
#define PWM_Motor_INST                                                     TIMA1
#define PWM_Motor_INST_IRQHandler                               TIMA1_IRQHandler
#define PWM_Motor_INST_INT_IRQN                                 (TIMA1_INT_IRQn)
#define PWM_Motor_INST_CLK_FREQ                                         80000000
/* GPIO defines for channel 0 */
#define GPIO_PWM_Motor_C0_PORT                                             GPIOB
#define GPIO_PWM_Motor_C0_PIN                                      DL_GPIO_PIN_2
#define GPIO_PWM_Motor_C0_IOMUX                                  (IOMUX_PINCM15)
#define GPIO_PWM_Motor_C0_IOMUX_FUNC                 IOMUX_PINCM15_PF_TIMA1_CCP0
#define GPIO_PWM_Motor_C0_IDX                                DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_PWM_Motor_C1_PORT                                             GPIOB
#define GPIO_PWM_Motor_C1_PIN                                      DL_GPIO_PIN_3
#define GPIO_PWM_Motor_C1_IOMUX                                  (IOMUX_PINCM16)
#define GPIO_PWM_Motor_C1_IOMUX_FUNC                 IOMUX_PINCM16_PF_TIMA1_CCP1
#define GPIO_PWM_Motor_C1_IDX                                DL_TIMER_CC_1_INDEX

/* Defines for PWM_Servo_A0 */
#define PWM_Servo_A0_INST                                                  TIMA0
#define PWM_Servo_A0_INST_IRQHandler                            TIMA0_IRQHandler
#define PWM_Servo_A0_INST_INT_IRQN                              (TIMA0_INT_IRQn)
#define PWM_Servo_A0_INST_CLK_FREQ                                        500000
/* GPIO defines for channel 1 */
#define GPIO_PWM_Servo_A0_C1_PORT                                          GPIOB
#define GPIO_PWM_Servo_A0_C1_PIN                                   DL_GPIO_PIN_9
#define GPIO_PWM_Servo_A0_C1_IOMUX                               (IOMUX_PINCM26)
#define GPIO_PWM_Servo_A0_C1_IOMUX_FUNC              IOMUX_PINCM26_PF_TIMA0_CCP1
#define GPIO_PWM_Servo_A0_C1_IDX                             DL_TIMER_CC_1_INDEX
/* GPIO defines for channel 2 */
#define GPIO_PWM_Servo_A0_C2_PORT                                          GPIOB
#define GPIO_PWM_Servo_A0_C2_PIN                                  DL_GPIO_PIN_12
#define GPIO_PWM_Servo_A0_C2_IOMUX                               (IOMUX_PINCM29)
#define GPIO_PWM_Servo_A0_C2_IOMUX_FUNC              IOMUX_PINCM29_PF_TIMA0_CCP2
#define GPIO_PWM_Servo_A0_C2_IDX                             DL_TIMER_CC_2_INDEX
/* GPIO defines for channel 3 */
#define GPIO_PWM_Servo_A0_C3_PORT                                          GPIOB
#define GPIO_PWM_Servo_A0_C3_PIN                                  DL_GPIO_PIN_13
#define GPIO_PWM_Servo_A0_C3_IOMUX                               (IOMUX_PINCM30)
#define GPIO_PWM_Servo_A0_C3_IOMUX_FUNC              IOMUX_PINCM30_PF_TIMA0_CCP3
#define GPIO_PWM_Servo_A0_C3_IDX                             DL_TIMER_CC_3_INDEX

/* Defines for PWM_Servo_G0 */
#define PWM_Servo_G0_INST                                                  TIMG0
#define PWM_Servo_G0_INST_IRQHandler                            TIMG0_IRQHandler
#define PWM_Servo_G0_INST_INT_IRQN                              (TIMG0_INT_IRQn)
#define PWM_Servo_G0_INST_CLK_FREQ                                        500000
/* GPIO defines for channel 0 */
#define GPIO_PWM_Servo_G0_C0_PORT                                          GPIOB
#define GPIO_PWM_Servo_G0_C0_PIN                                  DL_GPIO_PIN_10
#define GPIO_PWM_Servo_G0_C0_IOMUX                               (IOMUX_PINCM27)
#define GPIO_PWM_Servo_G0_C0_IOMUX_FUNC              IOMUX_PINCM27_PF_TIMG0_CCP0
#define GPIO_PWM_Servo_G0_C0_IDX                             DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_PWM_Servo_G0_C1_PORT                                          GPIOB
#define GPIO_PWM_Servo_G0_C1_PIN                                  DL_GPIO_PIN_11
#define GPIO_PWM_Servo_G0_C1_IOMUX                               (IOMUX_PINCM28)
#define GPIO_PWM_Servo_G0_C1_IOMUX_FUNC              IOMUX_PINCM28_PF_TIMG0_CCP1
#define GPIO_PWM_Servo_G0_C1_IDX                             DL_TIMER_CC_1_INDEX



/* Defines for TIMER_0 */
#define TIMER_0_INST                                                     (TIMG6)
#define TIMER_0_INST_IRQHandler                                 TIMG6_IRQHandler
#define TIMER_0_INST_INT_IRQN                                   (TIMG6_INT_IRQn)
#define TIMER_0_INST_LOAD_VALUE                                           (499U)



/* Defines for UART_0 */
#define UART_0_INST                                                        UART0
#define UART_0_INST_FREQUENCY                                           40000000
#define UART_0_INST_IRQHandler                                  UART0_IRQHandler
#define UART_0_INST_INT_IRQN                                      UART0_INT_IRQn
#define GPIO_UART_0_RX_PORT                                                GPIOA
#define GPIO_UART_0_TX_PORT                                                GPIOA
#define GPIO_UART_0_RX_PIN                                        DL_GPIO_PIN_11
#define GPIO_UART_0_TX_PIN                                        DL_GPIO_PIN_10
#define GPIO_UART_0_IOMUX_RX                                     (IOMUX_PINCM22)
#define GPIO_UART_0_IOMUX_TX                                     (IOMUX_PINCM21)
#define GPIO_UART_0_IOMUX_RX_FUNC                      IOMUX_PINCM22_PF_UART0_RX
#define GPIO_UART_0_IOMUX_TX_FUNC                      IOMUX_PINCM21_PF_UART0_TX
#define UART_0_BAUD_RATE                                                (115200)
#define UART_0_IBRD_40_MHZ_115200_BAUD                                      (21)
#define UART_0_FBRD_40_MHZ_115200_BAUD                                      (45)





/* Defines for ADC_Tracking */
#define ADC_Tracking_INST                                                   ADC0
#define ADC_Tracking_INST_IRQHandler                             ADC0_IRQHandler
#define ADC_Tracking_INST_INT_IRQN                               (ADC0_INT_IRQn)
#define ADC_Tracking_ADCMEM_ADC0_CH0                          DL_ADC12_MEM_IDX_0
#define ADC_Tracking_ADCMEM_ADC0_CH0_REF         DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define ADC_Tracking_ADCMEM_ADC0_CH0_REF_VOLTAGE_V                                     3.3
#define GPIO_ADC_Tracking_C0_PORT                                          GPIOA
#define GPIO_ADC_Tracking_C0_PIN                                  DL_GPIO_PIN_27



/* Port definition for Pin Group LED */
#define LED_PORT                                                         (GPIOA)

/* Defines for LED0: GPIOA.15 with pinCMx 37 on package pin 8 */
#define LED_LED0_PIN                                            (DL_GPIO_PIN_15)
#define LED_LED0_IOMUX                                           (IOMUX_PINCM37)
/* Port definition for Pin Group BUZZ */
#define BUZZ_PORT                                                        (GPIOA)

/* Defines for Buzzer: GPIOA.28 with pinCMx 3 on package pin 35 */
#define BUZZ_Buzzer_PIN                                         (DL_GPIO_PIN_28)
#define BUZZ_Buzzer_IOMUX                                         (IOMUX_PINCM3)
/* Port definition for Pin Group Key */
#define Key_PORT                                                         (GPIOB)

/* Defines for K1: GPIOB.14 with pinCMx 31 on package pin 2 */
#define Key_K1_PIN                                              (DL_GPIO_PIN_14)
#define Key_K1_IOMUX                                             (IOMUX_PINCM31)
/* Defines for K2: GPIOB.15 with pinCMx 32 on package pin 3 */
#define Key_K2_PIN                                              (DL_GPIO_PIN_15)
#define Key_K2_IOMUX                                             (IOMUX_PINCM32)
/* Defines for K3: GPIOB.16 with pinCMx 33 on package pin 4 */
#define Key_K3_PIN                                              (DL_GPIO_PIN_16)
#define Key_K3_IOMUX                                             (IOMUX_PINCM33)
/* Port definition for Pin Group Switch */
#define Switch_PORT                                                      (GPIOB)

/* Defines for SW1: GPIOB.6 with pinCMx 23 on package pin 58 */
#define Switch_SW1_PIN                                           (DL_GPIO_PIN_6)
#define Switch_SW1_IOMUX                                         (IOMUX_PINCM23)
/* Defines for SW2: GPIOB.8 with pinCMx 25 on package pin 60 */
#define Switch_SW2_PIN                                           (DL_GPIO_PIN_8)
#define Switch_SW2_IOMUX                                         (IOMUX_PINCM25)
/* Defines for OLED_CK: GPIOA.17 with pinCMx 39 on package pin 10 */
#define OLED_OLED_CK_PORT                                                (GPIOA)
#define OLED_OLED_CK_PIN                                        (DL_GPIO_PIN_17)
#define OLED_OLED_CK_IOMUX                                       (IOMUX_PINCM39)
/* Defines for OLED_DI: GPIOA.16 with pinCMx 38 on package pin 9 */
#define OLED_OLED_DI_PORT                                                (GPIOA)
#define OLED_OLED_DI_PIN                                        (DL_GPIO_PIN_16)
#define OLED_OLED_DI_IOMUX                                       (IOMUX_PINCM38)
/* Defines for OLED_RST: GPIOB.21 with pinCMx 49 on package pin 20 */
#define OLED_OLED_RST_PORT                                               (GPIOB)
#define OLED_OLED_RST_PIN                                       (DL_GPIO_PIN_21)
#define OLED_OLED_RST_IOMUX                                      (IOMUX_PINCM49)
/* Defines for OLED_DC: GPIOB.23 with pinCMx 51 on package pin 22 */
#define OLED_OLED_DC_PORT                                                (GPIOB)
#define OLED_OLED_DC_PIN                                        (DL_GPIO_PIN_23)
#define OLED_OLED_DC_IOMUX                                       (IOMUX_PINCM51)
/* Defines for OLED_CS: GPIOB.22 with pinCMx 50 on package pin 21 */
#define OLED_OLED_CS_PORT                                                (GPIOB)
#define OLED_OLED_CS_PIN                                        (DL_GPIO_PIN_22)
#define OLED_OLED_CS_IOMUX                                       (IOMUX_PINCM50)
/* Defines for L_A: GPIOA.7 with pinCMx 14 on package pin 49 */
#define Encoder_L_A_PORT                                                 (GPIOA)
// pins affected by this interrupt request:["L_A","R_A"]
#define Encoder_INT_IRQN                                        (GPIOA_INT_IRQn)
#define Encoder_INT_IIDX                        (DL_INTERRUPT_GROUP1_IIDX_GPIOA)
#define Encoder_L_A_IIDX                                     (DL_GPIO_IIDX_DIO7)
#define Encoder_L_A_PIN                                          (DL_GPIO_PIN_7)
#define Encoder_L_A_IOMUX                                        (IOMUX_PINCM14)
/* Defines for L_B: GPIOA.3 with pinCMx 8 on package pin 43 */
#define Encoder_L_B_PORT                                                 (GPIOA)
#define Encoder_L_B_PIN                                          (DL_GPIO_PIN_3)
#define Encoder_L_B_IOMUX                                         (IOMUX_PINCM8)
/* Defines for R_A: GPIOA.8 with pinCMx 19 on package pin 54 */
#define Encoder_R_A_PORT                                                 (GPIOA)
#define Encoder_R_A_IIDX                                     (DL_GPIO_IIDX_DIO8)
#define Encoder_R_A_PIN                                          (DL_GPIO_PIN_8)
#define Encoder_R_A_IOMUX                                        (IOMUX_PINCM19)
/* Defines for R_B: GPIOB.7 with pinCMx 24 on package pin 59 */
#define Encoder_R_B_PORT                                                 (GPIOB)
#define Encoder_R_B_PIN                                          (DL_GPIO_PIN_7)
#define Encoder_R_B_IOMUX                                        (IOMUX_PINCM24)
/* Port definition for Pin Group Motor */
#define Motor_PORT                                                       (GPIOB)

/* Defines for Motor_IO1: GPIOB.4 with pinCMx 17 on package pin 52 */
#define Motor_Motor_IO1_PIN                                      (DL_GPIO_PIN_4)
#define Motor_Motor_IO1_IOMUX                                    (IOMUX_PINCM17)
/* Defines for Motor_IO2: GPIOB.5 with pinCMx 18 on package pin 53 */
#define Motor_Motor_IO2_PIN                                      (DL_GPIO_PIN_5)
#define Motor_Motor_IO2_IOMUX                                    (IOMUX_PINCM18)
/* Port definition for Pin Group Tracking */
#define Tracking_PORT                                                    (GPIOA)

/* Defines for S0: GPIOA.26 with pinCMx 59 on package pin 30 */
#define Tracking_S0_PIN                                         (DL_GPIO_PIN_26)
#define Tracking_S0_IOMUX                                        (IOMUX_PINCM59)
/* Defines for S1: GPIOA.25 with pinCMx 55 on package pin 26 */
#define Tracking_S1_PIN                                         (DL_GPIO_PIN_25)
#define Tracking_S1_IOMUX                                        (IOMUX_PINCM55)
/* Defines for S2: GPIOA.24 with pinCMx 54 on package pin 25 */
#define Tracking_S2_PIN                                         (DL_GPIO_PIN_24)
#define Tracking_S2_IOMUX                                        (IOMUX_PINCM54)
/* Port definition for Pin Group LSM6DSR */
#define LSM6DSR_PORT                                                     (GPIOA)

/* Defines for LSM6DSR_SCL: GPIOA.12 with pinCMx 34 on package pin 5 */
#define LSM6DSR_LSM6DSR_SCL_PIN                                 (DL_GPIO_PIN_12)
#define LSM6DSR_LSM6DSR_SCL_IOMUX                                (IOMUX_PINCM34)
/* Defines for LSM6DSR_MISO: GPIOA.14 with pinCMx 36 on package pin 7 */
#define LSM6DSR_LSM6DSR_MISO_PIN                                (DL_GPIO_PIN_14)
#define LSM6DSR_LSM6DSR_MISO_IOMUX                               (IOMUX_PINCM36)
/* Defines for LSM6DSR_MOSI: GPIOA.13 with pinCMx 35 on package pin 6 */
#define LSM6DSR_LSM6DSR_MOSI_PIN                                (DL_GPIO_PIN_13)
#define LSM6DSR_LSM6DSR_MOSI_IOMUX                               (IOMUX_PINCM35)
/* Defines for LSM6DSR_CS: GPIOA.2 with pinCMx 7 on package pin 42 */
#define LSM6DSR_LSM6DSR_CS_PIN                                   (DL_GPIO_PIN_2)
#define LSM6DSR_LSM6DSR_CS_IOMUX                                  (IOMUX_PINCM7)



/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_PWM_Motor_init(void);
void SYSCFG_DL_PWM_Servo_A0_init(void);
void SYSCFG_DL_PWM_Servo_G0_init(void);
void SYSCFG_DL_TIMER_0_init(void);
void SYSCFG_DL_UART_0_init(void);
void SYSCFG_DL_ADC_Tracking_init(void);

void SYSCFG_DL_SYSTICK_init(void);

bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
