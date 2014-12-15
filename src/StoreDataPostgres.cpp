/* 
 * File:   StoreDataPostgres.cpp
 * Author: xy
 * 
 * Created on 2014年12月13日, 下午10:18
 */
#include <stdio.h>
#include "cmhead.h"
#include "StoreDataPostgres.h"

StoreDataPostgres::StoreDataPostgres() {

  int tlen = 64;
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

  ot_flux_table = (char*) malloc(tlen * sizeof (char));
  sprintf(ot_flux_table, "%s", "");
}

StoreDataPostgres::StoreDataPostgres(const StoreDataPostgres& orig) {
}

StoreDataPostgres::~StoreDataPostgres() {
}

void StoreDataPostgres::store(StarFile *starFile) {

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

  //initDbInfo();

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
    } else if (strcmp(tmpStr, "ot_flux_table") == 0) {
      tmpStr = strtok(NULL, delim);
      if (tmpStr != NULL)
        strcpy(ot_flux_table, tmpStr);
      else
        strcpy(ot_flux_table, "");
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
    free(ot_flux_table);
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

void StoreDataPostgres::writeToDBBinary(struct SAMPLE *points, char *fileName, int fileType) {

    if (points == NULL) return;

    long start, end;
    start = clock();

    PGconn *conn = NULL;
    PGresult *pgrst = NULL;


    conn = PQsetdbLogin(host, port, options, tty, dbname, user, password);
    if (PQstatus(conn) == CONNECTION_BAD) {
        fprintf(stderr, "connect db failed! %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        return;
    }

    float magDiff = 0.0;
    if (!fileType) {
        magDiff = getMagDiff(points);
        fluxNorm(points, magDiff);
    }

    //!fileType
    if (0) {
        char *cleanSql = (char*) malloc(256 * sizeof (char));
        sprintf(cleanSql, "delete from %s where catid=(select catid from %s where catfile='%s');", match_table, catfile_table, fileName); //delete match
        PQexec(conn, cleanSql);
        sprintf(cleanSql, "delete from %s where catid=(select catid from %s where catfile='%s');", ot_table, catfile_table, fileName); //delete ot
        PQexec(conn, cleanSql);
        sprintf(cleanSql, "delete from %s where catid=(select catid from %s where catfile='%s');", ot_flux_table, catfile_table, fileName); //delete ot
        PQexec(conn, cleanSql);
        sprintf(cleanSql, "delete from %s where catfile='%s';", catfile_table, fileName); //delete catfile
        PQexec(conn, cleanSql);
        free(cleanSql);
    }
    //printf("delete data from database!\n");

    char *sqlBuf = (char*) malloc(256 * sizeof (char));
    //get fileName's catfile_id catid
    sprintf(sqlBuf, "select catid from %s where catfile='%s'", catfile_table, fileName);
    pgrst = PQexec(conn, sqlBuf);

    if (PQresultStatus(pgrst) != PGRES_TUPLES_OK) {
        PQclear(pgrst);
        printf("query %s failure!\n", catfile_table);
        printf("sql = %s\n", sqlBuf);
        return;
    }

    long catid = 0;
    //if fileName not in table catfile_id, add, and get catid again
    if (PQntuples(pgrst) == 0) { //PQgetisnull
        PQclear(pgrst);
        char *fileTypeStr = "true";
        if (!fileType)
            fileTypeStr = "false";
        sprintf(sqlBuf, "insert into %s(catfile,airmass,magdiff,jd,is_ref)values('%s',%lf,%f,%lf,%s)", catfile_table, fileName, airmass, magDiff, jd, fileTypeStr);
        pgrst = PQexec(conn, sqlBuf);
        if (PQresultStatus(pgrst) != PGRES_COMMAND_OK) {
            PQclear(pgrst);
            printf("insert %s failure!\n", catfile_table);
            printf("sql = %s\n", sqlBuf);
            return;
        } else {
            //printf("insert catfile_id success!\n");
        }

        sprintf(sqlBuf, "select catid from %s where catfile='%s'", catfile_table, fileName);
        pgrst = PQexec(conn, sqlBuf);
        if (PQresultStatus(pgrst) != PGRES_TUPLES_OK) {
            PQclear(pgrst);
            printf("query %s failure!\n", catfile_table);
            printf("sql = %s\n", sqlBuf);
            return;
        }
    } else {
        PQfinish(conn);
        //printf("%s already in table %s! please check!\n", fileName, catfile_table);
        free(sqlBuf);
        return;
    }
    catid = atoi(PQgetvalue(pgrst, 0, 0));
    //printf("catid = %d\n", catid);
    PQclear(pgrst);

    char *buf = (char*) malloc(1024 * sizeof (char));


    unsigned short fieldNum = 21;
    char *sendHeader = "PGCOPY\n\377\r\n\0";
    unsigned int zero = '\0';
    struct strBuffer strBuf;
    strBuf.data = buf;
    strBuf.len = MAX_BUFFER;

    int i = 0;
    int j = 0;
    int k = 0;
    struct SAMPLE *tSample;
    if (fileType) { //insert reference file
        //total 19 column, not include cid 
        //char *sql1 = "COPY crossmatch_id(\
            starid,crossid,catid,magnorm,ra,dec,background,classstar,ellipticity,flags,mag,mage,fwhm,pixx,pixy,thetaimage,vignet,\
            magcalib,magcalibe \
            )FROM STDIN WITH BINARY";
        sprintf(sqlBuf, "COPY %s(\
            starid,crossid,catid,magnorm,ra,dec,background,classstar,ellipticity,flags,mag,mage,fwhm,pixx,pixy,thetaimage,vignet,\
            magcalib,magcalibe,pixx1,pixy1 \
            )FROM STDIN WITH BINARY", match_table);
        pgrst = PQexec(conn, sqlBuf);
        if (PQresultStatus(pgrst) == PGRES_COPY_IN) {
            strBuf.cursor = 0;
            memcpy(strBuf.data, sendHeader, 11);
            strBuf.cursor += 11;
            memcpy(strBuf.data + strBuf.cursor, (char*) &zero, 4);
            strBuf.cursor += 4;
            memcpy(strBuf.data + strBuf.cursor, (char*) &zero, 4);
            strBuf.cursor += 4;
            
            tSample = points->next;
            while (tSample) {
                addInt16(&strBuf, fieldNum); //column number, when add or delete colume, must change this number
                addInt64(&strBuf, tSample->id);
                addInt64(&strBuf, tSample->crossid);
                addInt64(&strBuf, catid);
                addFloat8(&strBuf, -1.0);
                addFloat8(&strBuf, tSample->alpha);
                addFloat8(&strBuf, tSample->delta);
                addFloat8(&strBuf, tSample->background);
                addFloat8(&strBuf, tSample->classstar);
                addFloat8(&strBuf, tSample->ellipticity);
                addFloat8(&strBuf, tSample->flags);
                addFloat8(&strBuf, tSample->mag);
                addFloat8(&strBuf, tSample->mage);
                addFloat8(&strBuf, tSample->fwhm);
                addFloat8(&strBuf, tSample->pixx);
                addFloat8(&strBuf, tSample->pixy);
                addFloat8(&strBuf, tSample->thetaimage);
                addFloat8(&strBuf, tSample->vignet);
                addFloat8(&strBuf, tSample->magcalib);
                addFloat8(&strBuf, tSample->magcalibe);
                addFloat8(&strBuf, tSample->pixx);
                addFloat8(&strBuf, tSample->pixy);
                int copydatares = PQputCopyData(conn, strBuf.data, strBuf.cursor);
                //if(i >5) break;
                i++;
                tSample = tSample->next;
                strBuf.cursor = 0;
            }

            PQputCopyEnd(conn, NULL);
        } else {
            printf("can not copy in!\n");
        }
        PQclear(pgrst);
    }else{// insert sample file  !fileType
        //insert sample file matched
        fieldNum = 22;
        strBuf.cursor = 0;
        memcpy(strBuf.data, sendHeader, 11);
        strBuf.cursor += 11;
        memcpy(strBuf.data + strBuf.cursor, (char*) &zero, 4);
        strBuf.cursor += 4;
        memcpy(strBuf.data + strBuf.cursor, (char*) &zero, 4);
        strBuf.cursor += 4;

        sprintf(sqlBuf, "COPY %s(\
                starid,crossid,catid,magnorm,ra,dec,background,classstar,ellipticity,flags,mag,mage,fwhm,pixx,pixy,thetaimage,vignet,\
                magcalib,magcalibe,pixx1,pixy1,fluxRatio \
                )FROM STDIN WITH BINARY", match_table);

        pgrst = PQexec(conn, sqlBuf);
        if (PQresultStatus(pgrst) == PGRES_COPY_IN) {
            tSample = points->next;
            while (tSample) {
                if ((tSample->reference != NULL) && (tSample->error < areaBox)) {
                    addInt16(&strBuf, fieldNum); //column number, when add or delete colume, must change this number
                    addInt64(&strBuf, tSample->id);
                    addInt64(&strBuf, tSample->crossid);
                    addInt64(&strBuf, catid);
                    addFloat8(&strBuf, tSample->magnorm);
                    addFloat8(&strBuf, tSample->alpha);
                    addFloat8(&strBuf, tSample->delta);
                    addFloat8(&strBuf, tSample->background);
                    addFloat8(&strBuf, tSample->classstar);
                    addFloat8(&strBuf, tSample->ellipticity);
                    addFloat8(&strBuf, tSample->flags);
                    addFloat8(&strBuf, tSample->mag);
                    addFloat8(&strBuf, tSample->mage);
                    addFloat8(&strBuf, tSample->fwhm);
                    addFloat8(&strBuf, tSample->pixx);
                    addFloat8(&strBuf, tSample->pixy);
                    addFloat8(&strBuf, tSample->thetaimage);
                    addFloat8(&strBuf, tSample->vignet);
                    addFloat8(&strBuf, tSample->magcalib);
                    addFloat8(&strBuf, tSample->magcalibe);
                    addFloat8(&strBuf, tSample->pixx1);
                    addFloat8(&strBuf, tSample->pixy1);
                    addFloat8(&strBuf, tSample->fluxRatio);
                
                    int copydatares = PQputCopyData(conn, strBuf.data, strBuf.cursor);
                    i++;
                    strBuf.cursor = 0;
                }
                tSample = tSample->next;
            }
            PQputCopyEnd(conn, NULL);
        }
        PQclear(pgrst);
        
        if(fluxRatioSDTimes>0)
        {
        //insert sample file matched and filter by flux
        fieldNum = 22;
        strBuf.cursor = 0;
        memcpy(strBuf.data, sendHeader, 11);
        strBuf.cursor += 11;
        memcpy(strBuf.data + strBuf.cursor, (char*) &zero, 4);
        strBuf.cursor += 4;
        memcpy(strBuf.data + strBuf.cursor, (char*) &zero, 4);
        strBuf.cursor += 4;

        sprintf(sqlBuf, "COPY %s(\
                starid,crossid,catid,magnorm,ra,dec,background,classstar,ellipticity,flags,mag,mage,fwhm,pixx,pixy,thetaimage,vignet,\
                magcalib,magcalibe,pixx1,pixy1,fluxRatio \
                )FROM STDIN WITH BINARY", ot_flux_table);

        pgrst = PQexec(conn, sqlBuf);
        if (PQresultStatus(pgrst) == PGRES_COPY_IN) {
            tSample = points->next;
            //printf("fluxRatioSDTimes=%d\n",fluxRatioSDTimes);
            //printf("standardDeviation=%f\n",standardDeviation);
            //printf("fluxRatioAverage=%f\n",fluxRatioAverage);
            //printf("standardDeviation*fluxRatioSDTimes=%f\n",standardDeviation*fluxRatioSDTimes);
            double timesOfSD = fluxRatioSDTimes*standardDeviation;
            while (tSample) {
                if ((tSample->reference != NULL) && (tSample->error < areaBox)) { // && (tSample->mage < 0.05)
                    double ratioAbs = fabs(tSample->fluxRatio - fluxRatioMedian);
                    //double ratioAbs = 0;
                    //if(tIndex<100)
                    //printf("magDiffs[%d]=%f\tratioAbs=%f\n", tIndex, magDiffs[tIndex],ratioAbs);
                    if(ratioAbs > timesOfSD)
                    {
                        addInt16(&strBuf, fieldNum); //column number, when add or delete colume, must change this number
                        addInt64(&strBuf, tSample->id);
                        addInt64(&strBuf, tSample->crossid);
                        addInt64(&strBuf, catid);
                        addFloat8(&strBuf, tSample->magnorm);
                        addFloat8(&strBuf, tSample->alpha);
                        addFloat8(&strBuf, tSample->delta);
                        addFloat8(&strBuf, tSample->background);
                        addFloat8(&strBuf, tSample->classstar);
                        addFloat8(&strBuf, tSample->ellipticity);
                        addFloat8(&strBuf, tSample->flags);
                        addFloat8(&strBuf, tSample->mag);
                        addFloat8(&strBuf, tSample->mage);
                        addFloat8(&strBuf, tSample->fwhm);
                        addFloat8(&strBuf, tSample->pixx);
                        addFloat8(&strBuf, tSample->pixy);
                        addFloat8(&strBuf, tSample->thetaimage);
                        addFloat8(&strBuf, tSample->vignet);
                        addFloat8(&strBuf, tSample->magcalib);
                        addFloat8(&strBuf, tSample->magcalibe);
                        addFloat8(&strBuf, tSample->pixx1);
                        addFloat8(&strBuf, tSample->pixy1);
                        addFloat8(&strBuf, tSample->fluxRatio);

                        int copydatares = PQputCopyData(conn, strBuf.data, strBuf.cursor);
                        k++;
                        strBuf.cursor = 0;
                    }
                }
                tSample = tSample->next;
            }
            PQputCopyEnd(conn, NULL);
        }
        PQclear(pgrst);
        }

        //insert sample file unmatched
        fieldNum = 22;
        strBuf.cursor = 0;
        memcpy(strBuf.data, sendHeader, 11);
        strBuf.cursor += 11;
        memcpy(strBuf.data + strBuf.cursor, (char*) &zero, 4);
        strBuf.cursor += 4;
        memcpy(strBuf.data + strBuf.cursor, (char*) &zero, 4);
        strBuf.cursor += 4;
        //char *sql2 = "COPY OT_id(\
                starid,otid,catid,magnorm,ra,dec,background,classstar,ellipticity,flags,mag,mage,fwhm,pixx,pixy,thetaimage,vignet,inarea,\
                magcalib,magcalibe \
                )FROM STDIN WITH BINARY";
        sprintf(sqlBuf, "COPY %s(\
                starid,otid,catid,magnorm,ra,dec,background,classstar,ellipticity,flags,mag,mage,fwhm,pixx,pixy,thetaimage,vignet,inarea,\
                magcalib,magcalibe,pixx1,pixy1 \
                )FROM STDIN WITH BINARY", ot_table);
        //printf("%s\n",sqlBuf);
        pgrst = PQexec(conn, sqlBuf);
        if (PQresultStatus(pgrst) == PGRES_COPY_IN) {
            tSample = points->next;
            while (tSample) {
                if ((tSample->error >= areaBox) && (tSample->inarea == 1)) {

                    addInt16(&strBuf, fieldNum); //column number, when add or delete colume, must change this number
                    addInt64(&strBuf, tSample->id);
                    addInt64(&strBuf, tSample->crossid);
                    addInt64(&strBuf, catid);
                    addFloat8(&strBuf, tSample->magnorm);
                    addFloat8(&strBuf, tSample->alpha);
                    addFloat8(&strBuf, tSample->delta);
                    addFloat8(&strBuf, tSample->background);
                    addFloat8(&strBuf, tSample->classstar);
                    addFloat8(&strBuf, tSample->ellipticity);
                    addFloat8(&strBuf, tSample->flags);
                    addFloat8(&strBuf, tSample->mag);
                    addFloat8(&strBuf, tSample->mage);
                    addFloat8(&strBuf, tSample->fwhm);
                    addFloat8(&strBuf, tSample->pixx);
                    addFloat8(&strBuf, tSample->pixy);
                    addFloat8(&strBuf, tSample->thetaimage);
                    addFloat8(&strBuf, tSample->vignet);
                    addInt32(&strBuf, tSample->inarea);
                    addFloat8(&strBuf, tSample->magcalib);
                    addFloat8(&strBuf, tSample->magcalibe);
                    addFloat8(&strBuf, tSample->pixx1);
                    addFloat8(&strBuf, tSample->pixy1);
                
                    int copydatares = PQputCopyData(conn, strBuf.data, strBuf.cursor);

                    j++;
                    strBuf.cursor = 0;
                }
                tSample = tSample->next;
            }
            PQputCopyEnd(conn, NULL);
        }
        PQclear(pgrst);
    }


    PQfinish(conn);
    free(buf);
    free(sqlBuf);

    end = clock();
    if (showProcessInfo) {
        printf("write table %s %d row\n", match_table, i);
        if (!fileType) {
            printf("write table %s %d row\n", ot_table, j);
            printf("write table %s %d row\n", ot_flux_table, k);
        }
        printf("time of write DB is: %fs\n", (end - start)*1.0 / ONESECOND);
    }
}