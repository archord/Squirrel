/* 
 * File:   cmhead.h
 * Author: xy
 *
 * Created on October 21, 2013, 8:39 AM
 */

#ifndef CMHEAD_H
#define	CMHEAD_H


#define CHECK_IS_NULL(var,varname) \
        {if(var==NULL){\
            sprintf(statusstr, "Error Code: %d\n"\
                "File %s line %d, the input parameter \"%s\" is NULL!\n", \
                GWAC_FUNCTION_INPUT_NULL, __FILE__, __LINE__, varname);\
            return GWAC_FUNCTION_INPUT_NULL;}}
#define CHECK_STRING_NULL_OR_EMPTY(var, varname) \
        {if(var==NULL || strcmp(var, "") == 0){\
            sprintf(statusstr, "Error Code: %d\n"\
                "File %s line %d, string \"%s\" is NULL or empty!\n", \
                GWAC_STRING_NULL_OR_EMPTY, __FILE__, __LINE__, varname);\
            return GWAC_FUNCTION_INPUT_NULL;}}
#define CHECK_OPEN_FILE(fp,fname) \
        {if(fp==NULL){\
            sprintf(statusstr, "Error Code: %d\n"\
                "File %s line %d, open file \"%s\" error!\n", \
                GWAC_OPEN_FILE_ERROR, __FILE__, __LINE__, fname);\
            return GWAC_OPEN_FILE_ERROR;}}

class CMStar {
public:
    CMStar *next;
    int starId;
    int id;
    int crossid;
    float alpha;
    float delta;
    int catid;
    float background;
    float classstar;
    float ellipticity;
    float flags;
    float mag;
    float mage;
    float magnorm;
    float fwhm;
    float magcalib;
    float magcalibe;
    float pixx;
    float pixy;
    float pixx1;
    float pixy1;
    float thetaimage;
    float vignet;
    CMStar *match; //sample are the same with reference
    float error;
    int inarea; //is sample in reference rejection area, in 1, not in -1, not judge 0
    float fluxRatio; //
    char *line;
    int matchNum;
};

class CMZone {
public:
    int starNum;
    CMStar *star;
};

#define FIND_MOST_LOW
#define PRINT_CM_DETAIL

#define TREE_NODE_LENGTH 64800		//the totle number of tree node (360*180)
#define AREA_WIDTH 360
#define AREA_HEIGHT 180
//#define 3.141592653589793238462643
#define BLOCK_BASE 32
#define ERROR_GREAT_CIRCLE 0.005555555556			//(20.0/3600.0)=0.005555555556
#define	SUBAREA	0.05555555556			//(60.0/3600.0)=0.016666666667 this value must big enough, to insure all data all find.
#define LINE 1024
#define MAX_BUFFER 1024
#define ONESECOND CLOCKS_PER_SEC
#define ANG_TO_RAD 0.017453293
#define RAD_TO_ANG 57.295779513
#define INDEX_SIZE 1<<20

static const int MaxStringLength = 1024;
static const float CompareFloat = 0.000001;
static const long MaxMallocMemory = 2147483648l; //1GB=1073741824l 2GB=2147483648l

float getLineDistance(CMStar *p1, CMStar *p2);
double getGreatCircleDistance(CMStar *p1, CMStar *p2);
template<typename T>
inline T getMax(const T& a, const T& b);
long countFileLines(char *fName);
double angToRad(double angle);
float getAngleFromGreatCircle(double dec, double dist);

bool isEmpty(char *str);
bool hasNumber(char *str);
void ltrim(char *s);
void rtrim(char *s);
void trim(char *s);
void trimAll(char *s);

void quickSort(int min, int max, double a[]);
double getMedian(double array[], int len);
double getAverage(double array[], int len);
double getStandardDeviation(double array[], int len, double average);

#endif	/* CMHEAD_H */

