Name:           adit-ipod_player
Version:        %{adit_git_tag}
Release:        1
Summary:        iPod player for communication software with Apple device using iPod control library.
Group:          Applications/System
Vendor:         ADIT
License:        ADIT Proprietary license
Source0:        %{name}-%{version}.tar.bz2
Requires:       adit-ipod_ctrl, adit-ipod_plugin, adit-ipod_lib

%description
This package contains iPod player software.

%package devel
Group: Development/Library
Summary: Development files for client application on iPod Player.

%description devel
This package contains development files for client application on iPod Player.

%prep
if [ ! -e %{_topdir}/BUILD/%{name}-%{version} ]; then
	ln -sf %{_topdir}/../ipod/platform/ipod_player %{_topdir}/BUILD/%{name}-%{version}
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
/lib/libiPodPlayerIF.a
/bin/iPodPlayerCore.out
/etc/pfcfg/IPOD_PLAYER_CFG.xml
/usr/lib/systemd/system/ipod-player.service

%files devel
/lib/libiPodPlayerIF.a
/bin/iPodPlayerCore.out
/etc/pfcfg/IPOD_PLAYER_CFG.xml
/usr/lib/systemd/system/ipod-player.service
