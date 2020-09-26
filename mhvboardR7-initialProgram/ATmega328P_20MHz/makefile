################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
LSS += \
mhvboardR7-initialProgram.lss \

FLASH_IMAGE += \
mhvboardR7-initialProgram.hex \

EEPROM_IMAGE += \
mhvboardR7-initialProgram.eep \

SIZEDUMMY += \
sizedummy \


# All Target
all: mhvboardR7-initialProgram.elf secondary-outputs

# Tool invocations
mhvboardR7-initialProgram.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: AVR C++ Linker'
	avr-g++ -Wl,-Map,mhvboardR7-initialProgram.map,--cref -Wl,--gc-sections -L"A:\eclipse\mhvlib/ATmega328P_20MHz" -mmcu=atmega328p -o"mhvboardR7-initialProgram.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

mhvboardR7-initialProgram.lss: mhvboardR7-initialProgram.elf
	@echo 'Invoking: AVR Create Extended Listing'
	-avr-objdump -h -S mhvboardR7-initialProgram.elf  >"mhvboardR7-initialProgram.lss"
	@echo 'Finished building: $@'
	@echo ' '

mhvboardR7-initialProgram.hex: mhvboardR7-initialProgram.elf
	@echo 'Create Flash image (ihex format)'
	-avr-objcopy -R .eeprom -O ihex mhvboardR7-initialProgram.elf  "mhvboardR7-initialProgram.hex"
	@echo 'Finished building: $@'
	@echo ' '

mhvboardR7-initialProgram.eep: mhvboardR7-initialProgram.elf
	@echo 'Create eeprom image (ihex format)'
	-avr-objcopy -j .eeprom --no-change-warnings --change-section-lma .eeprom=0 -O ihex mhvboardR7-initialProgram.elf  "mhvboardR7-initialProgram.eep"
	@echo 'Finished building: $@'
	@echo ' '

sizedummy: mhvboardR7-initialProgram.elf
	@echo 'Invoking: Print Size'
	-avr-size --format=avr --mcu=atmega328p mhvboardR7-initialProgram.elf
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EEPROM_IMAGE)$(ELFS)$(FLASH_IMAGE)$(LSS)$(S_DEPS)$(CXX_DEPS)$(S_UPPER_DEPS)$(C++_DEPS)$(ASM_DEPS)$(CC_DEPS)$(CPP_DEPS)$(C_UPPER_DEPS)$(SIZEDUMMY) mhvboardR7-initialProgram.elf
	-@echo ' '

secondary-outputs: $(LSS) $(FLASH_IMAGE) $(EEPROM_IMAGE) $(SIZEDUMMY)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
