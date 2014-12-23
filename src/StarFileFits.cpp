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
}

StarFileFits::StarFileFits(char* fileName, float areaBox, int fitsHDU, int wcsext,
        int fluxRatioSDTimes, float magErrThreshold, int gridSize) {

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
    this->gridSize = gridSize;
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

    CMStar *tStar = (CMStar *) malloc((nelem) * sizeof (CMStar));
    for (int i = 0; i < nelem - 1; i++) {
        tStar[i].id = id[i];
        tStar[i].alpha = ra[i];
        tStar[i].delta = dec[i];
        tStar[i].pixx = pixx[i];
        tStar[i].pixy = pixy[i];
        tStar[i].mag = mag[i];
        tStar[i].mage = mage[i];
        tStar[i].thetaimage = thetaimage[i];
        tStar[i].flags = flags[i];
        tStar[i].ellipticity = ellipticity[i];
        tStar[i].classstar = classstar[i];
        tStar[i].background = background[i];
        tStar[i].fwhm = fwhm[i];
        tStar[i].vignet = vignet[i];
        if (colNum == 16) {
            tStar[i + 1].pixx1 = pixx1[i];
            tStar[i + 1].pixy1 = pixy1[i];
        } else {
            tStar[i + 1].pixx1 = pixx[i];
            tStar[i + 1].pixy1 = pixy[i];
        }
        tStar[i].next = &tStar[i + 1];
        tStar[i].match = NULL;
        tStar[i].error = 100.0;

        tStar[i].catid = 0.0;
        tStar[i].crossid = 0.0;
        tStar[i].magnorm = 0.0;
        tStar[i].magcalib = 0.0;
        tStar[i].magcalibe = 0.0;
        tStar[i].fluxRatio = 0.0;
        tStar[i].inarea = 0;
        tStar[i].matchNum = 0;
        tStar[i].gridIdx = 0;
        tStar[i].fluxVarTag = 0;
    }
    tStar[nelem - 1].id = id[nelem - 1];
    tStar[nelem - 1].alpha = ra[nelem - 1];
    tStar[nelem - 1].delta = dec[nelem - 1];
    tStar[nelem - 1].pixx = pixx[nelem - 1];
    tStar[nelem - 1].pixy = pixy[nelem - 1];
    tStar[nelem - 1].mag = mag[nelem - 1];
    tStar[nelem - 1].mage = mage[nelem - 1];
    tStar[nelem - 1].thetaimage = thetaimage[nelem - 1];
    tStar[nelem - 1].flags = flags[nelem - 1];
    tStar[nelem - 1].ellipticity = ellipticity[nelem - 1];
    tStar[nelem - 1].classstar = classstar[nelem - 1];
    tStar[nelem - 1].background = background[nelem - 1];
    tStar[nelem - 1].fwhm = fwhm[nelem - 1];
    tStar[nelem - 1].vignet = vignet[nelem - 1];
    if (colNum == 16) {
        tStar[nelem].pixx1 = pixx1[nelem - 1];
        tStar[nelem].pixy1 = pixy1[nelem - 1];
    } else {
        tStar[nelem].pixx1 = pixx[nelem - 1];
        tStar[nelem].pixy1 = pixy[nelem - 1];
    }
    tStar[nelem - 1].next = NULL;
    tStar[nelem - 1].match = NULL;
    tStar[nelem - 1].error = 100.0;

    tStar[nelem - 1].catid = 0.0;
    tStar[nelem - 1].crossid = 0.0;
    tStar[nelem - 1].magnorm = 0.0;
    tStar[nelem - 1].magcalib = 0.0;
    tStar[nelem - 1].magcalibe = 0.0;
    tStar[nelem - 1].fluxRatio = 0.0;
    tStar[nelem - 1].inarea = 0;
    tStar[nelem - 1].matchNum = 0;
    tStar[nelem - 1].gridIdx = 0;
    tStar[nelem - 1].fluxVarTag = 0;

    starList = tStar;

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

    fits_read_tblbytes(fptr, 1, 1, naxes[0], (unsigned char*) wcsHeader, &status);

    struct WorldCoor *wcs = GetFITSWCS(fileName, wcsHeader, 0, &cra, &cdec, &dra, &ddec, &secpix,
            &wp, &hp, &sysout, &eqout);

    double result = 0;
    if (nowcs(wcs)) {
        fprintf(stderr, "No WCS in image file %s\n", fileName);
    } else {
        hgetr8(wcsHeader, field, &result);
    }


    if (fits_close_file(fptr, &status))
        printerror(status);

    free(wcsHeader);
    wcsfree(wcs);
    return result;
}

void StarFileFits::getMagDiff() {

    if (starList == NULL) return;

    fluxPtn = (FluxPartition*) malloc(sizeof (FluxPartition) * gridSize * gridSize);
    memset(fluxPtn, 0, sizeof (FluxPartition) * gridSize * gridSize);
    float minRaf = 360.0;
    float maxRaf = 0.0;
    float minDecf = 90.0;
    float maxDecf = -90.0;
    CMStar *tStar = starList;
    while (tStar) {
        if (tStar->pixx < minRaf) {
            minRaf = tStar->pixx;
        }
        if (tStar->pixx > maxRaf) {
            maxRaf = tStar->pixx;
        }
        if (tStar->pixy < minDecf) {
            minDecf = tStar->pixy;
        }
        if (tStar->pixy > maxDecf) {
            maxDecf = tStar->pixy;
        }
        tStar = tStar->next;
    }

    int minRai = floor(minRaf);
    int maxRai = ceil(maxRaf);
    int minDeci = floor(minDecf);
    int maxDeci = ceil(maxDecf);

    float raGridLen = (maxRai - minRai) *1.0 / gridSize;
    float decGridLen = (maxDeci - minDeci) *1.0 / gridSize;

    tStar = starList;
    //统计每个分区中星的个数
    while (tStar) {
        int xIdx = (tStar->pixx - minRai) / raGridLen;
        int yIdx = (tStar->pixy - minDeci) / decGridLen;
        tStar->gridIdx = yIdx * gridSize + xIdx;
        if (tStar->gridIdx > gridSize * gridSize - 1)
            printf("gridIdx=%d\n", tStar->gridIdx);
        if ((tStar->match != NULL) && (tStar->error < areaBox)) {
            if (tStar->mage < magErrThreshold)
                fluxPtn[tStar->gridIdx].number1++;
            fluxPtn[tStar->gridIdx].number2++;
        }
        tStar = tStar->next;
    }

    //为分区数组分配空间
    for (int i = 0; i < gridSize; i++) {
        for (int j = 0; j < gridSize; j++) {
            int idx = i * gridSize + j;
            fluxPtn[idx].fluxRatios1 = (double*) malloc(fluxPtn[idx].number1 * sizeof (double));
            fluxPtn[idx].fluxRatios2 = (double*) malloc(fluxPtn[idx].number2 * sizeof (double));
        }
    }

    tStar = starList;
    //对分区数组赋值
    while (tStar) {
        if ((tStar->match != NULL) && (tStar->error < areaBox)) {
            tStar->fluxRatio = pow10(-0.4 * (tStar->match->mag - tStar->mag));
            if (tStar->mage < magErrThreshold)
                fluxPtn[tStar->gridIdx].fluxRatios1[fluxPtn[tStar->gridIdx].curIdx1++] = tStar->fluxRatio;
            fluxPtn[tStar->gridIdx].fluxRatios2[fluxPtn[tStar->gridIdx].curIdx2++] = tStar->fluxRatio;
        }
        tStar = tStar->next;
    }

    for (int i = 0; i < gridSize; i++) {
        for (int j = 0; j < gridSize; j++) {
            int idx = i * gridSize + j;
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

    for (int i = 0; i < gridSize; i++) {
        for (int j = 0; j < gridSize; j++) {
            int idx = i * gridSize + j;
            free(fluxPtn[idx].fluxRatios1);
            free(fluxPtn[idx].fluxRatios2);
        }
    }
    free(fluxPtn);
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
    for (int i = 0; i < gridSize; i++) {
        for (int j = 0; j < gridSize; j++) {
            total += fluxPtn[i * gridSize + j].standardDeviation;
        }
    }
    standardDeviation = total / (gridSize * gridSize);
}

void StarFileFits::setFluxRatioMedian() {
    if (NULL == fluxPtn)
        return;
    float total = 0;
    for (int i = 0; i < gridSize; i++) {
        for (int j = 0; j < gridSize; j++) {
            total += fluxPtn[i * gridSize + j].fluxRatioMedian;
        }
    }
    fluxRatioMedian = total / (gridSize * gridSize);
}

void StarFileFits::setFluxRatioAverage() {
    if (NULL == fluxPtn)
        return;
    float total = 0;
    for (int i = 0; i < gridSize; i++) {
        for (int j = 0; j < gridSize; j++) {
            total += fluxPtn[i * gridSize + j].fluxRatioAverage;
        }
    }
    fluxRatioAverage = total / (gridSize * gridSize);
}

void StarFileFits::setMagDiff() {
    if (NULL == fluxPtn)
        return;
    float total = 0;
    for (int i = 0; i < gridSize; i++) {
        for (int j = 0; j < gridSize; j++) {
            total += fluxPtn[i * gridSize + j].magDiff;
        }
    }
    magDiff = total / (gridSize * gridSize);
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