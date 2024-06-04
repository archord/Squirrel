Squirrel
========

# install library: 

## cfitsio

在astrometry.net的安装依赖中会安装这个包：libcfitsio-dev
export LD_LIBRARY_PATH=/home/disk1/opt/svomproc/gwacdblc/gwacdblc/Squirrel/library/lib:${LD_LIBRARY_PATH}

## wcstools 

在astrometry.net的安装依赖中会安装这个包：wcslib-dev，不再默认路径中，需要手动安装

make -j 2

make install

include和lib在libwcs中
cp library_src/wcstools-3.9.7/libwcs/*.a library/wcstools-3.8.5/lib/



## astrometry.net

sudo apt install build-essential curl git file pkg-config swig \
       libcairo2-dev libnetpbm10-dev netpbm libpng-dev libjpeg-dev \
       zlib1g-dev libbz2-dev libcfitsio-dev wcslib-dev \
       python3 python3-pip python3-dev \
       python3-numpy python3-scipy python3-pil


# compile
Makefile is the Netbeans IDE's default auto compile.
Makefile1 is used for manual compile.

to compile run command:
make -f Makefile1

run example
./crossmatch -method sphere -grid 4,4 -errorRadius 0.06 -fitsHDU 3 -ref  data/P_130112_25d4458_33d9878-210.Fcat -sample data/P_130112_25d4458_33d9878-119.Fcat -dbConfigFile data/database_config.txt -fluxSDTimes 1
./crossmatch -method plane  -g 4,4 -errorRadius 1.5 -searchRadius 50 -width 3096 -height 3096 -fitsHDU 3 -ref  data/P_130112_25d4458_33d9878-210.Fcat -sample data/P_130112_25d4458_33d9878-119.Fcat -dbConfigFile data/database_config.txt -fluxSDTimes 1

note:
1,remember add library/astrometry.net-0.38/lib to LD_LIBRARY_PATH
export LD_LIBRARY_PATH=project_path/library/astrometry.net-0.38/lib:${LD_LIBRARY_PATH}

2,install libpq-dev library to use postgres database
for Ubuntu systems: sudo apt-get install libpq-dev
for RHEL systems: yum install postgresql-devel
for Mac: brew install postgresql

4, head file path
RHEL: /usr/include/postgresql/libpq-fe.h

3,library path
Ubuntu: /usr/lib/postgresql/9.3/
RHEL: /usr/include/postgresql


netbeans run command
for crassmatch
"${OUTPUT_PATH}"  -method plane -grid 4,4 -errorRadius 1.5 -searchRadius 50 -fitsHDU 3 -ref  data/ref.Fcat -sample data/M3_05_141025_1_074020_0873.Fcat -dbConfigFile config/database_config.txt -width 3016 -height 3016
for createtable
"${OUTPUT_PATH}" -c 20160402
