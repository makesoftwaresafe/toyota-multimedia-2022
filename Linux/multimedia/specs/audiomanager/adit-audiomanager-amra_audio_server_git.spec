Name:    audiomanager-amra_audio_server
Version: %{adit_git_tag}
Release: 1
Summary: %{name} - routing adapter for KP audio server
Group:   System Environment/Base
Vendor:  ADIT
License: Proprietary
Requires: audiomanager-amrp


%define multimedia_root_dir %{_topdir}/BUILD/multimedia-%{version}
%define subcomponent_rel_path platform/audiomanager/amra_audio_server

%description
ADIT-JV: routing adapter for KP audio server

%package devel
Summary: %{name} : Development Headers
Group:   Development

%description devel
This component development headers for dbus handling for hotplug and routing interface of routing adapter for KP Audio Server

%prep
if [ ! -e %{_topdir}/BUILD/multimedia-%{version} ]; then
	ln -s %{_topdir}/../multimedia %{_topdir}/BUILD/multimedia-%{version}
fi


%build
cd %{multimedia_root_dir}
make -C %{subcomponent_rel_path} clean
make -C %{subcomponent_rel_path}

%install
cd %{multimedia_root_dir}
make -C %{subcomponent_rel_path} install DEST_DIR=%{_topdir}/../sysroot
make -C %{subcomponent_rel_path} install DEST_DIR=%{buildroot}

%clean
rm -rf %{buildroot}

%files
/etc/dbus-1/system.d/routingadapter.conf
/etc/routingadapter/KPAudioDomain.xml
%{_libdir}/systemd/system/routingadapter.service
%{_libdir}/libAmRaDBusHotplug.so
%attr(0755,root,root)
%{_bindir}/KPRoutingAdaptor
/usr/share/audiomanager/HotplugReceiver.xml
/usr/share/audiomanager/HotplugSender.xml

%files devel
%{_includedir}/kpra/IRaHotplugReceive.h
%{_includedir}/kpra/IRaHotplugSend.h
%{_includedir}/kpra/IRaHotplugClient.h
