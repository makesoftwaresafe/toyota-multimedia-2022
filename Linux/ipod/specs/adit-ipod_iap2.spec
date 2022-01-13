Name:           adit-ipod_iap2
Version:        %{adit_git_tag}
Release:        1
Summary:        Platform iPod (iAP2)
Group:          Applications/System
Vendor:         ADIT
License:        ADIT Proprietary license
Source0:        %{name}-%{version}.tar.bz2

%description
This package contains iAP2 software.

%package devel
Group: Development/Library
Summary: Development files for client application on iPod Player.

%description devel
This package contains development files for client application on iPod Player.

%define ipod_iap2 %{name}-%{version}

%prep
if [ ! -e %{_topdir}/BUILD/%{name}-%{version} ]; then
	ln -sf %{_topdir}/../ipod/platform/iAP2 %{_topdir}/BUILD/%{name}-%{version}
fi

%build
cd %{ipod_iap2}/
make

%install
cd %{ipod_iap2}/
make install DEST_DIR=%{_topdir}/../sysroot
make install DEST_DIR=%{buildroot}

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
/lib/libipod-iap2-interface.so
/lib/libipod-iap2-linklayer.so
/lib/libipod-iap2-session.so
/lib/libipod-iap2-transport.so
/lib/libipod-iap2-usb-role-switch.so
%{_includedir}/adit-components/iap2_callbacks.h
%{_includedir}/adit-components/iap2_commands.h
%{_includedir}/adit-components/iap2_cs_callbacks.h
%{_includedir}/adit-components/iap2_datacom.h
%{_includedir}/adit-components/iap2_defines.h
%{_includedir}/adit-components/iap2_dlt_log.h
%{_includedir}/adit-components/iap2_enums.h
%{_includedir}/adit-components/iap2_external_accessory_protocol_session.h
%{_includedir}/adit-components/iap2_file_transfer.h
%{_includedir}/adit-components/iap2_init.h
%{_includedir}/adit-components/iap2_parameter_free.h
%{_includedir}/adit-components/iap2_parameters.h
%{_includedir}/adit-components/iap2_subparameters.h
%{_includedir}/adit-components/iap2_usb_role_switch.h

%files devel
/lib/libipod-iap2-interface.so
/lib/libipod-iap2-linklayer.so
/lib/libipod-iap2-session.so
/lib/libipod-iap2-transport.so
/lib/libipod-iap2-usb-role-switch.so
%{_includedir}/adit-components/iap2_callbacks.h
%{_includedir}/adit-components/iap2_commands.h
%{_includedir}/adit-components/iap2_cs_callbacks.h
%{_includedir}/adit-components/iap2_datacom.h
%{_includedir}/adit-components/iap2_defines.h
%{_includedir}/adit-components/iap2_dlt_log.h
%{_includedir}/adit-components/iap2_enums.h
%{_includedir}/adit-components/iap2_external_accessory_protocol_session.h
%{_includedir}/adit-components/iap2_file_transfer.h
%{_includedir}/adit-components/iap2_init.h
%{_includedir}/adit-components/iap2_parameter_free.h
%{_includedir}/adit-components/iap2_parameters.h
%{_includedir}/adit-components/iap2_subparameters.h
%{_includedir}/adit-components/iap2_usb_role_switch.h

