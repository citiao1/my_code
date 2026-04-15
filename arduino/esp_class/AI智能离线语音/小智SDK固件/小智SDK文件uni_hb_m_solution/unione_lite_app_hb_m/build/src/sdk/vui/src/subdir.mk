################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/sdk/vui/src/uni_lasr_result_parser.c \
../src/sdk/vui/src/uni_nlu.c \
../src/sdk/vui/src/uni_recog_engine_control.c \
../src/sdk/vui/src/uni_recog_preproc.c \
../src/sdk/vui/src/uni_recog_service.c \
../src/sdk/vui/src/uni_vui_interface.c 

OBJS += \
./src/sdk/vui/src/uni_lasr_result_parser.o \
./src/sdk/vui/src/uni_nlu.o \
./src/sdk/vui/src/uni_recog_engine_control.o \
./src/sdk/vui/src/uni_recog_preproc.o \
./src/sdk/vui/src/uni_recog_service.o \
./src/sdk/vui/src/uni_vui_interface.o 

C_DEPS += \
./src/sdk/vui/src/uni_lasr_result_parser.d \
./src/sdk/vui/src/uni_nlu.d \
./src/sdk/vui/src/uni_recog_engine_control.d \
./src/sdk/vui/src/uni_recog_preproc.d \
./src/sdk/vui/src/uni_recog_service.d \
./src/sdk/vui/src/uni_vui_interface.d 


# Each subdirectory must supply rules for building sources it contributes
src/sdk/vui/src/%.o: ../src/sdk/vui/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DFUNC_OS_EN=1 -DFLASH_BOOT_EN=$(UART_UPDATE) -I"$(CODE_ROOT)/middleware/rtos/freertos/inc" -I"$(CODE_ROOT)/middleware/rtos/rtos_api"   -I"$(CODE_ROOT)/src" -I"$(CODE_ROOT)/src/app/inc" -I"$(CODE_ROOT)" -I"$(CODE_ROOT)/include" -I"$(CODE_ROOT)/user/inc" -I"$(CODE_ROOT)/include/include" -I"$(CODE_ROOT)/include/include/osal"  -I"$(CODE_ROOT)/src/utils/float2string/inc" -I"$(CODE_ROOT)/src/utils/hash/inc" -I"$(CODE_ROOT)/src/utils/crc16/inc" -I"$(CODE_ROOT)/src/utils/interruptable_sleep/inc" -I"$(CODE_ROOT)/src/utils/log/inc" -I"$(CODE_ROOT)/src/utils/event_list/inc" -I"$(CODE_ROOT)/src/utils/event/inc" -I"$(CODE_ROOT)/src/utils/event_route/inc" -I"$(CODE_ROOT)/src/utils/list/inc" -I"$(CODE_ROOT)/src/utils/data_buf/inc" -I"$(CODE_ROOT)/src/utils/black_board/inc" -I"$(CODE_ROOT)/src/utils/cJSON/inc" -I"$(CODE_ROOT)/src/utils/config/inc" -I"$(CODE_ROOT)/src/utils/fsm/inc" -I"$(CODE_ROOT)/src/utils/float2string/inc" -I"$(CODE_ROOT)/src/utils/arpt/inc" -I"$(CODE_ROOT)/src/utils/timer/inc" -I"$(CODE_ROOT)/src/utils/auto_string/inc" -I"$(CODE_ROOT)/src/utils/string/inc" -I"$(CODE_ROOT)/src/utils/bitmap/inc" -I"$(CODE_ROOT)/src/utils/crc16/inc" -I"$(CODE_ROOT)/src/utils/hash/inc" -I"$(CODE_ROOT)/src/utils/uart/inc" -I"$(CODE_ROOT)/src/app/inc" -I"$(CODE_ROOT)/src/app/inc/sessions" -I"$(CODE_ROOT)/src/hal/inc" -I"$(CODE_ROOT)/src/sdk/audio/audio_common/inc" -I"$(CODE_ROOT)/src/sdk/audio/audio_player/inc" -I"$(CODE_ROOT)/src/sdk/idle_detect/inc" -I"$(CODE_ROOT)/src/sdk/player/inc" -I"$(CODE_ROOT)/src/sdk/player/src/pcm/inc" -I"$(CODE_ROOT)/src/sdk/vui/inc" -I"$(CODE_ROOT)/src/sdk/uart/inc" -O1 -g3 -mcmodel=medium -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -fsingle-precision-constant -ffunction-sections -fdata-sections -mext-dsp -mext-zol -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


