## Process this file with automake to produce Makefile.in

SUBDIRS		= manual
if USE_NLS
SUBDIRS		+= fr ja ko pl ru sk
endif

noinst_MANS	= rpmcache.8 rpmgraph.8
man_MANS	= gendiff.1 rpm.8 rpmbuild.8 rpmdeps.8 \
		rpm2cpio.8 rpmconstant.8 rpmmtree.8

EXTRA_DIST	= $(man_MANS)

install-data-hook:
if WITH_PATH_VERSIONED
	-for p in $(man_MANS); do \
	    case "$$p" in \
		    *.1 ) mv $(DESTDIR)$(man1dir)/$${p} \
	                 `echo "$(DESTDIR)$(man1dir)/$${p}" | sed 's/\.1$$/-$(VERSION).1/'` ;; \
		    *.8 ) mv $(DESTDIR)$(man8dir)/$${p} \
	                 `echo "$(DESTDIR)$(man1dir)/$${p}" | sed 's/\.8$$/-$(VERSION).8/'` ;; \
	    esac; \
	done
endif

