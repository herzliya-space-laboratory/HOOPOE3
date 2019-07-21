################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/Tests/subdir.mk
-include src/Demo/subdir.mk
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
hal-demo.bin \

SECONDARY_SIZE += \
hal-demo.siz \


# All Target
all: hal-demo.elf secondary-outputs

# Tool invocations
hal-demo.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Cross ARM C Linker'
	arm-none-eabi-gcc -mcpu=arm926ej-s -O0 -fmessage-length=0 -ffunction-sections -Wall -Wextra -Werror  -g -T C:/Users/user1/Documents/hoopoe-2/hal/demo/..//at91/linker-scripts/sdram.lds -nostartfiles -Xlinker --gc-sections -L"C:/Users/user1/Documents/hoopoe-2/hal/demo/..//at91/lib" -L"C:/Users/user1/Documents/hoopoe-2/hal/demo/..//freertos/lib" -L"C:/Users/user1/Documents/hoopoe-2/hal/demo/..//hal/lib" -L"C:/Users/user1/Documents/hoopoe-2/hal/demo/..//hcc/lib" -Wl,-Map,"hal-demo.map" --specs=nano.specs -lc -u _printf_float -u _scanf_float -o "hal-demo.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

hal-demo.bin: hal-demo.elf
	@echo 'Invoking: Cross ARM GNU Create Flash Image'
	arm-none-eabi-objcopy -O binary "hal-demo.elf"  "hal-demo.bin"
	@echo 'Finished building: $@'
	@echo ' '

hal-demo.siz: hal-demo.elf
	@echo 'Invoking: Cross ARM GNU Print Size'
	arm-none-eabi-size --format=berkeley "hal-demo.elf"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(SECONDARY_SIZE)$(OBJS)$(C_DEPS)$(ASM_DEPS)$(SECONDARY_FLASH)$(S_UPPER_DEPS) hal-demo.elf
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_SIZE)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
