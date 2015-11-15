/* 
 * File:   StoreDataPostgres.h
 * Author: xy
 *
 * Created on 2014年12月13日, 下午10:18
 */

#ifndef STOREDATAPOSTGRES_H
#define	STOREDATAPOSTGRES_H

#include "libpq-fe.h" 
#include "StoreData.h"
#include "StarFileFits.h"

struct strBuffer {
  char *data;
  int len;
  int cursor;
};

class StoreDataPostgres : public StoreData {
public:

  PGconn *conn;
  long catid;

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
  char *ot_record_table;
  char *ot_flux_table;
  char *mag_diff_table;

  StoreDataPostgres();
  StoreDataPostgres(const StoreDataPostgres& orig);
  virtual ~StoreDataPostgres();

  void readDbInfo(char *configFile);
  void store(StarFile *starFile);
  void store(StarFileFits *starFile, int fileType);
  void freeDbInfo();
private:
  void storeOTRecord(StarFileFits *starFile);
  void updateOT(StarFileFits *starFile);
  void matchOT(StarFileFits *starFile);
  void storeMagDiff(StarFileFits *starFile);
  void storeCatfileInfo(StarFileFits *starFile, int fileType);
  void storeCatlog(StarFileFits *starFile, int fileType);
  void storeOT(StarFileFits *starFile);
  void storeOTFlux(StarFileFits *starFile);
  void addInt16(struct strBuffer* strBuf, unsigned short i);
  void addInt32(struct strBuffer* strBuf, int i);
  void addInt64(struct strBuffer* strBuf, long int li);
  void addFloat4(struct strBuffer* strBuf, float f);
  void addFloat8(struct strBuffer* strBuf, double d);
  void initBinaryCopyBuf(struct strBuffer *strBuf);
  void starToBinaryBuf(CMStar * tStar, int fileType, struct strBuffer *strBuf);
  void starToBinaryBufOt(CMStar * tStar, struct strBuffer *strBuf);
};

#endif	/* STOREDATAPOSTGRES_H */

