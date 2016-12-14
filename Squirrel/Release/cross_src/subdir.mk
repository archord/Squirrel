################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../cross_src/CreateTable.cpp \
../cross_src/CrossMatch.cpp \
../cross_src/CrossMatchSphere.cpp \
../cross_src/Partition.cpp \
../cross_src/PartitionSphere.cpp \
../cross_src/StarFile.cpp \
../cross_src/StarFileFits.cpp \
../cross_src/StoreDataPostgres.cpp \
../cross_src/cmutils.cpp \
../cross_src/ctable.cpp \
../cross_src/main.cpp 

OBJS += \
./cross_src/CreateTable.o \
./cross_src/CrossMatch.o \
./cross_src/CrossMatchSphere.o \
./cross_src/Partition.o \
./cross_src/PartitionSphere.o \
./cross_src/StarFile.o \
./cross_src/StarFileFits.o \
./cross_src/StoreDataPostgres.o \
./cross_src/cmutils.o \
./cross_src/ctable.o \
./cross_src/main.o 

CPP_DEPS += \
./cross_src/CreateTable.d \
./cross_src/CrossMatch.d \
./cross_src/CrossMatchSphere.d \
./cross_src/Partition.d \
./cross_src/PartitionSphere.d \
./cross_src/StarFile.d \
./cross_src/StarFileFits.d \
./cross_src/StoreDataPostgres.d \
./cross_src/cmutils.d \
./cross_src/ctable.d \
./cross_src/main.d 


# Each subdirectory must supply rules for building sources it contributes
cross_src/%.o: ../cross_src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


