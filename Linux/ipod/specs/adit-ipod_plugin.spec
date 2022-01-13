Name:           adit-ipod_plugin
Version:        %{adit_git_tag}
Release:        1
Summary:        iPod plugin for communication driver with Apple CP or Apple device.
Group:          Applications/System
Vendor:         ADIT
License:        ADIT Proprietary license
Source0:        %{name}-%{version}.tar.bz2
Requires:       adit-ipod_lib

%description
This package contains iPod Plugin for communication driver with Apple CP or Apple device.

%package devel
Group: Development/Library
Summary: Development files for client application on iPod Player.

%description devel
This package contains development files for client application on iPod Player.

%define datacom_usb_hidapi %{_topdir}/BUILD/%{name}-%{version}/datacom_usb_hidapi
%define auth_i2c_plugin %{_topdir}/BUILD/%{name}-%{version}/auth_i2c_plugin

%prep
if [ ! -e %{_topdir}/BUILD/%{name}-%{version} ]; then
	ln -sf %{_topdir}/../ipod/platform/ipod_plugin %{_topdir}/BUILD/%{name}-%{version}
fi

%build
cd %{datacom_usb_hidapi}/
make
cd %{auth_i2c_plugin}/
make

%install
cd %{datacom_usb_hidapi}/
make install DEST_DIR=%{_topdir}/../sysroot
make install DEST_DIR=%{buildroot}
cd %{auth_i2c_plugin}/
make install DEST_DIR=%{_topdir}/../sysroot
make install DEST_DIR=%{buildroot}

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
/lib/libipod-usb-host_hidapi.so
/lib/libipod-auth-com.so

%files devel
/lib/libipod-usb-host_hidapi.so
/lib/libipod-auth-com.so
%{_includedir}/adit-components/ipodauth.h
