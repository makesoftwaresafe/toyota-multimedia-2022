AM_CPPFLAGS = -I$(top_srcdir)/include

AUTOMAKE_OPTIONS = gnu

noinst_LTLIBRARIES = libcxxlang.la

cxxlangdir=$(pkgincludedir)/c++/lang

libcxxlang_la_SOURCES = \
Character.cxx \
Integer.cxx \
Long.cxx \
Object.cxx \
String.cxx \
StringBuffer.cxx \
StringBuilder.cxx \
System.cxx \
Thread.cxx \
Throwable.cxx

TESTS = teststring testthread testnotify

check_PROGRAMS = teststring testthread testnotify

teststring_SOURCES = teststring.cxx
teststring_LDADD = ../libbeecrypt_cxx.la

testthread_SOURCES = testthread.cxx
testthread_LDADD = ../libbeecrypt_cxx.la

testnotify_SOURCES = testnotify.cxx
testnotify_LDADD = ../libbeecrypt_cxx.la
