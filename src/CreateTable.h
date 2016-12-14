/* 
 * File:   CreateTable.h
 * Author: xy
 *
 * Created on November 14, 2015, 9:10 PM
 */

#ifndef CREATETABLE_H
#define	CREATETABLE_H

class CreateTable {
public:
  CreateTable();
  virtual ~CreateTable();
  
  void showHelp();
  void initDBInfo();
  void freeDBInfo();
  void getDatabaseInfo(char *configFile);
  void getTableInfo(char *fileName, char *tlbInfo);
  void generateCrossMatchDBConfigFile(char *fileName, char *dateTime);
  void createTable(char *tableFileName, char *tablePrefix, char *dateTime, int flag);
  void deleteTable(char *tablePrefix, char *dateTime);
  void createMain(int flag, char *commandConfigFile, char *outFile, char *tableDate);
  
private:

  char *configFile;
  char *host;
  char *port;
  char *dbname;
  char *user;
  char *password;
  char *options;
  char *tty;

  /*table prefix*/
  char *catfile_table;
  char *match_table;
  char *ot_table;
  char *ot_record_table;
  char *ot_flux_table;
  char *mag_diff_table;
  /*table structure file name*/
  char *catfile_table_filename;
  char *match_table_filename;
  char *ot_table_filename;
  char *ot_record_table_filename;
  char *ot_flux_table_filename;
  char *mag_diff_table_filename;

  char *out_config_filename;

public:
  //for table inherit
  int tableNumber;
  int fileNumber;
};

#endif	/* CREATETABLE_H */

