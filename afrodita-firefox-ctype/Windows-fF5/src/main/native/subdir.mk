################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/main/native/AfroditaF.cpp \
../src/main/native/FFElement.cpp \
../src/main/native/FFWebApplication.cpp 

OBJS += \
./src/main/native/AfroditaF.o \
./src/main/native/FFElement.o \
./src/main/native/FFWebApplication.o 

CPP_DEPS += \
./src/main/native/AfroditaF.d \
./src/main/native/FFElement.d \
./src/main/native/FFWebApplication.d 


# Each subdirectory must supply rules for building sources it contributes
src/main/native/%.o: ../src/main/native/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	i586-mingw32msvc-g++ -DWIN32 -DXP_WIN=1 -DFF_VERSION=5 -I/usr/i586-mingw32msvc/include -O0 -g3 -Wall -c -fmessage-length=0 -fshort-wchar -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


