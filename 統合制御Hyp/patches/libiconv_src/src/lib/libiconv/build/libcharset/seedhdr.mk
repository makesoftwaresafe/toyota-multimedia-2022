# Some subportions like to gather headers together before
# building to avoid symlinks and / or huge EXTRA_INCVPATH.
# This used to be acomplished with a simple PRE_TARGET of
# hinstall but people got excited when a straight 'make'
# would dare install anything (especially when running
# as root without a stage).  As a compromise, gather the
# headers to $(PROJECT_ROOT)/inc_work for those components
# that require it.
#
# The usefulness if this is debatable as a 'make install'
# has to be performed at some point to build the entire tree
# so that dependencies are built in the correct order and
# resolved from somewhere (proper stage or not).

ifneq ($(filter .pre_hinstall, $(PRE_TARGET)),)

.PHONY: .pre_hinstall .pre_huninstall

.pre_hinstall: override INSTALL_ROOT_HDR= $(PROJECT_ROOT)/inc_work
.pre_hinstall:
	$(TARGET_HINSTALL)
EXTRA_INCVPATH+= $(PROJECT_ROOT)/inc_work


.pre_huninstall: override INSTALL_ROOT_HDR= $(PROJECT_ROOT)/inc_work
.pre_huninstall:
	$(TARGET_HUNINSTALL)
ifneq ($(filter Linux QNX, $(shell uname -s)),)
	@-rm -rf $(PROJECT_ROOT)/inc_work
endif

endif
