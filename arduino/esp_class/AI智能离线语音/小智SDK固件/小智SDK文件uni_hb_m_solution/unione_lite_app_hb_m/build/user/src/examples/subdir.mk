################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../user/src/examples/hb_smart_ac.c \
../user/src/examples/hb_uart.c \
../user/src/examples/hb_uart_ucp.c \
../user/src/examples/ac_device_simulator.c \
../user/src/examples/hb_housekeeper.c \
../user/src/examples/hb_gpio_adc.c \
../user/src/examples/hb_gpio_key.c \
../user/src/examples/hb_pwm_led.c \
../user/src/examples/hb_timer_buzzer.c \
../user/src/examples/hb_sw_timers.c \
../user/src/examples/hb_flash_example.c \
. /user/src/examples/hb_asr_control.c \
../user/src/examples/hb_player.c \
../user/src/examples/housekeeper_irc.c \
../user/src/examples/hb_uart_sucp.c \
../user/src/examples/hb_auto_gpio.c \
../user/src/examples/hb_power_sleep.c

OBJS += \
./user/src/examples/hb_smart_ac.o \
./user/src/examples/hb_uart.o \
./user/src/examples/hb_uart_ucp.o \
./user/src/examples/ac_device_simulator.o \
./user/src/examples/hb_housekeeper.o \
./user/src/examples/hb_gpio_adc.o \
./user/src/examples/hb_gpio_key.o \
./user/src/examples/hb_pwm_led.o \
./user/src/examples/hb_timer_buzzer.o \
./user/src/examples/hb_sw_timers.o \
./user/src/examples/hb_flash_example.o \
./user/src/examples/hb_asr_control.o \
./user/src/examples/hb_player.o \
./user/src/examples/housekeeper_irc.o \
./user/src/examples/hb_uart_sucp.o \
./user/src/examples/hb_auto_gpio.o \
./user/src/examples/hb_power_sleep.o

C_DEPS += \
./user/src/examples/hb_smart_ac.d \
./user/src/examples/hb_uart.d \
./user/src/examples/hb_uart_ucp.d \
./user/src/examples/ac_device_simulator.d \
./user/src/examples/hb_housekeeper.d \
./user/src/examples/hb_gpio_adc.d \
./user/src/examples/hb_gpio_key.d \
./user/src/examples/hb_pwm_led.d \
./user/src/examples/hb_timer_buzzer.d \
./user/src/examples/hb_sw_timers.d \
./user/src/examples/hb_flash_example.d \
./user/src/examples/hb_asr_control.d \
./user/src/examples/hb_player.d \
./user/src/examples/housekeeper_irc.d \
./user/src/examples/hb_uart_sucp.d \
./user/src/examples/hb_auto_gpio.d \
./user/src/examples/hb_power_sleep.d

# Each subdirectory must supply rules for building sources it contributes
user/src/examples/%.o: ../user/src/examples/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DFUNC_OS_EN=1 -DFLASH_BOOT_EN=$(UART_UPDATE) -I"$(CODE_ROOT)/middleware/rtos/freertos/inc" -I"$(CODE_ROOT)/middleware/rtos/rtos_api"   -I"$(CODE_ROOT)/src" -I"$(CODE_ROOT)/src/app/inc" -I"$(CODE_ROOT)" -I"$(CODE_ROOT)/include" -I"$(CODE_ROOT)/user/inc" -I"$(CODE_ROOT)/include/include" -I"$(CODE_ROOT)/include/include/osal"  -I"$(CODE_ROOT)/src/utils/float2string/inc" -I"$(CODE_ROOT)/src/utils/hash/inc" -I"$(CODE_ROOT)/src/utils/crc16/inc" -I"$(CODE_ROOT)/src/utils/interruptable_sleep/inc" -I"$(CODE_ROOT)/src/utils/log/inc" -I"$(CODE_ROOT)/src/utils/event_list/inc" -I"$(CODE_ROOT)/src/utils/event/inc" -I"$(CODE_ROOT)/src/utils/event_route/inc" -I"$(CODE_ROOT)/src/utils/list/inc" -I"$(CODE_ROOT)/src/utils/data_buf/inc" -I"$(CODE_ROOT)/src/utils/black_board/inc" -I"$(CODE_ROOT)/src/utils/cJSON/inc" -I"$(CODE_ROOT)/src/utils/config/inc" -I"$(CODE_ROOT)/src/utils/fsm/inc" -I"$(CODE_ROOT)/src/utils/float2string/inc" -I"$(CODE_ROOT)/src/utils/arpt/inc" -I"$(CODE_ROOT)/src/utils/timer/inc" -I"$(CODE_ROOT)/src/utils/auto_string/inc" -I"$(CODE_ROOT)/src/utils/string/inc" -I"$(CODE_ROOT)/src/utils/bitmap/inc" -I"$(CODE_ROOT)/src/utils/crc16/inc" -I"$(CODE_ROOT)/src/utils/hash/inc" -I"$(CODE_ROOT)/src/utils/uart/inc" -I"$(CODE_ROOT)/src/app/inc" -I"$(CODE_ROOT)/src/app/inc/sessions" -I"$(CODE_ROOT)/src/hal/inc" -I"$(CODE_ROOT)/src/sdk/audio/audio_common/inc" -I"$(CODE_ROOT)/src/sdk/audio/audio_player/inc" -I"$(CODE_ROOT)/src/sdk/idle_detect/inc" -I"$(CODE_ROOT)/src/sdk/player/inc" -I"$(CODE_ROOT)/src/sdk/player/src/pcm/inc" -I"$(CODE_ROOT)/src/sdk/vui/inc" -I"$(CODE_ROOT)/src/sdk/uart/inc" -I"$(CODE_ROOT)/user/src/examples/protocol" -I"$(CODE_ROOT)/user/src/examples/drivers" -I"$(CODE_ROOT)/user/src/examples" -O1 -g3 -mcmodel=medium -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -fsingle-precision-constant -ffunction-sections -fdata-sections -mext-dsp -mext-zol -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

-include user/src/examples/drivers/subdir.mk
