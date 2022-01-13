set( AUDIO_CFG_VERSION_MAJOR    10 )
set( AUDIO_CFG_VERSION_MINOR    0 )
set( AUDIO_CFG_VERSION_REVISION 0 )

IasInitPlugin( audio configuration_brd2 )

  IasSetEntityInstallPrefix( audio )
  IasSetEntityBinaryInstallPath(usr/${CMAKE_INSTALL_LIBDIR}/audio-plugins)

  IasUseEntity( core_libraries foundation )
  IasUseEntity( audio rtprocessingfwconfig )
  IasUseEntity( audio mixer )
  IasUseEntity( audio volume )
  IasUseEntity( audio equalizer )
  IasUseEntity( audio limiter )
  IasUseEntity( audio delay )

  IasAddSources(
    IasAudioSubsystemConfiguration_brd2.cpp
  )

  IasAddPublicHeaders(
    IasAudioIdentifiers.hpp
  )
  
  # copy alsa config
  IasInstallFiles( "public/brd2/alsaconfig" "etc" "" base asound.conf IS_ABSOLUTE_DESTINATION )
  IasInstallFiles( "public/res" "usr/share/alsa/alsa.conf.d" "" base 50-jack.conf IS_ABSOLUTE_DESTINATION )
  
  # copy systemd config files 
  IasInstallFiles("public/brd2/systemd" "${CMAKE_INSTALL_PREFIX}/usr/lib/systemd/system" "" base IS_ABSOLUTE_DESTINATION
    audio_daemon.service
    audio_daemon.path
    jackd.service
  )

  # copy lpe firmware for the requested configuration
  IasInstallFiles( "public/brd2/lpefirmware" "lib/firmware" "" base fw_sst_0f28.bin IS_ABSOLUTE_DESTINATION )
    
IasBuildPlugin()

IasCreateObsSpecFile(audio ias-audio-configuration_brd2 ${AUDIO_CFG_VERSION_MAJOR} ${AUDIO_CFG_VERSION_MINOR} ${AUDIO_CFG_VERSION_REVISION}
  SUMMARY  "IAS audio configuration for brd2"
  DESCRIPTION "IAS audio configuration for brd2"
  GROUP "Media/Audio"
  PACKAGE ias-audio-configuration_brd2
    PACKAGE_SUMMARY IAS audio configuration for brd2
    PACKAGE_ENTITIES configuration_brd2
)
