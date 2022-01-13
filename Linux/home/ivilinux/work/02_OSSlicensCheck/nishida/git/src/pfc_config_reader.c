/******************************************************************************
 * Project         Persistency
 * (c) copyright   2014
 * Company         XS Embedded GmbH
 *****************************************************************************/
/******************************************************************************
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a  copy of the MPL was not distributed
 * with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
******************************************************************************/
 /**
 * @file           persistence_file_cache_config_reader.c
 * @ingroup        Persistence file cache
 * @author         Ingo Huerner
 * @brief          Implementation of persistence configuration reader
 * @see
 */


#include "pfc_config_reader.h"
#include "pfc_file_helper.h"
#include "pfc_def.h"


#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>



/// the size of the token array
enum configConstants
{
   TOKENARRAYSIZE = 1024
};


static const char gCharLookup[] =
{
   0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  // from 0x0 (NULL)  to 0x1F (unit seperator)
   0,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  // from 020 (space) to 0x2F (?)
   1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  // from 040 (@)     to 0x5F (_)
   1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1     // from 060 (')     to 0x7E (~)

};


char* gpConfigFileMap = 0;
static char* gpTokenArray[TOKENARRAYSIZE];
int gTokenCounter = 0;
unsigned int gConfigFileSize = 0;


// local function prototypes
int readConfigFile(const char* filename);
void releaseConfigFile(void);
void fillCharTokenArray();
void getConfigData(const char* itemName, unsigned int* cacheSize);

//----------------------------------------------------------



int getConfiguration(const char* appID, unsigned int* cacheSize)
{
   int rval = 0;
   const char *filename = getenv("PERS_FILE_CACHE_CFG");

   if(filename == NULL)
   {
      filename = gDefaultConfig;  	// use default filename
      DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("configReader::getConfiguration ==> using DEFAULT conf file:"), DLT_STRING(filename));
   }
   else
   {
   	DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("configReader::getConfiguration ==> using environment PERS_FILE_CACHE_CFG conf file:"), DLT_STRING(filename));
   }

   if(readConfigFile(filename) != -1)
   {
      getConfigData(appID, cacheSize);
      releaseConfigFile();
   }
   else
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("configReader::getConfiguration ==> error reading config file:"), DLT_STRING(filename));
      rval = -1;
   }

   return rval;
}




int readConfigFile(const char* filename)
{
   int fd = 0;
   struct stat buffer;

   memset(&buffer, 0, sizeof(buffer));

   if(stat(filename, &buffer) != -1)
   {
      if(buffer.st_size > 0) 	// check for empty file
      {
      	gConfigFileSize = buffer.st_size;

			fd = open(filename, O_RDONLY);
			if(fd == -1)
			{
				DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("configReader::readConfigFile ==> Error file open: "),
						  DLT_STRING(filename), DLT_STRING("err msg: "), DLT_STRING(strerror(errno)) );
				return -1;
			}

			// map the config file into memory
			gpConfigFileMap = (char*)mmap(0, gConfigFileSize, PROT_WRITE, MAP_PRIVATE, fd, 0);

			if(gpConfigFileMap == MAP_FAILED)
			{
				DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("configReader::readConfigFile ==> Error mapping the file"));
				gpConfigFileMap = 0;
				close(fd);
				return -1;
			}

			gTokenCounter = 0;	// reset the token counter
			fillCharTokenArray();
			close(fd);
   	}
      else
      {
      	DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("configReader::readConfigFile ==> Error file size is 0"));
      	return -1;
      }
	}
   else
   {
   	DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("configReader::readConfigFile ==> failed to stat file"));
   	return -1;
   }

   return 0;
}



void releaseConfigFile(void)
{
   // unmap the mapped config file if successfully mapped
   if(gpConfigFileMap != 0)
   {
      if(munmap(gpConfigFileMap, gConfigFileSize) == -1)
      {
      	DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("configReader::releaseConfigFile ==> failed to unmap file: "), DLT_STRING(strerror(errno)));
      }
   }
}



void fillCharTokenArray()
{
   unsigned int i=0;
   int blankCount=0;

   if(gpConfigFileMap != 0)
   {
		char* tmpPointer = gpConfigFileMap;

		// set the first pointer to the start of the file
		gpTokenArray[blankCount] = tmpPointer;
		blankCount++;

		while(i < gConfigFileSize)
		{
			if(1 != gCharLookup[(int)*tmpPointer])
			{
				*tmpPointer = 0;

				// check if we are at the end of the token array
				if(blankCount >= TOKENARRAYSIZE)
				{
					break;
				}
				gpTokenArray[blankCount] = tmpPointer+1;
				blankCount++;
				gTokenCounter++;

			}
			tmpPointer++;
			i++;
		}
   }
   else
   {
   	DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("configReader::fillCharTokenArray ==> invalid file mapping (mmap failed)"));
   }
}


void getConfigData(const char* itemName, unsigned int* cacheSize)
{
   int i = 0;

   // search the application ----------------------------------------
   while( i < TOKENARRAYSIZE-1 )
   {
      if(gpTokenArray[i] != 0 && gpTokenArray[i+1])
      {
         if(0 == strcmp("BackupRequested", gpTokenArray[i]))   // want backup
         {
            gCreateBackup = atoi(gpTokenArray[i+1]);
         }
         else if(0 == strcmp("CachePath", gpTokenArray[i]))      // cache path
         {
            memset(gCachPathLocation, 0, PathMaxLen);
            strncpy(gCachPathLocation, gpTokenArray[i+1], PathMaxLen);
            helperFormatDirString(gCachPathLocation);    // remove '/' at the end of the string if there is one
         }
         else if(0 == strcmp("BackupPath", gpTokenArray[i]))     // backup path
         {
            memset(gBackupPathLocation, 0, PathMaxLen);
            strncpy(gBackupPathLocation, gpTokenArray[i+1], PathMaxLen);
            helperFormatDirString(gBackupPathLocation);  // remove '/' at the end of the string if there is one
         }
         else if(0 == strcmp(itemName, gpTokenArray[i]))      // cache size
         {
            *cacheSize = atoi(gpTokenArray[i+1]);
            break;
         }
      }

      i+=2;       // move to the next configuration file entry
   }
}


