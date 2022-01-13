Name:           adit-ipod_iap2_smoketest
Version:        %{adit_git_tag}
Release:        1
Summary:        iPod iAP2 smoketest.
Group:          Applications/System
Vendor:         ADIT
License:        ADIT Proprietary license
Source0:        %{name}-%{version}.tar.bz2
Requires:       adit-ipod_iap2, adit-ipod_iap2_plugin

%description
This package contains iPod iAP2 smoketest

%package devel
Group: Development/Library
Summary: Development files for client application on iPod Player.

%description devel
This package contains development files for client application on iPod Player.

%define iap2_smoketest %{name}-%{version}

%prep
if [ ! -e %{_topdir}/BUILD/%{name}-%{version} ]; then
	ln -sf %{_topdir}/../ipod/test/iAP2/iap2_smoketest %{_topdir}/BUILD/%{name}-%{version}
fi

%build
cd %{iap2_smoketest}
make

%install
cd %{iap2_smoketest}
make install DEST_DIR=%{_topdir}/../sysroot
make install DEST_DIR=%{buildroot}

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
/bin/ipod-iap2_smoketest.out
/opt/platform/ipod-iap2-smoketest.sh

%files devel
/bin/ipod-iap2_smoketest.out
/opt/platform/ipod-iap2-smoketest.sh


