################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/sub-systemCode/payload/ImgCompressor/subdir.mk
-include src/sub-systemCode/payload/subdir.mk
-include src/sub-systemCode/Tests/subdir.mk
-include src/sub-systemCode/Main/CMD/subdir.mk
-include src/sub-systemCode/Main/subdir.mk
-include src/sub-systemCode/Global/subdir.mk
-include src/sub-systemCode/COMM/subdir.mk
-include src/sub-systemCode/ADCS/subdir.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
Elai.bin \

SECONDARY_SIZE += \
Elai.siz \


# All Target
all: Elai.elf secondary-outputs

# Tool invocations
Elai.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Cross ARM C Linker'
	arm-none-eabi-gcc -mcpu=arm926ej-s -O0 -fmessage-length=0 -ffunction-sections -Wall -Wextra  -g -T C:/Users/user1/Desktop/Elai/satellite-subsystems/demo/../..//hal/at91/linker-scripts/sdram.lds -nostartfiles -Xlinker --gc-sections -L"C:/Users/user1/Desktop/Elai/satellite-subsystems/demo/../..//hal/at91/lib" -L"C:/Users/user1/Desktop/Elai/satellite-subsystems/demo/../..//hal/freertos/lib" -L"C:/Users/user1/Desktop/Elai/satellite-subsystems/demo/../..//hal/hal/lib" -L"C:/Users/user1/Desktop/Elai/satellite-subsystems/demo/../..//hal/hcc/lib" -L"C:/Users/user1/Desktop/Elai/satellite-subsystems/demo/..//satellite-subsystems/lib" -Wl,-Map,"Elai.map" --specs=nano.specs -lc -u _printf_float -u _scanf_float -o "Elai.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

Elai.bin: Elai.elf
	@echo 'Invoking: Cross ARM GNU Create Flash Image'
	arm-none-eabi-objcopy -O binary "Elai.elf"  "Elai.bin"
	@echo 'Finished building: $@'
	@echo ' '

Elai.siz: Elai.elf
	@echo 'Invoking: Cross ARM GNU Print Size'
	arm-none-eabi-size --format=berkeley "Elai.elf"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(SECONDARY_SIZE)$(OBJS)$(C_DEPS)$(ASM_DEPS)$(SECONDARY_FLASH)$(S_UPPER_DEPS) Elai.elf
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_SIZE)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
