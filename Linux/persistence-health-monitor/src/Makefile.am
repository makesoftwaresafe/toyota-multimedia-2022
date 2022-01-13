AUTOMAKE_OPTIONS = foreign


if DEBUG
AM_CFLAGS = -fprofile-arcs -ftest-coverage $(DEPS_CFLAGS) -g -I../include -Idbus-1.0
else
AM_CFLAGS = $(DEPS_CFLAGS) -I../include -Idbus-1.0
#AM_CFLAGS = -fprofile-arcs -ftest-coverage  $(DEPS_CFLAGS) -I../include -Idbus-1.0
endif


#include_HEADERS = ../include/persistence_.h


bin_PROGRAMS = persistence_health_monitor
persistence_health_monitor_SOURCES = \
                                     persistence_health_monitor.c \
                                     persistence_hm_dbus_service.c \
                                     persistence_hm_dbus_message.c \
                                     persistence_hm_fs_tools.c \
                                     persistence_hm_disk_mon.c \
                                     crc32.c \
                                     rbtree.c
 
persistence_health_monitor_LDADD = $(DEPS_LIBS) -lpers_admin_access_lib

