################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../Alluxio.cc \
../AlluxioTest.cc \
../JNIHelper.cc \
../Util.cc 

CC_DEPS += \
./Alluxio.d \
./AlluxioTest.d \
./JNIHelper.d \
./Util.d 

OBJS += \
./Alluxio.o \
./AlluxioTest.o \
./JNIHelper.o \
./Util.o 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


