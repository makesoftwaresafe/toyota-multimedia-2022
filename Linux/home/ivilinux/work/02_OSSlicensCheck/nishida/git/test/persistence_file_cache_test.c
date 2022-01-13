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
 * @file           persistence_file_cache_test.c
 * @ingroup        Persistence file cache library test
 * @author         Ingo Huerner
 * @brief          Test of persistence file cache library
 * @see            
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>     /* exit */
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <dlt/dlt.h>
#include <dlt/dlt_common.h>



#include "persCheck.h"
//#include <check.h>

#include "../include/persistence_file_cache.h"
#include "../include/persistence_file_cache_error_def.h"

#define NUM_FILES 20

// used for conversion
#define SECONDS2NANO 1000000000L

// define for the used clock: "CLOCK_MONOTONIC" or "CLOCK_REALTIME"
#define CLOCK_ID  CLOCK_MONOTONIC

#define SOURCE_PATH "/Data/mnt-c/"


// helper functions form persistence_file_cache.c
// ONLY FOR INTERNAL USE
extern void statsGetHandleToRemove(unsigned int startValue, int* handle);
extern int pfcHelperDelFilesRecursive(const char* dir_name);
extern inline int statsGetCacheUsage(void);
// --------------------------------------------------------------------


// cache path
static const char* gCachePath  = "/tmp/mycache/";

static const char* gPathPrefix     = SOURCE_PATH "pfc_mycache/filesToCache/cache_";
static const char* gPathSegemnts[] = {"pfc_mycache/", "filesToCache/", NULL };

struct timespec gStartTime;

char* gWriteBuffer[NUM_FILES] = {0};
char* gFinalBuffer[NUM_FILES] = {0};

// definition of weekday to generate random string
char* dayOfWeek[] = { "Sunday   ",
                      "Monday   ",
                      "Tuesday  ",
                      "Wednesday",
                      "Thursday ",
                      "Friday   ",
                      "Saturday "};


/// file handle structure definition
typedef struct _PfcTestStruct_s
{
   char*            data;   // the cache path
   PfcCacheAffinity  prio;   // cache priotiry

} PfcTestStruct_s;

static PfcTestStruct_s gDataBuf[] = {
   { "cache 00: - 00",              		CacheAffBelowMedium },
   { "cache 01: -- 01",						CacheAffLow },
   { "cache 02: --- 02",                    CacheAffMedium },
   { "cache 03: ---- 03",  					CacheAffAboveMedium },
   { "cache 04: ----- 04",                  CacheAffHighest },
   { "cache 05: ------ 05",                 CacheAffLow },
   { "cache 06: ------- 06",                CacheAffBelowMedium},
   { "cache 07: -------- 07",  				CacheAffMedium},
   { "cache 08: --------- 08",              CacheAffAboveMedium },
   { "cache 09: ---------- 09",             CacheAffHighest},
   { "cache 10: ----------- 10",  			CacheAffLow },
   { "cache 11: ------------ 11",           CacheAffBelowMedium},
   { "cache 12: ------------- 12",          CacheAffMedium},
   { "cache 13: -------------- 13",  		CacheAffMedium },
   { "cache 14: ---------------- 14",       CacheAffLow},
   { "cache 15: ----------------- 15",      CacheAffHighest},
   { "cache 16: ------------------ 16",     CacheAffLow},
   { "cache 17: -------------------- 17",  	CacheAffBelowMedium},
   { "cache 18: --------------------- 18",  CacheAffBelowMedium},
   { "cache 19: ---------------------- 19", CacheAffMedium } };



inline long long getNsDuration(struct timespec* start, struct timespec* end)
{
   return ((end->tv_sec * SECONDS2NANO) + end->tv_nsec) - ((start->tv_sec * SECONDS2NANO) + start->tv_nsec);
}


void data_setup(void)
{
   int i = 0, handle = 0;
   char thePath[128]    = {0};
   char createPath[128] = {0};

   clock_gettime(CLOCK_ID, &gStartTime);

   // create directory
   snprintf(createPath, 128, "%s", SOURCE_PATH );
   while(gPathSegemnts[i] != NULL)
   {
   	strncat(createPath, gPathSegemnts[i++], 128-1);
   	mkdir(createPath, 0744);
   }

   for(i=0; i < NUM_FILES; i++)
   {
      // start with defined file content
      snprintf(thePath, 128, "%s%02d%s", gPathPrefix, i, ".pers");

      handle = open(thePath, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
      if(write(handle, gDataBuf[i].data, strlen(gDataBuf[i].data)) == -1)
      {
         printf("setup test: failed to write test data: cache_0%d.pers\n", i);
      }
      close(handle);
   }

   //
   // clean the cache if there is something left
   //
   pfcHelperDelFilesRecursive(gCachePath);
}
void data_teardown(void)
{
   // nothing
   printf("* * * tear down * * *\n");
}




START_TEST(test_OpenFile)
{

   X_TEST_REPORT_TEST_NAME("persistence_file_cache_test");
   X_TEST_REPORT_COMP_NAME("libpersistence_file_cache_library");
   X_TEST_REPORT_REFERENCE("NONE");
   X_TEST_REPORT_DESCRIPTION("Test open the file");
   X_TEST_REPORT_TYPE(GOOD);

   int handle[NUM_FILES] = {0};
   int ret = 0, i = 0;

   char thePath[128]     = {0};
   char readBuffer[2048] = {0};

   // init
   pfcInitCache("testApp");

   //
   // do testing
   //
   for(i=0; i < NUM_FILES; i++)
   {
      memset(readBuffer, 0, 2048);  // make sure the buffer is "empty"

      snprintf(thePath, 128, "%s%02d%s", gPathPrefix, i, ".pers");

      handle[i] = pfcOpenFileAffinity(thePath, CreateFile, gDataBuf[i].prio);
      fail_unless(handle[i] >= 0, "Failed to open handle ==> filesToCache/cache_01.pers");

      ret = pfcReadFile(handle[i], readBuffer, 2048);
      /*printf("FileSize => ist: %d => soll: %d \n", ret, (int)strlen(gDataBuf[i].data));
      printf("ReadBuffer => ist : \"%s\"\n", readBuffer);
      printf("           => soll: \"%s\"\n", gDataBuf[i].data);*/
      fail_unless(ret == (int)strlen(gDataBuf[i].data), "File size size incorrect");
      fail_unless(strncmp(readBuffer, gDataBuf[i].data, ret) == 0, "Buffer not correctly read");
   }


   for(i=0; i < NUM_FILES; i++)
   {
      pfcCloseFile(handle[i]);
   }

   pfcDeinitCache();

}
END_TEST



START_TEST(test_WrongHandle)
{
   X_TEST_REPORT_TEST_NAME("persistence_file_cache_test");
   X_TEST_REPORT_COMP_NAME("libpersistence_file_cache_library");
   X_TEST_REPORT_REFERENCE("NONE");
   X_TEST_REPORT_DESCRIPTION("Test wrong file handle");
   X_TEST_REPORT_TYPE(GOOD);

   int ret = 0;
   char readBuffer[1024] = {0};

   ret = pfcReadFile(5783, readBuffer, 1024);
   fail_unless(ret == EPRS_MAX_HANDLE, "Read: Exceed handle max limit");

   ret = pfcReadFile(-1, readBuffer, 1024);
   fail_unless(ret == EPRS_MAX_HANDLE, "Read: Exceed handle max limit");

   ret = pfcWriteFile(5783, "Something", strlen("Something"));
   fail_unless(ret == EPRS_MAX_HANDLE, "Write: Exceed handle max limit");

   ret =  pfcFileSeek(5783, 0, SEEK_END);
   fail_unless(ret == EPRS_MAX_HANDLE, "Seek: Exceed handle max limit");

   ret = pfcFileGetSize(5783);
   fail_unless(ret == EPRS_MAX_HANDLE, "GetSize: Exceed handle max limit");

   ret = pfcCloseFile(5783);
   fail_unless(ret == EPRS_MAX_HANDLE, "GetSize: Exceed handle max limit");

}
END_TEST


START_TEST(test_WriteFile)
{
   X_TEST_REPORT_TEST_NAME("persistence_file_cache_test");
   X_TEST_REPORT_COMP_NAME("libpersistence_file_cache_library");
   X_TEST_REPORT_REFERENCE("NONE");
   X_TEST_REPORT_DESCRIPTION("Test write the file");
   X_TEST_REPORT_TYPE(GOOD);

   int handle[NUM_FILES] = {0}, i = 0;
   int ret = 0;

   char readBuffer[2048] = {0};
   char thePath[128]     = {0};


   // init
   pfcInitCache("testApp");

   //
   // do testing - W R I T E
   //
   for(i=0; i < NUM_FILES; i++)
   {
      snprintf(thePath, 128, "%s%02d%s", gPathPrefix, i, ".pers");

      handle[i] = pfcOpenFileAffinity(thePath, CreateFile, gDataBuf[i].prio);
      fail_unless(handle[i] >= 0, "Failed to open handle ==> filesToCache/cache_XX.pers");

      // write to the end of the file
      ret = pfcFileSeek(handle[i], 0, SEEK_END);
      fail_unless(ret == (int)strlen(gDataBuf[i].data), "Failed to seek to the end of the file");

      //
      ret = pfcWriteFile(handle[i], gWriteBuffer[i], strlen(gWriteBuffer[i]));
      fail_unless(ret == (int)strlen(gWriteBuffer[i]), "Wrong size written");
   }

   // close opened files
   for(i=0; i< NUM_FILES; i++)
   {
   	pfcCloseFile(handle[i]);
   }

   //
   // do testing - verify the written data from test above
   //
   for(i=0; i < NUM_FILES; i++)
   {
      memset(readBuffer, 0, 2048);  // make sure the buffer is "empty"

      snprintf(thePath, 128, "%s%02d%s", gPathPrefix, i, ".pers");

      handle[i] = pfcOpenFileAffinity(thePath, CreateFile, gDataBuf[i].prio);
      fail_unless(handle[i] >= 0, "Failed to open handle ==> filesToCache/cache_01.pers");

      ret = pfcReadFile(handle[i], readBuffer, 2048);
      fail_unless(ret == (int)strlen(gFinalBuffer[i]), "Wrong size read");
      fail_unless(strncmp(readBuffer, gFinalBuffer[i], ret) == 0, "Buffer not correctly read");
   }


   // close opened files
   for(i=0; i< NUM_FILES; i++)
   {
      pfcCloseFile(handle[i]);
   }

   pfcDeinitCache();
}
END_TEST



START_TEST(test_GetSizeFile)
{
   char thePath[128]     = {0};
   int handle[NUM_FILES] = {0}, i = 0;
   int size = 0;

   // init
   pfcInitCache("testApp");

   //
	// do testing - W R I T E
	//
	for(i=0; i < NUM_FILES; i++)
	{
		snprintf(thePath, 128, "%s%02d%s", gPathPrefix, i, ".pers");

		handle[i] = pfcOpenFileAffinity(thePath, CreateFile, gDataBuf[i].prio);
		fail_unless(handle[i] >= 0, "Failed to open handle ==> filesToCache/cache_XX.pers");

		size = pfcFileGetSize(handle[i]);
		fail_unless(size == (int)strlen(gDataBuf[i].data), "Wrong size");

		size = pfcWriteFile(handle[i], gWriteBuffer[i], strlen(gWriteBuffer[i]));

		size = pfcFileGetSize(handle[i]);
		fail_unless(size == (int)strlen(gFinalBuffer[i]), "Wrong size");
	}

   // close opened files
   for(i=0; i< NUM_FILES; i++)
   {
      pfcCloseFile(handle[i]);
   }

   pfcDeinitCache();
}
END_TEST



START_TEST(test_NodeHealthTest)
{
	pfcInitCache("node-health-monitor");


	pfcDeinitCache();
}
END_TEST



static Suite * persistenceFileCache_suite()
{
   Suite * s  = suite_create("Persistency file cache");

   TCase * tc_OpenFile = tcase_create("OpenFile");
   tcase_add_checked_fixture(tc_OpenFile, data_setup, data_teardown);
   tcase_add_test(tc_OpenFile, test_OpenFile);

   TCase * tc_WrongHandle = tcase_create("WrongHandle");
   tcase_add_test(tc_WrongHandle, test_WrongHandle);

   TCase * tc_WriteFile = tcase_create("WriteFile");
   tcase_add_test(tc_WriteFile, test_WriteFile);

   TCase * tc_GetSizeFile = tcase_create("GetSizeFile");
   tcase_add_checked_fixture(tc_GetSizeFile, data_setup, data_teardown);
   tcase_add_test(tc_GetSizeFile, test_GetSizeFile);

   TCase * tc_NodeHealthTest = tcase_create("NodeHealthTest");
   tcase_add_test(tc_NodeHealthTest, test_NodeHealthTest);

   /*
   TCase * tc_RemoveFromCache = tcase_create("RemoveFromCache");
   tcase_add_test(tc_RemoveFromCache, test_RemoveFromCache);
   */


   suite_add_tcase(s, tc_OpenFile);
   suite_add_tcase(s, tc_WrongHandle);
   suite_add_tcase(s, tc_WriteFile);

   suite_add_tcase(s, tc_GetSizeFile);
   suite_add_tcase(s, tc_NodeHealthTest);
   // suite_add_tcase(s, tc_RemoveFromCache);

   return s;
}



int main(int argc, char *argv[])
{
   int nr_failed = 0,
          nr_run = 0,
               i = 0;

   TestResult** tResult;
   struct timespec curTime;

   (void)argc;
   (void)argv;

   //
   // create test data
   //
   for(i=0; i< NUM_FILES; i++)
   {
      long long timeDurationNs = 0;
      struct tm *locTime;
      time_t t = time(0);
      locTime = localtime(&t);

      clock_gettime(CLOCK_ID, &curTime);  // get the current time
      timeDurationNs = getNsDuration(&gStartTime, &curTime);

      gWriteBuffer[i] = malloc(2048);
      gFinalBuffer[i] = malloc(2048);
      memset(gWriteBuffer[i], 0, 2048);
      memset(gFinalBuffer[i], 0, 2048);

      snprintf(gWriteBuffer[i], 2048, " - Time %d: \"%s %.2d.%.2d.%d - %d:%.2d:%.2d Uhr\" [time and date] - Timestamp: %05d - %lld", i, dayOfWeek[locTime->tm_wday],
                                           locTime->tm_mday, (locTime->tm_mon)+1, (locTime->tm_year+1900),
                                           locTime->tm_hour, locTime->tm_min, locTime->tm_sec, i, timeDurationNs);

      usleep(1000);
      // create the string the final file content should look like
      snprintf(gFinalBuffer[i], 2048, "%s%s", gDataBuf[i].data, gWriteBuffer[i]);
      //printf("\n-------------------------------\nVerify the string: [%d]\n\"%s\"\n-------------------------------\n", i, gFinalBuffer[i]);
   }

   /// debug log and trace (DLT) setup
   DLT_REGISTER_APP("test","tests the persistence file cache");

#if 1
   Suite * s = persistenceFileCache_suite();
   SRunner * sr = srunner_create(s);
   srunner_set_xml(sr, "/tmp/persistenceFileCacheTest.xml");
   srunner_set_log(sr, "/tmp/persistenceFileCacheTest.log");
   srunner_run_all(sr, /*CK_NORMAL*/ CK_VERBOSE);

   nr_failed = srunner_ntests_failed(sr);
   nr_run = srunner_ntests_run(sr);

   tResult = srunner_results(sr);
   for(i = 0; i< nr_run; i++)
   {
      (void)tr_rtype(tResult[i]);  // get status of each test
   }

   srunner_free(sr);
#endif


   // free allocated memory
   for(i=0; i< NUM_FILES; i++)
   {
      free(gWriteBuffer[i]);
   }

   dlt_free();

   return (0==nr_failed)?EXIT_SUCCESS:EXIT_FAILURE;

}














#if 0
START_TEST(test_RemoveFromCache)
{
#if 0
   X_TEST_REPORT_TEST_NAME("persistence_file_cache_test");
   X_TEST_REPORT_COMP_NAME("libpersistence_file_cache_library");
   X_TEST_REPORT_REFERENCE("NONE");
   X_TEST_REPORT_DESCRIPTION("Remove file from cache");
   X_TEST_REPORT_TYPE(GOOD);
#endif

#if 0
   int handle[NUM_FILES] = {0}, i = 0, calcCacheSize = 0, theHandle = -1, ret = 0;
   char* buffer = "\nupdate";
   char thePath[128]     = {0};
   char readBuffer[2048] = {0};
   char finalBuffer[2048] = {0};

   struct stat statBuf, statBuf2;

   //
   // init
   //
   pfcInitCache("testApp");

   //
   // first open all the files
   //
   for(i=0; i < NUM_FILES; i++)
   {
      snprintf(thePath, 128, "%s%02d%s", gPathPrefix, i, ".pers");
      handle[i] = pfcOpenFileAffinity(thePath, gDataBuf[i].prio);

      fail_unless(handle[i] >= 0, "Failed to open handle ==> filesToCache/cache_01.pers");
      fail_unless(statsGetCacheUsage() == 0, "Open => Wrong calculated cache size");
   }

   //
   // write to files
   //
   for(i=0; i < NUM_FILES; i++)
   {
      fstat(handle[i], &statBuf);
      ret = pfcWriteFile(handle[i], "\nupdate", strlen(buffer));
      fstat(handle[i], &statBuf2);

      fail_unless(ret == strlen(buffer), "Wrong size written");
      calcCacheSize += statBuf.st_size + ret;
      printf("Wrong calculated cache size: soll: %d | ist: %d\n", calcCacheSize, statsGetCacheUsage());
      fail_unless(calcCacheSize == statsGetCacheUsage(), "Wrong calculated cache size");

#if 0
      printf("* * * Cachesize: %d | calcCacheSize: %d\n", statsGetCacheUsage(), calcCacheSize);
      printf("* * * FileSize: %d | FileSize: %d ==> ret: %d\n\n", (int)(statBuf.st_size + ret), (int)statBuf2.st_size, ret);
#endif
   }

   for(i=0; i< NUM_FILES; i++)
   {
      pfcCloseFile(handle[i]);
   }

#if 0

   // now write file to force files to be removed forme cache
   /*
   snprintf(thePath, 128, "%s%05d%s", gPathPrefix, 10000, ".pers");
   theHandle = pfcOpenFile(thePath, CachePrioHighest);
   ret = pfcWriteFile(theHandle, buffer, strlen(buffer));
   pfcCloseFile(theHandle);
   */



   //
   // do testing - verify the written data from test above (synced correctly from cache back to filesystem)
   //
   for(i=0; i < NUM_FILES; i++)
   {
      // create the verification string
      memset(gFinalBuffer[i], 0, 2048);
      snprintf(gFinalBuffer[i], 2048, "%s%s%s", gDataBuf[i].data, gWriteBuffer[i], buffer);
      printf("\n-------------------------------\nVerify the string: [%d]\n\"%s\"\n-------------------------------\n", i, gFinalBuffer[i]);

      snprintf(thePath, 128, "%s%02d%s", gPathPrefix, i, ".pers");
      handle[i] = pfcOpenFileAffinity(thePath, gDataBuf[i].prio);
      fail_unless(handle[i] >= 0, "Failed to open handle ==> filesToCache/cache_01.pers");

      ret = pfcReadFile(handle[i], readBuffer, 2048);
      pfcCloseFile(handle[i]);
      printf("1 * * SizeCheck => soll: %d | ist: %d \n\n\n", strlen(gFinalBuffer[i]), ret);
      fail_unless(ret == strlen(gFinalBuffer[i]), "Wrong size read!!!!");
      fail_unless(strncmp(readBuffer, gFinalBuffer[i], ret) == 0, "Buffer not correctly read!!!!");
   }


   //
   // verify file
   //
   snprintf(finalBuffer, 2048, "%s%s", gSomeText, buffer);
   theHandle = pfcOpenFileAffinity(thePath, CacheAffHighest);

   ret = pfcReadFile(theHandle, readBuffer, 2048);
   pfcCloseFile(theHandle);
   printf("* * SizeCheck => soll: %d | ist: %d \n\n\n", strlen(finalBuffer), ret);
   fail_unless(ret == strlen(finalBuffer), "Wrong size read 2");
   fail_unless(strncmp(readBuffer, finalBuffer, ret) == 0, "Buffer not correctly read");
#endif
#endif

}
END_TEST
#endif




