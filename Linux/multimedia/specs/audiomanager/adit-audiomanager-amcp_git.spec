Name:    audiomanager-amcp
Version: %{adit_git_tag}
Release: 1
Summary: %{name} - ADIT command plugin and wrapper
Group:   System Environment/Base
Vendor:  ADIT
License: Proprietary
Provides: libamcp_dbus_plugin.so, libAudioManagerCommand.so


%define multimedia_root_dir %{_topdir}/BUILD/multimedia-%{version}
%define subcomponent_rel_path platform/audiomanager/amcp_dbus_wrp
%define subcomponent_rel_path1 platform/audiomanager/amcp_dbus_plugin

%description
ADIT-JV: ADIT command plugin and C++ wrapper

%package devel
Summary: %{name} : Development Headers
Group:   Development

%description devel
This component development headers for audiomanager-amcp_dbus_wrp

%prep
if [ ! -e %{_topdir}/BUILD/multimedia-%{version} ]; then
	ln -s %{_topdir}/../multimedia %{_topdir}/BUILD/multimedia-%{version}
fi


%build
cd %{multimedia_root_dir}
make -C %{subcomponent_rel_path1}
make -C %{subcomponent_rel_path}

%install
cd %{multimedia_root_dir}
make -C %{subcomponent_rel_path1} install DEST_DIR=%{_topdir}/../sysroot
make -C %{subcomponent_rel_path1} install DEST_DIR=%{buildroot}
make -C %{subcomponent_rel_path} install DEST_DIR=%{_topdir}/../sysroot
make -C %{subcomponent_rel_path} install DEST_DIR=%{buildroot}

%clean
rm -rf %{buildroot}

%files
%{_libdir}/audiomanager/command/libamcp_dbus_plugin.so
%{_libdir}/libAudioManagerCommand.so
/etc/dbus-1/system.d/amcp_dbus.conf


%files devel
%{_includedir}/AMCommandAPI/IAmCommandClient.h
