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
 * @file           persistence_hm_dbus_service.c
 * @ingroup        Persistence Health Monitor
 * @author         Ingo Huerner
 * @brief          Implementation of the persistence health monitor dbus mainloop.
 * @see
 */

#include "persistence_hm_dbus_service.h"
#include "persistence_hm_definitions.h"
#include "persistence_hm_dbus_message.h"
#include "persistence_hm_disk_mon.h"

#include <NodeStateTypes.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>


pthread_mutex_t gDbusPendingRegMtx   = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t gDeliverpMtx         = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t gMainCondMtx         = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  gMainLoopCond        = PTHREAD_COND_INITIALIZER;

/*
const char* gDbusPersHmInterface = "org.genivi.persistence.health";
const char* gPersHmPath      		= "/org/genivi/persistence/health";
const char* gDbusPersHmPath      = "/org/genivi/persistence/health";
const char* gDbusPersHmMsg       = "PersHmPartError";
*/

/// pid file name and path
static const char* gPidFileName = "/var/run/persistence_health_monitor.pid";

/// communication channel into the dbus mainloop
static int gPipeFd[2] = {0};


typedef enum EDBusObjectType
{
   OT_NONE = 0,
   OT_WATCH,
   OT_TIMEOUT
} tDBusObjectType;



/// object entry
typedef struct SObjectEntry
{
   tDBusObjectType objtype;	/// libdbus' object
   union
   {
      DBusWatch * watch;		/// watch "object"
      DBusTimeout * timeout;	/// timeout "object"
   };
} tObjectEntry;



/// polling structure
typedef struct SPollInfo
{
   int nfds;						/// number of polls
   struct pollfd fds[10];		/// poll file descriptors array
   tObjectEntry objects[10];	/// poll object
} tPollInfo;


/// polling information
static tPollInfo gPollInfo;	/// polling information

int bContinue = 0;				/// indicator if dbus mainloop shall continue

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))



void sigHandler(int signo)
{
	MainLoopData_u data;
	data.message.cmd = (uint32_t)CMD_QUIT;
	data.message.string[0] = '\0'; 	// no string parameter, set to 0

	printf("sigHandler -> send CMD_QUIT\n");

   // send quit command to dbus mainloop
   deliverToMainloop_NM(&data);


}



int createPidFile(const char* pidFileName)
{
   int fd = -1, rval=1;
   ssize_t  size = 0;
   char pidBuffer[32];

   fd = open(pidFileName, O_CREAT|O_RDWR|O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
   if(fd != -1)
   {
     snprintf(pidBuffer, 32-1, "%d", getpid());
      if((size = write(fd, pidBuffer, strlen(pidBuffer))) == -1)
      {
        DLT_LOG(phmContext, DLT_LOG_WARN, DLT_STRING("Failed to write to persistency pidfile"),
                          DLT_STRING(pidFileName), DLT_STRING(strerror(errno)));
        rval = -1;
      }

   }
   else
   {
      DLT_LOG(phmContext, DLT_LOG_WARN, DLT_STRING("Failed to open/create persistency pidfile"),
                                                    DLT_STRING(pidFileName), DLT_STRING(strerror(errno)));
      rval = -1;
   }
   close(fd);

   return rval;
}

/* function to unregister ojbect path message handler */
static void unregisterMessageHandler(DBusConnection *connection, void *user_data)
{
   (void)connection;
   (void)user_data;
   DLT_LOG(phmContext, DLT_LOG_INFO, DLT_STRING("unregisterObjectPath\n"));
}



/* catches messages not directed to any registered object path ("garbage collector") */
static DBusHandlerResult handleObjectPathMessageFallback(DBusConnection * connection, DBusMessage * message, void * user_data)
{
   (void)user_data;

   printf("handleObjectPathMessageFallback => message: %s\n", dbus_message_get_member(message));
   DLT_LOG(phmContext, DLT_LOG_INFO, DLT_STRING("handleObjectPathMessageFallback: "),
                                     DLT_STRING(dbus_message_get_interface(message)),
                                     DLT_STRING(dbus_message_get_member(message)));
   return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

   return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}



static void  unregisterObjectPathFallback(DBusConnection *connection, void *user_data)
{
   (void)connection;
   (void)user_data;
   DLT_LOG(phmContext, DLT_LOG_INFO, DLT_STRING("unregisterObjectPathFallback\n"));
}


int sendNsmMessage(DBusConnection* conn, const char* method, int seatID, int sessionState)
{
   int rval = 0;

   DBusError error;
   dbus_error_init (&error);

   if(conn != NULL)
   {
      DBusMessage* message = dbus_message_new_method_call("org.genivi.NodeStateManager",              // destination
                                                          "/org/genivi/NodeStateManager/Consumer",    // path
                                                          "org.genivi.NodeStateManager.Consumer",     // interface
                                                          method);                         // method
      if(message != NULL)
      {
         const char* sessionName = "PersistenceFailure";
         const char* sessionOwner = "PersistenceHealthMonitor";

         dbus_message_append_args(message, DBUS_TYPE_STRING, &sessionName,
                                           DBUS_TYPE_STRING, &sessionOwner,
                                           DBUS_TYPE_INT32, &seatID,
                                           DBUS_TYPE_INT32, &sessionState, DBUS_TYPE_INVALID);


         if(!dbus_connection_send(conn, message, 0))
         {
            DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("sendLcmReg - Access denied"), DLT_STRING(error.message) );
         }
         dbus_connection_flush(conn);
         dbus_message_unref(message);
      }
      else
      {
         DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("sendLcmReg - Invalid msg"));
      }
   }
   else
   {
      DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("sendLcmReg - con is NULL"));
   }

   return rval;
}


int setup_dbus_mainloop(void)
{
   int rval = 0;
   DBusError err;
   DBusConnection* conn = NULL;

   const char *pAddress = getenv("PERS_CLIENT_DBUS_ADDRESS");

   // persistence health message message
   static const struct DBusObjectPathVTable vtablePersHM
         = {unregisterMessageHandler, checkPersPhmMsg, NULL, };

   // fallback
   static const struct DBusObjectPathVTable vtableFallback
      = {unregisterObjectPathFallback, handleObjectPathMessageFallback, NULL, NULL, NULL, NULL};


   dbus_error_init(&err);

   // Connect to the bus and check for errors
   if(pAddress != NULL)
   {
      DLT_LOG(phmContext, DLT_LOG_INFO, DLT_STRING("setup_dbus_mainloop -> Use specific dbus address:"), DLT_STRING(pAddress) );

      conn = dbus_connection_open(pAddress, &err);

      if(conn != NULL)
      {
         if(!dbus_bus_register(conn, &err))
         {
            DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("dbus_bus_register() Error :"), DLT_STRING(err.message) );
            dbus_error_free (&err);
            return -1;
         }
         else
         {
            DLT_LOG(phmContext, DLT_LOG_INFO, DLT_STRING("Registered dbus connection successfully!"));
         }
      }
      else
      {
         DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("dbus_connection_open() Error :"), DLT_STRING(err.message) );
         dbus_error_free(&err);
         return -1;
      }
   }
   else
   {
      DLT_LOG(phmContext, DLT_LOG_INFO, DLT_STRING("Use default dbus bus (DBUS_BUS_SYSTEM)"));

      conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);

      if(conn == NULL)
      {
         DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("dbus_bus_get() Error "), DLT_STRING(err.message));
         return -1;
      }
   }

   // register persistence failure session:
   sendNsmMessage(conn, "RegisterSession", NsmSeat_Driver, NsmSessionState_Inactive);


   sleep(5);

   printf("Set failure state to active\n");
   sendNsmMessage(conn, "SetSessionState", NsmSeat_Driver, NsmSessionState_Active);

   // setup the dbus
   mainLoop(vtablePersHM, vtableFallback, conn);

   return rval;
}





static dbus_bool_t addWatch(DBusWatch *watch, void *data)
{
   dbus_bool_t result = FALSE;
   (void)data;

   if (ARRAY_SIZE(gPollInfo.fds)>gPollInfo.nfds)
   {
      int flags = dbus_watch_get_flags(watch);

      tObjectEntry * const pEntry = &gPollInfo.objects[gPollInfo.nfds];
      pEntry->objtype = OT_WATCH;
      pEntry->watch = watch;

      gPollInfo.fds[gPollInfo.nfds].fd = dbus_watch_get_unix_fd(watch);

      if (TRUE==dbus_watch_get_enabled(watch))
      {
         MainLoopData_u data;

         if (flags&DBUS_WATCH_READABLE)
         {
            gPollInfo.fds[gPollInfo.nfds].events |= POLLIN;
         }
         if (flags&DBUS_WATCH_WRITABLE)
         {
            gPollInfo.fds[gPollInfo.nfds].events |= POLLOUT;
         }

         ++gPollInfo.nfds;
         /* wakeup main-loop, just in case */
			data.message.cmd = (uint32_t)CMD_REQUEST_NAME;
			data.message.string[0] = '\0'; 	// no string parameter, set to 0

			printf("addWatch -> send CMD_REQUEST_NAME\n");

			// send quit command to dbus mainloop
			deliverToMainloop_NM(&data);
      }

      result = TRUE;
   }

   return result;
}



static void removeWatch(DBusWatch *watch, void *data)
{
   void* w_data = dbus_watch_get_data(watch);

   (void)data;

   DLT_LOG(phmContext, DLT_LOG_INFO, DLT_STRING("removeWatch called "), DLT_INT( (int)watch) );

   if(w_data)
      free(w_data);

   dbus_watch_set_data(watch, NULL, NULL);
}



static void watchToggled(DBusWatch *watch, void *data)
{
   (void)data;
   DLT_LOG(phmContext, DLT_LOG_INFO, DLT_STRING("watchToggled called "), DLT_INT( (int)watch) );

   if(dbus_watch_get_enabled(watch))
      addWatch(watch, data);
   else
      removeWatch(watch, data);
}



static dbus_bool_t addTimeout(DBusTimeout *timeout, void *data)
{
   (void)data;
   dbus_bool_t ret = FALSE;

   if (ARRAY_SIZE(gPollInfo.fds)>gPollInfo.nfds)
   {
      const int interval = dbus_timeout_get_interval(timeout);
      if ((0<interval)&&(TRUE==dbus_timeout_get_enabled(timeout)))
      {
         const int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
         if (-1!=tfd)
         {
            const struct itimerspec its = { .it_value= {interval/1000, interval%1000} };
            if (-1!=timerfd_settime(tfd, 0, &its, NULL))
            {
               tObjectEntry * const pEntry = &gPollInfo.objects[gPollInfo.nfds];
               pEntry->objtype = OT_TIMEOUT;
               pEntry->timeout = timeout;
               gPollInfo.fds[gPollInfo.nfds].fd = tfd;
               gPollInfo.fds[gPollInfo.nfds].events |= POLLIN;
               ++gPollInfo.nfds;
               ret = TRUE;
            }
            else
            {
               DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("addTimeout => timerfd_settime() failed"), DLT_STRING(strerror(errno)) );
            }
         }
         else
         {
            DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("addTimeout => timerfd_create() failed"), DLT_STRING(strerror(errno)) );
         }
      }
   }
   else
   {
      DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("addTimeout => cannot create another fd to be poll()'ed"));
   }
   return ret;
}



static void removeTimeout(DBusTimeout *timeout, void *data)
{
   int i = gPollInfo.nfds;
   (void)data;

   while ((0<i--)&&(timeout!=gPollInfo.objects[i].timeout));

   if (0<i)
   {
      if (-1==close(gPollInfo.fds[i].fd))
      {
         DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("removeTimeout => close() timerfd"), DLT_STRING(strerror(errno)) );
      }

      --gPollInfo.nfds;
      while (gPollInfo.nfds>i)
      {
         gPollInfo.fds[i] = gPollInfo.fds[i+1];
         gPollInfo.objects[i] = gPollInfo.objects[i+1];
         ++i;
      }

      gPollInfo.fds[gPollInfo.nfds].fd = -1;
      gPollInfo.objects[gPollInfo.nfds].objtype = OT_NONE;
   }
}



/** callback for libdbus' when timeout changed */
static void timeoutToggled(DBusTimeout *timeout, void *data)
{
   int i = gPollInfo.nfds;
   (void)data;

   while ((0<i--)&&(timeout!=gPollInfo.objects[i].timeout));
   DLT_LOG(phmContext, DLT_LOG_INFO, DLT_STRING("timeoutToggled") );
   if (0<i)
   {
      const int interval = (TRUE==dbus_timeout_get_enabled(timeout))?dbus_timeout_get_interval(timeout):0;
      const struct itimerspec its = { .it_value= {interval/1000, interval%1000} };
      if (-1!=timerfd_settime(gPollInfo.fds[i].fd, 0, &its, NULL))
      {
         DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("timeoutToggled => timerfd_settime()"), DLT_STRING(strerror(errno)) );
      }
   }
}



int mainLoop(DBusObjectPathVTable vtable, DBusObjectPathVTable vtableFallback, void* userData)
{
   DBusError err;

   signal(SIGTERM, sigHandler);
   signal(SIGQUIT, sigHandler);
   signal(SIGINT,  sigHandler);

   DBusConnection * conn = (DBusConnection*)userData;
   dbus_error_init(&err);

   if (dbus_error_is_set(&err))
   {
      DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("mainLoop => Connection Error:"), DLT_STRING(err.message) );
      dbus_error_free(&err);
   }
   else if (NULL != conn)
   {
      dbus_connection_set_exit_on_disconnect(conn, FALSE);
      if (-1 == (pipe(gPipeFd)))
      {
         DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("mainLoop => eventfd() failed w/ errno:"), DLT_INT(errno) );
      }
      else
      {
         int ret;
         memset(&gPollInfo, 0 , sizeof(gPollInfo));

         gPollInfo.nfds = 1;
         gPollInfo.fds[0].fd = gPipeFd[0];
         gPollInfo.fds[0].events = POLLIN;

         // register for messages
         if (   (TRUE==dbus_connection_register_object_path(conn, "/org/genivi/persistence/health", &vtable, userData))
             && (TRUE==dbus_connection_register_fallback(conn, "/", &vtableFallback, userData)) )
         {
            if(   (TRUE!=dbus_connection_set_watch_functions(conn, addWatch, removeWatch, watchToggled, NULL, NULL))
               || (TRUE!=dbus_connection_set_timeout_functions(conn, addTimeout, removeTimeout, timeoutToggled, NULL, NULL)) )
            {
               DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("mainLoop => dbus_connection_set_watch_functions() failed"));
            }
            else
            {
               if(createPidFile(gPidFileName) == -1 )
               {
                  DLT_LOG(phmContext, DLT_LOG_WARN, DLT_STRING("Failed to create pid file"));
               }

               DLT_LOG(phmContext, DLT_LOG_INFO, DLT_STRING("mainLoop => START mainloop"));
               do
               {
               	printf("mainLoop -> WHILE - START\n");
                  bContinue = 0; /* assume error */

                  while(DBUS_DISPATCH_DATA_REMAINS==dbus_connection_dispatch(conn));

                  while ((-1==(ret=poll(gPollInfo.fds, gPollInfo.nfds, -1)))&&(EINTR==errno));

                  if (0>ret)
                  {
                     DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("mainLoop => poll() failed w/ errno "), DLT_INT(errno) );
                  }
                  else if (0==ret)
                  {
                     /* poll time-out */
                  }
                  else
                  {
                     int i;
                     int bQuit = FALSE;

                     for (i=0; gPollInfo.nfds>i && !bQuit; ++i)
                     {
                        /* anything to do */
                        if (0!=gPollInfo.fds[i].revents)
                        {
                           if (OT_TIMEOUT==gPollInfo.objects[i].objtype)
                           {
                              /* time-out occured */
                              unsigned long long nExpCount = 0;

                              if ((ssize_t)sizeof(nExpCount)!=read(gPollInfo.fds[i].fd, &nExpCount, sizeof(nExpCount)))
                              {
                                 DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("mainLoop => read failed"));
                              }
                              DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("mainLoop => timeout"));

                              if (FALSE==dbus_timeout_handle(gPollInfo.objects[i].timeout))
                              {
                                 DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("mainLoop => dbus_timeout_handle() failed!?"));
                              }
                              bContinue = TRUE;
                           }
                           else if (gPollInfo.fds[i].fd == gPipeFd[0])
                           {
                              /* internal command */
                              if (0!=(gPollInfo.fds[i].revents & POLLIN))
                              {
                              	MainLoopData_u readData;
                                 bContinue = TRUE;
                                 while ((-1==(ret = read(gPollInfo.fds[i].fd, readData.payload, 128)))&&(EINTR == errno));
                                 if(ret < 0)
                                 {
                                    DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("mainLoop => read() failed"), DLT_STRING(strerror(errno)) );
                                 }
                                 else
                                 {
                                    pthread_mutex_lock(&gMainCondMtx);
                                    printf("mainLoop -> RECEIVED command: ");
                                    //printf("--- *** --- Receive => mainloop => cmd: %d | string: %s | size: %d\n\n", readData.message.cmd, readData.message.string, ret);
                                    switch (readData.message.cmd)
                                    {
                                       case CMD_QUIT:
                                       	printf(" CMD_QUIT\n");
                                          bContinue = 0;
                                          bQuit = TRUE;
                                          break;
                                       case CMD_REQUEST_NAME:
                                       	printf(" CMD_REQUEST_NAME\n");
														if(DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER
																!= dbus_bus_request_name(conn, "org.genivi.persistence.health", DBUS_NAME_FLAG_DO_NOT_QUEUE, &err))
														{
															DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("Cannot acquire name 'org.genivi.persistence.health' - Bailing out"),
																										  DLT_STRING(err.message));
															dbus_error_free(&err);
															bContinue = FALSE;
														}
														break;
                                       default:
                                       	printf(" default -> nothing to do\n");
                                          DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("mainLoop => command not handled"), DLT_INT(readData.message.cmd) );
                                          break;
                                    }
                                    pthread_cond_signal(&gMainLoopCond);
                                    pthread_mutex_unlock(&gMainCondMtx);
                                 }
                              }
                           }
                           else
                           {
                              int flags = 0;

                              if (0!=(gPollInfo.fds[i].revents & POLLIN))
                              {
                                 flags |= DBUS_WATCH_READABLE;
                              }
                              if (0!=(gPollInfo.fds[i].revents & POLLOUT))
                              {
                                 flags |= DBUS_WATCH_WRITABLE;
                              }
                              if (0!=(gPollInfo.fds[i].revents & POLLERR))
                              {
                                 flags |= DBUS_WATCH_ERROR;
                              }
                              if (0!=(gPollInfo.fds[i].revents & POLLHUP))
                              {
                                 flags |= DBUS_WATCH_HANGUP;
                              }
                              bContinue = dbus_watch_handle(gPollInfo.objects[i].watch, flags);
                           }
                        }
                     }
                  }

               }
               while (0!=bContinue);
            }
            dbus_connection_unregister_object_path(conn, "/org/genivi/persistence/health");
            dbus_connection_unregister_object_path(conn, "/");
         }

         close(gPipeFd[0]);
         close(gPipeFd[1]);
      }
      dbus_connection_unref(conn);
      dbus_shutdown();

      remove(gPidFileName);

      freeRbTree();
   }
   return 0;
}



int deliverToMainloop(MainLoopData_u* payload)
{
   int rval = 0;

   pthread_mutex_lock(&gDeliverpMtx);

   pthread_mutex_lock(&gMainCondMtx);

   deliverToMainloop_NM(payload);

   pthread_cond_wait(&gMainLoopCond, &gMainCondMtx);
   pthread_mutex_unlock(&gMainCondMtx);


   pthread_mutex_unlock(&gDeliverpMtx);

   return rval;
}

int deliverToMainloop_NM(MainLoopData_u* payload)
{
   int rval = 0, length = 128;

   //length = sizeof(payload->message) + strlen(payload->message.string) + 1; // TODO calculate the correct length of the message

   //printf("--- *** --- Send => deliverToMainloop_NM => %d: | String: %s | size: %d\n", payload->message.cmd, payload->message.string, length);

   if(-1 == write(gPipeFd[1], payload->payload, length))
   {
     DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("deliverToMainloop => failed to write to pipe"), DLT_INT(errno));
     rval = -1;
   }
   return rval;
}

