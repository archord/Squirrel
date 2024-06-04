make clean -f Makefile2
make -f Makefile2
./createtable -d 20160402
./createtable -c 20160402
./crossmatch -method plane -grid 4,4 -errorRadius 1.5 -searchRadius 50 -fitsHDU 3 -ref  data/ref.Fcat -sample data/M3_05_141025_1_074020_0873.Fcat -dbConfigFile config/database_config.txt -width 3016 -height 3016