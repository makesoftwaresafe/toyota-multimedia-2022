Name:           adit-ipod_authentication
Version:        %{adit_git_tag}
Release:        1
Summary:        iPod authentication is library for communication with Apple CP.
Group:          Applications/System
Vendor:         ADIT
License:        ADIT Proprietary license
Source0:        %{name}-%{version}.tar.bz2

%description
This package contains iPod authentication library.

%package devel
Group: Development/Library
Summary: Development files for application.

%description devel
This package contains development files for application.

%prep
if [ ! -e %{_topdir}/BUILD/%{name}-%{version} ]; then
	ln -sf %{_topdir}/../ipod/platform/ipod_authentication %{_topdir}/BUILD/%{name}-%{version}
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
/lib/libipod-auth.so
/etc/pfcfg/IPOD_AUTH.cfg

%files devel
/lib/libipod-auth.so
/etc/pfcfg/IPOD_AUTH.cfg
%{_includedir}/adit-components/authentication.h

