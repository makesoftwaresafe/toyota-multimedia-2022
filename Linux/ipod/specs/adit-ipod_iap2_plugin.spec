Name:           adit-ipod_iap2_plugin
Version:        %{adit_git_tag}
Release:        1
Summary:        iPod iAP2 plugin for communication driver with Apple CP or Apple device.
Group:          Applications/System
Vendor:         ADIT
License:        ADIT Proprietary license
Source0:        %{name}-%{version}.tar.bz2
Requires:       adit-ipod_iap2

%description
This package contains iPod iAP2 Plugin for communication driver with Apple CP or Apple device.

%package devel
Group: Development/Library
Summary: Development files for client application on iPod Player.

%description devel
This package contains development files for client application on iPod Player.

%define iap2_bluetooth_plugin %{_topdir}/BUILD/%{name}-%{version}/iap2_bluetooth_plugin
%define iap2_usb_device_mode_plugin %{_topdir}/BUILD/%{name}-%{version}/iap2_usb_device_mode_plugin
%define iap2_usb_host_mode_plugin %{_topdir}/BUILD/%{name}-%{version}/iap2_usb_host_mode_plugin

%prep
if [ ! -e %{_topdir}/BUILD/%{name}-%{version} ]; then
    ln -sf %{_topdir}/../ipod/platform/ipod_plugin %{_topdir}/BUILD/%{name}-%{version}
fi

%build
cd %{iap2_bluetooth_plugin}
make
cd %{iap2_usb_device_mode_plugin}
make
cd %{iap2_usb_host_mode_plugin}
make


%install
cd %{iap2_bluetooth_plugin}
make install DEST_DIR=%{_topdir}/../sysroot
make install DEST_DIR=%{buildroot}
cd %{iap2_usb_device_mode_plugin}
make install DEST_DIR=%{_topdir}/../sysroot
make install DEST_DIR=%{buildroot}
cd %{iap2_usb_host_mode_plugin}
make install DEST_DIR=%{_topdir}/../sysroot
make install DEST_DIR=%{buildroot}

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
/lib/libipod-iap2_bluetooth_plugin.so
/lib/libipod-iap2_usb_device_mode_plugin.so
/lib/libipod-iap2_usb_host_mode_plugin.so

%files devel
/lib/libipod-iap2_bluetooth_plugin.so
/lib/libipod-iap2_usb_device_mode_plugin.so
/lib/libipod-iap2_usb_host_mode_plugin.so
