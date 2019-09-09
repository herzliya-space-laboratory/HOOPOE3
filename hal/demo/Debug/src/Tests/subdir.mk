################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Tests/ADCtest.c \
../src/Tests/FloatingPointTest.c \
../src/Tests/I2CslaveTest.c \
../src/Tests/I2Ctest.c \
../src/Tests/LEDtest.c \
../src/Tests/PWMtest.c \
../src/Tests/PinTest.c \
../src/Tests/SDCardTest.c \
../src/Tests/SPI_FRAM_RTCtest.c \
../src/Tests/SupervisorTest.c \
../src/Tests/ThermalTest.c \
../src/Tests/TimeTest.c \
../src/Tests/UARTtest.c \
../src/Tests/USBdeviceTest.c \
../src/Tests/boardTest.c \
../src/Tests/checksumTest.c 

OBJS += \
./src/Tests/ADCtest.o \
./src/Tests/FloatingPointTest.o \
./src/Tests/I2CslaveTest.o \
./src/Tests/I2Ctest.o \
./src/Tests/LEDtest.o \
./src/Tests/PWMtest.o \
./src/Tests/PinTest.o \
./src/Tests/SDCardTest.o \
./src/Tests/SPI_FRAM_RTCtest.o \
./src/Tests/SupervisorTest.o \
./src/Tests/ThermalTest.o \
./src/Tests/TimeTest.o \
./src/Tests/UARTtest.o \
./src/Tests/USBdeviceTest.o \
./src/Tests/boardTest.o \
./src/Tests/checksumTest.o 

C_DEPS += \
./src/Tests/ADCtest.d \
./src/Tests/FloatingPointTest.d \
./src/Tests/I2CslaveTest.d \
./src/Tests/I2Ctest.d \
./src/Tests/LEDtest.d \
./src/Tests/PWMtest.d \
./src/Tests/PinTest.d \
./src/Tests/SDCardTest.d \
./src/Tests/SPI_FRAM_RTCtest.d \
./src/Tests/SupervisorTest.d \
./src/Tests/ThermalTest.d \
./src/Tests/TimeTest.d \
./src/Tests/UARTtest.d \
./src/Tests/USBdeviceTest.d \
./src/Tests/boardTest.d \
./src/Tests/checksumTest.d 


# Each subdirectory must supply rules for building sources it contributes
src/Tests/%.o: ../src/Tests/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=arm926ej-s -O0 -fmessage-length=0 -ffunction-sections -Wall -Wextra -Werror  -g -Dsdram -Dat91sam9g20 -DTRACE_LEVEL=5 -DDEBUG=1 -D'BASE_REVISION_NUMBER=$(REV)' -D'BASE_REVISION_HASH_SHORT=$(REVHASH_SHORT)' -D'BASE_REVISION_HASH=$(REVHASH)' -I"C:\Users\user1\Documents\hoopoe-2\hal\demo\src" -I"C:/Users/user1/Documents/hoopoe-2/hal/demo/..//freertos/include" -I"C:/Users/user1/Documents/hoopoe-2/hal/demo/..//hal/include" -I"C:/Users/user1/Documents/hoopoe-2/hal/demo/..//at91/include" -I"C:/Users/user1/Documents/hoopoe-2/hal/demo/..//hcc/include" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


