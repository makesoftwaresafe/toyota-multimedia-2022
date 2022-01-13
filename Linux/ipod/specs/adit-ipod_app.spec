Name:           adit-ipod_app
Version:        %{adit_git_tag}
Release:        1
Summary:        iPod player application for communication with iPodPlayer.
Group:          Applications/System
Vendor:         ADIT
License:        ADIT Proprietary license
Source0:        %{name}-%{version}.tar.bz2
Requires:       adit-ipod_player, adit-ipod_ctrl, adit-ipod_plugin, adit-ipod_lib

%description
This package contains iPod player application.

%package devel
Group: Development/Library
Summary: Development files for client application on iPod Player.

%description devel
This package contains development files for client application on iPod Player.

%prep
if [ ! -e %{_topdir}/BUILD/%{name}-%{version} ]; then
	ln -sf %{_topdir}/../ipod/platform/ipod_app %{_topdir}/BUILD/%{name}-%{version}
fi

%build
cd %{name}-%{version}/
make

%install
cd %{name}-%{version}/
make install DEST_DIR=%{_topdir}/../sysroot
make install DEST_DIR=%{buildroot}

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
/bin/iPodPlayerAudio
/bin/iPodPlayerDeviceDetection
/bin/ipod-sample_app
/opt/ltp/runtest/smoketest/ipod-smoketest_01
/opt/ltp/testcases/bin/ipod-smoketest_01.sh
/opt/ltp/testscripts/run_ipod-smoketest.sh
/opt/platform/ipod-audio-test.sh
/opt/platform/ipod-player-smoketest.sh

%files devel
/bin/iPodPlayerAudio
/bin/iPodPlayerDeviceDetection
/bin/ipod-sample_app
/opt/platform/ipod-audio-test.sh
/opt/platform/ipod-player-smoketest.sh


