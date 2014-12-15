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
## $Id: function.cpp,v 1.2 2012/04/16 08:11:15 cyxu Exp $
#======================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <arpa/inet.h>

#include "fitsio.h"
#include "libpq-fe.h"

#include "anwcs.h"
#include "wcs.h"
#include "fitsfile.h"


#include "function.h"


extern struct WorldCoor *GetFITSWCS(char *filename,
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

float getAngleFromGreatCircle(double dec, double errorRadius);

/*
 *算法说明：
 *1，按照天文坐标，赤经（0~360），赤纬（-90~+90），按每一度为单位将整个球面坐标分成360*180区域，每个区域的大小为赤经1度，赤纬1度。
 *2，建立一个大小为360*180的AREANODE结构体数组（后面称该数组为树干），按照赤经为行，赤纬为列的行列顺序进行存储，即180行，360列，
 **  每个AREANODE结构体中有一个指针，该指针指向该结构体所对应区域中的所有点所组成的链表（后面称该数组为树枝）。
 *3，从源文件中读取数据点，每个数据点对应一个SAMPLE结构体，将该数据点添加到对应的区域链表当中（树干中的对应树枝），
 **  区域链表（树枝）按平衡四叉树进行排列。
 **  例如：点（95.25988，61.11838），对应树干（共360*180个区域）中的第360*61+95个区域（赤经95，赤纬61），将该点添加到该区域的子链表
 **  （树枝）中，该子链表（树枝）按平衡四叉树进行排列。
 *4，从目标文件中读出需要匹配的数据点，先找到该点在树干中对应的区域，再在该区域的树枝中找到对应匹配的点。
 *5，将匹配成功的点对添加到结果链表中去。
 *6，将结果链表输出到文件。
 */

int showResult = 0; //0输出所有结果，1输出匹配的结果，2输出不匹配的结果
int printResult = 0; //0将匹配结果不输出到终端，1输出到终端
int showProcessInfo = 0; //0不输出处理过程信息，1输出
int fitsHDU = 3; //fitsHDU=3: read fits file from the 3rd hdu
int useCross = 0;
double areaBox = ERROR_GREAT_CIRCLE; //判断两颗星是一颗星的最大误差，默认为20角秒
double minZoneLength = ERROR_GREAT_CIRCLE; //3 times of areaBox
double searchRadius = ERROR_GREAT_CIRCLE; //search radius, great circle

/*database configure information*/
int dbConfigInCommandLine = 0; //use command line config databaseinfo or use file
char *configFile = NULL;
char *host = NULL;
char *port = NULL;
char *dbname = NULL;
char *user = NULL;
char *password = NULL;
char *options = NULL;
char *tty = NULL;

/*table information*/
char *catfile_table = NULL;
char *match_table = NULL;
char *ot_table = NULL;
char *ot_flux_table = NULL;

double airmass = 0.0;
double jd = 0.0;

//for sphere coordiante's partition
long raMini;
long decMini;
long raMaxi;
long decMaxi;

float raMinf;
float decMinf;
float raMaxf;
float decMaxf;

int absDecMin; //in north or south, the max is different,
int absDecMax;

int decNode; //number of subarea in dec
int raNode; //number of subarea in ra
int zoneLength; //arc second, length of subarea's width or height
double factor; //=3600/zoneLength

float *raRadiusIndex = NULL;

//for plane coordiante's partition
int areaWidth = 0;
int areaHeight = 0;
float zoneInterval = 0;
int planeZoneX = 0;
int planeZoneY = 0;
float planeErrorRedius = 0.0;

int colNum = 0;

//to filter the flus
double standardDeviation = 0.0;
int fluxRatioSDTimes = 0; //abs(ratioRatio - flusRatioAverage) > flusRatioSD * standardDeviation;
double fluxRatioAverage = 0.0;
double fluxRatioMedian = 0.0;


void initDbInfo() {

    configFile = (char*) malloc(256 * sizeof (char));
    sprintf(configFile, "%s", "database_config.txt");

    host = (char*) malloc(20 * sizeof (char));
    sprintf(host, "%s", "localhost");

    port = (char*) malloc(20 * sizeof (char));
    sprintf(port, "%s", "5432");

    dbname = (char*) malloc(40 * sizeof (char));
    sprintf(dbname, "%s", "svomdb");

    user = (char*) malloc(20 * sizeof (char));
    sprintf(user, "%s", "wanmeng");

    password = (char*) malloc(20 * sizeof (char));
    sprintf(password, "%s", "");

    options = (char*) malloc(20 * sizeof (char));
    sprintf(options, "%s", "");

    tty = (char*) malloc(20 * sizeof (char));
    sprintf(tty, "%s", "");

    catfile_table = (char*) malloc(40 * sizeof (char));
    sprintf(catfile_table, "%s", "catfile_id");

    match_table = (char*) malloc(40 * sizeof (char));
    sprintf(match_table, "%s", "crossmatch_id");

    ot_table = (char*) malloc(40 * sizeof (char));
    sprintf(ot_table, "%s", "ot_id");

    ot_flux_table = (char*) malloc(40 * sizeof (char));
    sprintf(ot_flux_table, "%s", "ot_flux_id");
}

void freeDbInfo() {

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

void getDatabaseInfo(char *configFile) {

    FILE *fp = NULL;
    char *line = NULL;
    if (strlen(configFile) == 0) {
        strcpy(configFile, "database_config.txt");
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

        if ((*line == '\n') || (strlen(line) == 0)) continue;

        tmpStr = strtok(line, "\n");
        tmpStr = strtok(tmpStr, delim);
        if ((tmpStr == NULL) || (strlen(tmpStr) == 0)) continue;
        if (strcmp(tmpStr, "host") == 0) {
            tmpStr = strtok(NULL, delim);
            if (tmpStr != NULL)
                strcpy(host, tmpStr);
            else
                strcpy(host, "localhost");
        } else if (strcmp(tmpStr, "port") == 0) {
            tmpStr = strtok(NULL, delim);
            if (tmpStr != NULL)
                strcpy(port, tmpStr);
            else
                strcpy(port, "5432");
        } else if (strcmp(tmpStr, "dbname") == 0) {
            tmpStr = strtok(NULL, delim);
            if (tmpStr != NULL)
                strcpy(dbname, tmpStr);
            else
                strcpy(dbname, "svomdb");
        } else if (strcmp(tmpStr, "user") == 0) {
            tmpStr = strtok(NULL, delim);
            if (tmpStr != NULL)
                strcpy(user, tmpStr);
            else
                strcpy(user, "wanmeng");
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
                strcpy(catfile_table, "catfile_id");
        } else if (strcmp(tmpStr, "match_table") == 0) {
            tmpStr = strtok(NULL, delim);
            if (tmpStr != NULL)
                strcpy(match_table, tmpStr);
            else
                strcpy(match_table, "crossmatch_id");
        } else if (strcmp(tmpStr, "ot_table") == 0) {
            tmpStr = strtok(NULL, delim);
            if (tmpStr != NULL)
                strcpy(ot_table, tmpStr);
            else
                strcpy(ot_table, "ot_id");
        } else if (strcmp(tmpStr, "ot_flux_table") == 0) {
            tmpStr = strtok(NULL, delim);
            if (tmpStr != NULL)
                strcpy(ot_flux_table, tmpStr);
            else
                strcpy(ot_flux_table, "ot_flux_id");
        }
    }
    free(buf);
    fclose(fp);
}

void printAreaInfo() {
    printf("min ra : %f\n", raMinf);
    printf("min dec: %f\n", decMinf);
    printf("max ra : %f\n", raMaxf);
    printf("max dec: %f\n", decMaxf);
    printf("zone subarea length: %d arc second\n", zoneLength);
    printf("subarea columns(ra): %d\n", raNode);
    printf("subarea rows(dec)  : %d\n", decNode);
}

//init search index radius for the sample's match. 
//though each point's search radius are the same, 
//but the search radius in ra direction is different. 
//we should convert great circle distance to ra angle.
//see function getAngleFromGreatCircle for more detail.

void initRaRadiusIndex() {

    if (decMini > 0 && decMaxi > 0) {
        absDecMin = decMini;
        absDecMax = decMaxi;
    } else if (decMini < 0 && decMaxi < 0) {
        absDecMin = abs(decMaxi);
        absDecMax = abs(decMini);
    } else { // if(decMini<0 && decMaxi>0)
        absDecMin = 0;
        if (abs(raMini) < abs(decMaxi)) {
            absDecMax = abs(decMaxi);
        } else {
            absDecMax = abs(decMini);
        }
    }

    long num = ceil((absDecMax - absDecMin) / searchRadius);
    raRadiusIndex = (float *) malloc(num * sizeof (long));
    long i = 0;
    float tmpDec = 0.0;
    for (i = 0; i < num; i++) {
        tmpDec = absDecMin + i*searchRadius;
        raRadiusIndex[i] = getAngleFromGreatCircle(tmpDec, searchRadius);
    }

    if (showProcessInfo) {
        printf("ra radius index length: %d\n", num);
    }
}

void getAreaBoundary(struct SAMPLE *head) {

    struct SAMPLE *listHead = head;
    struct SAMPLE *tmp = listHead->next; //一般第一个节点为头结点，不存储数据，第二个节点才是第一个数据点
    float raMin;
    float decMin;
    float raMax;
    float decMax;
    if (tmp != NULL) {
        raMin = tmp->alpha;
        raMax = tmp->alpha;
        decMin = tmp->delta;
        decMax = tmp->delta;
        tmp = tmp->next;
    }
    while (tmp) {
        if (tmp->alpha > raMax) {
            raMax = tmp->alpha;
        } else if (tmp->alpha < raMin) {
            raMin = tmp->alpha;
        }
        if (tmp->delta > decMax) {
            decMax = tmp->delta;
        } else if (tmp->delta < decMin) {
            decMin = tmp->delta;
        }
        tmp = tmp->next;
    }

    raMinf = raMin;
    decMinf = decMin;
    raMaxf = raMax;
    decMaxf = decMax;

    decMini = floor(decMin - areaBox);
    decMaxi = ceil(decMax + areaBox);

    float maxDec = fabs(decMaxi) > fabs(decMini) ? fabs(decMaxi) : fabs(decMini);
    float areaBoxForRa = getAngleFromGreatCircle(maxDec, areaBox);
    raMini = floor(raMin - areaBoxForRa);
    raMaxi = ceil(raMax + areaBoxForRa);
}

//get zone's width and hight, width=hight

void getZoneLength() {

    long totalNode = (INDEX_SIZE) / sizeof (struct AREANODE);
    float zoneLengthf = sqrt((decMaxi - decMini + 1)*(raMaxi - raMini + 1)*3600.0 * 3600.0 / totalNode);
    zoneLength = ceil(zoneLengthf);
    if (zoneLength < minZoneLength * 3600)
        zoneLength = ceil(minZoneLength * 3600);
    factor = 3600 / zoneLength;

    decNode = ceil((decMaxi - decMini + 1) * factor);
    raNode = ceil((raMaxi - raMini + 1) * factor);
}

/********************
 *功能：初始化分区信息
 *输入：
 *输出：
 */
struct AREANODE *initAreaNode(struct SAMPLE *point) {

    getAreaBoundary(point);
    getZoneLength();
    initRaRadiusIndex();

    int totalNode = decNode*raNode;
    struct AREANODE *areaTree = (struct AREANODE *) malloc(sizeof (struct AREANODE) *totalNode);

    int i = 0;
    for (i = 0; i < totalNode; i++) {
        areaTree[i].subArea = NULL;
        areaTree[i].nodeNum = 0;
    }
    return areaTree;
}

/********************
 *功能：输出分支信息到文件
 *输入：
 *输出：
 */
void showArea(char *fName, struct AREANODE *area) {

    printf("show area tree\n");
    FILE *fp;
    if ((fp = fopen(fName, "w")) == NULL) {
        printf("open file error!!\n");
        return;
    }

    int i = 0;
    int j = 0;
    for (i = 0; i < 360 * 180; i++) {

        if (area[i].nodeNum > 0) {
            j++;
            fprintf(fp, "%8d%5d%5d%8d", i + 1, i % 360, i / 360, area[i].nodeNum);
            struct SAMPLE *tmp = area[i].subArea;
            while (tmp) {
                fprintf(fp, "%15.8f", tmp->alpha);
                tmp = tmp->next;
            }
            fprintf(fp, "\n");
        }
    }
    printf("total number of area is:%d\n", j);
    fclose(fp);
}

/********************
 *功能：获得点所属分区
 *输入：点
 *输出：分支索引
 */
long getPointBranch(struct SAMPLE *point) {

    float alpha = (point->alpha - raMini);
    float delta = (point->delta - decMini);
    int x = (int) (alpha * factor);
    int y = (int) (delta * factor);

    return y * raNode + x;
}

//dec:angle, errorRadius: angle

float getAngleFromGreatCircle(double dec, double errorRadius) {
    double rst = acos((cos(errorRadius * ANG_TO_RAD) - pow(sin(dec * ANG_TO_RAD), 2)) / pow(cos(dec * ANG_TO_RAD), 2));
    return rst*RAD_TO_ANG;
}

/********************
 *功能：获得点point的可能搜索分支
 *输入：点
 *输出：搜索分支个数number，分支索引数组branch
 */
long * getPointSearchBranch(struct SAMPLE *point, long *number) {

    int height, width;

    float alpha = point->alpha;
    float delta = point->delta;

    float up = delta + searchRadius; //on north, up > down
    float down = delta - searchRadius; //on south, down > up
    float maxDec = 0.0;
    if (up > 0.0 && down > 0.0) {
        maxDec = up;
    } else if (up < 0.0 && down < 0.0) {
        maxDec = fabs(down);
    } else {
        if (fabs(up) > fabs(down))
            maxDec = fabs(up);
        else
            maxDec = fabs(down);
    }
    /*
        float raRadius = getAngleFromGreatCircle(maxDec, searchRadius);
     */
    int tIndex = ceil((maxDec - absDecMin) / searchRadius);
    float raRadius = raRadiusIndex[tIndex];

    float left = alpha - raRadius;
    float right = alpha + raRadius;

    //-zoneLength/3600
    if (up > 90.0) {
        up = 90.0;
        left = raMinf;
        right = raMaxf;
    } else if (down < -90.0) {
        down = -90.0;
        left = raMinf;
        right = raMaxf;
    }

    int indexUp = (up - decMini) * factor;
    int indexDown = (down - decMini) * factor;
    int indexLeft = (left - raMini) * factor;
    int indexRight = (right - raMini) * factor;

    if (indexUp >= decNode) indexUp = decNode - 1;
    if (indexDown < 0) indexDown = 0;
    if (indexRight >= raNode) indexRight = raNode - 1;
    if (indexLeft < 0) indexLeft = 0;

    height = abs(indexUp - indexDown) + 1;
    width = indexRight - indexLeft + 1;
    *number = height*width;
    long *branch = (long *) malloc(*number * sizeof (long));

    int i, j;
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            branch[i * width + j] = (indexDown + i) * raNode + indexLeft + j;
        }
    }

    return branch;
}

/********************
 *功能：将点添加point到分支branch
 *输入：
 *输出：
 */
void addPointToBranchSort(struct SAMPLE *point, struct AREANODE *branch) {

    point->next = NULL;
    struct SAMPLE *tmp = branch->subArea;
    if (tmp == NULL) {
        branch->subArea = point;
    } else {
        /**************************************/
        /*该部分是否考虑用二叉树或四叉树等其他优化查找的算法来实现*/
        /*当前按alpha的值从小到大排列*/
        /**/
        if (point->alpha < tmp->alpha) {
            branch->subArea = point;
            point->next = tmp;
        } else {
            struct SAMPLE *before = tmp;
            tmp = before->next;
            while ((tmp) && (point->alpha >= tmp->alpha)) { //当tmp的next为空时，tmp的下一个就是point的位置
                before = tmp;
                tmp = before->next;
            }
            before->next = point;
            point->next = tmp;
        }
    }
    branch->nodeNum = branch->nodeNum + 1;
}

void addPointToBranchNotSort(struct SAMPLE *point, struct AREANODE *branch) {

    point->next = NULL;
    struct SAMPLE *tmp = branch->subArea;
    if (tmp == NULL) {
        branch->subArea = point;
    } else {
        struct SAMPLE *before = tmp;
        tmp = before->next;
        before->next = point;
        point->next = tmp;
    }
    branch->nodeNum = branch->nodeNum + 1;
}

/********************
 *功能：将数据链表listHead添加到区域索引areaTree
 *输入：
 *输出：添加的总节点个数
 */
long addDataToTree(struct SAMPLE *head, struct AREANODE *areaTree) {

    long start, end;
    start = clock();

    struct SAMPLE *listHead = head;
    struct SAMPLE *tmp = listHead->next; //一般第一个节点为头结点，不存储数据，第二个节点才是第一个数据点
    long branch = 0;
    long i = 0;
    while (tmp) {
        listHead->next = tmp->next; //把tmp点从数据表中移除
        branch = getPointBranch(tmp); //获得tmp所属的树枝位置
        addPointToBranchNotSort(tmp, areaTree + branch); //把tmp点加入到树干的对应树枝中
        tmp = listHead->next; //取下一个点
        i++;
    }
    end = clock();
    if (showProcessInfo) {
        printf("totle point in index: %d\n", i);
        printf("time of init index is: %fs\n", (end - start)*1.0 / ONESECOND);
    }
    return i;
}

/********************
 *功能：返回总数据区域areaTree中有数据的区域的个数
 *输入：
 *输出：总数据区域中有数据的区域的个数
 */
int getTreeNodeNum(struct AREANODE *areaTree) {

    long num = 0;
    int i = 0;
    for (i = 0; i < 360 * 180; i++) {
        if (areaTree[i].nodeNum > 0) {
            num++;
        }
    }
    return num;
}

/********************
 *功能：将数值从角度转换为弧度
 *输入：角度
 *输出：弧度
 */
double angToRad(double angle) {
    return angle * ANG_TO_RAD;
}

/********************
 *功能：计算两个点的大圆距离
 *输入：两点
 *输出：距离
 */
double getGreatCircleDistance(struct SAMPLE *p1, struct SAMPLE *p2) {
    double rst = RAD_TO_ANG * acos(sin(ANG_TO_RAD * (p1->delta)) * sin(ANG_TO_RAD * (p2->delta)) +
            cos(ANG_TO_RAD * (p1->delta)) * cos(ANG_TO_RAD * (p2->delta)) * cos(ANG_TO_RAD * (fabs(p1->alpha - p2->alpha))));
    return rst;
}

/********************
 *功能：计算两个点得直线距离,p1 from raference catalog , p2 from sample catalog
 *输入：两点
 *输出：距离
 */
double getLineDistance(struct SAMPLE *p1, struct SAMPLE *p2) {
    double tmp = (pow(p1->pixx - p2->pixx1, 2) + pow(p1->pixy - p2->pixy1, 2));
    return sqrt(tmp);
}

/********************
 *功能：判断两个点是否匹配
 *输入：两点
 *输出：1匹配，0不匹配
 */
int isSimilarPoint(struct SAMPLE *p1, struct SAMPLE *p2) {
    double distance = 180.0 * acos(sin(angToRad(p1->delta)) * sin(angToRad(p2->delta)) +
            cos(angToRad(p1->delta)) * cos(angToRad(p2->delta)) * cos(angToRad(fabs(p1->alpha - p2->alpha)))) / PI;
    return distance < areaBox ? 1 : 0;
}

/********************
 *功能：在SAMPLE链表branch中寻找与point最匹配的点
 *输入：SAMPLE链表branch， POINTNOD点point
 *输出：误差error，目标掉goalPoint
 */
double searchSimilarPoint(struct SAMPLE *branch, struct SAMPLE *point, struct SAMPLE **goalPoint) {

    double error = areaBox;
    //double error = 1.0;

    /*branch按照alpha进行排序，可以匹配的区域（branch + - 0.016667）只是一个范围，
    进入这个范围之后，start=1，当出了这个范围时，说明后面的点已经不满足要求，直接break*/
    int start = 0;
    struct SAMPLE *tSample = branch;
    while (tSample) {

        //if((branch->alpha > minerror) && (branch->alpha < maxerror) ){
        /*
                if ((tSample->alpha + SUBAREA > point->alpha) && (tSample->alpha - SUBAREA < point->alpha)) {
         */
        start = 1;
        float distance = getGreatCircleDistance(tSample, point);
        //if(point->id == 9841 && tSample->id == 12569)
        //printf("diatance = %f\n", distance);
        //float distance = getLineDistance(branch, point);
        /**************************************/
        /*如果数据量大，上面建表时将采用二叉树等算法，这里采用对应的查找算法*/
        if (distance < error) {
            *goalPoint = tSample;
            error = distance;
            //break;		//现在是找到小于ERROR_GREAT_CIRCLE的点就结束循环，可以继续循环下去，找distance最小的点，有没有必要？
        }
        /*
                } else if (start == 1) { //当start==1时，说明point点已经从进入区域（branch + - 0.016667）后，又出来了
                    break;
                }
         */
        tSample = tSample->next;
    }
    return error;
}

/********************
 *功能：将sample dataB与reference areaTree表进行匹配
 *输入：sample dataB，reference areaTree
 *输出：匹配链表MATCHLIST
 */
void matchPoints(struct AREANODE *areaTree, struct SAMPLE *dataB) {

    long start, end;
    start = clock();
    /**/
    double error = areaBox;
    double minError = areaBox;
    //double error = 1.0;
    //double minError = 1.0;

    struct SAMPLE *samplePoint = NULL;
    struct SAMPLE *tmpPoint = NULL;
    struct SAMPLE *minPoint = NULL;
    samplePoint = dataB->next;
    long *branchIndex = NULL;

    int i = 0;
    int j = 0;
    int outData = 0;
    while (samplePoint) {
        if (samplePoint->alpha > raMaxi || samplePoint->alpha < raMini || samplePoint->delta > decMaxi || samplePoint->delta < decMini) {
            samplePoint->reference = NULL;
            samplePoint->crossid = -1;
            samplePoint->error = 100;
            samplePoint = samplePoint->next;
            outData++;
            continue;
        }

        long numArea = 0;
        branchIndex = getPointSearchBranch(samplePoint, &numArea);

        //error = minError = 1.0;
        error = minError = areaBox;

        minPoint = NULL;
        tmpPoint = NULL;

        for (i = 0; i < numArea; i++) {
            struct SAMPLE *branch = areaTree[branchIndex[i]].subArea;
            error = searchSimilarPoint(branch, samplePoint, &tmpPoint);
            if (minError > error && tmpPoint != NULL) {
                minError = error;
                minPoint = tmpPoint;
            }
        }
        if (minPoint) {
            samplePoint->reference = minPoint;
            samplePoint->crossid = minPoint->id;
            samplePoint->error = minError;
        } else {
            samplePoint->reference = NULL;
            samplePoint->crossid = -1;
            samplePoint->error = 100;
        }
        samplePoint = samplePoint->next;
        j++;
    }

    end = clock();
    if (showProcessInfo) {
        printf("time of cross match is: %fs\n", (end - start)*1.0 / ONESECOND);
        printf("out data: %d\n", outData);
    }

}

void getFitsColumn(char *fileName, char *columnName, int typeSize) {

}

/*cfitsio error output*/
void printerror(int status) {
    /*****************************************************/
    /* Print out cfitsio error messages and exit program */
    /*****************************************************/
    if (status) {
        fits_report_error(stderr, status); /* print error report */
        exit(status); /* terminate the program, returning error status */
    }
    return;
}

struct SAMPLE *getReferenceFromFits(char *fileName, int *lineNum) {

    long start, end;
    start = clock();

    fitsfile *fptr; /* pointer to the FITS file, defined in fitsio.h */
    int status, hdunum, hdutype, nfound, anynull, ii;
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

    status = 0;
    hdunum = fitsHDU;

    if (fits_open_file(&fptr, fileName, READONLY, &status)) {
        printf("Open file :%s error!\n", fileName);
        printerror(status);
        return NULL;
    }
    /* move to the HDU */
    if (fits_movabs_hdu(fptr, fitsHDU, &hdutype, &status)) {
        printf("fits movabs hdu error!\n");
        printerror(status);
        return NULL;
    }

    /* read the NAXIS1 and NAXIS2 keyword to get table size */
    if (fits_read_keys_lng(fptr, "NAXIS", 1, 2, naxes, &nfound, &status)) {
        printerror(status);
        return NULL;
    }

    if (lineNum != NULL)
        *lineNum = naxes[1];

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

    //for (ii = 0; ii < 3; ii++)      /* allocate space for the column labels */
    //ttype[ii] = (char *) malloc(FLEN_VALUE);  /* max label length = 69 */


    if (hdutype == ASCII_TBL && showProcessInfo)
        printf("\nReading ASCII table in HDU %d:\n", hdunum);
    else if (hdutype == BINARY_TBL && showProcessInfo)
        printf("\nReading binary table in HDU %d:\n", hdunum);
    else if (showProcessInfo) {
        printf("Error: this HDU is not an ASCII or binary table\n");
        printerror(status);
        return NULL;
    }

    /* read the column names from the TTYPEn keywords */
    //fits_read_keys_str(fptr, "TTYPE", 1, 3, ttype, &nfound, &status);

    //printf(" Row  %10s %10s %10s\n", ttype[0], ttype[1], ttype[2]);

    frow = 1;
    felem = 1;
    nelem = naxes[1];
    strcpy(strnull, " ");
    longnull = 0;
    floatnull = 0.;

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

    if (fits_close_file(fptr, &status))
        printerror(status);

    struct SAMPLE *sample = (struct SAMPLE *) malloc((nelem + 1) * sizeof (struct SAMPLE));
    /*
        for(int i=0; i<nelem; i++){
            sample[i].id = id[i];
            sample[i].alpha = ra[i];
            sample[i].delta = dec[i];
        }
     */
    sample[0].id = -1;
    sample[0].alpha = 0.0;
    sample[0].delta = 0.0;
    sample[0].pixx = 0.0;
    sample[0].pixy = 0.0;
    sample[0].mag = 0.0;
    sample[0].mage = 0.0;
    sample[0].thetaimage = 0.0;
    sample[0].flags = 0.0;
    sample[0].ellipticity = 0.0;
    sample[0].classstar = 0.0;
    sample[0].background = 0.0;
    sample[0].fwhm = 0.0;
    sample[0].vignet = 0.0;
    sample[0].next = &sample[1];
    sample[0].reference = NULL;
    sample[0].error = 100.0;

    int i = 0;
    for (i = 0; i < nelem; i++) {
        sample[i + 1].id = id[i];
        sample[i + 1].alpha = ra[i];
        sample[i + 1].delta = dec[i];
        sample[i + 1].pixx = pixx[i];
        sample[i + 1].pixy = pixy[i];
        sample[i + 1].mag = mag[i];
        sample[i + 1].mage = mage[i];
        sample[i + 1].thetaimage = thetaimage[i];
        ;
        sample[i + 1].flags = flags[i];
        sample[i + 1].ellipticity = ellipticity[i];
        sample[i + 1].classstar = classstar[i];
        sample[i + 1].background = background[i];
        sample[i + 1].fwhm = fwhm[i];
        sample[i + 1].vignet = vignet[i];
        sample[i + 1].next = &sample[i + 2];
        sample[i + 1].reference = NULL;
        sample[i + 1].error = 100.0;
    }
    sample[nelem].id = id[nelem - 1];
    sample[nelem].alpha = ra[nelem - 1];
    sample[nelem].delta = dec[nelem - 1];
    sample[nelem].pixx = pixx[nelem - 1];
    sample[nelem].pixy = pixy[nelem - 1];
    sample[nelem].mag = mag[nelem - 1];
    sample[nelem].mage = mage[nelem - 1];
    sample[nelem].thetaimage = thetaimage[nelem - 1];
    sample[nelem].flags = flags[nelem - 1];
    sample[nelem].ellipticity = ellipticity[nelem - 1];
    sample[nelem].classstar = classstar[nelem - 1];
    sample[nelem].background = background[nelem - 1];
    sample[nelem].fwhm = fwhm[nelem - 1];
    sample[nelem].vignet = vignet[nelem - 1];
    sample[nelem].next = NULL;
    sample[nelem].reference = NULL;
    sample[nelem].error = 100.0;

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
    /*
        for (ii = 0; ii < 3; ii++)   
            free( ttype[ii] );
     */
    end = clock();
    if (showProcessInfo) {
        printf("read lines: %d\n", nelem);
        printf("time of read file %s is: %fs\n", fileName, (end - start)*1.0 / ONESECOND);
    }
    return sample;
}

struct SAMPLE *getSampleFromFits(char *fileName, int *lineNum) {

    long start, end;
    start = clock();

    fitsfile *fptr; /* pointer to the FITS file, defined in fitsio.h */
    int status, hdunum, hdutype, nfound, anynull, ii;
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
        return NULL;
    }
    /* move to the HDU */
    if (fits_movabs_hdu(fptr, fitsHDU, &hdutype, &status)) {
        printf("fits movabs hdu error!\n");
        printerror(status);
        return NULL;
    }

    /* read the NAXIS1 and NAXIS2 keyword to get table size */
    if (fits_read_keys_lng(fptr, "NAXIS", 1, 2, naxes, &nfound, &status)) {
        printerror(status);
        return NULL;
    }

    if (lineNum != NULL)
        *lineNum = naxes[1];

    fits_get_num_cols(fptr, &colNum, &status);
    //printf("nCols=%d\n",nCols);
    //printf("row=%d, clo=%d",naxes[0],naxes[1]);

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


    if (hdutype == ASCII_TBL && showProcessInfo)
        printf("\nReading ASCII table in HDU %d:\n", hdunum);
    else if (hdutype == BINARY_TBL && showProcessInfo)
        printf("\nReading binary table in HDU %d:\n", hdunum);
    else if (showProcessInfo) {
        printf("Error: this HDU is not an ASCII or binary table\n");
        printerror(status);
        return NULL;
    }

    /* read the column names from the TTYPEn keywords */
    //fits_read_keys_str(fptr, "TTYPE", 1, 3, ttype, &nfound, &status);

    //printf(" Row  %10s %10s %10s\n", ttype[0], ttype[1], ttype[2]);

    frow = 1;
    felem = 1;
    nelem = naxes[1];
    strcpy(strnull, " ");
    longnull = 0;
    floatnull = 0.;

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

    struct SAMPLE *sample = (struct SAMPLE *) malloc((nelem + 1) * sizeof (struct SAMPLE));
    /*
        for(int i=0; i<nelem; i++){
            sample[i].id = id[i];
            sample[i].alpha = ra[i];
            sample[i].delta = dec[i];
        }
     */
    sample[0].id = -1;
    sample[0].alpha = 0.0;
    sample[0].delta = 0.0;
    sample[0].pixx = 0.0;
    sample[0].pixy = 0.0;
    sample[0].mag = 0.0;
    sample[0].mage = 0.0;
    sample[0].thetaimage = 0.0;
    sample[0].flags = 0.0;
    sample[0].ellipticity = 0.0;
    sample[0].classstar = 0.0;
    sample[0].background = 0.0;
    sample[0].fwhm = 0.0;
    sample[0].vignet = 0.0;
    sample[0].pixx1 = 0.0;
    sample[0].pixy1 = 0.0;
    sample[0].next = &sample[1];
    sample[0].reference = NULL;
    sample[0].error = 100.0;

    int i = 0;
    for (i = 0; i < nelem; i++) {
        sample[i + 1].id = id[i];
        sample[i + 1].alpha = ra[i];
        sample[i + 1].delta = dec[i];
        sample[i + 1].pixx = pixx[i];
        sample[i + 1].pixy = pixy[i];
        sample[i + 1].mag = mag[i];
        sample[i + 1].mage = mage[i];
        sample[i + 1].thetaimage = thetaimage[i];
        ;
        sample[i + 1].flags = flags[i];
        sample[i + 1].ellipticity = ellipticity[i];
        sample[i + 1].classstar = classstar[i];
        sample[i + 1].background = background[i];
        sample[i + 1].fwhm = fwhm[i];
        sample[i + 1].vignet = vignet[i];
        if (colNum == 16) {
            sample[i + 1].pixx1 = pixx1[i];
            sample[i + 1].pixy1 = pixy1[i];
        } else {
            sample[i + 1].pixx1 = pixx[i];
            sample[i + 1].pixy1 = pixy[i];
        }
        sample[i + 1].next = &sample[i + 2];
        sample[i + 1].reference = NULL;
        sample[i + 1].error = 100.0;
    }
    sample[nelem].id = id[nelem - 1];
    sample[nelem].alpha = ra[nelem - 1];
    sample[nelem].delta = dec[nelem - 1];
    sample[nelem].pixx = pixx[nelem - 1];
    sample[nelem].pixy = pixy[nelem - 1];
    sample[nelem].mag = mag[nelem - 1];
    sample[nelem].mage = mage[nelem - 1];
    sample[nelem].thetaimage = thetaimage[nelem - 1];
    sample[nelem].flags = flags[nelem - 1];
    sample[nelem].ellipticity = ellipticity[nelem - 1];
    sample[nelem].classstar = classstar[nelem - 1];
    sample[nelem].background = background[nelem - 1];
    sample[nelem].fwhm = fwhm[nelem - 1];
    sample[nelem].vignet = vignet[nelem - 1];

    if (colNum == 16) {
        sample[nelem].pixx1 = pixx1[nelem - 1];
        sample[nelem].pixy1 = pixy1[nelem - 1];
    } else {
        sample[nelem].pixx1 = pixx[nelem - 1];
        sample[nelem].pixy1 = pixy[nelem - 1];
    }
    sample[nelem].next = NULL;
    sample[nelem].reference = NULL;
    sample[nelem].error = 100.0;

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
    free(pixx1);
    free(pixy1);
    /*
        for (ii = 0; ii < 3; ii++)   
            free( ttype[ii] );
     */
    end = clock();
    if (showProcessInfo) {
        printf("read lines: %d\n", nelem);
        printf("time of read file %s is: %fs\n", fileName, (end - start)*1.0 / ONESECOND);
    }
    return sample;
}

void quickSort(int min, int max, double a[]) {
    double key = a[min];
    int i = min;
    int j = max;
    //float temp;
    if (min >= max)
        return;
    while (i < j) {

        while ((i < j) && (key <= a[j])) {
            j--;
        }
        if (key > a[j]) {
            a[i] = a[j];
            a[j] = key;
            i++;
        }

        while ((i < j) && (key >= a[i])) {
            i++;
        }
        if (key < a[i]) {
            a[j] = a[i];
            a[i] = key;
            j--;
        }

    }
    quickSort(min, i - 1, a);
    quickSort(i + 1, max, a);
}

double getMedian(double array[], int len) {

    double median = 0.0;
    if (len % 2 == 0) {
        median = (array[len / 2 - 1] + array[len / 2]) / 2;
    } else {
        median = array[len / 2];
    }
    return median;
}

double getAverage(double array[], int len) {

    if (array == NULL) return 0;

    double total = 0.0;
    int i = 0;
    for (i = 0; i < len; i++)
        total += array[i];

    return total / len;
}

double getStandardDeviation(double array[], int len, double average) {
    int i;
    double total = 0.0;
    double tmp;
    for (i = 0; i < len; i++) {
        tmp = array[i] - average;
        total += tmp * tmp;
    }
    double msd = sqrt(total / len);
    return msd;
}

float getMagDiff(struct SAMPLE *sample) {

    if (sample == NULL) return 0;

    int tMatched = 0;
    int totalSample = 0;
    struct SAMPLE *tSample = sample->next;
    while (tSample) {
        if ((tSample->reference != NULL) && (tSample->error < areaBox))
        {
            if(tSample->mage < 0.05)
                tMatched++;
            totalSample++;
        }
        tSample = tSample->next;
    }

    double* magDiffs = (double*) malloc(tMatched * sizeof (double));
    double* samplesArray = (double*) malloc(totalSample * sizeof (double));
    tSample = sample->next;
    int i = 0, j = 0;
    while (tSample) {
        if ((tSample->reference != NULL) && (tSample->error < areaBox)) {
            tSample->fluxRatio = pow10(-0.4 * (tSample->reference->mag - tSample->mag));
            samplesArray[j++] = tSample->fluxRatio;
            if((tSample->mage < 0.05))
            {
                magDiffs[i++] = tSample->fluxRatio;
            }
        }
        tSample = tSample->next;
    }
    
    fluxRatioAverage = getAverage(samplesArray, totalSample);
    standardDeviation = getStandardDeviation(samplesArray, totalSample, fluxRatioAverage);

    quickSort(0, tMatched - 1, magDiffs);
    double median = getMedian(magDiffs, tMatched);
    //fluxRatioMedian = getMedian(samplesArray, totalSample);
    fluxRatioMedian = median;
    free(magDiffs);
    free(samplesArray);
    float magDiff = -2.5 * log10(median);
    return magDiff;
}

void fluxNorm(struct SAMPLE *sample, float magDiff) {
    if (sample == NULL) return;

    struct SAMPLE *tSample = sample->next;
    while (tSample) {
        //if ((tSample->reference!=NULL)&&(tSample->error<=areaBox)) {
        tSample->magnorm = tSample->mag + magDiff;
        //}
        tSample = tSample->next;
    }
}


//PQprepare not success

void writeToDBPre(struct SAMPLE *sample) {

    if (sample == NULL) return;
    //fprintf(fp, "%10s%15s%15s%10s%15s%15s%15s","ida","alpha","delta","idb","alpha","delta","error\n");

    long start, end;
    start = clock();

    char *host = "localhost";
    char *port = "5432";
    char *options = NULL;
    char *tty = NULL;
    char *dbname = "svomdb";
    char *user = "wanmeng";
    char *pwd = NULL;
    PGconn *conn;
    PGresult *pgrst;

    //1.connect db
    conn = PQsetdbLogin(host, port, options, tty, dbname, user, pwd);
    if (PQstatus(conn) == CONNECTION_BAD) {
        fprintf(stderr, "connect db failed! %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        return;
    }

    union {
        float f;
        unsigned int i;
    } swap;

    int32_t a1;
    int32_t a2;
    int32_t a3;
    int32_t a4;
    int32_t a5;
    int32_t a6;
    int nParams = 6;
    char *values[6] = {0};
    int lengths[6] = {sizeof (a1), sizeof (a1), sizeof (a1), sizeof (a1), sizeof (a1), sizeof (a1)};
    int binary[6] = {1, 1, 1, 1, 1, 1};
    //int binary[6]  = {0,0,0,0,0,0};
    int one = -1;



    char *sql = "insert into test_xy(id_s,ra_s,dec_s,id_r,ra_r,dec_r) \
				values($1::int4,$2::float4,$3::float4,$4::int4,$5::float4,$6::float4)";
    pgrst = PQprepare(conn, "insert", sql, nParams, NULL);

    struct SAMPLE *tSample = sample->next;
    int i = 0;

    while (tSample) {
        if (showResult == 0) {
            a1 = htonl(tSample->id);
            swap.f = tSample->alpha;
            a2 = htonl(swap.i);
            swap.f = tSample->delta;
            a3 = htonl(swap.i);
            if (tSample->reference != NULL) {
                a4 = htonl(tSample->reference->id);
                swap.f = tSample->reference->alpha;
                a5 = htonl(swap.i);
                swap.f = tSample->reference->delta;
                a6 = htonl(swap.i);
            } else {
                a4 = htonl(-1);
                swap.f = -1.0;
                a5 = htonl(swap.i);
                swap.f = -1.0;
                a6 = htonl(swap.i);
            }
        } else if ((showResult == 1) && (tSample->reference != NULL)) {
            a1 = htonl(tSample->id);
            swap.f = tSample->alpha;
            a2 = htonl(swap.i);
            swap.f = tSample->delta;
            a3 = htonl(swap.i);

            a4 = htonl(tSample->reference->id);
            swap.f = tSample->reference->alpha;
            a5 = htonl(swap.i);
            swap.f = tSample->reference->delta;
            a6 = htonl(swap.i);
        } else if ((showResult == 2) && (tSample->reference == NULL)) {
            a1 = htonl(tSample->id);
            swap.f = tSample->alpha;
            a2 = htonl(swap.i);
            swap.f = tSample->delta;
            a3 = htonl(swap.i);

            a4 = htonl(-1);
            swap.f = -1.0;
            a5 = htonl(swap.i);
            swap.f = -1.0;
            a6 = htonl(swap.i);
        } else {
            tSample = tSample->next;
            continue;
        }

        values[0] = (char*) &a1;
        values[1] = (char*) &a2;
        values[2] = (char*) &a3;
        values[3] = (char*) &a4;
        values[4] = (char*) &a5;
        values[5] = (char*) &a6;
        PQexecPrepared(conn, "insert", nParams, values, lengths, binary, 0);

        //if (i > 5) break;
        i++;
        tSample = tSample->next;
    }

    PQclear(pgrst);
    PQfinish(conn);

    end = clock();
    if (showProcessInfo) {
        printf("write row:%d\n", i);
        printf("time of write DB is: %fs\n", (end - start)*1.0 / ONESECOND);
    }
}

/********************
 *char copy success
 *input: star array points, file name, file type(reference file: fileType=true; sample file: fileType=false)
 *output:
 */
void writeToDBWithChar(struct SAMPLE *points, char *fileName, int fileType) {

    if (points == NULL) return;

    long start, end;
    start = clock();

    char *host = "localhost";
    char *port = "5432";
    char *options = NULL;
    char *tty = NULL;
    char *dbname = "svomdb";
    char *user = "wanmeng";
    char *pwd = NULL;
    PGconn *conn = NULL;
    PGresult *pgrst = NULL;


    conn = PQsetdbLogin(host, port, options, tty, dbname, user, pwd);
    if (PQstatus(conn) == CONNECTION_BAD) {
        fprintf(stderr, "connect db failed! %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        return;
    }

    float medMagDiff = 0.0;
    if (!fileType) {
        medMagDiff = getMagDiff(points);
        fluxNorm(points, medMagDiff);
    }

    char *sqlBuf = (char*) malloc(256 * sizeof (char));
    //get fileName's catfile_id catid
    sprintf(sqlBuf, "select catid from catfile_id where catfile='%s'", fileName);
    pgrst = PQexec(conn, sqlBuf);
    if (PQresultStatus(pgrst) != PGRES_TUPLES_OK) {
        PQclear(pgrst);
        printf("query catfile_id failure!\n");
        printf("sql = %s\n", sqlBuf);
        return;
    }

    long catid = 0;
    //if fileName not in table catfile_id, add, and get catid again
    if (PQgetisnull(pgrst, 0, 0)) {
        PQclear(pgrst);
        char *fileTypeStr = "true";
        if (!fileType)
            fileTypeStr = "false";
        sprintf(sqlBuf, "insert into catfile_id(catfile,magdiff,is_ref)values('%s',%f,%s)", fileName, medMagDiff, fileTypeStr);
        pgrst = PQexec(conn, sqlBuf);
        if (PQresultStatus(pgrst) != PGRES_COMMAND_OK) {
            PQclear(pgrst);
            printf("insert catfile_id failure!\n");
            printf("sql = %s\n", sqlBuf);
            return;
        } else {
            printf("insert catfile_id success!\n");
        }

        sprintf(sqlBuf, "select catid from catfile_id where catfile='%s'", fileName);
        pgrst = PQexec(conn, sqlBuf);
        if (PQresultStatus(pgrst) != PGRES_TUPLES_OK) {
            PQclear(pgrst);
            printf("query catfile_id failure!\n");
            printf("sql = %s\n", sqlBuf);
            return;
        }
    } else {
        printf("%s already in table catfile_id! please check!\n", fileName);
        return;
    }
    catid = atoi(PQgetvalue(pgrst, 0, 0));
    //printf("catid = %d\n", catid);
    PQclear(pgrst);

    char *buf = (char*) malloc(1024 * sizeof (char));

    int i = 0;
    struct SAMPLE *tSample = points->next;

    //total 19 column, not include cid 
    char *sql1 = "COPY crossmatch_id(\
            starid,crossid,catid,magnorm,ra,dec,background,classstar,ellipticity,flags,mag,mage,fwhm,pixx,pixy,thetaimage,vignet,magcalib,magcalibe \
            )FROM STDIN";
    pgrst = PQexec(conn, sql1);
    if (PQresultStatus(pgrst) == PGRES_COPY_IN) {
        if (fileType) { //insert reference file
            while (tSample) {
                sprintf(buf, "%d\t%d\t%d\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\n",
                        tSample->id, tSample->crossid, catid,
                        -1.0,
                        tSample->alpha,
                        tSample->delta,
                        tSample->background,
                        tSample->classstar,
                        tSample->ellipticity,
                        tSample->flags,
                        tSample->mag,
                        tSample->mage,
                        tSample->fwhm,
                        tSample->pixx,
                        tSample->pixy,
                        tSample->thetaimage,
                        tSample->vignet,
                        tSample->magcalib,
                        tSample->magcalibe,
                        0.0, 0.0);
                //printf("%s\n",buf);
                int copydatares = PQputCopyData(conn, buf, strlen(buf));
                //if(i >2) break;
                i++;
                tSample = tSample->next;
            }
        } else { //insert sample file matched
            while (tSample) {
                if (tSample->reference != NULL) {
                    sprintf(buf, "%d\t%d\t%d\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\n",
                            tSample->id, tSample->crossid, catid,
                            tSample->magnorm,
                            tSample->alpha,
                            tSample->delta,
                            tSample->background,
                            tSample->classstar,
                            tSample->ellipticity,
                            tSample->flags,
                            tSample->mag,
                            tSample->mage,
                            tSample->fwhm,
                            tSample->pixx,
                            tSample->pixy,
                            tSample->thetaimage,
                            tSample->vignet,
                            tSample->magcalib,
                            tSample->magcalibe,
                            0.0, 0.0);
                } else {
                    tSample = tSample->next;
                    continue;
                }
                //printf("%s\n",buf);
                int copydatares = PQputCopyData(conn, buf, strlen(buf));
                //if(i > 2) break;
                i++;
                tSample = tSample->next;
            }
        }
        PQputCopyEnd(conn, NULL);
    } else {
        printf("can not copy in!\n");
    }
    PQclear(pgrst);
    //	PQfinish(conn);
    //    
    //	conn = PQsetdbLogin(host,port,options,tty,dbname,user,pwd);
    //	if(PQstatus(conn) == CONNECTION_BAD)
    //	{
    //		fprintf(stderr,"connect db failed! %s\n",PQerrorMessage(conn));
    //		PQfinish(conn);
    //		return;
    //	}
    if (!fileType) { //insert sample file unmatched
        char *sql2 = "COPY OT_id(\
                starid,otid,catid,magnorm,ra,dec,background,classstar,ellipticity,flags,mag,mage,fwhm,pixx,pixy,thetaimage,vignet,\
                magcalib,magcalibe \
                )FROM STDIN";
        pgrst = PQexec(conn, sql2);
        if (PQresultStatus(pgrst) == PGRES_COPY_IN) {
            tSample = points->next;
            while (tSample) {
                if (tSample->reference == NULL) {
                    sprintf(buf, "%d\t%d\t%d\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\n",
                            tSample->id, tSample->crossid, catid,
                            tSample->magnorm,
                            tSample->alpha,
                            tSample->delta,
                            tSample->background,
                            tSample->classstar,
                            tSample->ellipticity,
                            tSample->flags,
                            tSample->mag,
                            tSample->mage,
                            tSample->fwhm,
                            tSample->pixx,
                            tSample->pixy,
                            tSample->thetaimage,
                            tSample->vignet,
                            tSample->magcalib,
                            tSample->magcalibe,
                            0.0, 0.0);
                } else {
                    tSample = tSample->next;
                    continue;
                }
                //printf("%s\n",buf);
                int copydatares = PQputCopyData(conn, buf, strlen(buf));
                //if(i > 2) break;
                i++;
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
        printf("write row:%d\n", i);
        printf("time of write DB is: %fs\n", (end - start)*1.0 / ONESECOND);
    }
}

void addInt16(struct strBuffer* strBuf, unsigned short i) {

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

void addInt32(struct strBuffer* strBuf, int i) {

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

void addInt64(struct strBuffer* strBuf, long int li) {

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

void addFloat4(struct strBuffer* strBuf, float f) {

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

void addFloat8(struct strBuffer* strBuf, double d) {

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

void writeToDBBinary(struct SAMPLE *points, char *fileName, int fileType) {

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

void writeToDBBinary2(struct SAMPLE *points, char *fileName, int fileType) {

    if (points == NULL) return;

    clock_t startTime, endTime;

    PGconn *conn = NULL;
    PGresult *pgrst = NULL;


    conn = PQsetdbLogin(host, port, options, tty, dbname, user, password);
    if (PQstatus(conn) == CONNECTION_BAD) {
        fprintf(stderr, "connect db failed! %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        return;
    }

    float magDiff = 1.0;


    if (!fileType) {
        char *cleanSql = (char*) malloc(256 * sizeof (char));
        sprintf(cleanSql, "delete from %s where catid=(select catid from %s where catfile='%s');", match_table, catfile_table, fileName); //delete match
        //printf("%s\n",cleanSql);
        PQexec(conn, cleanSql);
        sprintf(cleanSql, "delete from %s where catid=(select catid from %s where catfile='%s');", ot_table, catfile_table, fileName); //delete ot
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
        printf("%s already in table %s! please check!\n", catfile_table, fileName);
        return;
    }
    catid = atoi(PQgetvalue(pgrst, 0, 0));
    //printf("catid = %d\n", catid);
    PQclear(pgrst);

    char *buf = (char*) malloc(1024 * sizeof (char));

    printf("start timer\n");
    startTime = clock();
    unsigned short fieldNum = 19;
    char *sendHeader = "PGCOPY\n\377\r\n\0";
    struct strBuffer strBuf;
    strBuf.data = buf;
    strBuf.len = MAX_BUFFER;

    int i = 0;
    int j = 0;
    struct SAMPLE *tSample = points->next;
    //total 19 column, not include cid 
    //char *sql1 = "COPY crossmatch_id(\
            starid,crossid,catid,magnorm,ra,dec,background,classstar,ellipticity,flags,mag,mage,fwhm,pixx,pixy,thetaimage,vignet,\
            magcalib,magcalibe \
            )FROM STDIN WITH BINARY";
    sprintf(sqlBuf, "COPY %s(\
            starid,crossid,catid,magnorm,ra,dec,background,classstar,ellipticity,flags,mag,mage,fwhm,pixx,pixy,theataimage,vignet,\
            magcalib,magcalibe \
            )FROM STDIN WITH BINARY", match_table);
    //printf("%s\n",sqlBuf);
    pgrst = PQexec(conn, sqlBuf);
    if (PQresultStatus(pgrst) == PGRES_COPY_IN) {
        strBuf.cursor = 0;
        memcpy(strBuf.data, sendHeader, 11);
        strBuf.cursor += 11;
        unsigned int zero = '\0';
        memcpy(strBuf.data + strBuf.cursor, (char*) &zero, 4);
        strBuf.cursor += 4;
        memcpy(strBuf.data + strBuf.cursor, (char*) &zero, 4);
        strBuf.cursor += 4;

        for (i = 1; i <= 220000; i++) {
            addInt16(&strBuf, fieldNum);
            addInt64(&strBuf, i);
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
            int copydatares = PQputCopyData(conn, strBuf.data, strBuf.cursor);

            strBuf.cursor = 0;
        }
        PQputCopyEnd(conn, NULL);
    } else {
        printf("can not copy in!\n");
    }
    PQclear(pgrst);


    PQfinish(conn);
    free(buf);
    free(sqlBuf);

    endTime = clock();
    printf("end timer\n");
    if (showProcessInfo) {
        printf("write table %s %d row\n", match_table, i);
        if (!fileType) {
            printf("write table %s %d row\n", ot_table, j);
        }
        printf("time2 of write DB is: %fms\n\n", (double) ((endTime - startTime)*1000.0 / (double) CLOCKS_PER_SEC));
    }
}

//binary copy,  success

void writeToDBTest(struct SAMPLE * sample) {

    if (sample == NULL) return;
    //fprintf(fp, "%10s%15s%15s%10s%15s%15s%15s","ida","alpha","delta","idb","alpha","delta","error\n");

    long start, end;
    start = clock();

    char *host = "localhost";
    char *port = "5432";
    char *options = NULL;
    char *tty = NULL;
    char *dbname = "svomdb";
    char *user = "wanmeng";
    char *pwd = NULL;
    PGconn *conn;
    PGresult *pgrst;

    //1.connect db
    conn = PQsetdbLogin(host, port, options, tty, dbname, user, pwd);
    if (PQstatus(conn) == CONNECTION_BAD) {
        fprintf(stderr, "connect db failed! %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        return;
    }

    char *sql = "COPY test_xy(id_s,ra_s,dec_s,id_r,ra_r,dec_r) FROM STDIN WITH BINARY"; //
    char *buf = (char*) malloc(MAX_BUFFER * sizeof (char));
    int i = 0;
    char *sendHeader = "PGCOPY\n\377\r\n\0";
    pgrst = PQexec(conn, sql);
    if (PQresultStatus(pgrst) == PGRES_COPY_IN) {
        struct SAMPLE *tSample = sample->next;
        unsigned short fieldNum = 6;
        struct strBuffer strBuf;
        strBuf.data = buf;
        strBuf.len = MAX_BUFFER;
        strBuf.cursor = 0;
        memcpy(strBuf.data, sendHeader, 11);
        strBuf.cursor += 11;
        unsigned int zero = '\0';
        memcpy(strBuf.data + strBuf.cursor, (char*) &zero, 4);
        strBuf.cursor += 4;
        memcpy(strBuf.data + strBuf.cursor, (char*) &zero, 4);
        strBuf.cursor += 4;

        while (tSample) {
            if (showResult == 0) {
                addInt16(&strBuf, fieldNum);
                addInt32(&strBuf, tSample->id);
                addFloat8(&strBuf, tSample->alpha);
                addFloat8(&strBuf, tSample->delta);
                if (tSample->reference != NULL) {
                    addInt32(&strBuf, tSample->reference->id);
                    addFloat8(&strBuf, tSample->reference->alpha);
                    addFloat8(&strBuf, tSample->reference->delta);
                } else {
                    addInt32(&strBuf, -1);
                    addFloat8(&strBuf, -1.0);
                    addFloat8(&strBuf, -1.0);
                }
            } else if ((showResult == 1) && (tSample->reference != NULL)) {
                addInt16(&strBuf, fieldNum);
                addInt32(&strBuf, tSample->id);
                addFloat8(&strBuf, tSample->alpha);
                addFloat8(&strBuf, tSample->delta);
                addInt32(&strBuf, tSample->reference->id);
                addFloat8(&strBuf, tSample->reference->alpha);
                addFloat8(&strBuf, tSample->reference->delta);
            } else if ((showResult == 2) && (tSample->reference == NULL)) {
                addInt16(&strBuf, fieldNum);
                addInt32(&strBuf, tSample->id);
                addFloat8(&strBuf, tSample->alpha);
                addFloat8(&strBuf, tSample->delta);
                addInt32(&strBuf, -1);
                addFloat8(&strBuf, -1.0);
                addFloat8(&strBuf, -1.0);
            } else {
                tSample = tSample->next;
                continue;
            }
            //printf("%x\n",strBuf.data);
            int copydatares = PQputCopyData(conn, strBuf.data, strBuf.cursor);
            if (i > 1) break;
            i++;
            tSample = tSample->next;
            strBuf.cursor = 0;
        }
        PQputCopyEnd(conn, NULL);
    }

    PQclear(pgrst);
    PQfinish(conn);
    free(buf);
    end = clock();
    if (showProcessInfo) {
        printf("write row:%d\n", i);
        printf("time of write DB is: %fs\n", (end - start)*1.0 / ONESECOND);
    }
}

void freeList(void *link) {
    long next;
    if (link == NULL) return;
    while (link != NULL) {
        next = *((long*) link);
        free(link);
        link = (void*) next;
    }
    return;
}

void getAreaMap(long *areaTreeRefIdx, long *areaTreeSampIdx, long **map, long refNum, long sampNum) {

    long *tmpMap = (long *) malloc(sampNum * sizeof (long));
    int idx = 0;
    int stage = 0;
    int total = 0;
    int i = 0;
    int j = 0;
    for (i = 0; i < sampNum; i++) {
        stage = 0;
        for (j = idx; j < refNum; j++) {
            if (areaTreeSampIdx[i] == areaTreeRefIdx[j]) {
                tmpMap[i] = j;
                idx = j + 1;
                total++;
                stage = 1;
                break;
            }
        }
        if (stage == 0) {
            tmpMap[i] = -1;
            printf("can not map idx: %l\n", areaTreeSampIdx[i]);
        }
    }
    printf("total map: %d\n", total);
    *map = tmpMap;
}

int isInAreaBox(int x, int y, struct AREABOX ab) {
    int flag = 0;
    if ((x > ab.left) && (x < ab.right) && (y > ab.down) && (y < ab.top))
        flag = 1;
    return flag;
}

void wcsJudge(char *fileName, int wcsext, struct SAMPLE * data) {

    long start, end;
    start = clock();

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


    /*
        printf("row=%d, clo=%d\n",naxes[0],naxes[1]);
        printf("sizeof(sip_t):%d\n",sizeof(sip_t));    //anwcs_t
     */


    //for (ii = 0; ii < 3; ii++)      /* allocate space for the column labels */
    //ttype[ii] = (char *) malloc(FLEN_VALUE);  /* max label length = 69 */


    if (hdutype == ASCII_TBL && showProcessInfo)
        printf("\nReading ASCII table in HDU %d:\n", hdunum);
    else if (hdutype == BINARY_TBL && showProcessInfo)
        printf("\nReading binary table in HDU %d:\n", hdunum);
    else if (showProcessInfo) {
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

        struct AREABOX ab;
        ab.left = 0;
        ab.down = 0;

        hgeti4(wcsHeader, "NAXIS1", &(ab.top));
        hgeti4(wcsHeader, "NAXIS2", &(ab.right));

        struct SAMPLE *tSample = data->next;
        while (tSample) {
            /*
                        if(tSample->reference==NULL){
             */
            if (tSample->error > areaBox) {
                wcsc2pix(wcs, tSample->alpha, tSample->delta, coorsys, &x, &y, &offscale);

                if (isInAreaBox(x, y, ab)) {
                    tSample->inarea = 1;
                } else {
                    tSample->inarea = -1;
                    outArea++;
                    /*
                                        printf("id =%d, ra=%f, dec=%f, x=%f, y=%f\n",tSample->id, tSample->alpha, tSample->delta, x, y);
                     */
                }
                //if(notMatched<5)
                // printf("id =%d, ra=%f, dec=%f, x=%f, y=%f\n",tSample->id, tSample->alpha, tSample->delta, x, y);
                notMatched++;
            }
            tSample = tSample->next;
        }
    }


    if (fits_close_file(fptr, &status))
        printerror(status);

    free(wcsHeader);
    wcsfree(wcs);

    end = clock();
    if (showProcessInfo) {
        printf("\nnot matched: %d, out area: %d\n", notMatched, outArea);
        printf("time of judge is: %fs\n", (end - start)*1.0 / ONESECOND);
    }

}

double getFieldFromWCSFloat(char *fileName, int wcsext, char *field) {


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


    if (hdutype == ASCII_TBL && showProcessInfo)
        printf("\nReading ASCII table in HDU %d:\n", hdunum);
    else if (hdutype == BINARY_TBL && showProcessInfo)
        printf("\nReading binary table in HDU %d:\n", hdunum);
    else if (showProcessInfo) {
        printf("Error: this HDU is not an ASCII or binary table\n");
        printerror(status);
        return;
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

/*
 *judge a star whether in the reference rejection box or not
 */
void wcsJudge2(char *fileName, int wcsext, struct SAMPLE * data) {

    if (data == NULL) return;
    anwcs_t* wcs = anwcs_open(fileName, wcsext);

    if (!wcs) {
        printf("Failed to read WCS file \"%s\", extension %i\n", fileName, wcsext);
        return;
    }

    double x, y;
    struct AREABOX ab;
    ab.left = 0;
    ab.down = 0;
    ab.right = 1401;
    ab.top = 1401;

    struct SAMPLE *tSample = data->next;
    while (tSample) {
        if (tSample->reference == NULL) {
            if (anwcs_radec2pixelxy(wcs, tSample->alpha, tSample->delta, &x, &y)) {
                tSample->inarea = -1;
                continue;
            }
            if (isInAreaBox(x, y, ab)) {
                tSample->inarea = 1;
            } else {
                tSample->inarea = -1;
            }
            //printf("id =%d, ra=%f, dec=%f, x=%f, y=%f\n",tSample->delta, tSample->alpha, tSample->delta, x, y);
        }
        tSample = tSample->next;
    }
    printf("\n\n");
}

void writeNotMatchedToFile(char *fileName, struct SAMPLE * sample) {

    if (showProcessInfo) {
        printf("start write file!\n");
    }
    long start, end;
    start = clock();

    if (sample == NULL) return;
    FILE *fp = NULL;

    char *nameBuf = (char*) malloc(256);
    char *tmpBuf = (char*) malloc(256);

    char *pos1, *pos2;
    pos1 = strrchr(fileName, '\\');
    if (pos1 == NULL) {
        pos1 = strrchr(fileName, '/');
        if (pos1 == NULL) {
            pos1 = fileName;
        } else {
            pos1 = pos1 + 1;
        }
    } else {
        pos1 = pos1 + 1;
    }
    pos2 = strrchr(fileName, '.');
    if (pos2 == NULL)
        pos2 = fileName + strlen(fileName);
    strncpy(nameBuf, pos1, pos2 - pos1);
    nameBuf[pos2 - pos1] = '\0';
    /*
        printf("\nnew file name= %s\n", nameBuf);
     */
    sprintf(tmpBuf, "log_%s.txt", nameBuf);
    /*
        printf("new file name= %s\n\n", tmpBuf);
     */


    if ((fp = fopen(tmpBuf, "w")) == NULL) {
        printf("open file %s error!\n", tmpBuf);
        return;
    }
    fprintf(fp, "%10s%15s%15s%10s%15s%15s%15s", "ida", "alpha", "delta", "idb", "alpha", "delta", "error\n");

    struct SAMPLE *tSample = sample->next;
    long i = 0;
    while (tSample) {
        if (tSample->reference == NULL) {
            fprintf(fp, "%10d %15.7f %15.7f %10d %15.7f %15.7f %15.7f\n",
                    tSample->id, tSample->alpha, tSample->delta,
                    -1, -1.0, -1.0, -1.0);
            i++;
        } else if (tSample->error >= areaBox) {
            fprintf(fp, "%10d %15.7f %15.7f %10d %15.7f %15.7f %15.7f\n",
                    tSample->id, tSample->alpha, tSample->delta,
                    tSample->reference->id, tSample->reference->alpha, tSample->reference->delta,
                    tSample->error);
            i++;
        }
        /*
                if(tSample->error<areaBox){
                    fprintf(fp, "%10d %15.7f %15.7f %10d %15.7f %15.7f %15.7f\n", 
                        tSample->id, tSample->alpha, tSample->delta, 
                        tSample->reference->id, tSample->reference->alpha, tSample->reference->delta, 
                        tSample->error);
                    i++;
                }
         */
        tSample = tSample->next;
    }
    fclose(fp);

    end = clock();
    if (showProcessInfo) {
        printf("total unmatched stars:%d\n", i);
        printf("the unmatched stars was written to file %s use time: %fs\n", tmpBuf, (end - start)*1.0 / ONESECOND);
    }
    free(tmpBuf);
    free(nameBuf);
}

void writeToTXT(char *fileName, struct SAMPLE * sample) {

    if (showProcessInfo) {
        printf("start write file!\n");
    }

    if (sample == NULL) return;
    FILE *fp = NULL;

    char *nameBuf = (char*) malloc(256);
    char *tmpBuf = (char*) malloc(256);

    char *pos1, *pos2;
    pos1 = strrchr(fileName, '\\');
    if (pos1 == NULL) {
        pos1 = strrchr(fileName, '/');
        if (pos1 == NULL) {
            pos1 = fileName;
        } else {
            pos1 = pos1 + 1;
        }
    } else {
        pos1 = pos1 + 1;
    }
    pos2 = strrchr(fileName, '.');
    if (pos2 == NULL)
        pos2 = fileName + strlen(fileName);
    strncpy(nameBuf, pos1, pos2 - pos1);
    nameBuf[pos2 - pos1] = '\0';
    /*
        printf("\nnew file name= %s\n", nameBuf);
     */
    sprintf(tmpBuf, "%s.txt", nameBuf);
    /*
        printf("new file name= %s\n\n", tmpBuf);
     */


    if ((fp = fopen(tmpBuf, "w")) == NULL) {
        printf("open file %s error!\n", tmpBuf);
        return;
    }
    fprintf(fp, "#%10s%15s%15s", "ida", "alpha", "delta\n");

    struct SAMPLE *tSample = sample->next;
    long i = 0;
    while (tSample) {
        fprintf(fp, "%10d %15.7f %15.7f\n", tSample->id, tSample->alpha, tSample->delta);
        i++;
        tSample = tSample->next;
    }
    fclose(fp);

    if (showProcessInfo) {
        printf("write file %s total stars:%d\n", tmpBuf, i);
    }
    free(tmpBuf);
    free(nameBuf);
}

struct SAMPLE * matchAll(struct SAMPLE *dataA, struct SAMPLE * dataB) {

    struct SAMPLE *ta = NULL;
    struct SAMPLE *tb = dataB->next;

    struct SAMPLE *tb2, *dataB2Head;
    struct SAMPLE *dataB2 = (struct SAMPLE*) malloc(sizeof (struct SAMPLE));
    memcpy(dataB2, dataB, sizeof (struct SAMPLE));
    dataB2->next = NULL;
    dataB2Head = dataB2;

    long i = 0;
    while (tb) {
        ta = dataA->next;
        tb2 = (struct SAMPLE*) malloc(sizeof (struct SAMPLE));
        memcpy(tb2, tb, sizeof (struct SAMPLE));
        tb2->error = 100.0;
        tb2->next = NULL;
        dataB2->next = tb2;
        dataB2 = tb2;
        while (ta) {
            float distance = getGreatCircleDistance(ta, tb2);
            if (tb2->error > distance) {
                tb2->error = distance;
                tb2->reference = ta;
            }
            ta = ta->next;
            i++;
        }
        tb = tb->next;
    }
    return dataB2Head;
}

void writeDifferentToFile(char *fileName, struct SAMPLE *crossRst, struct SAMPLE * zoneRst) {

    if (showProcessInfo) {
        printf("start write different to file!\n");
    }
    long start, end;
    start = clock();

    if (crossRst == NULL || zoneRst == NULL) return;
    FILE *fp = NULL;

    char *nameBuf = (char*) malloc(256);
    char *tmpBuf = (char*) malloc(256);


    char *pos1, *pos2;
    pos1 = strrchr(fileName, '\\');
    if (pos1 == NULL) {
        pos1 = strrchr(fileName, '/');
        if (pos1 == NULL) {
            pos1 = fileName;
        } else {
            pos1 = pos1 + 1;
        }
    } else {
        pos1 = pos1 + 1;
    }
    pos2 = strrchr(fileName, '.');
    if (pos2 == NULL)
        pos2 = fileName + strlen(fileName);
    strncpy(nameBuf, pos1, pos2 - pos1);
    nameBuf[pos2 - pos1] = '\0';
    /*
        printf("\nnew file name= %s\n", nameBuf);
     */
    sprintf(tmpBuf, "cross_different_zone_%s.txt", nameBuf);
    /*
        printf("new file name= %s\n\n", tmpBuf);
     */


    if ((fp = fopen(tmpBuf, "w")) == NULL) {
        printf("open file %s error!\n", tmpBuf);
        return;
    }
    fprintf(fp, "#%10s%15s%15s%10s%15s%15s%15s", "ida", "alpha", "delta", "idb", "alpha", "delta", "error\n");

    struct SAMPLE *tCrossRst = crossRst->next;
    struct SAMPLE *tZoneRst = zoneRst->next;
    long crossMatched = 0;
    long zoneMatched = 0;
    while (tCrossRst) {
        if (tZoneRst->id == tCrossRst->id) {
            if (tCrossRst->error < areaBox) {
                if (tZoneRst->error >= areaBox) { //find the cross method matched but the zone method not matched, and output to file
                    fprintf(fp, "%10d %15.7f %15.7f %10d %15.7f %15.7f %15.7f\n",
                            tCrossRst->id, tCrossRst->alpha, tCrossRst->delta,
                            tCrossRst->reference->id, tCrossRst->reference->alpha, tCrossRst->reference->delta,
                            tCrossRst->error);
                    zoneMatched++;
                }
                crossMatched++;
            }
        } else {
            printf("copy sample data error, the sample data1(cross result) is different sampel data2(zone resutlt)! please check!\n");
        }
        tCrossRst = tCrossRst->next;
        tZoneRst = tZoneRst->next;
    }
    fclose(fp);

    end = clock();
    printf("cross method matched starts: %d\n", crossMatched);
    printf("zone method omitted stars: %d \n", zoneMatched);
    printf("the omitted star was written to file %s use time: %fs\n", tmpBuf, (end - start)*1.0 / ONESECOND);

    free(tmpBuf);
    free(nameBuf);
}

void writeDifferentToFileForPlane(char *fileName, struct SAMPLE *crossRst, struct SAMPLE * zoneRst) {

    if (showProcessInfo) {
        printf("start write different to file!\n");
    }
    long start, end;
    start = clock();

    if (crossRst == NULL || zoneRst == NULL) return;
    FILE *fp = NULL;

    char *nameBuf = (char*) malloc(256);
    char *tmpBuf = (char*) malloc(256);


    char *pos1, *pos2;
    pos1 = strrchr(fileName, '\\');
    if (pos1 == NULL) {
        pos1 = strrchr(fileName, '/');
        if (pos1 == NULL) {
            pos1 = fileName;
        } else {
            pos1 = pos1 + 1;
        }
    } else {
        pos1 = pos1 + 1;
    }
    pos2 = strrchr(fileName, '.');
    if (pos2 == NULL)
        pos2 = fileName + strlen(fileName);
    strncpy(nameBuf, pos1, pos2 - pos1);
    nameBuf[pos2 - pos1] = '\0';
    /*
        printf("\nnew file name= %s\n", nameBuf);
     */
    sprintf(tmpBuf, "cross_different_zone_%s.txt", nameBuf);
    /*
        printf("new file name= %s\n\n", tmpBuf);
     */


    if ((fp = fopen(tmpBuf, "w")) == NULL) {
        printf("open file %s error!\n", tmpBuf);
        return;
    }
    fprintf(fp, "#%10s%15s%15s%10s%15s%15s%15s", "smpid", "x", "y", "refid", "x", "y", "error\n");

    struct SAMPLE *tCrossRst = crossRst->next;
    struct SAMPLE *tZoneRst = zoneRst->next;
    long crossMatched = 0;
    long zoneMatched = 0;
    while (tCrossRst) {
        if (tZoneRst->id == tCrossRst->id) {
            if (tCrossRst->error < areaBox) {
                if (tZoneRst->error >= areaBox) { //find the cross method matched but the zone method not matched, and output to file
                    fprintf(fp, "%10d %15.7f %15.7f %10d %15.7f %15.7f %15.7f\n",
                            tCrossRst->id, tCrossRst->pixx1, tCrossRst->pixy1,
                            tCrossRst->reference->id, tCrossRst->reference->pixx, tCrossRst->reference->pixy,
                            tCrossRst->error);
                    zoneMatched++;
                }
                crossMatched++;
            }
        } else {
            printf("copy sample data error, the sample data1(cross result) is different sampel data2(zone resutlt)! please check!\n");
        }
        tCrossRst = tCrossRst->next;
        tZoneRst = tZoneRst->next;
    }
    fclose(fp);

    end = clock();
    printf("cross method matched starts: %d\n", crossMatched);
    printf("zone method omitted stars: %d \n", zoneMatched);
    printf("the omitted star was written to file %s use time: %fs\n", tmpBuf, (end - start)*1.0 / ONESECOND);

    free(tmpBuf);
    free(nameBuf);
}

void writeToTerminal(struct SAMPLE * sample) {

    if (sample == NULL) return;
    printf("%10s%15s%15s%10s%15s%15s%15s", "ida", "alpha", "delta", "idb", "alpha", "delta", "error\n");

    struct SAMPLE *tSample = sample->next;
    int i = 0;
    while (tSample) {
        if (showResult == 0) {//output all
            printf("%d %f %f ", tSample->id, tSample->alpha, tSample->delta);
            if (tSample->reference != NULL)
                printf("%d %f %f %.8f\n", tSample->reference->id, tSample->reference->alpha, tSample->reference->delta, tSample->error);
            else
                printf("-1 -1 -1 -1\n");
        } else if ((showResult == 1) && (tSample->reference != NULL)) { //output matched
            printf("%d %f %f %d %f %f %.8f\n",
                    tSample->id, tSample->alpha, tSample->delta,
                    tSample->reference->id, tSample->reference->alpha, tSample->reference->delta,
                    tSample->error);
        } else if ((showResult == 2) && (tSample->reference == NULL)) { //output unmatched
            printf("%d %f %f -1 -1 -1 -1\n", tSample->id, tSample->alpha, tSample->delta);
        }
        i++;
        tSample = tSample->next;
    }
    printf("total write lines: %d\n", i);
}


//get the value of name in src
//the src must follow the form "name1=value1,name2=value2..."
//return 0 not find name, return 1, find name;

int getStrValue(char *src, char *name, char *value) {

    char *str1 = NULL;
    char *str2 = NULL;
    char *start = NULL;
    str1 = strstr(src, name);
    if (str1 == 0)
        return 0;
    start = str1 + strlen(name) + 1;
    str2 = strchr(str1, ',');
    if (str2 == 0)
        str2 = src + strlen(src);
    int len = str2 - start;
    if (len != 0) {
        strncpy(value, start, len); //there is a =, so need add 1
        value[len] = '\0';
    } else {
        strcpy(value, "");
    }
    return 1;
}

void mainSphere(char *inFile1, char *inFile2, char *outFile) {

    printf("starting corss match...\n");

    long start, end;
    start = clock();
    struct SAMPLE *dataB2;


    airmass = getFieldFromWCSFloat(inFile1, 2, "AIRMASS");
    jd = getFieldFromWCSFloat(inFile1, 2, "JD");

    struct SAMPLE *dataA = getReferenceFromFits(inFile1, NULL);
    //writeToTXT(inFile1, dataA);
    if (dataA == NULL) return;
    if (printResult == 0)
        writeToDBBinary(dataA, inFile1, 1);
    //struct SAMPLE    *dataB = getSample(inFile2,NULL);


    struct SAMPLE *dataB = getSampleFromFits(inFile2, NULL);
    //writeToTXT(inFile2, dataB);
    if (dataB == NULL) return;

    if (useCross == 1) {
        dataB2 = matchAll(dataA, dataB);
    }

    struct AREANODE *areaTree = initAreaNode(dataA);

    addDataToTree(dataA, areaTree);
    //showArea("data/showArea.txt", areaTree) ;
    matchPoints(areaTree, dataB);

    //    wcsJudge2("data/reference.fit", 0, dataB);
    wcsJudge(inFile1, 2, dataB);

    //reference true, sample false
    airmass = getFieldFromWCSFloat(inFile2, 2, "AIRMASS");
    jd = getFieldFromWCSFloat(inFile2, 2, "JD");
    if (printResult == 0)
        writeToDBBinary(dataB, inFile2, 0);
    else
        writeToTerminal(dataB);

    if (useCross == 1) {
        writeDifferentToFile(inFile2, dataB2, dataB);
        freeList(dataB2);
    }
    if (showProcessInfo) {
        writeNotMatchedToFile(inFile2, dataB);
    }
    end = clock();
    if (showProcessInfo) {
        printf("total time is: %fs\n", (end - start)*1.0 / ONESECOND);
        printAreaInfo();
    }
    printf("total time is: %fs\n", (end - start)*1.0 / ONESECOND);
    printf("corss match done!\n");

    free(areaTree);
    //freeList(dataA);
    free(dataA);
    //freeList(dataB);
    free(dataB);

    freeDbInfo();
}

struct AREANODE * planePartition(int maxNum) {

    float zoneLength = sqrt(areaWidth * areaHeight * 1.0 / maxNum);
    if (zoneLength < minZoneLength)
        zoneLength = minZoneLength;
    zoneInterval = zoneLength;
    planeZoneX = ceil(areaWidth * 1.0 / zoneInterval);
    planeZoneY = ceil(areaHeight * 1.0 / zoneInterval);

    int totalNode = planeZoneX*planeZoneY;
    struct AREANODE *areaTree = (struct AREANODE *) malloc(sizeof (struct AREANODE) *totalNode);

    int i = 0;
    for (i = 0; i < totalNode; i++) {
        areaTree[i].subArea = NULL;
        areaTree[i].nodeNum = 0;
    }
    if (showProcessInfo) {
        printf("\ntotle partition number: %d\n", totalNode);
        printf("zoneInterval=%f;planeZoneX=%d;planeZoneY=%d\n\n", zoneInterval, planeZoneX, planeZoneY);
    }

    return areaTree;
}

long getPointBranchPlane(struct SAMPLE * point) {

    int x = (int) (point->pixx / zoneInterval);
    int y = (int) (point->pixy / zoneInterval);

    return y * planeZoneX + x;
}

long * getPointSearchBranchPlane(struct SAMPLE *point, long *number) {

    int height, width;

    float x = point->pixx1;
    float y = point->pixy1;

    int up = (y + searchRadius) / zoneInterval;
    int down = (y - searchRadius) / zoneInterval;
    int right = (x + searchRadius) / zoneInterval;
    int left = (x - searchRadius) / zoneInterval;

    if (up > planeZoneY - 1) up = planeZoneY - 1;
    if (down < 0) down = 0;
    if (right > planeZoneX - 1) right = planeZoneX - 1;
    if (left < 0) left = 0;


    height = up - down + 1;
    width = right - left + 1;
    *number = height*width;
    long *branch = (long *) malloc(*number * sizeof (long));

    int i, j;
    int baseIndex = down * planeZoneX + left;
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            branch[i * width + j] = i * planeZoneX + j + baseIndex;
        }
    }

    return branch;
}

long addDataToTreePlane(struct SAMPLE *head, struct AREANODE * areaTree) {

    long start, end;
    start = clock();

    struct SAMPLE *listHead = head;
    struct SAMPLE *tmp = listHead->next; //一般第一个节点为头结点，不存储数据，第二个节点才是第一个数据点
    long branch = 0;
    long i = 0;
    while (tmp) {
        listHead->next = tmp->next; //把tmp点从数据表中移除
        branch = getPointBranchPlane(tmp); //获得tmp所属的树枝位置
        addPointToBranchNotSort(tmp, areaTree + branch); //把tmp点加入到树干的对应树枝中
        tmp = listHead->next; //取下一个点
        i++;
    }
    end = clock();
    if (showProcessInfo) {
        printf("totle point in index: %d\n", i);
        printf("time of init index is: %fs\n", (end - start)*1.0 / ONESECOND);
    }
    return i;
}

void showPartitionInfo(char *fName, struct AREANODE * area) {

    printf("show area tree\n");
    FILE *fp;
    if ((fp = fopen(fName, "w")) == NULL) {
        printf("open file error!!\n");
        return;
    }

    int i = 0;
    int j = 0;
    for (i = 0; i < planeZoneX * planeZoneY; i++) {

        if (area[i].nodeNum > 0) {
            j++;
            fprintf(fp, "%8d%5d%5d%8d", i + 1, i % planeZoneX, i / planeZoneX, area[i].nodeNum);
            struct SAMPLE *tmp = area[i].subArea;
            while (tmp) {
                fprintf(fp, "%15.8f%15.8f", tmp->pixx, tmp->pixy);
                tmp = tmp->next;
            }
            fprintf(fp, "\n");
        }
    }
    printf("total number of area is:%d\n", j);
    fclose(fp);
}

double searchSimilarPointPlane(struct SAMPLE *branch, struct SAMPLE *point, struct SAMPLE **goalPoint) {

    double error = areaBox;

    struct SAMPLE *tSample = branch;
    while (tSample) {
        float distance = getLineDistance(tSample, point);
        if (distance < error) {
            *goalPoint = tSample;
            error = distance;
        }
        tSample = tSample->next;
    }
    return error;
}

void matchPointsPlane(struct AREANODE *areaTree, struct SAMPLE * dataB) {

    long start, end;
    start = clock();

    double error = areaBox;
    double minError = areaBox;
    struct SAMPLE *samplePoint = NULL;
    struct SAMPLE *tmpPoint = NULL;
    struct SAMPLE *minPoint = NULL;
    samplePoint = dataB->next;
    long *branchIndex = NULL;

    int i = 0;
    int j = 0;
    int outData = 0;
    while (samplePoint) {
        if (samplePoint->pixx1 > areaWidth || samplePoint->pixx1 < 0 || samplePoint->pixy1 > areaHeight || samplePoint->pixy1 < 0) {
            samplePoint->reference = NULL;
            samplePoint->crossid = -1;
            samplePoint->error = areaBox;
            samplePoint = samplePoint->next;
            outData++;
            continue;
        }

        long numArea = 0;
        branchIndex = getPointSearchBranchPlane(samplePoint, &numArea);
        error = minError = areaBox;
        minPoint = NULL;
        tmpPoint = NULL;

        for (i = 0; i < numArea; i++) {
            struct SAMPLE *branch = areaTree[branchIndex[i]].subArea;
            error = searchSimilarPointPlane(branch, samplePoint, &tmpPoint);
            if (minError > error && tmpPoint != NULL) {
                minError = error;
                minPoint = tmpPoint;
            }
        }
        if (minPoint) {
            samplePoint->reference = minPoint;
            samplePoint->crossid = minPoint->id;
            samplePoint->error = minError;
        } else {
            samplePoint->reference = NULL;
            samplePoint->crossid = -1;
            samplePoint->error = areaBox;
        }
        samplePoint = samplePoint->next;
    }

    end = clock();
    if (showProcessInfo) {
        printf("time of cross match is: %fs\n", (end - start)*1.0 / ONESECOND);
        printf("out area number: %d\n", outData);
    }

}

void judgeInAreaPlane(struct SAMPLE * data) {

    struct AREABOX ab;
    ab.left = 0;
    ab.down = 0;
    ab.top = areaHeight;
    ab.right = areaWidth;

    int notMatched = 0;
    int outArea = 0;

    struct SAMPLE *tSample = data->next;
    while (tSample) {
        if (tSample->error >= areaBox) {
            if (isInAreaBox(tSample->pixx1, tSample->pixy1, ab)) {
                tSample->inarea = 1;
            } else {
                tSample->inarea = -1;
                outArea++;
            }
            notMatched++;
        }
        tSample = tSample->next;
    }
    if (showProcessInfo) {
        printf("\nnot matched: %d, out area: %d\n", notMatched, outArea);
    }
}

struct SAMPLE * matchAllForPlane(struct SAMPLE *dataA, struct SAMPLE * dataB) {

    struct SAMPLE *ta = NULL;
    struct SAMPLE *tb = dataB->next;

    struct SAMPLE *tb2, *dataB2Head;
    struct SAMPLE *dataB2 = (struct SAMPLE*) malloc(sizeof (struct SAMPLE));
    memcpy(dataB2, dataB, sizeof (struct SAMPLE));
    dataB2->next = NULL;
    dataB2Head = dataB2;

    long i = 0;
    while (tb) {
        ta = dataA->next;
        tb2 = (struct SAMPLE*) malloc(sizeof (struct SAMPLE));
        memcpy(tb2, tb, sizeof (struct SAMPLE));
        tb2->error = areaBox;
        tb2->next = NULL;
        dataB2->next = tb2;
        dataB2 = tb2;
        while (ta) {
            float distance = getLineDistance(ta, tb2);
            if (tb2->error > distance) {
                tb2->error = distance;
                tb2->reference = ta;
            }
            ta = ta->next;
            i++;
        }
        tb = tb->next;
    }
    return dataB2Head;
}

void mainPlane(char *inFile1, char *inFile2, char *outFile) {

    printf("starting corss match...\n");

    long start, end;
    start = clock();
    struct SAMPLE *dataB2;


    airmass = getFieldFromWCSFloat(inFile1, 2, "AIRMASS");
    jd = getFieldFromWCSFloat(inFile1, 2, "JD");

    int refNum = 0;
    int trgNum = 0;
    int maxNum = 0;

    struct SAMPLE *dataA = getReferenceFromFits(inFile1, &refNum);
    if (dataA == NULL) return;
    if (printResult == 0)
        writeToDBBinary(dataA, inFile1, 1);
    //struct SAMPLE    *dataB = getSample(inFile2,NULL);


    struct SAMPLE *dataB = getSampleFromFits(inFile2, &trgNum);
    if (dataB == NULL) return;

    if (useCross == 1) {
        dataB2 = matchAllForPlane(dataA, dataB);
    }

    maxNum = refNum > trgNum ? refNum : trgNum;

    struct AREANODE *areaTree = planePartition(maxNum);

    addDataToTreePlane(dataA, areaTree);
    //showPartitionInfo("planePartion.txt", areaTree);

    matchPointsPlane(areaTree, dataB);

    judgeInAreaPlane(dataB);

    //reference true, sample false
    airmass = getFieldFromWCSFloat(inFile2, 2, "AIRMASS");
    jd = getFieldFromWCSFloat(inFile2, 2, "JD");
    if (printResult == 0)
        writeToDBBinary(dataB, inFile2, 0);
    else
        writeToTerminal(dataB);

    if (useCross == 1) {
        writeDifferentToFileForPlane(inFile2, dataB2, dataB);
        freeList(dataB2);
    }
    if (showProcessInfo) {
        writeNotMatchedToFile(inFile2, dataB);
    }

    end = clock();
    if (showProcessInfo) {
        printf("total time is: %fs\n", (end - start)*1.0 / ONESECOND);
        //printAreaInfo();
    }
    printf("total time is: %fs\n", (end - start)*1.0 / ONESECOND);
    printf("corss match done!\n");

    free(areaTree);
    //freeList(dataA);
    free(dataA);
    //freeList(dataB);
    free(dataB);

    freeDbInfo();
}
