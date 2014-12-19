/* 
 * File:   Partition.h
 * Author: xy
 *
 * Created on October 18, 2013, 8:47 AM
 */

#ifndef PARTITION_H
#define	PARTITION_H

#include "cmhead.h"
#include "StarFile.h"

class Partition {
protected:
    float maxx;
    float maxy;
    float minx;
    float miny;
    float fieldWidth; //星表视场的宽度
    float fieldHeight; //星表视场的高度

    float errRadius; //两颗星匹配的最小距离
    float searchRadius; //搜索匹配分区时的矩形搜索区域（边长为2*searchRadius）
    float minZoneLength; //最小分区长度 = 3*errRadius
    float zoneInterval; //实际分区长度
    float zoneIntervalRecp; //实际分区长度

    int zoneXnum; //分区在X方向上的个数
    int zoneYnum; //分区在Y方向上的个数
    int totalZone; //分区的总个数
    int totalStar; //星的总数
    CMZone *zoneArray; //分区数组

public:
    Partition();
    Partition(const Partition& orig);
    Partition(float errBox, float minZoneLen, float searchRds);
    virtual ~Partition();

    void partitonStarField(StarFile *starFile);
    void getMatchStar(CMStar *objStar);
    void printZoneDetail(char *fName);
    void freeZoneArray();

    void setSearchRadius(float searchRadius);
    float getSearchRadius() const;
    void setErrRadius(float errRadius);
    float getErrRadius() const;
    void setMinZoneLength(float minZoneLength);
    float getMinZoneLength() const;

protected:
    CMStar *searchSimilarStar(long zoneIdx, CMStar *star);
    long *getStarSearchZone(CMStar *star, long &sZoneNum);
    long getZoneIndex(CMStar * star);
    void getMinMaxXY(CMStar *starList);
    void addStarToZone(CMStar *star, long zoneIdx);
    void freeStarList(CMStar *starList);
};

#endif	/* PARTITION_H */

