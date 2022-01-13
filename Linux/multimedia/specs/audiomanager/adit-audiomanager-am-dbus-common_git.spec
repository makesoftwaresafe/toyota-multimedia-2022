Name:    audiomanager-am-dbus-common
Version: %{adit_git_tag}
Release: 1
Summary: %{name} - ADIT common dbus library
Group:   System Environment/Base
Vendor:  ADIT
License: Proprietary

%define multimedia_root_dir %{_topdir}/BUILD/multimedia-%{version}
%define subcomponent_rel_path platform/audiomanager/am_dbus_common

%description
ADIT-JV: ADIT common dbus library

%package devel
Summary: %{name} : Development Headers and Static Library for audiomanager-am-dbus-common
Group:   Development

%description devel
This component development headers and static library for audiomanager-am-dbus-common

%prep
if [ ! -e %{_topdir}/BUILD/multimedia-%{version} ]; then
	ln -s %{_topdir}/../multimedia %{_topdir}/BUILD/multimedia-%{version}
fi


%build
cd %{multimedia_root_dir}
make -C %{subcomponent_rel_path}

%install
cd %{multimedia_root_dir}
make -C %{subcomponent_rel_path} install DEST_DIR=%{_topdir}/../sysroot
make -C %{subcomponent_rel_path} install DEST_DIR=%{buildroot}

%clean
rm -rf %{buildroot}

%files devel
%{_includedir}/AMCommonDBus/CDBusCommon.h
%{_includedir}/AMCommonDBus/CDBusReceiver.h
%{_includedir}/AMCommonDBus/CDBusSender.h
%{_libdir}/libam_dbus_common.a
