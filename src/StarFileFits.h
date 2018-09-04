/* 
 * File:   StarFileRef.h
 * Author: xy
 *
 * Created on December 16, 2014, 9:37 AM
 */

#ifndef STARFILEREF_H
#define	STARFILEREF_H

#include "StarFile.h"

class AreaBox {
public:
  int top;
  int left;
  int right;
  int down;
};

class FluxPartition {
public:
  int curIdx1;
  int curIdx2;
  int number1;
  int number2;
  double *fluxRatios1;
  double *fluxRatios2;
  double magDiff;
  double fluxRatioAverage;
  double fluxRatioMedian;
  double standardDeviation;
  double timesOfSD;
};

class StarFileFits : public StarFile {
protected:
  float fieldWidth; //星表视场的宽度
  float fieldHeight; //星表视场的高度

public:
  
  int fileExist;
  
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

  float magErrThreshold; //对magerr小于magErrThreshold的星计算magDiff

  int gridX;
  int gridY;
  FluxPartition *fluxPtn;

  StarFileFits();
  StarFileFits(char * fileName);
  StarFileFits(char* fileName, float areaBox, int fitsHDU, int wcsext,
          int fluxRatioSDTimes, float magErrThreshold, int gridX, int gridY);
  StarFileFits(const StarFileFits& orig);
  virtual ~StarFileFits();

  void readStar();
  void readStar(char * fileName);
  void writeStar(char * outFile);
  void writeMatchStar(char * outFile);
  void readProerty();
  void setMagErrThreshold(float magErrThreshold);
  void setFitsHDU(int fitsHDU);
  void setAreaBox(float areaBox);
  void setFluxRatioSDTimes(int fluxRatioSDTimes);
  void setFileName(char* fileName);
  void getMagDiff();
  void fluxNorm();
  void tagFluxLargeVariation();
  void wcsJudge(int wcsext);
  void judgeInAreaPlane();
  void setFieldHeight(float fieldHeight);
  void setFieldWidth(float fieldWidth);
private:
  double getFieldFromWCSFloat(char *fileName, int wcsext, char *field);
  int isInAreaBox(int x, int y, AreaBox ab);
  void printerror(int status);
  void setStandardDeviation();
  void setFluxRatioMedian();
  void setFluxRatioAverage();
  void setMagDiff();
  void freeFluxPtn();
};

#endif	/* STARFILEREF_H */

