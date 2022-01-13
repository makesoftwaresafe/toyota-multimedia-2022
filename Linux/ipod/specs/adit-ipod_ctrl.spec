Name:           adit-ipod_ctrl
Version:        %{adit_git_tag}
Release:        1
Summary:        iPod control is library for communication with Apple device.
Group:          Applications/System
Vendor:         ADIT
License:        ADIT Proprietary license
Source0:        %{name}-%{version}.tar.bz2

%description
This package contains iPod control library.

%package devel
Group: Development/Library
Summary: Development files for client application on iPod Player.

%description devel
This package contains development files for client application on iPod Player.

%prep
if [ ! -e %{_topdir}/BUILD/%{name}-%{version} ]; then
	ln -sf %{_topdir}/../ipod/platform/ipod_ctrl %{_topdir}/BUILD/%{name}-%{version}
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
/lib/libipod-prot.so
/lib/libipod-trans.so
/etc/pfcfg/IPOD_CTRL.cfg

%files devel
/lib/libipod-prot.so
/lib/libipod-trans.so
/etc/pfcfg/IPOD_CTRL.cfg
%{_includedir}/adit-components/iPodDataCom.h
%{_includedir}/adit-components/iap_callback.h
%{_includedir}/adit-components/iap_commands.h
%{_includedir}/adit-components/iap_common.h
%{_includedir}/adit-components/iap_database.h
%{_includedir}/adit-components/iap_digitalaudio.h
%{_includedir}/adit-components/iap_display.h
%{_includedir}/adit-components/iap_displayremote.h
%{_includedir}/adit-components/iap_general.h
%{_includedir}/adit-components/iap_init.h
%{_includedir}/adit-components/iap_ipodout.h
%{_includedir}/adit-components/iap_playback.h
%{_includedir}/adit-components/iap_storage.h
%{_includedir}/adit-components/iap_types.h
%{_includedir}/adit-components/ipodcommon.h

