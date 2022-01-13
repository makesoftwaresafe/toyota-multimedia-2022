#
# Makefile
#

CERTSDIR = /usr/share/ca-certificates
SUBDIRS = mozilla

all:
	for dir in $(SUBDIRS); do \
	  $(MAKE) -C $$dir all; \
	done

clean:
	for dir in $(SUBDIRS); do \
	  $(MAKE) -C $$dir clean; \
	done

install:
	for dir in $(SUBDIRS); do \
	  mkdir $(DESTDIR)/$(CERTSDIR)/$$dir; \
	  $(MAKE) -C $$dir install CERTSDIR=$(DESTDIR)/$(CERTSDIR)/$$dir; \
	done
	for dir in sbin; do \
	  $(MAKE) -C $$dir install CERTSDIR=$(DESTDIR)/$(CERTSDIR)/$$dir; \
	done

