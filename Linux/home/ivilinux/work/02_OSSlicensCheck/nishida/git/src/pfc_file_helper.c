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
 * @file           persistence_file_cache_backup.c
 * @ingroup        Persistence file cache
 * @author         Ingo Huerner
 * @brief          Implementation of persistence file cache backup
 * @see
 */

#include "pfc_file_helper.h"
#include "crc32.h"



#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/sendfile.h>


// function prototype declaration
int backupRecoverFromBackup(int backupFd, const char* original);



int helperCreateAndOpenFile(const char* path)
{
   const char* delimiters = "/\n";   // search for blank and end of line
   char* tokenArray[128];
   char thePath[PathMaxLen] = {0};
   int numTokens = 0, i = 0, validPath = 1;
   int handle = -1;

   if(path != NULL)
   {
		memset(tokenArray, 0, 64);
		strncpy(thePath, path, PathMaxLen);
		tokenArray[numTokens++] = strtok(thePath, delimiters);

		while(tokenArray[numTokens-1] != NULL )
		{
		  tokenArray[numTokens] = strtok(NULL, delimiters);
		  if(tokenArray[numTokens] != NULL)
		  {
			  numTokens++;
			  if(numTokens >= 24)
			  {
				  validPath = 0;
				  break;
			  }
		  }
		  else
		  {
			  break;
		  }
		}

		if(validPath == 1)
		{
			char createPath[PathMaxLen] = {0};
			snprintf(createPath, PathMaxLen, "/%s",tokenArray[0] );
			for(i=1; i<numTokens-1; i++)
			{
				// create folders
				strncat(createPath, "/", PathMaxLen-1);
				strncat(createPath, tokenArray[i], PathMaxLen-1);
				mkdir(createPath, 0744);
			}
			// finally create the file
			strncat(createPath, "/", PathMaxLen-1);
			strncat(createPath, tokenArray[i], PathMaxLen-1);
			handle = open(createPath, O_CREAT|O_RDWR |O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
		}
		else
		{
			DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("backupCreateAndOpenFile ==> no valid path to create: "), DLT_STRING(path) );
		}
   }
   else
   {
   	DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("backupCreateAndOpenFile ==> path is NULL"));
   }

   return handle;
}



int helperVerifyConsistency(const char* origPath, const char* backupPath, const char* csumPath, int openFlags)
{
   int handle = 0, readSize = 0;
   int backupAvail = 0, csumAvail = 0;
   int fdCsum = 0, fdBackup = 0;

   char origCsumBuf[ChecksumBufSize] = {0};
   char backCsumBuf[ChecksumBufSize] = {0};
   char csumBuf[ChecksumBufSize]     = {0};

   // check if we have a backup and checksum file
   backupAvail = access(backupPath, F_OK);
   csumAvail   = access(csumPath, F_OK);

   // *************************************************
   // there is a backup file and a checksum
   // *************************************************
   if( (backupAvail == 0) && (csumAvail == 0) )
   {
      DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclVerifyConsistency => there is a backup file AND a checksum"));
      // calculate checksum form backup file
      fdBackup = open(backupPath,  O_RDONLY);
      if(fdBackup != -1)
      {
         helperCalcCrc32Csum(fdBackup, backCsumBuf);

         fdCsum = open(csumPath,  O_RDONLY);
         if(fdCsum != -1)
         {
            readSize = read(fdCsum, csumBuf, ChecksumBufSize);
            if(readSize > 0)
            {
               if(0 == strcmp(csumBuf, backCsumBuf))
               {
                  // checksum matches ==> replace with original file
                  handle = backupRecoverFromBackup(fdBackup, origPath);
               }
               else
               {
                  // checksum does not match, check checksum with original file
                  handle = open(origPath, openFlags);
                  if(handle != -1)
                  {
                     helperCalcCrc32Csum(handle, origCsumBuf);
                     if(strcmp(csumBuf, origCsumBuf)  != 0)
                     {
                        close(handle);
                        handle = -1;  // error: file corrupt
                     }
                     // else case: checksum matches ==> keep original file ==> nothing to do
                  }
                  else
                  {
                     close(handle);
                     handle = -1;     // error: file corrupt
                  }
               }
            }
            close(fdCsum);
         }
         else
         {
            close(fdCsum);
            handle = -1;     // error: file corrupt
         }
      }
      else
      {
         handle = -1;
      }
      close(fdBackup);
   }
   // *************************************************
   // there is ONLY a checksum file
   // *************************************************
   else if(csumAvail == 0)
   {
      DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclVerifyConsistency => there is ONLY a checksum file"));

      fdCsum = open(csumPath,  O_RDONLY);
      if(fdCsum != -1)
      {
         readSize = read(fdCsum, csumBuf, ChecksumBufSize);
         if(readSize <= 0)
         {
            DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pclVerifyConsistency => read checksum: invalid readSize"));
         }
         close(fdCsum);

         // calculate the checksum form the original file to see if it matches
         handle = open(origPath, openFlags);
         if(handle != -1)
         {
            helperCalcCrc32Csum(handle, origCsumBuf);

            if(strcmp(csumBuf, origCsumBuf)  != 0)
            {
                close(handle);
                handle = -1;  // checksum does NOT match ==> error: file corrupt
            }
            // else case: checksum matches ==> keep original file ==> nothing to do
         }
         else
         {
            close(handle);
            handle = -1;      // error: file corrupt
         }
      }
      else
      {
         close(fdCsum);
         handle = -1;         // error: file corrupt
      }
   }
   // *************************************************
   // there is ONLY a backup file
   // *************************************************
   else if(backupAvail == 0)
   {
      DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclVerifyConsistency => there is ONLY a backup file"));

      // calculate checksum form backup file
      fdBackup = open(backupPath,  O_RDONLY);
      if(fdBackup != -1)
      {
         helperCalcCrc32Csum(fdBackup, backCsumBuf);
         close(fdBackup);

         // calculate the checksum form the original file to see if it matches
         handle = open(origPath, openFlags);
         if(handle != -1)
         {
            helperCalcCrc32Csum(handle, origCsumBuf);

            if(strcmp(backCsumBuf, origCsumBuf)  != 0)
            {
               close(handle);
               handle = -1;   // checksum does NOT match ==> error: file corrupt
            }
            // else case: checksum matches ==> keep original file ==> nothing to do

         }
         else
         {
            close(handle);
            handle = -1;      // error: file corrupt
         }
      }
      else
      {
         close(fdBackup);
         handle = -1;         // error: file corrupt
      }
   }
   // for else case: nothing to do

   // if we are in an inconsistent state: delete file, backup and checksum
   if(handle == -1)
   {
      remove(origPath);
      remove(backupPath);
      remove(csumPath);

      DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("backupVerifyConsistency => inconsistent state, not possible to recover, delete files"));
   }

   return handle;
}



int backupRecoverFromBackup(int backupFd, const char* original)
{
   int handle = 0;

   handle = open(original, O_TRUNC | O_RDWR);
   if(handle != -1)
   {
      // copy data from one file to another
      if(helperDoFileCopy(backupFd, handle) == -1)
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pclRecoverFromBackup => failed to copy files"), DLT_STRING(strerror(errno)) );
      }
   }

   return handle;
}



int helperCreateBackup(const char* dstPath, int srcfd, const char* csumPath, const char* csumBuf)
{
   int dstFd = 0, csfd = 0;
   int readSize = -1;

   // create checksum file and and write checksum
   csfd = open(csumPath, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
   if(csfd != -1)
   {
      int csumSize = strlen(csumBuf);
      if(write(csfd, csumBuf, csumSize) != csumSize)
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pclCreateBackup => failed to write checksum to file"));
      }
      close(csfd);
   }
   else
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pclCreateBackup => failed to create checksum file:"), DLT_STRING(csumPath), DLT_STRING(strerror(errno)) );
   }

   // create backup file, user and group has read/write permission, others have read permission
   if(access(dstPath, F_OK) != 0)
   {
      char pathToCreate[PathMaxLen] = {0};
      strncpy(pathToCreate, dstPath, PathMaxLen);
      dstFd = helperCreateAndOpenFile(pathToCreate);
   }

   if(dstFd != -1)
   {
      // remember the current position
      off_t curPos = lseek(srcfd, 0, SEEK_CUR);

      // copy data from one file to another
      if(helperDoFileCopy(srcfd, dstFd) == -1)
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pcl_create_backup => Failed to copy file"));
      }

      if((readSize = close(dstFd)) == -1)
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pcl_create_backup => error closing fd"));

      // set back to the position
      lseek(srcfd, curPos, SEEK_SET);
   }
   else
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pclCreateBackup => failed to open backup file"),
                                          DLT_STRING(dstPath), DLT_STRING(strerror(errno)));
   }

   return readSize;
}



int helperCalcCrc32Csum(int fd, char crc32sum[])
{
   int rval = 1;

   if(crc32sum != 0)
   {
      char* buf;
      struct stat statBuf;

      fstat(fd, &statBuf);
      buf = malloc((unsigned int)statBuf.st_size);

      if(buf != 0)
      {
         off_t curPos = 0;
         // remember the current position
         curPos = lseek(fd, 0, SEEK_CUR);

         if(curPos != 0)
         {
            // set to beginning of the file
            lseek(fd, 0, SEEK_SET);
         }

         while((rval = read(fd, buf, statBuf.st_size)) > 0)
         {
            unsigned int crc = 0;
            crc = checksumCrc32(crc, (unsigned char*)buf, statBuf.st_size);
            snprintf(crc32sum, ChecksumBufSize-1, "%x", crc);
         }

         // set back to the position
         lseek(fd, curPos, SEEK_SET);

         free(buf);
      }
   }
   return rval;
}





void helperFormatDirString(char* dirString)
{
   int length = strlen(dirString);
   if(dirString[length-1] == '/')
   {
      dirString[length-1] = '\0';   // remove \ at the end of the directory string
   }
   else
   {
      dirString[PathMaxLen-1] = '\0'; // terminate string
   }
}



int helperDoFileCopy(int srcFd, int dstFd)
{
   int rval = 0;
   int offset;
   struct stat buf;
   memset(&buf, 0, sizeof(buf));

   offset = lseek(srcFd, 0, SEEK_CUR);
   if(offset != 0)
		lseek(srcFd, 0, SEEK_SET);		// correct the offset if not pointing to beginning

   fstat(srcFd, &buf);
   rval = sendfile(dstFd, srcFd, 0, buf.st_size);

   lseek(srcFd, offset, SEEK_SET);		// set offset back to original

   return rval;
}



void helperRemoveBackup(int handle)
{
   // remove backup file
   remove(handleGetBackupPath(handle));  // we don't care about return value
   // remove checksum file
   remove(handleGetChecksumPath(handle));    // we don't care about return value
}
