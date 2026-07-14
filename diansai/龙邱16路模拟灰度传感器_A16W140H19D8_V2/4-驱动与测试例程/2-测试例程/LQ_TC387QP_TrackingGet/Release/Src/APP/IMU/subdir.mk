################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../Src/APP/IMU/LQ_IIC_Gyro.c" \
"../Src/APP/IMU/LQ_MPU6050_DMP_test.c" \
"../Src/APP/IMU/LQ_SPI_Gryo.c" 

O_SRCS += \
"../Src/APP/IMU/LQ_MPU6050_DMP.o" 

COMPILED_SRCS += \
"Src/APP/IMU/LQ_IIC_Gyro.src" \
"Src/APP/IMU/LQ_MPU6050_DMP_test.src" \
"Src/APP/IMU/LQ_SPI_Gryo.src" 

C_DEPS += \
"./Src/APP/IMU/LQ_IIC_Gyro.d" \
"./Src/APP/IMU/LQ_MPU6050_DMP_test.d" \
"./Src/APP/IMU/LQ_SPI_Gryo.d" 

OBJS += \
"Src/APP/IMU/LQ_IIC_Gyro.o" \
"Src/APP/IMU/LQ_MPU6050_DMP_test.o" \
"Src/APP/IMU/LQ_SPI_Gryo.o" 


# Each subdirectory must supply rules for building sources it contributes
"Src/APP/IMU/LQ_IIC_Gyro.src":"../Src/APP/IMU/LQ_IIC_Gyro.c" "Src/APP/IMU/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 -D__CPU__=tc38x "-fF:/LQ_Project/TC264_297_3/TC387/TC387_SoftWare_LIB_260128/LQ_TC387_LIB_MBG_V7.0.1/Release/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O2 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"Src/APP/IMU/LQ_IIC_Gyro.o":"Src/APP/IMU/LQ_IIC_Gyro.src" "Src/APP/IMU/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"Src/APP/IMU/LQ_MPU6050_DMP_test.src":"../Src/APP/IMU/LQ_MPU6050_DMP_test.c" "Src/APP/IMU/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 -D__CPU__=tc38x "-fF:/LQ_Project/TC264_297_3/TC387/TC387_SoftWare_LIB_260128/LQ_TC387_LIB_MBG_V7.0.1/Release/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O2 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"Src/APP/IMU/LQ_MPU6050_DMP_test.o":"Src/APP/IMU/LQ_MPU6050_DMP_test.src" "Src/APP/IMU/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"Src/APP/IMU/LQ_SPI_Gryo.src":"../Src/APP/IMU/LQ_SPI_Gryo.c" "Src/APP/IMU/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 -D__CPU__=tc38x "-fF:/LQ_Project/TC264_297_3/TC387/TC387_SoftWare_LIB_260128/LQ_TC387_LIB_MBG_V7.0.1/Release/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O2 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"Src/APP/IMU/LQ_SPI_Gryo.o":"Src/APP/IMU/LQ_SPI_Gryo.src" "Src/APP/IMU/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-Src-2f-APP-2f-IMU

clean-Src-2f-APP-2f-IMU:
	-$(RM) ./Src/APP/IMU/LQ_IIC_Gyro.d ./Src/APP/IMU/LQ_IIC_Gyro.o ./Src/APP/IMU/LQ_IIC_Gyro.src ./Src/APP/IMU/LQ_MPU6050_DMP_test.d ./Src/APP/IMU/LQ_MPU6050_DMP_test.o ./Src/APP/IMU/LQ_MPU6050_DMP_test.src ./Src/APP/IMU/LQ_SPI_Gryo.d ./Src/APP/IMU/LQ_SPI_Gryo.o ./Src/APP/IMU/LQ_SPI_Gryo.src

.PHONY: clean-Src-2f-APP-2f-IMU

