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
 * @file           persistence_fs_tools.c
 * @ingroup        Persistence Health Monitor
 * @author         Ingo Huerner
 * @brief          Implementation of the persistence health monitor file system tools.
 * @see
 */

#include "persistence_hm_fs_tools.h"

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <stdio.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <string.h>
#include <sys/vfs.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "persistence_hm_definitions.h"


#define PHM_PIPE_READ 0
#define PHM_PIPE_WRITE 1


// file system type string array
// Note: order must be the same as defined in the
// FSType_e enumerator.
static const char* gFsTypeString[FSType_LastEntry] = {
      "ext2",
      "ext3",
      "ext4",
      "btrfs"
};



FSType_e getFsType(const char* fsId)
{
   FSType_e fsType = FSType_LastEntry;

   if(0 == strcmp(fsId, "ext2"))
   {
      fsType = FSType_Ext2;
   }
   else if (0 == strcmp(fsId, "ext3"))
   {
      fsType = FSType_Ext3;
   }
   else if (0 == strcmp(fsId, "ext4"))
   {
      fsType = FSType_Ext4;
   }
   else if (0 == strcmp(fsId, "btrfs"))
   {
      fsType = FSType_Btrfs;
   }

   return fsType;
}



static void logExecOutput(const char *prog, FILE* file)
{
   char buffer[4096]={'\0', };

   while(NULL != fgets(buffer, sizeof(buffer), file))
   {
      DLT_LOG(phmContext, DLT_LOG_INFO, DLT_STRING(prog), DLT_STRING(buffer));
   }
}


int ProcessExecuteBlocking(char* const args[])
{
   int retVal = 0;
   pid_t pid = -1;
   int status = -1;
   int errPipe[2] = { -1 },
       outPipe[2] = { -1 };
   struct pollfd pollFds[2];
   int programExited = 0;

   FILE *err = NULL, *out = NULL;

#if 1
   // debug
   printf("ProcessExecuteBlocking: ");
   int i = 0;
   do
   {
      printf("%s ", args[i++]);
   }
   while(args[i] != NULL);

   printf("\n");
#endif

   if(0 != pipe2(outPipe, O_NONBLOCK))
   {
      DLT_LOG(phmContext, DLT_LOG_WARN, DLT_STRING("Failed to create pipes for subprocess; Can't log output"), DLT_STRING(args[0]));
   }
   if(0 != pipe2(errPipe, O_NONBLOCK))
   {
       DLT_LOG(phmContext, DLT_LOG_WARN, DLT_STRING("Failed to create pipes for subprocess; Can't log output"), DLT_STRING(args[0]));
   }

   pid=fork();
   if(0 == pid)
   {
     if(-1 != errPipe[PHM_PIPE_READ])
      {
        close(errPipe[PHM_PIPE_READ]);
      }
      if(-1 != errPipe[PHM_PIPE_WRITE])
      {
         dup2(errPipe[PHM_PIPE_WRITE], STDERR_FILENO);
      }

      if(-1 != outPipe[PHM_PIPE_READ])
      {
        close(outPipe[PHM_PIPE_READ]);
      }
      if(-1 != outPipe[PHM_PIPE_WRITE])
      {
         dup2(outPipe[PHM_PIPE_WRITE], STDOUT_FILENO);
      }
      exit(execv(args[0], args));
   }
   else if(0>pid)
   {
      retVal = pid;
   }
   else
   {
      close(errPipe[PHM_PIPE_WRITE]);
      close(outPipe[PHM_PIPE_WRITE]);

      err = fdopen(errPipe[PHM_PIPE_READ], "r");
      out = fdopen(outPipe[PHM_PIPE_READ], "r");
      do
      {
         pollFds[0].events = POLLIN | POLLERR | POLLHUP;
         pollFds[0].fd     = outPipe[PHM_PIPE_READ];
         pollFds[1].events = POLLIN | POLLERR | POLLHUP;
         pollFds[1].fd     = errPipe[PHM_PIPE_READ];

         poll(pollFds, ARRAY_LENGTH(pollFds), 100);

         if(FLAGS_SET(pollFds[0].revents, POLLIN))
         {
            logExecOutput(args[0], out);
         }
         if(FLAGS_SET(pollFds[1].revents, POLLIN))
         {
            logExecOutput(args[0], err);
         }

         retVal = waitpid(pid, &status, WNOHANG);
         if(pid == retVal)
         {
            retVal = WEXITSTATUS(status);
            programExited = 1;
         }
      } while( 0 == programExited);

      close(errPipe[PHM_PIPE_READ]);
      close(outPipe[PHM_PIPE_READ]);
   }
   return retVal;
}



int mountFS(const char* deviceName,const char* mountPointPath, FSType_e fsType, unsigned long mountflags)
{
   int rval = -1;

   printf("mount(%s,%s,%s,%d\n", deviceName, mountPointPath, gFsTypeString[fsType], (int)mountflags);
   if( mount(deviceName, mountPointPath, gFsTypeString[fsType], mountflags, "") == 0)
   {
      DLT_LOG(phmContext, DLT_LOG_INFO, DLT_STRING("file system has been mounted"), DLT_STRING(deviceName), DLT_STRING(mountPointPath));
      rval = 0;
   }
   else
   {
      DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING(deviceName), DLT_STRING("- mounting file system failed:"), DLT_STRING(strerror(errno)));
      printf("mountFS => mounting file system failed: %s\n ", strerror(errno));
   }
   return rval;
}


int unmountFS(const char* mountPointPath, int mountflags)
{
   int rval = -1;

   if(umount2(mountPointPath, mountflags) == 0)
   {
      DLT_LOG(phmContext, DLT_LOG_INFO, DLT_STRING("file system has been unmounted"), DLT_STRING(mountPointPath));
      rval = 0;
   }
   else
   {
      DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING(mountPointPath), DLT_STRING("- unmounting file system failed:"), DLT_STRING(strerror(errno)));
      printf("unmountFS => unmounting file system failed: %s\n ", strerror(errno));
   }
   return rval;
}



int checkFS(const char* deviceName, FSType_e fsType)
{
   int rval = -1;
   const char* const checkArgs[4][5] = {
      {"/sbin/fsck.ext2",  deviceName, "-p", "-v", NULL},
      {"/sbin/fsck.ext3",  deviceName, "-p", "-v", NULL},
      {"/sbin/fsck.ext4",  deviceName, "-p", "-v", NULL},
      {"/sbin/fsck.btrfs", deviceName, "-p", "-v", NULL}
   };

   if(fsType < FSType_LastEntry)
   {
      rval = ProcessExecuteBlocking((char* const)checkArgs[fsType]);
      switch(rval )
      {
         case 0:     // 0 - No errors
         {
            DLT_LOG(phmContext, DLT_LOG_WARN, DLT_STRING("fsck file system: no errors"));
            break;
         }
         case  1:    // 1 - File system errors corrected
         {
            DLT_LOG(phmContext, DLT_LOG_WARN, DLT_STRING("fsck file system: errors corrected"));
            break;
         }
         case  2:    // 2 - System should be rebooted
         {
            DLT_LOG(phmContext, DLT_LOG_WARN, DLT_STRING("fsck file system: system should be rebooted"));
            break;
         }
         case  4:    // 4 - File system errors left uncorrected
         {
            DLT_LOG(phmContext, DLT_LOG_WARN, DLT_STRING("fsck file system: errors left uncorrected: "));
            break;
         }
         case  8:    //8 - Operational error
         {
            DLT_LOG(phmContext, DLT_LOG_WARN, DLT_STRING("fsck file system: operational error"));
            break;
         }
         case  16:   //16 - Usage or syntax error
         {
            DLT_LOG(phmContext, DLT_LOG_WARN, DLT_STRING("fsck file system: usage or syntax error"));
            break;
         }
         default:
             /// 32 - Fsck canceled by user request, 128 - Shared library error
            DLT_LOG(phmContext, DLT_LOG_WARN, DLT_STRING("Fsck canceled by user request or Shared library error"));
            break;
      }
   }
   return rval;
}


int createNewPartition(const char* deviceName, const char* mountpoint, FSType_e fsType, unsigned long mountflags)
{
   int rval = 0;

   const char* const formatArgs[4][4] =   {
         {"/sbin/mkfs.ext2", "-F", deviceName, NULL},
         {"/sbin/mkfs.ext3", "-F", deviceName, NULL},
         {"/sbin/mkfs.ext4", "-F", deviceName, NULL},
         {"/sbin/mkfs.btrfs", "-F", deviceName, NULL}
   };

   if(fsType < FSType_LastEntry)
   {
      rval = ProcessExecuteBlocking((char* const)formatArgs[fsType]);
      if(rval == 0)
      {
         DLT_LOG(phmContext, DLT_LOG_INFO, DLT_STRING("Successfully formated device:"), DLT_STRING(deviceName));
      }
      else
      {
         DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("Failed to format device:"), DLT_STRING(deviceName));
         return -1;
      }
   }

   return rval;
}
