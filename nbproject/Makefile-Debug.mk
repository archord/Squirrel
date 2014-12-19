#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/_ext/1246618051/main.o \
	${OBJECTDIR}/src/CrossMatch.o \
	${OBJECTDIR}/src/CrossMatchSphere.o \
	${OBJECTDIR}/src/Partition.o \
	${OBJECTDIR}/src/PartitionSphere.o \
	${OBJECTDIR}/src/StarFile.o \
	${OBJECTDIR}/src/StarFileFits.o \
	${OBJECTDIR}/src/StoreDataPostgres.o \
	${OBJECTDIR}/src/cmutils.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-Llibrary/astrometry.net-0.38/lib -Llibrary/cfitsio/lib -Llibrary/wcstools-3.8.5/lib

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/squirrel

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/squirrel: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/squirrel ${OBJECTFILES} ${LDLIBSOPTIONS} -lcfitsio -lm -lpq -lanutils -lpthread -lwcs -lbackend

${OBJECTDIR}/_ext/1246618051/main.o: ../Squirrel/src/main.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/1246618051
	${RM} $@.d
	$(COMPILE.cc) -g -Ilibrary/astrometry.net-0.38/include -Ilibrary/cfitsio/include -Ilibrary/wcstools-3.8.5/include -I../CrossMatchLibrary/src -I/usr/include/postgresql -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/1246618051/main.o ../Squirrel/src/main.cpp

${OBJECTDIR}/src/CrossMatch.o: src/CrossMatch.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -Ilibrary/astrometry.net-0.38/include -Ilibrary/cfitsio/include -Ilibrary/wcstools-3.8.5/include -I../CrossMatchLibrary/src -I/usr/include/postgresql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/CrossMatch.o src/CrossMatch.cpp

${OBJECTDIR}/src/CrossMatchSphere.o: src/CrossMatchSphere.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -Ilibrary/astrometry.net-0.38/include -Ilibrary/cfitsio/include -Ilibrary/wcstools-3.8.5/include -I../CrossMatchLibrary/src -I/usr/include/postgresql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/CrossMatchSphere.o src/CrossMatchSphere.cpp

${OBJECTDIR}/src/Partition.o: src/Partition.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -Ilibrary/astrometry.net-0.38/include -Ilibrary/cfitsio/include -Ilibrary/wcstools-3.8.5/include -I../CrossMatchLibrary/src -I/usr/include/postgresql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/Partition.o src/Partition.cpp

${OBJECTDIR}/src/PartitionSphere.o: src/PartitionSphere.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -Ilibrary/astrometry.net-0.38/include -Ilibrary/cfitsio/include -Ilibrary/wcstools-3.8.5/include -I../CrossMatchLibrary/src -I/usr/include/postgresql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/PartitionSphere.o src/PartitionSphere.cpp

${OBJECTDIR}/src/StarFile.o: src/StarFile.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -Ilibrary/astrometry.net-0.38/include -Ilibrary/cfitsio/include -Ilibrary/wcstools-3.8.5/include -I../CrossMatchLibrary/src -I/usr/include/postgresql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/StarFile.o src/StarFile.cpp

${OBJECTDIR}/src/StarFileFits.o: src/StarFileFits.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -Ilibrary/astrometry.net-0.38/include -Ilibrary/cfitsio/include -Ilibrary/wcstools-3.8.5/include -I../CrossMatchLibrary/src -I/usr/include/postgresql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/StarFileFits.o src/StarFileFits.cpp

${OBJECTDIR}/src/StoreDataPostgres.o: src/StoreDataPostgres.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -Ilibrary/astrometry.net-0.38/include -Ilibrary/cfitsio/include -Ilibrary/wcstools-3.8.5/include -I../CrossMatchLibrary/src -I/usr/include/postgresql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/StoreDataPostgres.o src/StoreDataPostgres.cpp

${OBJECTDIR}/src/cmutils.o: src/cmutils.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -Ilibrary/astrometry.net-0.38/include -Ilibrary/cfitsio/include -Ilibrary/wcstools-3.8.5/include -I../CrossMatchLibrary/src -I/usr/include/postgresql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cmutils.o src/cmutils.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/squirrel

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
