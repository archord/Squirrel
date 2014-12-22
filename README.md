Squirrel
========

Makefile is the Netbeans IDE's default auto compile.
Makefile1 is used for manual compile.

run example
./crossmatch  -method sphere -errorRadius 0.06 -fitsHDU 3 -ref  data/P_130112_177d55072_60d851283-472.Fcat -sample data/P_130112_177d55072_60d851283-473.Fcat -dbConfigFile data/database_config.txt -fluxSDTimes 1 -grid 2

note:
1,remember add library/astrometry.net-0.38/lib to LD_LIBRARY_PATH
export LD_LIBRARY_PATH=project_path/library/astrometry.net-0.38/lib:${LD_LIBRARY_PATH}

2,install libpq-dev library to use postgres database
/usr/include/postgresql/libpq-fe.h
for Ubuntu systems: sudo apt-get install libpq-dev
for RHEL systems: yum install postgresql-devel
for Mac: brew install postgresql
