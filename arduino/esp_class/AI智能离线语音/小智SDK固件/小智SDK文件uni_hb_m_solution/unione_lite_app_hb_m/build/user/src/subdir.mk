################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../user/src/user_asr.c \
../user/src/user_event.c \
../user/src/user_file.c \
../user/src/user_flash.c \
../user/src/user_gpio.c \
../user/src/user_main.c \
../user/src/user_player.c \
../user/src/user_power.c \
../user/src/user_record.c \
../user/src/user_uart.c \
../user/src/user_uni_ucp.c \
../user/src/user_timer.c \
../user/src/user_pwm.c \
../user/src/user_uni_sucp.c \
../user/src/user_start.c

OBJS += \
./user/src/user_asr.o \
./user/src/user_event.o \
./user/src/user_file.o \
./user/src/user_flash.o \
./user/src/user_gpio.o \
./user/src/user_main.o \
./user/src/user_player.o \
./user/src/user_power.o \
./user/src/user_record.o \
./user/src/user_uart.o \
./user/src/user_uni_ucp.o \
./user/src/user_timer.o \
./user/src/user_pwm.o \
./user/src/user_uni_sucp.o \
./user/src/user_start.o

C_DEPS += \
./user/src/user_asr.d \
./user/src/user_event.d \
./user/src/user_file.d \
./user/src/user_flash.d \
./user/src/user_gpio.d \
./user/src/user_main.d \
./user/src/user_player.d \
./user/src/user_power.d \
./user/src/user_record.d \
./user/src/user_uart.d \
./user/src/user_uni_ucp.d \
./user/src/user_timer.d \
./user/src/user_pwm.d \
./user/src/user_uni_sucp.d \
./user/src/user_start.d

# Each subdirectory must supply rules for building sources it contributes
user/src/%.o: ../user/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DFUNC_OS_EN=1 -DFLASH_BOOT_EN=$(UART_UPDATE) -I"$(CODE_ROOT)/middleware/rtos/freertos/inc" -I"$(CODE_ROOT)/middleware/rtos/rtos_api"   -I"$(CODE_ROOT)/src" -I"$(CODE_ROOT)/src/app/inc" -I"$(CODE_ROOT)" -I"$(CODE_ROOT)/include" -I"$(CODE_ROOT)/user/inc" -I"$(CODE_ROOT)/include/include" -I"$(CODE_ROOT)/include/include/osal"  -I"$(CODE_ROOT)/src/utils/float2string/inc" -I"$(CODE_ROOT)/src/utils/hash/inc" -I"$(CODE_ROOT)/src/utils/crc16/inc" -I"$(CODE_ROOT)/src/utils/interruptable_sleep/inc" -I"$(CODE_ROOT)/src/utils/log/inc" -I"$(CODE_ROOT)/src/utils/event_list/inc" -I"$(CODE_ROOT)/src/utils/event/inc" -I"$(CODE_ROOT)/src/utils/event_route/inc" -I"$(CODE_ROOT)/src/utils/list/inc" -I"$(CODE_ROOT)/src/utils/data_buf/inc" -I"$(CODE_ROOT)/src/utils/black_board/inc" -I"$(CODE_ROOT)/src/utils/cJSON/inc" -I"$(CODE_ROOT)/src/utils/config/inc" -I"$(CODE_ROOT)/src/utils/fsm/inc" -I"$(CODE_ROOT)/src/utils/float2string/inc" -I"$(CODE_ROOT)/src/utils/arpt/inc" -I"$(CODE_ROOT)/src/utils/timer/inc" -I"$(CODE_ROOT)/src/utils/auto_string/inc" -I"$(CODE_ROOT)/src/utils/string/inc" -I"$(CODE_ROOT)/src/utils/bitmap/inc" -I"$(CODE_ROOT)/src/utils/crc16/inc" -I"$(CODE_ROOT)/src/utils/hash/inc" -I"$(CODE_ROOT)/src/utils/uart/inc" -I"$(CODE_ROOT)/src/app/inc" -I"$(CODE_ROOT)/src/app/inc/sessions" -I"$(CODE_ROOT)/src/hal/inc" -I"$(CODE_ROOT)/src/sdk/audio/audio_common/inc" -I"$(CODE_ROOT)/src/sdk/audio/audio_player/inc" -I"$(CODE_ROOT)/src/sdk/idle_detect/inc" -I"$(CODE_ROOT)/src/sdk/player/inc" -I"$(CODE_ROOT)/src/sdk/player/src/pcm/inc" -I"$(CODE_ROOT)/src/sdk/vui/inc" -I"$(CODE_ROOT)/src/sdk/uart/inc" -I"$(CODE_ROOT)/user/src/examples/protocol" -I"$(CODE_ROOT)/user/src/examples/drivers" -I"$(CODE_ROOT)/user/src/examples" -O1 -g3 -mcmodel=medium -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -fsingle-precision-constant -ffunction-sections -fdata-sections -mext-dsp -mext-zol -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


