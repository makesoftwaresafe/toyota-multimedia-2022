Name:           ias-audio-configuration_brd2
Version:        10.0.0
Release:        0
Summary:        IAS audio configuration for brd2
Vendor:         Advanced Driver Information Technology Corporation
Group:          Media/Audio
License:        Advanced Driver Information Technology Corporation Proprietary
Provides:       ias-audio-configuration_brd2 = %{version}-%{release}

%description -n ias-audio-configuration_brd2
IAS audio configuration for brd2

%post -n ias-audio-configuration_brd2
/sbin/ldconfig

%postun -n ias-audio-configuration_brd2
/sbin/ldconfig

%files -n ias-audio-configuration_brd2
%defattr(-,root,root,-)
/etc/asound.conf
/usr/share/alsa/alsa.conf.d/50-jack.conf
/usr/lib/systemd/system/audio_daemon.service
/usr/lib/systemd/system/audio_daemon.path
/usr/lib/systemd/system/jackd.service
/lib/firmware/fw_sst_0f28.bin
/usr/%{_lib}/audio-plugins/libias-audio-configuration_brd2.so


%package -n ias-audio-configuration_brd2-devel
Summary:        ias-audio-configuration_brd2: Development files
Group:          Development/Libraries
Requires:       ias-audio-configuration_brd2 = %{version}-%{release}
Requires:       ias-audio-delay-devel ias-audio-limiter-devel ias-audio-equalizer-devel ias-audio-volume-devel ias-audio-mixer-devel ias-audio-rtprocessingfwconfig-devel ias-core_libraries-foundation-devel 
Provides:       ias-audio-configuration_brd2-devel = %{version}-%{release}

%description -n ias-audio-configuration_brd2-devel
This package provides the development headers and libraries for ias-audio-configuration_brd2.

%files -n ias-audio-configuration_brd2-devel
%defattr(-,root,root,-)
/opt/sdk/include/audio/configuration_brd2/IasAudioIdentifiers.hpp
/opt/sdk/cmake/Modules/Findias-audio-configuration_brd2.cmake

#
# Build process on OBS
#
%prep
if [ ! -e %{_topdir}/BUILD/%{name}-%{version} ]; then
	ln -s %{_topdir}/../multimedia/platform/audiomanager/ias-audio-configuration_brd2 %{_topdir}/BUILD/%{name}-%{version}
fi
cd %{name}-%{version}
rm -rf build

%build
cd %{name}-%{version}
SRC_DIR=`pwd`
unset CFLAGS
unset CXXFLAGS
unset FFLAGS
mkdir -p build
cd build
PKG_CONFIG_LIBDIR=%{_topdir}/../sysroot/usr/local/lib/pkgconfig:%{_topdir}/../sysroot/usr/lib/pkgconfig cmake -Wno-dev -DCMAKE_BUILD_TYPE=Release -DIAS_DISABLE_TESTS=true -DCMAKE_INSTALL_PREFIX=/ -DIAS_EXTERNAL_BUILD_SOURCE_DIR=$SRC_DIR/src-gen/%{name}-%{version} -DCMAKE_TOOLCHAIN_FILE=%{_topdir}/../sysroot/opt/build_tools/cmake/ias_toolchain_kc22.cmake -DIAS_SYSROOT=%{_topdir}/../sysroot %{_topdir}/../sysroot/opt/build_tools/cmake
make

%install
rm -rf $RPM_BUILD_ROOT
cd %{name}-%{version}
cd build
make install DESTDIR=$RPM_BUILD_ROOT
make install DESTDIR=%{_topdir}/../sysroot

%clean
rm -rf $RPM_BUILD_ROOT
