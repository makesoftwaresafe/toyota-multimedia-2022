Summary: Runs postinstall scripts on first boot of the target device
Name: run-postinsts
Version: 1.0
Release: r9
License: MIT
Group: devel
Packager: Mentor Graphics Corporation <embedded_support@mentor.com>
BuildRequires: systemd-systemctl-native
Requires: /bin/sh
Requires(post): /bin/sh
Requires(preun): /bin/sh

%description
Runs postinstall scripts on first boot of the target device.

%package -n run-postinsts-dbg
Summary: Runs postinstall scripts on first boot of the target device - Debugging files
Group: devel

%description -n run-postinsts-dbg
Runs postinstall scripts on first boot of the target device.  This package
contains ELF symbols and related sources for debugging purposes.

%package -n run-postinsts-staticdev
Summary: Runs postinstall scripts on first boot of the target device - Development files (Static Libraries)
Group: devel
Requires: run-postinsts-dev = 1.0-r9

%description -n run-postinsts-staticdev
Runs postinstall scripts on first boot of the target device.  This package
contains static libraries for software development.

%package -n run-postinsts-dev
Summary: Runs postinstall scripts on first boot of the target device - Development files
Group: devel
Requires: run-postinsts = 1.0-r9

%description -n run-postinsts-dev
Runs postinstall scripts on first boot of the target device.  This package
contains symbolic links, header files, and related items necessary for
software development.

%package -n run-postinsts-doc
Summary: Runs postinstall scripts on first boot of the target device - Documentation files
Group: doc

%description -n run-postinsts-doc
Runs postinstall scripts on first boot of the target device.  This package
contains documentation.

%package -n run-postinsts-locale
Summary: Runs postinstall scripts on first boot of the target device
Group: devel

%description -n run-postinsts-locale
Runs postinstall scripts on first boot of the target device.

%post
# run-postinsts - postinst
#!/bin/sh
OPTS=""

if [ -n "$D" ]; then
    OPTS="--root=$D"
fi

if type systemctl >/dev/null 2>/dev/null; then
	systemctl $OPTS enable run-postinsts.service

	if [ -z "$D" -a "enable" = "enable" ]; then
		systemctl restart run-postinsts.service
	fi
fi


%preun
# run-postinsts - prerm
#!/bin/sh
if [ "$1" = "0" ] ; then
OPTS=""

if [ -n "$D" ]; then
    OPTS="--root=$D"
fi

if type systemctl >/dev/null 2>/dev/null; then
	if [ -z "$D" ]; then
		systemctl stop run-postinsts.service
	fi

	systemctl $OPTS disable run-postinsts.service
fi
fi

%files
%defattr(-,-,-,-)
%dir "/lib"
%dir "/usr"
%dir "/etc"
%dir "/lib/systemd"
%dir "/lib/systemd/system"
"/lib/systemd/system/run-postinsts.service"
%dir "/usr/sbin"
"/usr/sbin/run-postinsts"

%files -n run-postinsts-dbg
%defattr(-,-,-,-)

%files -n run-postinsts-dev
%defattr(-,-,-,-)

