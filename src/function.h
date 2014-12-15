/* 
 * File:   function.h
 * Author: xy
 *
 * Created on 2014年12月13日, 下午6:04
 */

#ifndef FUNCTION_H
#define	FUNCTION_H

/*
######################################################################
## Time-stamp: 
## Filename:      $Name:  $       
## Version:       $Revision: 1.2 $ 
## Author:        Yang Xu <yxuctgu@gmail.com>
## Purpose:       cross match of astrometry.
## CVSAuthor:     $Author: cyxu $ 
## Note:          
#-                
## $Id: function.h,v 1.2 2012/04/16 08:11:15 cyxu Exp $
#======================================================
*/

#define VERSION "cross match 1.4"
#define TREE_NODE_LENGTH 64800		//the totle number of tree node (360*180)
#define AREA_WIDTH 360
#define AREA_HEIGHT 180

#define BLOCK_BASE 32

//#define PI 3.141592653
#define ERROR_GREAT_CIRCLE 0.005555555556			//(20.0/3600.0)=0.005555555556
#define	SUBAREA	0.05555555556			//(60.0/3600.0)=0.016666666667 this value must big enough, to insure all data all find.

#define LINE 1024
#define MAX_BUFFER 1024
#define ONESECOND CLOCKS_PER_SEC

#define ANG_TO_RAD 0.017453293
#define RAD_TO_ANG 57.295779513

#define INDEX_SIZE 1<<20

#define SPHERE_METHOD 1 
#define PLANE_METHOD 2


struct REFERENCE{
	struct REFERENCE *next;
	int id;
	float alpha;
	float delta;
};

struct SAMPLE{
	struct SAMPLE *next;
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
	struct SAMPLE *reference;	//sample are the same with reference
	float error;
    int inarea; //is sample in reference rejection area, in 1, not in -1, not judge 0
    float fluxRatio; //
};

struct MATCHLIST{
	struct MATCHLIST *next;
	struct REFERENCE *dataA;
	struct REFERENCE *dataB;
	float error;
};

struct AREABOX{
    int top;
    int left;
    int right;
    int down;
};

/*·ÖÇøœÚµã*/
struct AREANODE{
	//AREANODE *next;
	struct SAMPLE *subArea;
	long nodeNum;
};
//used by the COPY command of PostgreSQL, send buffer unit
struct BUFFERUNIT{
	int id_s;
	float alpha_s;
	float delta_s;
	int id_r;
	float alpha_r;
	float delta_r;
	//float error;
};

struct strBuffer{
    char *data;
    int len;
    int cursor;
};

//void initAreaNode(AREANODE *areaTree);
//void matchPoints(AREANODE *areaTree, SAMPLE *dataB);
//void writeToFile(char *fileName, SAMPLE *sample);
//void freeList(void *link);
void initDbInfo();
int getStrValue(char *src, char *name, char *value);
void mainSphere(char *inFile1, char *inFile2,char *outFile);
void mainPlane(char *inFile1, char *inFile2, char *outFile);
void test();

#endif	/* FUNCTION_H */

