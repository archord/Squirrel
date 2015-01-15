Squirrel
========

Makefile is the Netbeans IDE's default auto compile.
Makefile1 is used for manual compile.

run example
./crossmatch -method sphere -grid 4,4 -errorRadius 0.06 -fitsHDU 3 -ref  data/P_130112_25d4458_33d9878-210.Fcat -sample data/P_130112_25d4458_33d9878-119.Fcat -dbConfigFile data/database_config.txt -fluxSDTimes 1
./crossmatch -method plane  -g 4,4 -errorRadius 1.5 -searchRadius 50 -width 3096 -height 3096 -fitsHDU 3 -ref  data/P_130112_25d4458_33d9878-210.Fcat -sample data/P_130112_25d4458_33d9878-119.Fcat -dbConfigFile data/database_config.txt -fluxSDTimes 1

note:
1,remember add library/astrometry.net-0.38/lib to LD_LIBRARY_PATH
export LD_LIBRARY_PATH=project_path/library/astrometry.net-0.38/lib:${LD_LIBRARY_PATH}

2,install libpq-dev library to use postgres database
/usr/include/postgresql/libpq-fe.h
for Ubuntu systems: sudo apt-get install libpq-dev
for RHEL systems: yum install postgresql-devel
for Mac: brew install postgresql
