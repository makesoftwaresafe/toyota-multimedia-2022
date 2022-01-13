TEMPLATE = subdirs

SUBDIRS *= sqldrivers
!winrt:qtHaveModule(network): SUBDIRS += bearer
qtHaveModule(gui): SUBDIRS *= imageformats platforms platforminputcontexts platformthemes generic
qtHaveModule(widgets): SUBDIRS += accessible

!winrt:!wince*:qtHaveModule(widgets):SUBDIRS += printsupport

INCLUDEPATH += ${INSPIRIUM_EXTENSION_INCLUDE_DIR}
LIBS += -L${INSPIRIUM_EXTENSION_LIB_DIR} -lInspiriumExtension -L${CUSTOM_SYSROOT}/additional/lib/ -l${TIMEINFOLIB} -l${TIMECALCULATIONLIB} -lrt
