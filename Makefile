PERFIX = ./library
WCSTOOL = ${PERFIX}/wcstools-3.8.5
ASTROMETRY = ${PERFIX}/astrometry.net-0.38
CFITSIO = ${PERFIX}/cfitsio

POSTGRESQL = /usr/include/postgresql
#POSTGRESQL = /home/optdata/opt2/DB

LDIR = -L ${ASTROMETRY}/lib -L ${POSTGRESQL}/lib -L ${WCSTOOL}/lib
IDIR =  -I ${ASTROMETRY}/include -I ${POSTGRESQL}  -I ${WCSTOOL}/include
LIBS = -lcfitsio -lm -lpq -lanutils -lpthread -lwcs -lbackend

ALL:crossmatch createtable

crossmatch:
	g++ ${IDIR} src/CrossMatch.cpp src/CrossMatchSphere.cpp src/Partition.cpp src/PartitionSphere.cpp src/StarFile.cpp src/StarFileFits.cpp src/StoreDataPostgres.cpp src/cmutils.cpp src/main.cpp -o crossmatch  ${LDIR} ${LIBS}

createtable:
	g++ ${IDIR} src/cmutils.cpp src/CreateTable.cpp src/ctable.cpp -o createtable ${LDIR} ${LIBS}
clean:
	rm -rf *.o crossmatch createtable

