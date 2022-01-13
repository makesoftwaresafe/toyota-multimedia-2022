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
 * @file           persistence_hm_disk_mon.c
 * @ingroup        Persistence Health Monitor
 * @author         Ingo Huerner
 * @brief          Implementation of the persistence health monitor disk free space monitor
 * @see
 */


#include "persistence_hm_definitions.h"
#include "persistence_hm_disk_mon.h"
#include "rbtree.h"
#include "crc32.h"

#include <pthread.h>
#include <dirent.h>
#include <sys/vfs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdlib.h>


/// structure definition for a key value item
typedef struct _key_value_s
{
   unsigned int key;
   unsigned int value;
}key_value_s;


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


static char* gpConfigFileMap = 0;
static char* gpTokenArray[TOKENARRAYSIZE];
static int gTokenCounter = 0;
static unsigned int gConfigFileSize = 0;

/// default configuration file location
const char* gDefaultConfig = "/etc/persistence_phm.conf";

/// the rb tree
static jsw_rbtree_t *gRb_tree_bl = NULL;

// local function prototypes
static int readConfigFile(const char* filename);
static void releaseConfigFile(void);
static void fillCharTokenArray();
static int getConfiguration(void);
static void  key_val_rel(void *p);
static void* key_val_dup(void *p);
static int key_val_cmp(const void *p1, const void *p2 );
static unsigned int findMaxSize(unsigned int folderName);
//----------------------------------------------------------

#define FILE_DIR_NOT_SELF_OR_PARENT(s) ((s)[0]!='.'&&(((s)[1]!='.'||(s)[2]!='\0')||(s)[1]=='\0'))

#define SUBDIR_ARRAYSIZE 10

pthread_t gMonitorThread;


static const char* gPersistencePath = "/Data/mnt-c";

static unsigned int gSizeSubDir[SUBDIR_ARRAYSIZE] = {0};


//static int getConfigSize(const char* appID, unsigned int* sizes);


static int checkDiskFreeSpace(const char* thePath, int theDepth)
{
   unsigned int sumSize = 0;
   struct dirent *dirent = NULL;

   DIR *dir = opendir(thePath);
   if(NULL != dir)
   {
      for(dirent = readdir(dir); NULL != dirent; dirent = readdir(dir))
      {
         if(FILE_DIR_NOT_SELF_OR_PARENT(dirent->d_name))
         {
            char path[256] = {0};
            snprintf(path, 256, "%s/%s", thePath, dirent->d_name);

            if(DT_DIR == dirent->d_type)
            {
               gSizeSubDir[theDepth+1] = checkDiskFreeSpace(path, theDepth+1);

               if(theDepth == 0)
               {
                  int i = 0;
                  unsigned int size = 0;
                  for(i = 1; i<SUBDIR_ARRAYSIZE; i++)
                  {
                     size += gSizeSubDir[i];
                  }

                  //size = (size/1024);
                  if(size != 0)
                  {
                     unsigned int maxSize = findMaxSize(pclCrc32(0, (unsigned char*)dirent->d_name, strlen(dirent->d_name)));
                     printf("       AppID: \"%s\" => Current: %u - Max: %d\n", dirent->d_name, size, maxSize);
                     printf("        Size: %d -> Size + 10 Prozent : %d\n", size, (int)(double)(size * 1.1));
                     if( (int)(double)(size * 1.1) >= maxSize)
                     {
                        printf("Disk space  A L M O S T  empty\n");
                     }
                     else if(size >= maxSize)
                     {
                        printf("Disk space E M P T Y\n");
                     }
                     else
                     {
                        printf("Disk space O K\n");
                     }

                     printf("\n");
                  }
               }
            }
            else if(DT_REG == dirent->d_type)
            {
               struct stat buf;

               if(stat(path, &buf) != -1)
               {
                  sumSize += (int)buf.st_size;
               }
            }
         }
      }
      closedir(dir);
   }
   return sumSize;
}



static void* runMonitorThread(void* dataPtr)
{
   while(1) // run forever
   {
      checkDiskFreeSpace(gPersistencePath, 0);

      sleep(4);
   }

   return NULL;
}


int startMonitorThread()
{
   int rval = -1;

   if(getConfiguration() != -1)   // read configuration file
   {
      rval = pthread_create(&gMonitorThread, NULL, runMonitorThread, NULL);
      if(rval)
      {
        DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("pthread_create( runMonitorThread ) ret err:"), DLT_INT(rval) );
        return -1;
      }

      (void)pthread_setname_np(gMonitorThread, "phmMonitorThread");
   }
   else
   {
      DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("Failed to read size configuration"));
   }

   return rval;
}


static int getConfiguration(void)
{
   int rval = 0;
   const char *filename = getenv("PERS_PHM_CFG");

   if(filename == NULL)
   {
      filename = gDefaultConfig;    // use default filename
      DLT_LOG(phmContext, DLT_LOG_INFO, DLT_STRING("configReader::getConfiguration ==> using DEFAULT conf file:"), DLT_STRING(filename));
   }
   else
   {
      DLT_LOG(phmContext, DLT_LOG_INFO, DLT_STRING("configReader::getConfiguration ==> using environment PERS_FILE_CACHE_CFG conf file:"), DLT_STRING(filename));
   }

   if(readConfigFile(filename) != -1)
   {
      int i = 0;
      // create new tree
      gRb_tree_bl = jsw_rbnew(key_val_cmp, key_val_dup, key_val_rel);

      while( i < TOKENARRAYSIZE-1 )
      {
         if(gpTokenArray[i] != NULL)
         {
            key_value_s* item;

            item = malloc(sizeof(key_value_s));
            if(item != NULL)
            {
               unsigned int key   = pclCrc32(0, (unsigned char*)gpTokenArray[i], strlen(gpTokenArray[i]));
               unsigned int value = atoi(gpTokenArray[i+1]);
               //printf("   config Data: %s - %d - %u -> %u\n", gpTokenArray[i], atoi(gpTokenArray[i+1]), key, value);

               item->key   = key;
               item->value = value;

               jsw_rbinsert(gRb_tree_bl, item);

               free(item);
            }
            i+=2;       // move to the next configuration file entry
         }
         else
         {
            break;
         }
      }
      releaseConfigFile();
   }
   else
   {
      DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("configReader::getConfiguration ==> error reading config file:"), DLT_STRING(filename));
      rval = -1;
   }

   return rval;
}



static int readConfigFile(const char* filename)
{
   int fd = 0;
   struct stat buffer;

   memset(&buffer, 0, sizeof(buffer));

   if(stat(filename, &buffer) != -1)
   {
      if(buffer.st_size > 0)  // check for empty file
      {
         gConfigFileSize = buffer.st_size;

         fd = open(filename, O_RDONLY);
         if(fd == -1)
         {
            DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("configReader::readConfigFile ==> Error file open: "),
                    DLT_STRING(filename), DLT_STRING("err msg: "), DLT_STRING(strerror(errno)) );
            return -1;
         }

         // map the config file into memory
         gpConfigFileMap = (char*)mmap(0, gConfigFileSize, PROT_WRITE, MAP_PRIVATE, fd, 0);

         if(gpConfigFileMap == MAP_FAILED)
         {
            DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("configReader::readConfigFile ==> Error mapping the file"));
            gpConfigFileMap = 0;
            close(fd);
            return -1;
         }

         gTokenCounter = 0;   // reset the token counter
         fillCharTokenArray();
         close(fd);
      }
      else
      {
         DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("configReader::readConfigFile ==> Error file size is 0"));
         return -1;
      }
   }
   else
   {
      DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("configReader::readConfigFile ==> failed to stat file"));
      return -1;
   }

   return 0;
}



static void fillCharTokenArray()
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
      DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("configReader::fillCharTokenArray ==> invalid file mapping (mmap failed)"));
   }
}



static void releaseConfigFile(void)
{
   // unmap the mapped config file if successfully mapped
   if(gpConfigFileMap != 0)
   {
      if(munmap(gpConfigFileMap, gConfigFileSize) == -1)
      {
         DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("configReader::releaseConfigFile ==> failed to unmap file: "), DLT_STRING(strerror(errno)));
      }
   }
}


/// compare function for tree key_value_s item
static int key_val_cmp(const void *p1, const void *p2 )
{
   int rval = -1;
   key_value_s* first;
   key_value_s* second;

   first  = (key_value_s*)p1;
   second = (key_value_s*)p2;

   if(second->key == first->key)
   {
      rval = 0;
   }
   else if(second->key < first->key)
   {
      rval = -1;
   }
   else
   {
      rval = 1;
   }

   return rval;
 }



/// duplicate function for key_value_s item
static void* key_val_dup(void *p)
{
   key_value_s* src = NULL;
   key_value_s* dst = NULL;

   src = (key_value_s*)p;

   // allocate memory for node
   dst = malloc(sizeof(key_value_s));
   if(dst != NULL)
   {
      // duplicate hash key
      dst->key = src->key;
      // duplicate value
      dst->value = src->value;
   }
   return dst;
}



/// release function for key_value_s item
static void  key_val_rel(void *p )
{
   key_value_s* rel = NULL;
   rel = (key_value_s*)p;

   if(rel != NULL)
      free(rel);
}



static unsigned int findMaxSize(unsigned int folderName)
{
   unsigned int rval = 0;
   key_value_s* item = NULL;

   item = malloc(sizeof(key_value_s));
   if(item != NULL && gRb_tree_bl != NULL)
   {
      key_value_s* foundItem = NULL;
      item->key = folderName;
      foundItem = (key_value_s*)jsw_rbfind(gRb_tree_bl, item);
      if(foundItem != NULL)
      {
         rval = foundItem->value;
      }
      free(item);
   }
   else
   {
      if(item!=NULL)
        free(item);
   }

   return rval;
}



void freeRbTree()
{
   if(gRb_tree_bl != NULL)
      jsw_rbdelete (gRb_tree_bl);
}


