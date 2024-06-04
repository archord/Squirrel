/* 
 * File:   StarFileRef.cpp
 * Author: xy
 * 
 * Created on December 16, 2014, 9:37 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "fitsio.h"
#include "anwcs.h"
#include "wcs.h"
#include "fitsfile.h"
#include "StarFileFits.h"

extern "C" {
  struct WorldCoor *GetFITSWCS(char *filename,
          char *header,
          int verbose,
          double *cra,
          double *cdec,
          double *dra,
          double *ddec,
          double *secpix,
          int *wp,
          int *hp,
          int *sysout,
          double *eqout);
}

StarFileFits::StarFileFits() {

  showProcessInfo = 0;
  areaBox = 0.0;
  airmass = 0.0;
  jd = 0.0;
  magDiff = 0.0;
  fluxRatioAverage = 0.0;
  fluxRatioMedian = 0.0;
  standardDeviation = 0.0;
  fluxRatioSDTimes = 0;
  magErrThreshold = 0.05;

  fluxPtn = NULL;
}

StarFileFits::StarFileFits(char* fileName) : StarFile(fileName) {

  showProcessInfo = 0;
  areaBox = 0.0;
  airmass = 0.0;
  jd = 0.0;
  magDiff = 0.0;
  fluxRatioAverage = 0.0;
  fluxRatioMedian = 0.0;
  standardDeviation = 0.0;
  fluxRatioSDTimes = 0;
  magErrThreshold = 0.05;

  fluxPtn = NULL;
}

StarFileFits::StarFileFits(char* fileName, float areaBox, int fitsHDU, int wcsext,
        int fluxRatioSDTimes, float magErrThreshold, int gridX, int gridY) {

  this->fileExist = 0;

  this->showProcessInfo = 0;
  this->airmass = 0.0;
  this->jd = 0.0;
  this->magDiff = 0.0;
  this->fluxRatioAverage = 0.0;
  this->fluxRatioMedian = 0.0;
  this->standardDeviation = 0.0;

  this->fileName = fileName;
  this->areaBox = areaBox;
  this->fitsHDU = fitsHDU;
  this->wcsext = wcsext;
  this->fluxRatioSDTimes = fluxRatioSDTimes;
  this->magErrThreshold = magErrThreshold;

  this->gridX = gridX;
  this->gridY = gridY;
  this->fluxPtn = NULL;
}

StarFileFits::StarFileFits(const StarFileFits& orig) : StarFile(orig) {
}

StarFileFits::~StarFileFits() {
  freeFluxPtn();
}

void StarFileFits::readStar() {
  readStar(fileName);
}

void StarFileFits::readStar(char * fileName) {

  fitsfile *fptr; /* pointer to the FITS file, defined in fitsio.h */
  int status, hdunum, hdutype, nfound, anynull, ii, colNum;
  long naxes[2], frow, felem, nelem, longnull;
  float floatnull;
  char strnull[10], *ttype[3];
  long *id = NULL;
  float *ra = NULL;
  float *dec = NULL;
  float *pixx = NULL;
  float *pixy = NULL;
  float *mag = NULL;
  float *mage = NULL;
  float *thetaimage = NULL;
  float *flags = NULL;
  float *ellipticity = NULL;
  float *classstar = NULL;
  float *background = NULL;
  float *fwhm = NULL;
  float *vignet = NULL;
  float *pixx1 = NULL;
  float *pixy1 = NULL;

  status = 0;
  hdunum = fitsHDU;

  if (fits_open_file(&fptr, fileName, READONLY, &status)) {
    printf("Open file :%s error!\n", fileName);
    printerror(status);
    return;
  }
  /* move to the HDU */
  if (fits_movabs_hdu(fptr, fitsHDU, &hdutype, &status)) {
    printf("fits movabs hdu error!\n");
    printerror(status);
    return;
  }

  /* read the NAXIS1 and NAXIS2 keyword to get table size */
  if (fits_read_keys_lng(fptr, "NAXIS", 1, 2, naxes, &nfound, &status)) {
    printerror(status);
    return;
  }

  starNum = naxes[1];

  fits_get_num_cols(fptr, &colNum, &status);

  //printf("row=%d, clo=%d\n",naxes[0],naxes[1]);

  id = (long *) malloc(naxes[1] * sizeof (long));
  ra = (float *) malloc(naxes[1] * sizeof (float));
  dec = (float *) malloc(naxes[1] * sizeof (float));
  pixx = (float *) malloc(naxes[1] * sizeof (float));
  pixy = (float *) malloc(naxes[1] * sizeof (float));
  mag = (float *) malloc(naxes[1] * sizeof (float));
  mage = (float *) malloc(naxes[1] * sizeof (float));
  thetaimage = (float *) malloc(naxes[1] * sizeof (float));
  flags = (float *) malloc(naxes[1] * sizeof (float));
  ellipticity = (float *) malloc(naxes[1] * sizeof (float));
  classstar = (float *) malloc(naxes[1] * sizeof (float));
  background = (float *) malloc(naxes[1] * sizeof (float));
  fwhm = (float *) malloc(naxes[1] * sizeof (float));
  vignet = (float *) malloc(naxes[1] * sizeof (float));
  pixx1 = (float *) malloc(naxes[1] * sizeof (float));
  pixy1 = (float *) malloc(naxes[1] * sizeof (float));

  //for (ii = 0; ii < 3; ii++)      /* allocate space for the column labels */
  //ttype[ii] = (char *) malloc(FLEN_VALUE);  /* max label length = 69 */


  if (hdutype != ASCII_TBL && hdutype != BINARY_TBL) {
    printf("Error: this HDU is not an ASCII or binary table\n");
    printerror(status);
    return;
  }

  /* read the column names from the TTYPEn keywords */
  //fits_read_keys_str(fptr, "TTYPE", 1, 3, ttype, &nfound, &status);

  //printf(" Row  %10s %10s %10s\n", ttype[0], ttype[1], ttype[2]);

  frow = 1;
  felem = 1;
  nelem = naxes[1];
  strcpy(strnull, " ");
  longnull = 0;
  floatnull = 0;

  /*  read the columns &floatnull*/
  fits_read_col(fptr, TLONG, 1, frow, felem, nelem, &longnull, id, &anynull, &status);
  printerror(status);
  fits_read_col(fptr, TFLOAT, 2, frow, felem, nelem, &floatnull, ra, &anynull, &status);
  printerror(status);
  fits_read_col(fptr, TFLOAT, 3, frow, felem, nelem, &floatnull, dec, &anynull, &status);
  printerror(status);
  fits_read_col(fptr, TFLOAT, 4, frow, felem, nelem, &floatnull, pixx, &anynull, &status);
  printerror(status);
  fits_read_col(fptr, TFLOAT, 5, frow, felem, nelem, &floatnull, pixy, &anynull, &status);
  printerror(status);
  fits_read_col(fptr, TFLOAT, 6, frow, felem, nelem, &floatnull, mag, &anynull, &status);
  printerror(status);
  fits_read_col(fptr, TFLOAT, 7, frow, felem, nelem, &floatnull, mage, &anynull, &status);
  printerror(status);
  fits_read_col(fptr, TFLOAT, 8, frow, felem, nelem, &floatnull, thetaimage, &anynull, &status);
  printerror(status);
  fits_read_col(fptr, TFLOAT, 9, frow, felem, nelem, &floatnull, flags, &anynull, &status);
  printerror(status);
  fits_read_col(fptr, TFLOAT, 10, frow, felem, nelem, &floatnull, ellipticity, &anynull, &status);
  printerror(status);
  fits_read_col(fptr, TFLOAT, 11, frow, felem, nelem, &floatnull, classstar, &anynull, &status);
  printerror(status);
  fits_read_col(fptr, TFLOAT, 12, frow, felem, nelem, &floatnull, background, &anynull, &status);
  printerror(status);
  fits_read_col(fptr, TFLOAT, 13, frow, felem, nelem, &floatnull, fwhm, &anynull, &status);
  printerror(status);
  fits_read_col(fptr, TFLOAT, 14, frow, felem, nelem, &floatnull, vignet, &anynull, &status);
  printerror(status);
  if (colNum == 16) {
    fits_read_col(fptr, TFLOAT, 15, frow, felem, nelem, &floatnull, pixx1, &anynull, &status);
    printerror(status);
    fits_read_col(fptr, TFLOAT, 16, frow, felem, nelem, &floatnull, pixy1, &anynull, &status);
    printerror(status);
  }

  if (fits_close_file(fptr, &status))
    printerror(status);

  CMStar *bfStar = NULL;
  for (int i = 0; i < nelem; i++) {
    CMStar *tStar = (CMStar *) malloc(sizeof (CMStar));
    if (i == 0) {
      starList = tStar;
    } else {
      bfStar->next = tStar;
    }
    bfStar = tStar;

    tStar->id = id[i];
    tStar->alpha = ra[i];
    tStar->delta = dec[i];
    tStar->pixx = pixx[i];
    tStar->pixy = pixy[i];
    tStar->mag = mag[i];
    tStar->mage = mage[i];
    tStar->thetaimage = thetaimage[i];
    tStar->flags = flags[i];
    tStar->ellipticity = ellipticity[i];
    tStar->classstar = classstar[i];
    tStar->background = background[i];
    tStar->fwhm = fwhm[i];
    tStar->vignet = vignet[i];
    if (colNum == 16) {
      tStar->pixx1 = pixx1[i];
      tStar->pixy1 = pixy1[i];
    } else {
      tStar->pixx1 = pixx[i];
      tStar->pixy1 = pixy[i];
    }
    tStar->next = NULL;
    tStar->match = NULL;
    tStar->error = 100.0;

    tStar->catid = 0;
    tStar->crossid = 0;
    tStar->magnorm = 0.0;
    tStar->magcalib = 0.0;
    tStar->magcalibe = 0.0;
    tStar->fluxRatio = 0.0;
    tStar->inarea = 0;
    tStar->matchNum = 0;
    tStar->gridIdx = 0;
    tStar->fluxVarTag = 0;
    tStar->line = NULL;
  }

#ifdef PRINT_CM_DETAIL
  printf("total read star %d\n", starNum);
#endif

  free(id);
  free(ra);
  free(dec);
  free(pixx);
  free(pixy);
  free(mag);
  free(mage);
  free(thetaimage);
  free(flags);
  free(ellipticity);
  free(classstar);
  free(background);
  free(fwhm);
  free(vignet);
}

void StarFileFits::readProerty() {

  airmass = getFieldFromWCSFloat(fileName, wcsext, "AIRMASS");
  jd = getFieldFromWCSFloat(fileName, wcsext, "JD");
}

void StarFileFits::setMagErrThreshold(float magErrThreshold) {
  this->magErrThreshold = magErrThreshold;
}

void StarFileFits::setFitsHDU(int fitsHDU) {
  this->fitsHDU = fitsHDU;
}

void StarFileFits::setAreaBox(float areaBox) {
  this->areaBox = areaBox;
}

void StarFileFits::setFluxRatioSDTimes(int fluxRatioSDTimes) {
  this->fluxRatioSDTimes = fluxRatioSDTimes;
}

void StarFileFits::setFileName(char* fileName) {
  this->fileName = fileName;
}

double StarFileFits::getFieldFromWCSFloat(char *fileName, int wcsext, char *field) {

  fitsfile *fptr; /* pointer to the FITS file, defined in fitsio.h */
  int status, hdunum, hdutype, nfound, anynull, ii;
  long naxes[2], frow, felem, nelem, longnull;
  float floatnull;
  char strnull[10];
  char *wcsHeader = NULL;

  status = 0;
  hdunum = wcsext;

  if (fits_open_file(&fptr, fileName, READONLY, &status)) {
    printf("Open file :%s error!\n", fileName);
    printerror(status);
    return 0.0;
  }
  /* move to the HDU */
  if (fits_movabs_hdu(fptr, wcsext, &hdutype, &status)) {
    printf("fits movabs hdu error!\n");
    printerror(status);
    return 0.0;
  }

  /* read the NAXIS1 and NAXIS2 keyword to get table size */
  if (fits_read_keys_lng(fptr, "NAXIS", 1, 2, naxes, &nfound, &status)) {
    printerror(status);
    return 0.0;
  }


  if (hdutype != ASCII_TBL && hdutype != BINARY_TBL) {
    printf("Error: this HDU is not an ASCII or binary table\n");
    printerror(status);
    return 0.0;
  }

  //printf("naxes[0]=%d naxes[1]=%d\n", naxes[0], naxes[1]);
  frow = 1;
  felem = 1;
  nelem = naxes[1];
  strcpy(strnull, " ");
  longnull = 0;
  floatnull = 0.0;

  wcsHeader = (char *) malloc(naxes[0]);

  double cra, cdec, dra, ddec, secpix, drot;
  double eqout = 0.0;
  double eqin = 0.0;
  int sysout = 0;
  int wp, hp;
  double result = 0;

  fits_read_tblbytes(fptr, 1, 1, naxes[0], (unsigned char*) wcsHeader, &status);
  hgetr8(wcsHeader, field, &result);

  if (fits_close_file(fptr, &status))
    printerror(status);

  free(wcsHeader);
  return result;
}

void StarFileFits::getMagDiff() {

  if (starList == NULL) return;

  int gridNUmber = gridX * gridY;
  fluxPtn = (FluxPartition*) malloc(sizeof (FluxPartition) * gridNUmber);
  memset(fluxPtn, 0, sizeof (FluxPartition) * gridNUmber);

  CMStar *tStar = starList;
  float minXf = tStar->pixx;
  float maxXf = tStar->pixx;
  float minYf = tStar->pixy;
  float maxYf = tStar->pixy;
  tStar = tStar->next;

  while (tStar) {
    if (tStar->pixx < minXf) {
      minXf = tStar->pixx;
    }
    if (tStar->pixx > maxXf) {
      maxXf = tStar->pixx;
    }
    if (tStar->pixy < minYf) {
      minYf = tStar->pixy;
    }
    if (tStar->pixy > maxYf) {
      maxYf = tStar->pixy;
    }
    tStar = tStar->next;
  }
  
  maxXf = maxXf + 1;
  maxYf = maxYf + 1;
  minXf = minXf - 1;
  minYf = minYf - 1;

  minXf = minXf < 0 ? 0 : minXf;
  minYf = minYf < 0 ? 0 : minYf;

  int minXi = floor(minXf);
  int maxXi = ceil(maxXf);
  int minYi = floor(minYf);
  int maxYi = ceil(maxYf);

  float xGridLen = (maxXi - minXi) *1.0 / gridX;
  float yGridLen = (maxYi - minYi) *1.0 / gridY;

  tStar = starList;
  //统计每个分区中星的个数

  while (tStar) {
    int xIdx = (tStar->pixx - minXi) / xGridLen;
    int yIdx = (tStar->pixy - minYi) / yGridLen;

    if (xIdx >= gridX) xIdx = gridX-1;
    if (yIdx >= gridY) yIdx = gridY-1;

    tStar->gridIdx = yIdx * gridX + xIdx;

    if (tStar->gridIdx > gridNUmber - 1) {
      printf("error: point(%f,%f) gridIdx=%d, bigger then maxGridIDx=%d, minx=%f, miny=%f, maxx=%f, maxy=%f\n",
              tStar->pixx, tStar->pixy, tStar->gridIdx, gridNUmber - 1, minXf, minYf, maxXf, maxYf);
    }

    if (tStar->gridIdx < 0) {
      tStar->gridIdx = 0;
    } else if (tStar->gridIdx > gridNUmber - 1) {
      tStar->gridIdx = gridNUmber - 1;
    }

    if ((tStar->match != NULL) && (tStar->error < areaBox)) {
      if (tStar->mage < magErrThreshold)
        fluxPtn[tStar->gridIdx].number1++;
      fluxPtn[tStar->gridIdx].number2++;
    }

    tStar = tStar->next;
  }

  //为分区数组分配空间
  for (int i = 0; i < gridY; i++) {
    for (int j = 0; j < gridX; j++) {
      int idx = i * gridX + j;
      fluxPtn[idx].fluxRatios1 = (double*) malloc(fluxPtn[idx].number1 * sizeof (double));
      fluxPtn[idx].fluxRatios2 = (double*) malloc(fluxPtn[idx].number2 * sizeof (double));
    }
  }

  tStar = starList;
  //对分区数组赋值
  while (tStar) {
    if ((tStar->match != NULL) && (tStar->error < areaBox)) {
      tStar->fluxRatio = pow(10, -0.4 * (tStar->match->mag - tStar->mag));
      if (tStar->mage < magErrThreshold)
        fluxPtn[tStar->gridIdx].fluxRatios1[fluxPtn[tStar->gridIdx].curIdx1++] = tStar->fluxRatio;
      fluxPtn[tStar->gridIdx].fluxRatios2[fluxPtn[tStar->gridIdx].curIdx2++] = tStar->fluxRatio;
    }
    tStar = tStar->next;
  }

  for (int i = 0; i < gridY; i++) {
    for (int j = 0; j < gridX; j++) {
      int idx = i * gridX + j;
      fluxPtn[idx].fluxRatioAverage = getAverage(fluxPtn[idx].fluxRatios2, fluxPtn[idx].number2);
      fluxPtn[idx].standardDeviation =
              getStandardDeviation(fluxPtn[idx].fluxRatios2, fluxPtn[idx].number2, fluxPtn[idx].fluxRatioAverage);
      fluxPtn[idx].timesOfSD = fluxRatioSDTimes * fluxPtn[idx].standardDeviation;
      quickSort(0, fluxPtn[idx].number1 - 1, fluxPtn[idx].fluxRatios1);
      double median = getMedian(fluxPtn[idx].fluxRatios1, fluxPtn[idx].number1);
      fluxPtn[idx].fluxRatioMedian = median;
      fluxPtn[idx].magDiff = -2.5 * log10(median);
    }
  }

  setStandardDeviation();
  setFluxRatioMedian();
  setFluxRatioAverage();
  setMagDiff();
}

void StarFileFits::freeFluxPtn() {

  if (NULL != fluxPtn) {
    for (int i = 0; i < gridY; i++) {
      for (int j = 0; j < gridX; j++) {
        int idx = i * gridX + j;
        free(fluxPtn[idx].fluxRatios1);
        free(fluxPtn[idx].fluxRatios2);
      }
    }
    free(fluxPtn);
  }
}

void StarFileFits::fluxNorm() {

  if (NULL == starList || NULL == fluxPtn) return;

  CMStar *tStar = starList;
  while (tStar) {
    if ((tStar->match != NULL)&&(tStar->error <= areaBox)) {
      tStar->magnorm = tStar->mag + fluxPtn[tStar->gridIdx].magDiff;
    } else {
      tStar->magnorm = tStar->mag + magDiff;
    }
    tStar = tStar->next;
  }
}

/**
 * 标识光变大的星
 * 将fabs(CMStar->fluxRatio-fluxRatioMedian)大于fluxRatioSDTimes*standardDeviation
 * 的CMStar的fluxVarTag值设置为1
 */
void StarFileFits::tagFluxLargeVariation() {

  CMStar *tStar = starList;
  while (tStar) {
    if ((tStar->match != NULL) && (tStar->error < areaBox)) { // && (tStar->mage < 0.05)
      double ratioAbs = fabs(tStar->fluxRatio - fluxPtn[tStar->gridIdx].fluxRatioMedian);
      if (ratioAbs > fluxPtn[tStar->gridIdx].timesOfSD) {
        tStar->fluxVarTag = 1;
      }
    }
    tStar = tStar->next;
  }
}

/**
 * 图像X，Y匹配，判断目标星的X，Y坐标是否越过模板图像的边界
 */
void StarFileFits::judgeInAreaPlane() {

  AreaBox ab;
  ab.left = 0;
  ab.down = 0;
  ab.top = fieldHeight;
  ab.right = fieldWidth;

  int notMatched = 0;
  int outArea = 0;

  CMStar *tStar = starList;
  while (tStar) {
    if (tStar->error >= areaBox) {
      if (isInAreaBox(tStar->pixx, tStar->pixy, ab)) {
        tStar->inarea = 1;
      } else {
        tStar->inarea = -1;
        outArea++;
      }
      notMatched++;
    }
    tStar = tStar->next;
  }
  if (showProcessInfo) {
    printf("\nnot matched: %d, out area: %d\n", notMatched, outArea);
  }
}

/**
 * 图像天球坐标匹配，判断目标星的X，Y坐标是否越过模板图像的边界，XY的值根据模板文件wcs头转换而来
 * @param wcsext wcs头在fits文件中的位置
 */
void StarFileFits::wcsJudge(int wcsext) {

  fitsfile *fptr; /* pointer to the FITS file, defined in fitsio.h */
  int status, hdunum, hdutype, nfound, anynull, ii;
  long naxes[2], frow, felem, nelem, longnull;
  float floatnull;
  char strnull[10];
  char *wcsHeader = NULL;

  status = 0;

  if (fits_open_file(&fptr, fileName, READONLY, &status)) {
    printf("Open file :%s error!\n", fileName);
    printerror(status);
    return;
  }
  /* move to the HDU */
  if (fits_movabs_hdu(fptr, wcsext, &hdutype, &status)) {
    printf("fits movabs hdu error!\n");
    printerror(status);
    return;
  }

  /* read the NAXIS1 and NAXIS2 keyword to get table size */
  if (fits_read_keys_lng(fptr, "NAXIS", 1, 2, naxes, &nfound, &status)) {
    printerror(status);
    return;
  }

  if (hdutype != ASCII_TBL && hdutype != BINARY_TBL) {
    printf("Error: this HDU is not an ASCII or binary table\n");
    printerror(status);
    return;
  }

  frow = 1;
  felem = 1;
  nelem = naxes[1];
  strcpy(strnull, " ");
  longnull = 0;
  floatnull = 0.0;

  wcsHeader = (char *) malloc(naxes[0]);

  double cra, cdec, dra, ddec, secpix, drot;
  double eqout = 0.0;
  double eqin = 0.0;
  int sysout = 0;
  int wp, hp;

  fits_read_tblbytes(fptr, 1, 1, naxes[0], (unsigned char*) wcsHeader, &status);

  struct WorldCoor *wcs = GetFITSWCS(fileName, wcsHeader, showProcessInfo, &cra, &cdec, &dra, &ddec, &secpix,
          &wp, &hp, &sysout, &eqout);

  int notMatched = 0;
  int outArea = 0;
  if (nowcs(wcs)) {
    fprintf(stderr, "No WCS in image file %s\n", fileName);
  } else {

    double x, y, ra, dec;
    ra = 60;
    dec = 60;
    int offscale = 1; //4976.6050 -4979.14447
    char *coorsys = "j2000";

    AreaBox ab;
    ab.left = 0;
    ab.down = 0;

    hgeti4(wcsHeader, "NAXIS1", &(ab.top));
    hgeti4(wcsHeader, "NAXIS2", &(ab.right));

    CMStar *tStar = starList;
    while (tStar) {
      /*
                  if(tStar->match==NULL){
       */
      if (tStar->error > areaBox) {
        wcsc2pix(wcs, tStar->alpha, tStar->delta, coorsys, &x, &y, &offscale);

        if (isInAreaBox(x, y, ab)) {
          tStar->inarea = 1;
        } else {
          tStar->inarea = -1;
          outArea++;
        }
        notMatched++;
      }
      tStar = tStar->next;
    }
  }


  if (fits_close_file(fptr, &status))
    printerror(status);

  free(wcsHeader);
  wcsfree(wcs);

}

void StarFileFits::setStandardDeviation() {
  if (NULL == fluxPtn)
    return;
  float total = 0;
  for (int i = 0; i < gridY; i++) {
    for (int j = 0; j < gridX; j++) {
      total += fluxPtn[i * gridX + j].standardDeviation;
    }
  }
  standardDeviation = total / (gridX * gridY);
}

void StarFileFits::setFluxRatioMedian() {
  if (NULL == fluxPtn)
    return;
  float total = 0;
  for (int i = 0; i < gridY; i++) {
    for (int j = 0; j < gridX; j++) {
      total += fluxPtn[i * gridX + j].fluxRatioMedian;
    }
  }
  fluxRatioMedian = total / (gridX * gridY);
}

void StarFileFits::setFluxRatioAverage() {
  if (NULL == fluxPtn)
    return;
  float total = 0;
  for (int i = 0; i < gridY; i++) {
    for (int j = 0; j < gridX; j++) {
      total += fluxPtn[i * gridX + j].fluxRatioAverage;
    }
  }
  fluxRatioAverage = total / (gridX * gridY);
}

void StarFileFits::setMagDiff() {
  if (NULL == fluxPtn)
    return;
  float total = 0;
  for (int i = 0; i < gridY; i++) {
    for (int j = 0; j < gridX; j++) {
      total += fluxPtn[i * gridX + j].magDiff;
    }
  }
  magDiff = total / (gridX * gridY);
}

int StarFileFits::isInAreaBox(int x, int y, AreaBox ab) {
  int flag = 0;
  if ((x > ab.left) && (x < ab.right) && (y > ab.down) && (y < ab.top))
    flag = 1;
  return flag;
}

/*cfitsio error output*/
void StarFileFits::printerror(int status) {
  /*****************************************************/
  /* Print out cfitsio error messages and exit program */
  /*****************************************************/
  if (status) {
    fits_report_error(stderr, status); /* print error report */
    exit(status); /* terminate the program, returning error status */
  }
  return;
}

void StarFileFits::setFieldHeight(float fieldHeight) {
  this->fieldHeight = fieldHeight;
}

void StarFileFits::setFieldWidth(float fieldWidth) {
  this->fieldWidth = fieldWidth;
}

void StarFileFits::writeStar(char * outFile) {

  FILE *fp = fopen(outFile, "w");
  fprintf(fp, "starId\tra\tdec\tx\ty\tx1\ty1\tmag\tmage\n");

  long count = 0;
  CMStar *tStar = starList;
  while (NULL != tStar) {
    fprintf(fp, "%8d %12f %12f %12f %12f %12f %12f %12f %12f\n",
            tStar->id, tStar->alpha, tStar->delta, tStar->pixx, tStar->pixy,
            tStar->pixx1, tStar->pixy1, tStar->mag, tStar->mage);
    count++;
    tStar = tStar->next;
  }
  printf("total write %ld stars\n", count);
  fclose(fp);
}

void StarFileFits::writeMatchStar(char * outFile) {

  FILE *fp = fopen(outFile, "w");
  fprintf(fp, "objId\tobjX\tobjY\ttmpId\ttmpX\ttmpY\terror\n");

  long count = 0;
  CMStar *tStar = starList;
  while (NULL != tStar) {
    if (NULL != tStar->match && tStar->error < this->areaBox & tStar->error>1.5) {
    fprintf(fp, "%8d %12f %12f %8d %12f %12f %12f\n",
            tStar->id, tStar->pixx, tStar->pixy,
            tStar->match->id, tStar->match->pixx, tStar->match->pixy, tStar->error);
    count++;
    }
    tStar = tStar->next;
  }
  printf("total write %ld stars\n", count);
  fclose(fp);
}