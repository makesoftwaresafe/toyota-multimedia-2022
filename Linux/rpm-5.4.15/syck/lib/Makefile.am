AUTOMAKE_OPTIONS = 1.4 foreign

AM_CPPFLAGS = -I$(top_srcdir)

LEXLIB = @LEXLIB@

YACC = @YACC@ 
AM_YFLAGS = -d -t -v -p syck

#lib_LTLIBRARIES = libsyck.la
#include_HEADERS = syck.h syck_st.h
noinst_LTLIBRARIES = libsyck.la
noinst_HEADERS = syck.h syck_st.h

libsyck_la_SOURCES = \
	emitter.c \
	handler.c \
	node.c \
	syck.c \
	syck_st.c \
	gram.y \
	bytecode.re \
	yaml2byte.c \
	token.re \
	implicit.re

# CURRENT:REVISION:AGE
# Bump REVISION if there's no API or ABI changes.
# Bump AGE and CURRENT and reset REVISION if there's no ABI changes.
# Bump CURRENT and reset AGE and REVISION if there's ABI changes.
libsyck_la_LDFLAGS = -version-info 0:55:0
# libsyck_a_LIBADD = $(LEXLIB)

REC = re2c
.re.c:
	$(REC) $< > $@.new && mv $@.new $@

LINT = splint

SYCK_SPLINT_SRCS = \
	emitter.c \
	handler.c \
	node.c \
	syck.c \
	syck_st.c \
	gram.c \
	bytecode.c \
	yaml2byte.c \
	token.c \
	implicit.c

gram.c:	gram.y
	$(YACC) -d -t -v -p syck -o $@ $<
	-@if test -f gram.c; then \
	{ echo "/*@-globs -mods -modnomods -moduncon -modunconnomods @*/";\
	  echo "/*@-noeffect -noeffectuncon @*/";\
	  echo "/*@-nullassign @*/";\
	  echo "/*@-readonlytrans @*/";\
	  echo "/*@-uniondef @*/";\
	  echo "/*@-warnlintcomments @*/";\
	  cat gram.c;\
	  echo "/*@=warnlintcomments @*/";\
	  echo "/*@=uniondef @*/";\
	  echo "/*@=readonlytrans @*/";\
	  echo "/*@=nullassign @*/";\
	  echo "/*@=noeffect =noeffectuncon @*/";\
	  echo "/*@=globs =mods =modnomods =moduncon =modunconnomods @*/";\
	} > _gram.c ;\
	  mv -f _gram.c gram.c; \
	fi

BUILT_SOURCES = gram.c gram.h

.PHONY: sources
sources:
	@echo $(SYCK_SPLINT_SRCS:%=syck/lib/%)

.PHONY: lint
lint:
	$(LINT) $(DEFS) $(AM_CPPFLAGS) $(SYCK_SPLINT_SRCS)
