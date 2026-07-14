################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../Src/User/LQ_PID.c" \
"../Src/User/LQ_Quat.c" 

COMPILED_SRCS += \
"Src/User/LQ_PID.src" \
"Src/User/LQ_Quat.src" 

C_DEPS += \
"./Src/User/LQ_PID.d" \
"./Src/User/LQ_Quat.d" 

OBJS += \
"Src/User/LQ_PID.o" \
"Src/User/LQ_Quat.o" 


# Each subdirectory must supply rules for building sources it contributes
"Src/User/LQ_PID.src":"../Src/User/LQ_PID.c" "Src/User/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 -D__CPU__=tc38x "-fF:/LQ_Project/TC264_297_3/TC387/TC387_SoftWare_LIB_260128/LQ_TC387_LIB_MBG_V7.0.1/Release/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O2 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"Src/User/LQ_PID.o":"Src/User/LQ_PID.src" "Src/User/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"Src/User/LQ_Quat.src":"../Src/User/LQ_Quat.c" "Src/User/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 -D__CPU__=tc38x "-fF:/LQ_Project/TC264_297_3/TC387/TC387_SoftWare_LIB_260128/LQ_TC387_LIB_MBG_V7.0.1/Release/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O2 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"Src/User/LQ_Quat.o":"Src/User/LQ_Quat.src" "Src/User/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-Src-2f-User

clean-Src-2f-User:
	-$(RM) ./Src/User/LQ_PID.d ./Src/User/LQ_PID.o ./Src/User/LQ_PID.src ./Src/User/LQ_Quat.d ./Src/User/LQ_Quat.o ./Src/User/LQ_Quat.src

.PHONY: clean-Src-2f-User

