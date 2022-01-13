Name:           adit-ipod_lib
Version:        %{adit_git_tag}
Release:        1
Summary:        iPod lib for IPC.
Group:          Applications/System
Vendor:         ADIT
License:        ADIT Proprietary license
Source0:        %{name}-%{version}.tar.bz2


%description
This package contains iPod lib for IPC and Utility.

%package devel
Group: Development/Library
Summary: Development files for client application on iPod Player.

%description devel
This package contains development files for client application on iPod Player.

%prep
if [ ! -e %{_topdir}/BUILD/%{name}-%{version} ]; then
	ln -sf %{_topdir}/../ipod/platform/ipod_lib %{_topdir}/BUILD/%{name}-%{version}
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
/lib/libiPodPlayerIPC.a
/lib/libiPodPlayerUtility.a

%files devel
/lib/libiPodPlayerIPC.a
/lib/libiPodPlayerUtility.a
%{_includedir}/adit-components/iPodPlayerIPCLib.h
%{_includedir}/adit-components/iPodPlayerUtilityLog.h
%{_includedir}/adit-components/iPodPlayerUtilityConfiguration.h

