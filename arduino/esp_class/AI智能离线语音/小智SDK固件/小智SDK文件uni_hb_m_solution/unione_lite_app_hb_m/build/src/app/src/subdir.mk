################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/app/src/main.c \
../src/app/src/uni_record_save.c \
../src/app/src/uni_session.c \
../src/app/src/flash_boot.c \
../src/app/src/uni_session_manage.c \
../src/app/src/uni_user_meeting.c

OBJS += \
./src/app/src/main.o \
./src/app/src/uni_record_save.o \
./src/app/src/uni_session.o \
./src/app/src/flash_boot.o \
./src/app/src/uni_session_manage.o \
./src/app/src/uni_user_meeting.o

C_DEPS += \
./src/app/src/main.d \
./src/app/src/uni_record_save.d \
./src/app/src/uni_session.d \
./src/app/src/flash_boot.d \
./src/app/src/uni_session_manage.d \
./src/app/src/uni_user_meeting.d


# Each subdirectory must supply rules for building sources it contributes
src/app/src/%.o: ../src/app/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DFUNC_OS_EN=1 -DFLASH_BOOT_EN=$(UART_UPDATE) -I"$(CODE_ROOT)/middleware/rtos/freertos/inc" -I"$(CODE_ROOT)/middleware/rtos/rtos_api"   -I"$(CODE_ROOT)/src" -I"$(CODE_ROOT)/src/app/inc" -I"$(CODE_ROOT)" -I"$(CODE_ROOT)/include" -I"$(CODE_ROOT)/user/inc" -I"$(CODE_ROOT)/include/include" -I"$(CODE_ROOT)/include/include/osal"  -I"$(CODE_ROOT)/src/utils/float2string/inc" -I"$(CODE_ROOT)/src/utils/hash/inc" -I"$(CODE_ROOT)/src/utils/crc16/inc" -I"$(CODE_ROOT)/src/utils/interruptable_sleep/inc" -I"$(CODE_ROOT)/src/utils/log/inc" -I"$(CODE_ROOT)/src/utils/event_list/inc" -I"$(CODE_ROOT)/src/utils/event/inc" -I"$(CODE_ROOT)/src/utils/event_route/inc" -I"$(CODE_ROOT)/src/utils/list/inc" -I"$(CODE_ROOT)/src/utils/data_buf/inc" -I"$(CODE_ROOT)/src/utils/black_board/inc" -I"$(CODE_ROOT)/src/utils/cJSON/inc" -I"$(CODE_ROOT)/src/utils/config/inc" -I"$(CODE_ROOT)/src/utils/fsm/inc" -I"$(CODE_ROOT)/src/utils/float2string/inc" -I"$(CODE_ROOT)/src/utils/arpt/inc" -I"$(CODE_ROOT)/src/utils/timer/inc" -I"$(CODE_ROOT)/src/utils/auto_string/inc" -I"$(CODE_ROOT)/src/utils/string/inc" -I"$(CODE_ROOT)/src/utils/bitmap/inc" -I"$(CODE_ROOT)/src/utils/crc16/inc" -I"$(CODE_ROOT)/src/utils/hash/inc" -I"$(CODE_ROOT)/src/utils/uart/inc" -I"$(CODE_ROOT)/src/app/inc" -I"$(CODE_ROOT)/src/app/inc/sessions" -I"$(CODE_ROOT)/src/hal/inc" -I"$(CODE_ROOT)/src/sdk/audio/audio_common/inc" -I"$(CODE_ROOT)/src/sdk/audio/audio_player/inc" -I"$(CODE_ROOT)/src/sdk/idle_detect/inc" -I"$(CODE_ROOT)/src/sdk/player/inc" -I"$(CODE_ROOT)/src/sdk/player/src/pcm/inc" -I"$(CODE_ROOT)/src/sdk/vui/inc" -I"$(CODE_ROOT)/src/sdk/uart/inc" -O1 -g3 -mcmodel=medium -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -fsingle-precision-constant -ffunction-sections -fdata-sections -mext-dsp -mext-zol -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


