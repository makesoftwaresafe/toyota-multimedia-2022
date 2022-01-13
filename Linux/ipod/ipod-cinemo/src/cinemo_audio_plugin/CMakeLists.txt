include_directories(
        ${ALSA_INCLUDE_DIRS}
        )

file(GLOB cinemo_audio_plugin_src "*.cpp" "*.c" "*.h")

message("Source files ${cinemo_audio_plugin_src}")

add_library(${CINEMO_AUDIO_LIB} SHARED "${cinemo_audio_plugin_src}")

target_link_libraries(${CINEMO_AUDIO_LIB} PUBLIC
        ${UTIL_AUDIO_LIBRARIES}
        ${ADIT_PFCFG_SYS_LIBRARIES}
        dl
        stdc++
        pthread
        ipod-dlt_logging
        ipod-iap2-dlt_logging
        ${ALSA_LIBRARIES})

install(TARGETS ${CINEMO_AUDIO_LIB}
        LIBRARY DESTINATION ${CINEMO_INSTALL_LIBDIR}
        COMPONENT runtime)

install (FILES ./CINEMO_AUDIO_PARAMS.cfg
        DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/pfcfg/
        COMPONENT runtime)
