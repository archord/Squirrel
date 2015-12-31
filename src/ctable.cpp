#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "CreateTable.h"
#include "cmhead.h"

int main1(int argc, char** argv) {
  int flag = CREATE_TABLE;
  char *configFileName = (char *) malloc(LINE);
  char *outFileName = (char *) malloc(LINE);
  char *tableDate = (char *) malloc(LINE);
  memset(configFileName, 0, LINE);
  memset(outFileName, 0, LINE);
  memset(tableDate, 0, LINE);

  CreateTable *ctable = new CreateTable();

  if (argc > 1) {
    int i = 0;
    for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-configfile") == 0 || strcmp(argv[i], "-f") == 0) {
        if (i + 1 >= argc || strlen(argv[i + 1]) == 0) {
          printf("-configfile must follow file name\n");
          return 0;
        }
        sprintf(configFileName, "%s", argv[i + 1]);
        i++;
      } else if (strcmp(argv[i], "-fNumber") == 0 || strcmp(argv[i], "-fn") == 0) {
        if (i + 1 >= argc || strlen(argv[i + 1]) == 0) {
          printf("-fNumber must follow number\n");
          return 0;
        }
        ctable->fileNumber = atoi(argv[i + 1]);
        i++;
      } else if (strcmp(argv[i], "-tNumber") == 0 || strcmp(argv[i], "-tn") == 0) {
        if (i + 1 >= argc || strlen(argv[i + 1]) == 0) {
          printf("-tNumber must follow number\n");
          return 0;
        }
        ctable->tableNumber = atoi(argv[i + 1]);
        i++;
      } else if (strcmp(argv[i], "-create") == 0 || strcmp(argv[i], "-c") == 0) {
        if (i + 1 >= argc || strlen(argv[i + 1]) == 0) {
          printf("-create must follow date \"YYYY-MM-dd\"\n");
          return 0;
        }
        sprintf(tableDate, "%s", argv[i + 1]);
        flag = CREATE_TABLE;
        i++;
      } else if (strcmp(argv[i], "-delete") == 0 || strcmp(argv[i], "-d") == 0) {
        if (i + 1 >= argc || strlen(argv[i + 1]) == 0) {
          printf("-delete must follow date \"YYYY-MM-dd\"\n");
          return 0;
        }
        sprintf(tableDate, "%s", argv[i + 1]);
        flag = DELETE_TABLE;
        i++;
      } else if (strcmp(argv[i], "-outfile") == 0 || strcmp(argv[i], "-o") == 0) {
        if (i + 1 >= argc || strlen(argv[i + 1]) == 0) {
          printf("-outfile must follow outfile name\n");
          return 0;
        }
        sprintf(outFileName, "%s", argv[i + 1]);
        i++;
      } else if (strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "-h") == 0) {
        ctable->showHelp();
        return 0;
      }
    }

    ctable->createMain(flag, configFileName, outFileName, tableDate);
  } else {
    ctable->showHelp();
  }

  delete ctable;
  free(outFileName);
  free(configFileName);
  free(tableDate);
  return 0;
}
