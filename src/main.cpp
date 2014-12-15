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
## $Id: main.cpp,v 1.2 2012/04/16 08:11:15 cyxu Exp $
#======================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "function.h"

extern int showResult;		//0输出所有结果，1输出匹配的结果，2输出不匹配的结果
extern int printResult;		//0将匹配结果不输出到终端，1输出到终端
extern int showProcessInfo;	//0不输出处理过程信息，1输出
extern int fitsHDU;         //fitsHDU=3: read fits file from the 3rd hdu
extern int useCross;        //use cross method
extern double minZoneLength;//the min length of the zone's side
extern double searchRadius;
extern double areaBox;      //判断两颗星是一颗星的最大误差，默认为20角秒

extern int dbConfigInCommandLine;
extern char *configFile;
extern char *host;
extern char *port;
extern char *dbname;
extern char *user;
extern char *password;

extern char *catfile_table;
extern char *match_table;
extern char *ot_table;
extern char *ot_flux_table;

extern int areaWidth;
extern int areaHeight;
extern float planeErrorRedius;

extern int fluxRatioSDTimes; //factor of flux filter

void showHelp(){
	printf("crossmatch v1.2 (2012 Nov 15)\n");
	printf("usage: crossmatch [-method <plane> ] [-errorRadius <20>] [-width <3096>] [...]\n");
	printf("usage: crossmatch [-method <sphere>] [-errorRadius <0.00556>] [...]\n");
	printf("-method <sphere|plane>:     cross match method, using sphere coordinate or plane corrdinate\n");
	printf("-width <number>:            on plane corrdinate partition, the max value of X axis\n");
	printf("-height <number>:           on plane corrdinate partition, the max value of Y axis\n");
	printf("-errorRadius <number>:      the max error between two point, unit is degree, defalut 0.005556\n");
	printf("-searchRadius <number>:     the search area's radius, unit is degree, defalut equals errorRadius\n");
	printf("-minZoneLength <number>:    the min length of the zone's side, unit is degree, defalut equals errorRadius\n");
	//printf("                            default, the total zone number equals the reference catalog star numbers\n");
	printf("-fitsHDU <number>:          read fits file from the fitsHDU-th data area\n");
	printf("-ref <path>:                reference table path\n");
	printf("-sample <path>:             sample table path\n");
	printf("-output <path>:             output table path\n");
	//printf("\t-mode\n");
	//printf("\t\tgpu: executed by gpu\n");
	//printf("\t\tcpu: executed by cpu\n");
	printf("-dbConfigFile <fileName>:   file contain database config information.\n");
    printf("                            Notice: -dbInfo is prior to -dbConfigFile\n");
	printf("-dbInfo \"<name1=value1,name2=value2...>\": this option include the database configure information\n");
	printf("                            host=localhost, IP Address or host name\n");
	printf("                            port=5432, the port of PostgreSQL use\n");
	printf("                            dbname=svomdb, database name\n");
	printf("                            user=wanmeng, database user name\n");
	printf("                            password= ,database user password\n");
	printf("                            catfile_table=catfile_id \n");
	printf("                            match_talbe=crossmatch_id \n");
	printf("                            ot_table=ot_id \n");
	printf("                            ot_flux_table=ot_flux_id \n");
	printf("-show <all|matched|unmatched>:\n");
	printf("                            all:show all stars in sample table including matched and unmatched\n");
	printf("                            matched:show matched stars in sample table\n");
	printf("                            unmatched:show unmatched stars in sample table\n");
	printf("-terminal:                  print result to terminal\n");
	printf("-cross:                     compare zone method with cross method, find the zone method omitted stars, and output to file\n");
	printf("-processInfo:               print process information\n");
	printf("-fluxSDTimes <number>:      the times of flux SD, use to filter matched star with mag\n");
	printf("-h or -help:                show help\n");
	printf("-v or -version:             show version number\n");
    printf("example: \n");
    printf("\t1: crossmatch -method sphere -errorRadius 0.006(20 arcsec) -searchRadius 0.018 -fitsHDU 2 -ref reference.cat -sample sample.cat -output output.cat -processInfo\n");
    printf("\t2: crossmatch -method plane  -errorRadius 10 -searchRadius 30 -width 3096 -height 3096 -fitsHDU 2 -ref reference.cat -sample sample.cat -output output.cat -processInfo\n");
	//printf("Notes: default area box is 0.005556 degree, output all result, not print to terminal, not print process information, not compare the result with cross method\n");
}

void getDBInfo(char *param){
    int flag = 0;
    flag = getStrValue(param, "host", host);
    if(flag==0){
        printf("error:input parameter must include \"host\"!\n");
    }
    flag = getStrValue(param, "port", port);
    if(flag==0){
        printf("error:input parameter must include \"port\"!\n");
    }
    flag = getStrValue(param, "dbname", dbname);
    if(flag==0){
        printf("error:input parameter must include \"dbname\"!\n");
    }
    flag = getStrValue(param, "user", user);
    if(flag==0){
        printf("error:input parameter must include \"user\"!\n");
    }
    flag = getStrValue(param, "password", password);
    if(flag==0){
        printf("error:input parameter must include \"password\"!\n");
    }
    flag = getStrValue(param, "catfile_table", catfile_table);
    flag = getStrValue(param, "match_table", match_table);
    flag = getStrValue(param, "ot_table", ot_table);
    flag = getStrValue(param, "ot_flux_table", ot_flux_table);
}

int main(int argc, char** argv){

	char *inFile1;
	char *inFile2;
	char *outFile;
	int cpu = 1;
    int method = PLANE_METHOD;

	if(argc == 1){
		showHelp();
		return 0;
	}

    initDbInfo();
    
    int i = 0;
	for(i=1; i<argc; i++){
        if(strcmp(argv[i],"-method")==0){
			if(i+1>=argc || strlen(argv[i+1])==0){printf("-width must follow a stirng\n");return 0;}
			if(strcmp(argv[i+1],"sphere")==0){
				method = SPHERE_METHOD;
			}else{
				method = PLANE_METHOD;
			}
			i++;
		}else if(strcmp(argv[i],"-errorRadius")==0){
			if(i+1>=argc || strlen(argv[i+1])==0){printf("-errorRadius must follow a number\n");return 0;}
			areaBox = atof(argv[i+1]);   
            minZoneLength = areaBox;       
            searchRadius = areaBox;
			i++;
		}else if(strcmp(argv[i],"-searchRadius")==0){
			if(i+1>=argc || strlen(argv[i+1])==0){printf("-searchRadius must follow a number\n");return 0;}
			searchRadius = atof(argv[i+1]);
			i++;
		}else if(strcmp(argv[i],"-minZoneLength")==0){
			if(i+1>=argc || strlen(argv[i+1])==0){printf("-minZoneLength must follow a number\n");return 0;}
			minZoneLength = atof(argv[i+1]);
			i++;
		}else if(strcmp(argv[i],"-width")==0){
			if(i+1>=argc || strlen(argv[i+1])==0){printf("-width must follow a number\n");return 0;}
			areaWidth = atoi(argv[i+1]);
			i++;
		}else if(strcmp(argv[i],"-height")==0){
			if(i+1>=argc || strlen(argv[i+1])==0){printf("-height must follow a number\n");return 0;}
			areaHeight = atoi(argv[i+1]);
			i++;
		}else if(strcmp(argv[i],"-fitsHDU")==0){
			if(i+1>=argc || strlen(argv[i+1])==0){printf("-fitsHDU must follow a number\n");return 0;}
			fitsHDU = atoi(argv[i+1]);
			i++;
		}else if(strcmp(argv[i],"-fluxSDTimes")==0){
			if(i+1>=argc || strlen(argv[i+1])==0){printf("-fluxSDTimes must follow a number\n");return 0;}
			fluxRatioSDTimes = atoi(argv[i+1]);
			i++;
		}else if(strcmp(argv[i],"-cross")==0){
			useCross = 1;
		}else if(strcmp(argv[i],"-ref")==0){
			if(i+1>=argc || strlen(argv[i+1])==0){printf("-ref must follow reference file name\n");return 0;}
			inFile1 = argv[i+1];
			i++;
		}else if(strcmp(argv[i],"-sample")==0){
			if(i+1>=argc || strlen(argv[i+1])==0){printf("-sample must follow sample file name\n");return 0;}
			inFile2 = argv[i+1];
			i++;
		}else if(strcmp(argv[i],"-output")==0){
			if(i+1>=argc || strlen(argv[i+1])==0){printf("-o must follow output file name\n");return 0;}
			outFile = argv[i+1];
			i++;
		}else if(strcmp(argv[i],"-mode")==0){
			if(i+1>=argc || strlen(argv[i+1])==0){printf("-mode must follow cpu or gpu\n");return 0;}
			if(strcmp(argv[i+1],"gpu")==0){
				cpu = 0;
			}
			i++;
		}else if(strcmp(argv[i],"-dbConfigFile")==0){
			if(i+1>=argc || strlen(argv[i+1])==0){printf("-mode must follow file name\n");return 0;}
            
            memcpy(configFile,argv[i+1],strlen(argv[i+1]));
            dbConfigInCommandLine = 0;
			i++;
		}else if(strcmp(argv[i],"-dbInfo")==0){
			if(i+1>=argc || strlen(argv[i+1])==0){printf("-mode must follow \"name1=value1,name2=value2...\"\n");return 0;}
            
            getDBInfo(argv[i+1]);
            dbConfigInCommandLine = 1;
			i++;
		}else if(strcmp(argv[i],"-show")==0){
			if(i+1>=argc || strlen(argv[i+1])==0){printf("-show must follow all, matched or unmatched\n");return 0;}
			if(strcmp(argv[i+1],"all")==0){
				showResult = 0;
			}else if(strcmp(argv[i+1],"matched")==0){
				showResult = 1;
			}else if(strcmp(argv[i+1],"unmatched")==0){
				showResult = 2;
			}else{
				printf("-show must follow all, matched or unmatched\n");
			}
			i++;
		}else if(strcmp(argv[i],"-terminal")==0){
			printResult = 1;
		}else if(strcmp(argv[i],"-processInfo")==0){
			showProcessInfo = 1;
		}else if(strcmp(argv[i],"-h")==0 || strcmp(argv[i],"-help")==0){
			showHelp();
			return 0;;
		}else if(strcmp(argv[i],"-v")==0 || strcmp(argv[i],"-version")==0){
			printf("%s\n",VERSION);
			return 0;;
		}else {
			printf("%s is unknow parameter\n",argv[i]);
			showHelp();
			return 0;
		}
		
	}
    
    if(dbConfigInCommandLine==0){
        getDatabaseInfo(configFile);
    }

	/*inFile1 = "../data/reference.fit"; 
	inFile2 = "../data/sample.cat";
	outFile = "../data/CPUMatchResult.cat";*/
	//
    if(method == PLANE_METHOD){
        if(areaWidth==0 || areaHeight==0){
            printf("in plane coordinate mode, must assign \"-width\" and \"-height\"\n");
            return 0;
        }
        mainPlane(inFile1,inFile2,outFile);
    }else{
        mainSphere(inFile1,inFile2,outFile);
    }
    
	return 0;
}
