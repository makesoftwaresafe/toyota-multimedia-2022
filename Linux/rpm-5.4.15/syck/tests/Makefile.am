AUTOMAKE_OPTIONS = 1.4 foreign

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/lib

TESTS = test-basic test-parse test-yts test-emit
noinst_PROGRAMS = test-basic test-parse test-yts test-emit
test_basic_SOURCES = Basic.c CuTest.c CuTest.h
test_basic_LDADD = -L$(top_builddir)/lib -lsyck
test_parse_SOURCES = Parse.c CuTest.c CuTest.h
test_parse_LDADD = -L$(top_builddir)/lib -lsyck
test_yts_SOURCES = YTS.c CuTest.c CuTest.h
test_yts_LDADD = -L$(top_builddir)/lib -lsyck
test_emit_SOURCES = Emit.c CuTest.c CuTest.h
test_emit_LDADD = -L$(top_builddir)/lib -lsyck
