Squirrel
========

项目仅依赖cfitsio、wcstools、astrometry.net，手动在Makefile完成配置后即可编译项目。
library目录包含这三个库在Ubuntu 22.04上的编译文件， 在Ubuntu 22.04上可以一键编译并测试createtable和crossmatch程序

在Ubuntu 22.04上只需执行以下3步：
1. apt update & apt-get install libpq-dev libcfitsio-dev -y 
2. 修改config/create_table_database_config.txt中的数据库配置：host，port，dbname，user，password
3. sh run.sh

# install library: 

## 安装postgres的C语言开发包

apt-get install libpq-dev -y 
 
apt install postgresql -y 

psql -U postgres -d postgres -h localhost -W
-W` 选项会提示你输入密码。
ALTER USER postgres WITH PASSWORD '123456';

 select count(*) from ot_20160402;

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

安装好上面的依赖后，可以一键编译成。

但是上面的依赖同时安装了libcfitsio-dev wcslib-dev，在编译crossmatch时，如果使用手动安装的cfitsio和wcslib或许会有冲突，这里后续需要优化。

astrometry.net-0.95编译安装完成后，在编译crossmatch是发现头文件有问题，使用astrometry.net-0.38的头文件可以解决，这里后续也需要优化。

libbackend是astrometry.net-0.38/lib中的一个库

apt install astrometry.net #是否安装了开发包？

# compile
Makefile is the Netbeans IDE's default auto compile.
Makefile1 is used for manual compile.

to compile run command:
make -f Makefile1

# run example
./crossmatch -method sphere -grid 4,4 -errorRadius 0.06 -fitsHDU 3 -ref  data/P_130112_25d4458_33d9878-210.Fcat -sample data/P_130112_25d4458_33d9878-119.Fcat -dbConfigFile data/database_config.txt -fluxSDTimes 1
./crossmatch -method plane  -g 4,4 -errorRadius 1.5 -searchRadius 50 -width 3096 -height 3096 -fitsHDU 3 -ref  data/P_130112_25d4458_33d9878-210.Fcat -sample data/P_130112_25d4458_33d9878-119.Fcat -dbConfigFile data/database_config.txt -fluxSDTimes 1

netbeans run command
for crassmatch
"${OUTPUT_PATH}"  -method plane -grid 4,4 -errorRadius 1.5 -searchRadius 50 -fitsHDU 3 -ref  data/ref.Fcat -sample data/M3_05_141025_1_074020_0873.Fcat -dbConfigFile config/database_config.txt -width 3016 -height 3016
for createtable
"${OUTPUT_PATH}" -c 20160402


# note
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


# error

使用library_src/cfitsio-4.4.0.tar.gz编译的cfitsio，运行crossmatch会报如下错误，或许这个包有问题，这个包和astrometry.net-0.95.tar.gz都来源于github，或许不是稳定版。

ERROR: Mismatch in the CFITSIO_SONAME value in the fitsio.h include file
that was used to build the CFITSIO library, and the value in the include file
that was used when compiling the application program:
   Version used to build the CFITSIO library   = 10
   Version included by the application program = 871916264

Fix this by recompiling and then relinking this application program 
with the CFITSIO library