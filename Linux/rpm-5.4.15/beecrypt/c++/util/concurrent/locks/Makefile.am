AM_CPPFLAGS = -I$(top_srcdir)/include

AUTOMAKE_OPTIONS = gnu

noinst_LTLIBRARIES = libcxxutilconcurrentlocks.la
cxxutilconcurrentlocksdir= $(pkgincludedir)/c++/util/concurrent/locks

libcxxutilconcurrentlocks_la_SOURCES = \
ReentrantLock.cxx

TESTS = testthread

check_PROGRAMS = testthread

testthread_SOURCES = testthread.cxx
testthread_LDADD = ../../../libbeecrypt_cxx.la
