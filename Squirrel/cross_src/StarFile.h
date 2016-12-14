/* 
 * File:   StarFile.h
 * Author: xy
 *
 * Created on December 4, 2014, 9:15 AM
 */

#ifndef STARFILE_H
#define	STARFILE_H

#include <string>
#include <vector>

class CMStar;

class StarFile {
public:
    const char * fileName;
    CMStar *starList;
    long starNum;
    int matchedCount;
    int OTStarCount;
    std::vector<std::string> redisStrings;

public:
    StarFile();
    StarFile(const char * fileName);
    StarFile(const StarFile& orig);
    virtual ~StarFile();

    virtual void readStar();
    virtual void readStar(const char * fileName);
    void writeStar(char * outFile);

private:
};

#endif	/* STARFILE_H */

