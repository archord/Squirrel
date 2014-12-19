/* 
 * File:   StarFile.h
 * Author: xy
 *
 * Created on December 4, 2014, 9:15 AM
 */

#ifndef STARFILE_H
#define	STARFILE_H

#include <string.h>
#include "cmhead.h"

class StarFile {
public:
    char * fileName;
    CMStar *starList;
    long starNum;

public:
    StarFile();
    StarFile(char * fileName);
    StarFile(const StarFile& orig);
    virtual ~StarFile();

    virtual void readStar();
    virtual void readStar(char * fileName);
    void writeStar(char * outFile);

private:
};

#endif	/* STARFILE_H */

