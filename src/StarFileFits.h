/* 
 * File:   StarFileRef.h
 * Author: xy
 *
 * Created on December 16, 2014, 9:37 AM
 */

#ifndef STARFILEREF_H
#define	STARFILEREF_H

#include "StarFile.h"

struct AREABOX{
    int top;
    int left;
    int right;
    int down;
};

class StarFileFits : public StarFile {
public:
    int showProcessInfo;

    char * fileName;
    float areaBox;
    int fitsHDU;
    int wcsext;
    double airmass;
    double jd;
    double magDiff;
    double fluxRatioAverage;
    double fluxRatioMedian;
    double standardDeviation;
    int fluxRatioSDTimes; //abs(ratioRatio - flusRatioAverage) > flusRatioSD * standardDeviation;

    float magErrThreshold;      //对magerr小于magErrThreshold的星计算magDiff

    StarFileFits();
    StarFileFits(char * fileName);
    StarFileFits(char* fileName, float areaBox, int fitsHDU, int wcsext, int fluxRatioSDTimes, float magErrThreshold);
    StarFileFits(const StarFileFits& orig);
    virtual ~StarFileFits();

    void readStar();
    void readStar(char * fileName);
    void readProerty();
    void setMagErrThreshold(float magErrThreshold);
    void setFitsHDU(int fitsHDU);
    void setAreaBox(float areaBox);
    void setFluxRatioSDTimes(int fluxRatioSDTimes);
    void setFileName(char* fileName);
    void getMagDiff();
    void fluxNorm();
    void wcsJudge(int wcsext);
private:
    double getFieldFromWCSFloat(char *fileName, int wcsext, char *field);
    int isInAreaBox(int x, int y, struct AREABOX ab);
    void printerror(int status);
};

#endif	/* STARFILEREF_H */

