## Process this file with automake to produce Makefile.in

@SET_MAKE@

man_MANS	= rpm.8 rpm2cpio.8
mandir		= @mandir@/ru

EXTRA_DIST	= $(man_MANS)

install-data-hook:
if WITH_PATH_VERSIONED
	-for p in $(man_MANS); do \
	    case "$$p" in \
		    *.1 ) mv $(DESTDIR)$(man1dir)/$${p} \
	                 `echo "$(DESTDIR)$(man1dir)/$${p}" | sed 's/\.1$$/-$(VERSION).1/'` ;; \
		    *.8 ) mv $(DESTDIR)$(man8dir)/$${p} \
	                 `echo "$(DESTDIR)$(man8dir)/$${p}" | sed 's/\.8$$/-$(VERSION).8/'` ;; \
	    esac; \
	done
endif

