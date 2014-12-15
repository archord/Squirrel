/* 
 * File:   StoreDataPostgres.h
 * Author: xy
 *
 * Created on 2014年12月13日, 下午10:18
 */

#ifndef STOREDATAPOSTGRES_H
#define	STOREDATAPOSTGRES_H

class StoreDataPostgres : public StarFile {
public:
  /*database connection*/
  char *configFile;
  char *host;
  char *port;
  char *dbname;
  char *user;
  char *password;
  char *options;
  char *tty;

  /*table information*/
  char *catfile_table;
  char *match_table;
  char *ot_table;
  char *ot_flux_table;

  StoreDataPostgres();
  StoreDataPostgres(const StoreDataPostgres& orig);
  virtual ~StoreDataPostgres();

  void readDbInfo(char *configFile);
  void store(StarFile *starFile);
  void freeDbInfo();
  void writeToDBBinary(struct SAMPLE *points, char *fileName, int fileType);
private:
  void addInt16(struct strBuffer* strBuf, unsigned short i);
  void addInt32(struct strBuffer* strBuf, int i);
  void addInt64(struct strBuffer* strBuf, long int li);
  void addFloat4(struct strBuffer* strBuf, float f);
  void addFloat8(struct strBuffer* strBuf, double d);
  void storeCatfileInfo(StarFile *starFile);
  void storeCatlog(StarFile *starFile);
  void storeOT(StarFile *starFile);
  void storeOTFlux(StarFile *starFile);
};

#endif	/* STOREDATAPOSTGRES_H */

