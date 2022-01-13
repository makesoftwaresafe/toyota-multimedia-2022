AM_CPPFLAGS = -I$(top_srcdir)/include

AUTOMAKE_OPTIONS = gnu

noinst_LTLIBRARIES = libcxxio.la
cxxiodir= $(pkgincludedir)/c++/io

libcxxio_la_SOURCES = \
ByteArrayInputStream.cxx \
ByteArrayOutputStream.cxx \
DataInputStream.cxx \
DataOutputStream.cxx \
FileInputStream.cxx \
FileOutputStream.cxx \
FilterInputStream.cxx \
FilterOutputStream.cxx \
InputStream.cxx \
OutputStream.cxx \
PrintStream.cxx \
PushbackInputStream.cxx \
Writer.cxx

libcxxio_la_LIBADD = -licuuc

TESTS = test

check_PROGRAMS = test

test_SOURCES = test.cxx
test_LDADD = $(top_builddir)/c++/libbeecrypt_cxx.la $(top_builddir)/libbeecrypt.la -licuuc -licuio
