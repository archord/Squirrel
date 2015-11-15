/* 
 * File:   StoreDataPostgres.cpp
 * Author: xy
 * 
 * Created on 2014年12月13日, 下午10:18
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <arpa/inet.h>
#include "cmhead.h"

#include "StoreDataPostgres.h"

StoreDataPostgres::StoreDataPostgres() {

  int tlen = 255;
  configFile = (char*) malloc(LINE * sizeof (char));
  sprintf(configFile, "%s", "");

  host = (char*) malloc(tlen * sizeof (char));
  sprintf(host, "%s", "");

  port = (char*) malloc(tlen * sizeof (char));
  sprintf(port, "%s", "");

  dbname = (char*) malloc(tlen * sizeof (char));
  sprintf(dbname, "%s", "");

  user = (char*) malloc(tlen * sizeof (char));
  sprintf(user, "%s", "");

  password = (char*) malloc(tlen * sizeof (char));
  sprintf(password, "%s", "");

  options = (char*) malloc(tlen * sizeof (char));
  sprintf(options, "%s", "");

  tty = (char*) malloc(tlen * sizeof (char));
  sprintf(tty, "%s", "");

  catfile_table = (char*) malloc(tlen * sizeof (char));
  sprintf(catfile_table, "%s", "");

  match_table = (char*) malloc(tlen * sizeof (char));
  sprintf(match_table, "%s", "");

  ot_table = (char*) malloc(tlen * sizeof (char));
  sprintf(ot_table, "%s", "");

  ot_record_table = (char*) malloc(tlen * sizeof (char));
  sprintf(ot_record_table, "%s", "");

  ot_flux_table = (char*) malloc(tlen * sizeof (char));
  sprintf(ot_flux_table, "%s", "");

  mag_diff_table = (char*) malloc(tlen * sizeof (char));
  sprintf(mag_diff_table, "%s", "");

  catid = 0;
}

StoreDataPostgres::StoreDataPostgres(const StoreDataPostgres& orig) {
}

StoreDataPostgres::~StoreDataPostgres() {
  freeDbInfo();
  //  PQfinish(conn);
}

void StoreDataPostgres::store(StarFile *starFile) {

}

void StoreDataPostgres::store(StarFileFits *starFile, int fileType) {

  conn = PQsetdbLogin(host, port, options, tty, dbname, user, password);
  if (PQstatus(conn) == CONNECTION_BAD) {
    fprintf(stderr, "connect db failed! %s\n", PQerrorMessage(conn));
    PQfinish(conn);
    return;
  }

  storeCatfileInfo(starFile, fileType);
  storeCatlog(starFile, fileType);
  if (!fileType) {
    storeMagDiff(starFile);
    matchOT(starFile);
    storeOT(starFile);
    updateOT(starFile);
    storeOTRecord(starFile);
    if (starFile->fluxRatioSDTimes > 0) {
      storeOTFlux(starFile);
    }
  }

  PQfinish(conn);
}

void StoreDataPostgres::readDbInfo(char *configFile) {

  FILE *fp = NULL;
  char *line = NULL;
  if (configFile == NULL || strlen(configFile) == 0) {
    printf("database config file name is empty!\n");
    return;
  }

  if ((fp = fopen(configFile, "r")) == NULL) {
    printf("open file %s error!\n", configFile);
    return;
  }

  char *delim = "=";
  char *tmpStr = NULL;
  char *buf = (char*) malloc(LINE * sizeof (char));
  while (line = fgets(buf, LINE, fp)) {

    trim(line);
    if ((*line == '\n') || (strlen(line) == 0)) continue;

    tmpStr = strtok(line, delim);
    if ((tmpStr == NULL) || (strlen(tmpStr) == 0)) continue;
    if (strcmp(tmpStr, "host") == 0) {
      tmpStr = strtok(NULL, delim);
      if (tmpStr != NULL)
        strcpy(host, tmpStr);
      else
        strcpy(host, "");
    } else if (strcmp(tmpStr, "port") == 0) {
      tmpStr = strtok(NULL, delim);
      if (tmpStr != NULL)
        strcpy(port, tmpStr);
      else
        strcpy(port, "");
    } else if (strcmp(tmpStr, "dbname") == 0) {
      tmpStr = strtok(NULL, delim);
      if (tmpStr != NULL)
        strcpy(dbname, tmpStr);
      else
        strcpy(dbname, "");
    } else if (strcmp(tmpStr, "user") == 0) {
      tmpStr = strtok(NULL, delim);
      if (tmpStr != NULL)
        strcpy(user, tmpStr);
      else
        strcpy(user, "");
    } else if (strcmp(tmpStr, "password") == 0) {
      tmpStr = strtok(NULL, delim);
      if (tmpStr != NULL)
        strcpy(password, tmpStr);
      else
        strcpy(password, "");
    } else if (strcmp(tmpStr, "options") == 0) {
      tmpStr = strtok(NULL, delim);
      if (tmpStr != NULL)
        strcpy(options, tmpStr);
      else
        strcpy(options, "");
    } else if (strcmp(tmpStr, "tty") == 0) {
      tmpStr = strtok(NULL, delim);
      if (tmpStr != NULL)
        strcpy(tty, tmpStr);
      else
        strcpy(tty, "");
    } else if (strcmp(tmpStr, "catfile_table") == 0) {
      tmpStr = strtok(NULL, delim);
      if (tmpStr != NULL)
        strcpy(catfile_table, tmpStr);
      else
        strcpy(catfile_table, "");
    } else if (strcmp(tmpStr, "match_table") == 0) {
      tmpStr = strtok(NULL, delim);
      if (tmpStr != NULL)
        strcpy(match_table, tmpStr);
      else
        strcpy(match_table, "");
    } else if (strcmp(tmpStr, "ot_table") == 0) {
      tmpStr = strtok(NULL, delim);
      if (tmpStr != NULL)
        strcpy(ot_table, tmpStr);
      else
        strcpy(ot_table, "");
    } else if (strcmp(tmpStr, "ot_record_table") == 0) {
      tmpStr = strtok(NULL, delim);
      if (tmpStr != NULL)
        strcpy(ot_record_table, tmpStr);
      else
        strcpy(ot_record_table, "");
    } else if (strcmp(tmpStr, "ot_flux_table") == 0) {
      tmpStr = strtok(NULL, delim);
      if (tmpStr != NULL)
        strcpy(ot_flux_table, tmpStr);
      else
        strcpy(ot_flux_table, "");
    } else if (strcmp(tmpStr, "mag_diff_table") == 0) {
      tmpStr = strtok(NULL, delim);
      if (tmpStr != NULL)
        strcpy(mag_diff_table, tmpStr);
      else
        strcpy(mag_diff_table, "");
    }
  }
  free(buf);
  fclose(fp);
}

void StoreDataPostgres::freeDbInfo() {

  free(configFile);
  free(host);
  free(port);
  free(dbname);
  free(user);
  free(password);
  free(options);
  free(tty);

  free(catfile_table);
  free(match_table);
  free(ot_table);
  free(ot_record_table);
  free(ot_flux_table);
  free(mag_diff_table);
}

void StoreDataPostgres::storeMagDiff(StarFileFits *starFile) {

  PGresult *pgrst = NULL;
  if (PQstatus(conn) == CONNECTION_BAD) {
    fprintf(stderr, "connect db failed! %s\n", PQerrorMessage(conn));
    PQfinish(conn);
    return;
  }

  char *sqlBuf = (char*) malloc(MaxStringLength * sizeof (char));

  for (int i = 0; i < starFile->gridY; i++) {
    for (int j = 0; j < starFile->gridX; j++) {
      int idx = i * starFile->gridX + j;
      sprintf(sqlBuf, "insert into %s(catid,gridid,mdvalue)values(%d,%d,%lf)", mag_diff_table, catid, idx, starFile->fluxPtn[idx].magDiff);
      pgrst = PQexec(conn, sqlBuf);
    }
  }

  free(sqlBuf);
  PQclear(pgrst);
}

void StoreDataPostgres::storeCatfileInfo(StarFileFits *starFile, int fileType) {

  PGresult *pgrst = NULL;
  if (PQstatus(conn) == CONNECTION_BAD) {
    fprintf(stderr, "connect db failed! %s\n", PQerrorMessage(conn));
    PQfinish(conn);
    return;
  }

  char *sqlBuf = (char*) malloc(MaxStringLength * sizeof (char));
  sprintf(sqlBuf, "select catid from %s where catfile='%s'", catfile_table, starFile->fileName);
  pgrst = PQexec(conn, sqlBuf);

  if (PQresultStatus(pgrst) != PGRES_TUPLES_OK) {
    PQclear(pgrst);
    printf("query %s failure!\n", catfile_table);
    printf("sql = %s\n", sqlBuf);
    free(sqlBuf);
    return;
  }

  //if fileName not in table catfile_id, add, and get catid again
  if (PQntuples(pgrst) == 0) { //PQgetisnull
    PQclear(pgrst);
    char *fileTypeStr = "true";
    if (!fileType)
      fileTypeStr = "false";
    sprintf(sqlBuf, "insert into %s(catfile,airmass,magdiff,jd,is_ref,gridx, gridy)values('%s',%lf,%f,%lf,%s,%d,%d)",
            catfile_table, starFile->fileName, starFile->airmass, starFile->magDiff, starFile->jd, fileTypeStr, starFile->gridX, starFile->gridY);
    pgrst = PQexec(conn, sqlBuf);
    if (PQresultStatus(pgrst) != PGRES_COMMAND_OK) {
      PQclear(pgrst);
      printf("insert %s failure!\n", catfile_table);
      printf("sql = %s\n", sqlBuf);
      free(sqlBuf);
      return;
    } else {
      //printf("insert catfile_id success!\n");
    }

    sprintf(sqlBuf, "select catid from %s where catfile='%s'", catfile_table, starFile->fileName);
    pgrst = PQexec(conn, sqlBuf);
    if (PQresultStatus(pgrst) != PGRES_TUPLES_OK) {
      PQclear(pgrst);
      printf("query %s failure!\n", catfile_table);
      printf("sql = %s\n", sqlBuf);
      free(sqlBuf);
      return;
    }
  } else if (!fileType) {
    printf("%s already in table %s! please check!\n", starFile->fileName, catfile_table);
  }

  catid = atoi(PQgetvalue(pgrst, 0, 0));
  free(sqlBuf);
  PQclear(pgrst);
}

/**
 * 存储模板星表和目标星表中匹配成功的已知星
 * @param fileType
 */
void StoreDataPostgres::storeCatlog(StarFileFits *starFile, int fileType) {

  PGresult *pgrst = NULL;
  if (PQstatus(conn) == CONNECTION_BAD) {
    fprintf(stderr, "connect db failed! %s\n", PQerrorMessage(conn));
    PQfinish(conn);
    return;
  }

  char *sqlBuf = (char*) malloc(MaxStringLength * sizeof (char));
  sprintf(sqlBuf, "select catid from %s where catfile='%s'", catfile_table, starFile->fileName);
  pgrst = PQexec(conn, sqlBuf);

  if (PQntuples(pgrst) > 0) {
    return;
  }
  memset(sqlBuf, 0, MaxStringLength * sizeof (char));
  //total 22 column, not include cid 
  sprintf(sqlBuf, "COPY %s(\
            starid,crossid,catid,magnorm,ra,dec,background,classstar,ellipticity,\
            flags,mag,mage,fwhm,pixx,pixy,thetaimage,vignet,magcalib,magcalibe,\
            pixx1,pixy1,fluxRatio)\
            FROM STDIN WITH BINARY", match_table);

  pgrst = PQexec(conn, sqlBuf);
  if (PQresultStatus(pgrst) == PGRES_COPY_IN) {

    struct strBuffer *strBuf = (struct strBuffer*) malloc(sizeof (struct strBuffer));
    strBuf->data = (char*) malloc(LINE * sizeof (char));
    strBuf->len = MAX_BUFFER;
    initBinaryCopyBuf(strBuf);
    int i = 0;
    CMStar *tStar = starFile->starList;
    while (tStar) {
      //存储模板星表，或者目标星表中匹配成功的已知星
      if (fileType || ((tStar->match != NULL) && (tStar->error < starFile->areaBox))) {
        starToBinaryBuf(tStar, fileType, strBuf);
        int copydatares = PQputCopyData(conn, strBuf->data, strBuf->cursor);
        if (copydatares == -1) {
          printf("copy error: %s\n", PQerrorMessage(conn));
        }
        strBuf->cursor = 0;
        i++;
        //if(i++ >5) break;
      }
      tStar = tStar->next;
    }
    free(strBuf);
    free(strBuf->data);
    PQputCopyEnd(conn, NULL);
  } else {
    printf("copy error: %s\n", PQerrorMessage(conn));
  }
  PQclear(pgrst);
  free(sqlBuf);
}

void StoreDataPostgres::matchOT(StarFileFits *starFile) {

  PGresult *pgrst = NULL;
  if (PQstatus(conn) == CONNECTION_BAD) {
    fprintf(stderr, "connect db failed! %s\n", PQerrorMessage(conn));
    PQfinish(conn);
    return;
  }

  char *sqlBuf = (char*) malloc(MaxStringLength * sizeof (char));

  CMStar *tStar = starFile->starList;
  while (tStar) {
    if ((tStar->error >= starFile->areaBox) && (tStar->inarea == 1)) {
      sprintf(sqlBuf, "SELECT cid, sqrt(power(pixx-%f,2)+power(pixy-%f,2)) dst \
              from %s where sqrt(power(pixx-%f,2)+power(pixy-%f,2))<%f order by dst",
              tStar->pixx, tStar->pixy, ot_table, tStar->pixx, tStar->pixy, starFile->areaBox);
      pgrst = PQexec(conn, sqlBuf);

      if (PQresultStatus(pgrst) != PGRES_TUPLES_OK) {
        PQclear(pgrst);
        printf("query %s failure!\n", catfile_table);
        printf("sql = %s\n", sqlBuf);
        free(sqlBuf);
        return;
      }

      int matchNum = PQntuples(pgrst);
      if (matchNum > 0) {
        tStar->crossid = atoi(PQgetvalue(pgrst, 0, 0));
      }
    }
    tStar = tStar->next;
  }

  free(sqlBuf);
  PQclear(pgrst);
}

void StoreDataPostgres::storeOT(StarFileFits *starFile) {

  PGresult *pgrst = NULL;
  if (PQstatus(conn) == CONNECTION_BAD) {
    fprintf(stderr, "connect db failed! %s\n", PQerrorMessage(conn));
    PQfinish(conn);
    return;
  }

  char *sqlBuf = (char*) malloc(MaxStringLength * sizeof (char));
  //total 23 column, not include cid 
  sprintf(sqlBuf, "COPY %s(\
            starid,otid,catid,magnorm,ra,dec,background,classstar,ellipticity,\
            flags,mag,mage,fwhm,pixx,pixy,thetaimage,vignet,inarea,\
            magcalib,magcalibe,pixx1,pixy1,fluxRatio \
            )FROM STDIN WITH BINARY", ot_table);

  pgrst = PQexec(conn, sqlBuf);
  if (PQresultStatus(pgrst) == PGRES_COPY_IN) {

    struct strBuffer *strBuf = (struct strBuffer*) malloc(sizeof (struct strBuffer));
    strBuf->data = (char*) malloc(LINE * sizeof (char));
    strBuf->len = MAX_BUFFER;
    initBinaryCopyBuf(strBuf);
    int i = 0;
    CMStar *tStar = starFile->starList;
    while (tStar) {
      if ((tStar->error >= starFile->areaBox) && (tStar->inarea == 1) && (tStar->crossid == 0)) {
        starToBinaryBufOt(tStar, strBuf);
        int copydatares = PQputCopyData(conn, strBuf->data, strBuf->cursor);
        if (copydatares == -1) {
          printf("copy error: %s\n", PQerrorMessage(conn));
        }
        strBuf->cursor = 0;
        i++;
      }
      tStar = tStar->next;
    }
    free(strBuf);
    free(strBuf->data);
    PQputCopyEnd(conn, NULL);
  } else {
    printf("copy error: %s\n", PQerrorMessage(conn));
  }
  PQclear(pgrst);
  free(sqlBuf);
}

void StoreDataPostgres::updateOT(StarFileFits *starFile) {

  PGresult *pgrst = NULL;
  if (PQstatus(conn) == CONNECTION_BAD) {
    fprintf(stderr, "connect db failed! %s\n", PQerrorMessage(conn));
    PQfinish(conn);
    return;
  }

  char *sqlBuf = (char*) malloc(MaxStringLength * sizeof (char));
  CMStar *tStar = starFile->starList;
  while (tStar) {
    if ((tStar->error >= starFile->areaBox) && (tStar->inarea == 1) && (tStar->crossid > 0)) {
      sprintf(sqlBuf, "update %s set pixx=%f, pixy=%f where cid=%d",
              ot_table, tStar->pixx, tStar->pixy, tStar->crossid);
      PQexec(conn, sqlBuf);
    }
    tStar = tStar->next;
  }

  PQclear(pgrst);
  free(sqlBuf);
}

void StoreDataPostgres::storeOTRecord(StarFileFits *starFile) {

  PGresult *pgrst = NULL;
  if (PQstatus(conn) == CONNECTION_BAD) {
    fprintf(stderr, "connect db failed! %s\n", PQerrorMessage(conn));
    PQfinish(conn);
    return;
  }

  char *sqlBuf = (char*) malloc(MaxStringLength * sizeof (char));
  //total 23 column, not include cid 
  sprintf(sqlBuf, "COPY %s(\
            starid,otid,catid,magnorm,ra,dec,background,classstar,ellipticity,\
            flags,mag,mage,fwhm,pixx,pixy,thetaimage,vignet,inarea,\
            magcalib,magcalibe,pixx1,pixy1,fluxRatio \
            )FROM STDIN WITH BINARY", ot_record_table);

  pgrst = PQexec(conn, sqlBuf);
  if (PQresultStatus(pgrst) == PGRES_COPY_IN) {

    struct strBuffer *strBuf = (struct strBuffer*) malloc(sizeof (struct strBuffer));
    strBuf->data = (char*) malloc(LINE * sizeof (char));
    strBuf->len = MAX_BUFFER;
    initBinaryCopyBuf(strBuf);
    int i = 0;
    CMStar *tStar = starFile->starList;
    while (tStar) {
      if ((tStar->error >= starFile->areaBox) && (tStar->inarea == 1) && (tStar->crossid > 0)) {
        starToBinaryBufOt(tStar, strBuf);
        int copydatares = PQputCopyData(conn, strBuf->data, strBuf->cursor);
        if (copydatares == -1) {
          printf("copy error: %s\n", PQerrorMessage(conn));
        }
        strBuf->cursor = 0;
        i++;
      }
      tStar = tStar->next;
    }
    free(strBuf);
    free(strBuf->data);
    PQputCopyEnd(conn, NULL);
  } else {
    printf("copy error: %s\n", PQerrorMessage(conn));
  }
  PQclear(pgrst);
  free(sqlBuf);
}

void StoreDataPostgres::storeOTFlux(StarFileFits *starFile) {

  PGresult *pgrst = NULL;
  if (PQstatus(conn) == CONNECTION_BAD) {
    fprintf(stderr, "connect db failed! %s\n", PQerrorMessage(conn));
    PQfinish(conn);
    return;
  }

  char *sqlBuf = (char*) malloc(MaxStringLength * sizeof (char));
  //total 23 column, not include cid 
  sprintf(sqlBuf, "COPY %s(\
            starid,crossid,catid,magnorm,ra,dec,background,classstar,ellipticity,\
            flags,mag,mage,fwhm,pixx,pixy,thetaimage,vignet,inarea,\
            magcalib,magcalibe,pixx1,pixy1,fluxRatio \
            )FROM STDIN WITH BINARY", ot_flux_table);

  pgrst = PQexec(conn, sqlBuf);
  if (PQresultStatus(pgrst) == PGRES_COPY_IN) {
    struct strBuffer *strBuf = (struct strBuffer*) malloc(sizeof (struct strBuffer));
    strBuf->data = (char*) malloc(LINE * sizeof (char));
    strBuf->len = MAX_BUFFER;
    initBinaryCopyBuf(strBuf);
    int i = 0;
    CMStar *tStar = starFile->starList;
    while (tStar) {
      if (tStar->fluxVarTag == 1) {
        starToBinaryBufOt(tStar, strBuf);
        int copydatares = PQputCopyData(conn, strBuf->data, strBuf->cursor);
        if (copydatares == -1) {
          printf("copy error: %s\n", PQerrorMessage(conn));
        }
        strBuf->cursor = 0;
        i++;
      }
      tStar = tStar->next;
    }
    free(strBuf);
    free(strBuf->data);
    PQputCopyEnd(conn, NULL);
  } else {
    printf("copy error: %s\n", PQerrorMessage(conn));
  }
  PQclear(pgrst);
  free(sqlBuf);
}

void StoreDataPostgres::initBinaryCopyBuf(struct strBuffer *strBuf) {

  char *sendHeader = "PGCOPY\n\377\r\n\0";
  unsigned int zero = '\0';

  strBuf->cursor = 0;
  memcpy(strBuf->data, sendHeader, 11);
  strBuf->cursor += 11;
  memcpy(strBuf->data + strBuf->cursor, (char*) &zero, 4);
  strBuf->cursor += 4;
  memcpy(strBuf->data + strBuf->cursor, (char*) &zero, 4);
  strBuf->cursor += 4;
}

/**
 * 存储模板及已知星
 * @param tStar
 * @param fileType
 * @param strBuf
 */
void StoreDataPostgres::starToBinaryBuf(CMStar * tStar, int fileType, struct strBuffer *strBuf) {

  unsigned short fieldNum = 22;
  addInt16(strBuf, fieldNum); //column number, when add or delete colume, must change this number
  addInt64(strBuf, tStar->id);
  if (fileType) {
    addInt64(strBuf, tStar->crossid);
  } else {
    addInt64(strBuf, tStar->match->id);
  }
  addInt64(strBuf, catid);
  addFloat8(strBuf, tStar->magnorm);
  addFloat8(strBuf, tStar->alpha);
  addFloat8(strBuf, tStar->delta);
  addFloat8(strBuf, tStar->background);
  addFloat8(strBuf, tStar->classstar);
  addFloat8(strBuf, tStar->ellipticity);
  addFloat8(strBuf, tStar->flags);
  addFloat8(strBuf, tStar->mag);
  addFloat8(strBuf, tStar->mage);
  addFloat8(strBuf, tStar->fwhm);
  addFloat8(strBuf, tStar->pixx);
  addFloat8(strBuf, tStar->pixy);
  addFloat8(strBuf, tStar->thetaimage);
  addFloat8(strBuf, tStar->vignet);
  addFloat8(strBuf, tStar->magcalib);
  addFloat8(strBuf, tStar->magcalibe);
  addFloat8(strBuf, tStar->pixx1);
  addFloat8(strBuf, tStar->pixy1);
  addFloat8(strBuf, tStar->fluxRatio);
}

/**
 * 存储OT
 * @param tStar
 * @param strBuf
 */
void StoreDataPostgres::starToBinaryBufOt(CMStar * tStar, struct strBuffer *strBuf) {

  unsigned short fieldNum = 23;
  addInt16(strBuf, fieldNum); //column number, when add or delete colume, must change this number
  addInt64(strBuf, tStar->id);
  addInt64(strBuf, tStar->crossid);
  addInt64(strBuf, catid);
  addFloat8(strBuf, tStar->magnorm);
  addFloat8(strBuf, tStar->alpha);
  addFloat8(strBuf, tStar->delta);
  addFloat8(strBuf, tStar->background);
  addFloat8(strBuf, tStar->classstar);
  addFloat8(strBuf, tStar->ellipticity);
  addFloat8(strBuf, tStar->flags);
  addFloat8(strBuf, tStar->mag);
  addFloat8(strBuf, tStar->mage);
  addFloat8(strBuf, tStar->fwhm);
  addFloat8(strBuf, tStar->pixx);
  addFloat8(strBuf, tStar->pixy);
  addFloat8(strBuf, tStar->thetaimage);
  addFloat8(strBuf, tStar->vignet);
  addInt32(strBuf, tStar->inarea);
  addFloat8(strBuf, tStar->magcalib);
  addFloat8(strBuf, tStar->magcalibe);
  addFloat8(strBuf, tStar->pixx1);
  addFloat8(strBuf, tStar->pixy1);
  addFloat8(strBuf, tStar->fluxRatio);
}

void StoreDataPostgres::addInt16(struct strBuffer* strBuf, unsigned short i) {

  unsigned short swap = i;
  swap = htons(swap);

  memcpy(strBuf->data + strBuf->cursor, (char*) &swap, 2);

  strBuf->cursor += 2;
  if (strBuf->cursor > strBuf->len) {
    printf("string buffer has full, please check!\n");
    strBuf->cursor -= 2;
    return;
  }

  strBuf->data[strBuf->cursor] = '\0';
}

void StoreDataPostgres::addInt32(struct strBuffer* strBuf, int i) {

  unsigned int swap = i;
  unsigned int len = 4;
  swap = htonl(swap);
  len = htonl(len);

  memcpy(strBuf->data + strBuf->cursor, (char*) &len, 4);
  strBuf->cursor += 4;
  memcpy(strBuf->data + strBuf->cursor, (char*) &swap, 4);
  strBuf->cursor += 4;

  if (strBuf->cursor > strBuf->len) {
    printf("string buffer has full, please check!\n");
    strBuf->cursor -= 8;
  }
  strBuf->data[strBuf->cursor] = '\0';
}

void StoreDataPostgres::addInt64(struct strBuffer* strBuf, long int li) {

  unsigned int len = 8;

  union {
    long int li;
    unsigned int i[2];
  } swap;

  swap.li = li;

  len = htonl(len);
  swap.i[0] = htonl(swap.i[0]);
  swap.i[1] = htonl(swap.i[1]);

  memcpy(strBuf->data + strBuf->cursor, (char*) &len, 4);
  strBuf->cursor += 4;
  memcpy(strBuf->data + strBuf->cursor, (char*) &swap.i[1], 4);
  strBuf->cursor += 4;
  memcpy(strBuf->data + strBuf->cursor, (char*) &swap.i[0], 4);
  strBuf->cursor += 4;

  if (strBuf->cursor > strBuf->len) {
    printf("string buffer has full, please check!\n");
    strBuf->cursor -= 12;
  }
  strBuf->data[strBuf->cursor] = '\0';
}

void StoreDataPostgres::addFloat4(struct strBuffer* strBuf, float f) {

  unsigned int len = 4;

  union {
    float f;
    unsigned int i;
  } swap;
  swap.f = f;
  swap.i = htonl(swap.i);
  len = htonl(len);

  memcpy(strBuf->data + strBuf->cursor, (char*) &len, 4);
  strBuf->cursor += 4;
  memcpy(strBuf->data + strBuf->cursor, (char*) &swap.i, 4);
  strBuf->cursor += 4;

  if (strBuf->cursor > strBuf->len) {
    printf("string buffer has full, please check!\n");
    strBuf->cursor -= 8;
  }
  strBuf->data[strBuf->cursor] = '\0';
}

void StoreDataPostgres::addFloat8(struct strBuffer* strBuf, double d) {

  unsigned int len = 8;

  union {
    double d;
    unsigned int i[2];
  } swap;
  swap.d = d;
  swap.i[0] = htonl(swap.i[0]);
  swap.i[1] = htonl(swap.i[1]);

  len = htonl(len);

  memcpy(strBuf->data + strBuf->cursor, (char*) &len, 4);
  strBuf->cursor += 4;
  memcpy(strBuf->data + strBuf->cursor, (char*) &swap.i[1], 4);
  strBuf->cursor += 4;
  memcpy(strBuf->data + strBuf->cursor, (char*) &swap.i[0], 4);
  strBuf->cursor += 4;

  if (strBuf->cursor > strBuf->len) {
    printf("string buffer has full, please check!\n");
    strBuf->cursor -= 12;
  }
  strBuf->data[strBuf->cursor] = '\0';
}