################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../Libraries/iLLD/TC38A/Tricore/Edsadc/Edsadc/IfxEdsadc_Edsadc.c" 

COMPILED_SRCS += \
"Libraries/iLLD/TC38A/Tricore/Edsadc/Edsadc/IfxEdsadc_Edsadc.src" 

C_DEPS += \
"./Libraries/iLLD/TC38A/Tricore/Edsadc/Edsadc/IfxEdsadc_Edsadc.d" 

OBJS += \
"Libraries/iLLD/TC38A/Tricore/Edsadc/Edsadc/IfxEdsadc_Edsadc.o" 


# Each subdirectory must supply rules for building sources it contributes
"Libraries/iLLD/TC38A/Tricore/Edsadc/Edsadc/IfxEdsadc_Edsadc.src":"../Libraries/iLLD/TC38A/Tricore/Edsadc/Edsadc/IfxEdsadc_Edsadc.c" "Libraries/iLLD/TC38A/Tricore/Edsadc/Edsadc/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 -D__CPU__=tc38x "-fF:/LQ_Project/TC264_297_3/TC387/TC387_SoftWare_LIB_260128/LQ_TC387_LIB_MBG_V7.0.1/Release/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O2 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/iLLD/TC38A/Tricore/Edsadc/Edsadc/IfxEdsadc_Edsadc.o":"Libraries/iLLD/TC38A/Tricore/Edsadc/Edsadc/IfxEdsadc_Edsadc.src" "Libraries/iLLD/TC38A/Tricore/Edsadc/Edsadc/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-Libraries-2f-iLLD-2f-TC38A-2f-Tricore-2f-Edsadc-2f-Edsadc

clean-Libraries-2f-iLLD-2f-TC38A-2f-Tricore-2f-Edsadc-2f-Edsadc:
	-$(RM) ./Libraries/iLLD/TC38A/Tricore/Edsadc/Edsadc/IfxEdsadc_Edsadc.d ./Libraries/iLLD/TC38A/Tricore/Edsadc/Edsadc/IfxEdsadc_Edsadc.o ./Libraries/iLLD/TC38A/Tricore/Edsadc/Edsadc/IfxEdsadc_Edsadc.src

.PHONY: clean-Libraries-2f-iLLD-2f-TC38A-2f-Tricore-2f-Edsadc-2f-Edsadc

