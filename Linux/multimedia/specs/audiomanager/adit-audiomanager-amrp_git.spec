Name:    audiomanager-amrp
Version: %{adit_git_tag}
Release: 1
Summary: %{name} - ADIT command plugin and C++ wrapper
Group:   System Environment/Base
Vendor:  ADIT
License: Proprietary
Provides: libAudioManagerRouting.so

%define multimedia_root_dir %{_topdir}/BUILD/multimedia-%{version}
%define subcomponent_rel_path platform/audiomanager/amrp_dbus_wrp
%define subcomponent_rel_path1 platform/audiomanager/amri_dbus_plugin

%description
ADIT-JV: ADIT command plugin and C++ wrapper

%package devel
Summary: %{name} : Development Headers
Group:   Development

%description devel
This component development headers for audiomanager-amrp_dbus_wrp

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
%{_libdir}/audiomanager/routing/libamri_dbus_plugin.so
%{_libdir}/libAudioManagerRouting.so
/etc/dbus-1/system.d/amrp_dbus.conf

%files devel
%{_includedir}/AMRoutingAPI/IAmRoutingClient.h
