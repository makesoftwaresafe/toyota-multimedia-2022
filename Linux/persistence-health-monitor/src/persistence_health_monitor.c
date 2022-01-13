#/******************************************************************************
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
 * @file           persistence_health_monitor.c
 * @ingroup        Persistence health monitor
 * @author         Ingo Huerner
 * @brief          Header of the persistence health monitor
 * @see
 */


#include "persistence_hm_definitions.h"
#include "persistence_hm_dbus_service.h"
#include "persistence_hm_disk_mon.h"


/// print the usage
void printfUsage()
{
   printf("\n");
   printf("Usage: persistence_health_monitor [-d] [-h]\n\n");
   printf("   -d start the PHM as deamon\n");
   printf("   -m start disk space monitoring\n");
   printf("   -h this help\n\n");
}


int main(int argc, char *argv[])
{
   int c  = 0, startDaemon = 0, diskMonitoring = 0;

   // parse the command line options
   while ((c = getopt(argc, argv, "dhm")) != -1)
   {
      switch(c)
      {
      case 'd':
         startDaemon = 1;
         break;
      case 'm':
         diskMonitoring = 1;
         break;
      case 'h':
         printfUsage();
         _exit(1);
         break;
      }
   }
   DLT_REGISTER_APP("PHM","Persistence health monitor");
   DLT_REGISTER_CONTEXT(phmContext,"PHM","Context for Logging");

   DLT_LOG(phmContext, DLT_LOG_INFO, DLT_STRING("START Persistence health monitor") );

   if(startDaemon == 1)
   {
      DLT_LOG(phmContext, DLT_LOG_INFO, DLT_STRING("Persistence health monitor - deamonize") );
      // make a daemon process
      if(daemon(1, 1) == -1)
      {
         DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("deamonize failed: Cannot detach!") );
         _exit(-1);
      }
   }


   DLT_LOG(phmContext, DLT_LOG_INFO, DLT_STRING("Persistence health monitor - do disk monitoring"), DLT_INT(diskMonitoring) );
   if(diskMonitoring == 1)
   {
      if(startMonitorThread() == -1)
      {
         DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("Failed to start disk monitor thread") );
      }
   }


   if(setup_dbus_mainloop() == -1 )
   {
      DLT_LOG(phmContext, DLT_LOG_INFO, DLT_STRING("Failed to setup dbus mainloop") );
   }

   DLT_LOG(phmContext, DLT_LOG_INFO, DLT_STRING("END Persistence health monitor") );

   DLT_UNREGISTER_CONTEXT(phmContext);
   DLT_UNREGISTER_APP();
   dlt_free();

   printf("persistence_health_monitor ==> ENDED\n");

   return 0;
}
